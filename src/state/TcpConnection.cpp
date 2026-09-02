#include "state/TcpConnection.h"
#include "state/FlagByte.h"
#include <limits>
#include <iostream>
#include <utility>

std::optional<Tcp> TcpConnection::handlePacket(Tcp &tcp) {
    if (!tcp.validateCheckSum()) {
        return std::nullopt;
    }
    FlagByte<Tcp::Flag> flags;
    switch (state_) {
        case State::Listen: {
            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }
            if (tcp.getFlags() & (Tcp::Flag::FIN | Tcp::Flag::PSH | Tcp::Flag::ACK |
                                  Tcp::Flag::URG) || !(tcp.getFlags() & (Tcp::Flag::SYN))) {
                break;
            }

            // Setting up the connection states
            connectionKey_.remoteIp = tcp.getSourceIP();
            connectionKey_.remotePort = tcp.getSourcePort();
            connectionKey_.localIp = tcp.getDestinationIP();
            connectionKey_.localPort = tcp.getDestPort();

            setTcpOptions(tcp.getParsedOptions());

            localSeqNumber_ = Tcp::generateIsn();
            sndUna_ = localSeqNumber_;
            localAckNumber_ = tcp.getSeqNumber();
            localWindow_ = std::numeric_limits<uint16_t>::max();
            remoteWindow_ = tcp.getWindow();

            localAckNumber_ += 1;
            flags.setFlag(Tcp::Flag::SYN);
            flags.setFlag(Tcp::Flag::ACK);
            reformatInboundPacket(tcp, flags);


            state_ = State::SynReceived;
            return tcp;
        }
        case State::SynReceived: {
            // Receiving Acknowledgement
            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }
            if (tcp.getFlags() & (Tcp::Flag::FIN | Tcp::Flag::PSH | Tcp::Flag::SYN |
                                  Tcp::Flag::URG) || !(tcp.getFlags() & (Tcp::Flag::ACK))) {
                break;
            }
            if (tcp.getAckNumber() != localSeqNumber_ + 1) {
                break;
            }
            localSeqNumber_ += 1;
            sndUna_ += 1;
            updateTimeStamps(tcp.getParsedOptions());

            remoteWindow_ = tcp.getWindow();

            std::cout << "Connection established with : " << net::ipToString(connectionKey_.remoteIp) << "::" <<
                    connectionKey_.remotePort << "\n\n";
            state_ = State::Established;
            break;
        }
        case State::Established: {
            if (!validateRemoteAck(tcp))break;
            processAck(tcp);

            setTcpOptions(tcp.getParsedOptions());

            tcpOptions_.mss.has_value()
                ? std::cout << "mss : " << static_cast<size_t>(tcpOptions_.mss.value()) << '\n'
                : std::cout << "mss : none" << '\n';
            tcpOptions_.windowScale.has_value()
                ? std::cout << "windowScale : " << static_cast<size_t>(tcpOptions_.windowScale.value()) << '\n'
                : std::cout << "windowScale : none" << '\n';
            tcpOptions_.tsVal.has_value()
                ? std::cout << "tsVal:" << tcpOptions_.tsVal.value() << '\n'
                : std::cout << "tsVal : none" << '\n';
            tcpOptions_.tsEcr.has_value()
                ? std::cout << "tsEcr:" << tcpOptions_.tsEcr.value() << '\n'
                : std::cout << "tsEcr : none" << '\n';

            std::cout << '\n';

            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }

            bool shouldReply{};

            remoteWindow_ = tcp.getWindow();

            if (tcp.getPayloadSize() > 0) {
                std::vector<uint8_t> buffer(tcp.getPayload().begin(), tcp.getPayload().end());
                addToReceivedBuffer(buffer);
                std::cout << "Data received from " << net::ipToString(connectionKey_.remoteIp) << "::" << connectionKey_
                        .remotePort << "\n";
                net::displayBytesAsText(buffer);

                localAckNumber_ += buffer.size();

                queueSend(buffer);

                shouldReply = true;
            }

            if (tcp.getFlags() & Tcp::Flag::FIN) {
                flags.setFlag(Tcp::Flag::FIN);
                flags.setFlag(Tcp::Flag::ACK);

                localAckNumber_ += 1;

                shouldReply = true;

                state_ = State::TearingDown;
                // temp instant send fin when receive fin , change in future when have sending data
            }
            if (shouldReply) {
                flags.setFlag(Tcp::Flag::ACK);
                Tcp tcpToSend{createTcpBaseConfig()};

                updateTimeStamps(tcp.getParsedOptions());

                if (auto buffer{sendNext()}; buffer.has_value()) {
                    tcpToSend.insertPayload(buffer.value());
                    tcpToSend.setOptions({});
                    flags.setFlag(Tcp::Flag::PSH);
                    std::cout << "Sending data to " << net::ipToString(connectionKey_.remoteIp) << "::" <<
                            connectionKey_.remotePort << "\n";
                }

                tcpToSend.setFlagByte(flags.getHexa());

                tcpToSend.calculateCheckSum();

                return tcpToSend;
            }

            break;
        }
        case State::TearingDown: {
            if (tcp.getAckNumber() != localSeqNumber_ + 1) {
                break;
            }

            processAck(tcp);

            state_ = State::Closed;

            break;
        }
        case State::Closed: {
            std::cout << "Attempted to connect to a closed connection\n";
            break;
        }
        default:
            break;
    }

    return std::nullopt;
}

void TcpConnection::queueSend(std::span<uint8_t> buffer) {
    sendBuffer_.insert(sendBuffer_.end(), buffer.begin(), buffer.end());
}

std::optional<std::vector<uint8_t> > TcpConnection::sendNext() {
    const size_t bufferBegin{localSeqNumber_ - sndUna_};

    if (bufferBegin >= sendBuffer_.size()) return std::nullopt;

    const uint8_t scale = tcpOptions_.windowScale.value_or(0);
    const size_t scaledWindow = static_cast<size_t>(remoteWindow_) << scale;
    const size_t mss = tcpOptions_.mss.value_or(536);


    const size_t sizeToSend = std::min({sendBuffer_.size() - bufferBegin, scaledWindow, mss});

    std::vector<uint8_t> buffer(sendBuffer_.begin() + static_cast<long>(bufferBegin),
                                sendBuffer_.begin() + static_cast<long>(sizeToSend) + static_cast<long>(bufferBegin));


    localSeqNumber_ += sizeToSend;

    return buffer;
}

void TcpConnection::abortConnection(Tcp &tcp) {
    tcp.abort();
    state_ = State::Closed;
}

TcpConstructConfig TcpConnection::createTcpBaseConfig() const {
    TcpConstructConfig config{};

    config.sourceIP = connectionKey_.localIp;
    config.destinationIP = connectionKey_.remoteIp;
    config.sourcePort = connectionKey_.localPort;
    config.destinationPort = connectionKey_.remotePort;
    config.seqNum = localSeqNumber_;
    config.ackNum = localAckNumber_;

    config.dataOffset = 0;
    config.flags = 0;
    config.windowSize = localWindow_;
    config.checkSum = 0;
    config.urgentPtr = 0;

    return config;
}

void TcpConnection::setTcpOptions(const TcpOptions &tcpOptions) {
    if (tcpOptions.mss.has_value()) {
        tcpOptions_.mss = *tcpOptions.mss;
    }

    if (tcpOptions.windowScale.has_value()) {
        tcpOptions_.windowScale = *tcpOptions.windowScale;
    }

    if (tcpOptions.tsVal.has_value()) {
        tcpOptions_.tsVal = *tcpOptions.tsVal;
    }

    if (tcpOptions.tsEcr.has_value()) {
        tcpOptions_.tsEcr = *tcpOptions.tsEcr;
    }
}

void TcpConnection::updateTimeStamps(const TcpOptions &tcpOptions) {
    if (tcpOptions.tsVal.has_value()) {
        tcpOptions_.tsVal = *tcpOptions.tsVal;
    }

    if (tcpOptions.tsEcr.has_value()) {
        tcpOptions_.tsEcr = *tcpOptions.tsEcr;
    }
}

void TcpConnection::processAck(const Tcp &tcp) {
    const uint32_t ack = tcp.getAckNumber();
    if (ack <= sndUna_) return;

    const size_t acked = ack - sndUna_;


    if (acked > sendBuffer_.size()) return;

    sndUna_ = ack;

    sendBuffer_.erase(sendBuffer_.begin(), sendBuffer_.begin() + static_cast<long>(acked));
}

void TcpConnection::reformatInboundPacket(Tcp &tcp, const std::optional<FlagByte<Tcp::Flag> > flags) const {
    tcp.swapSourceDestPort();
    if (flags.has_value()) {
        tcp.resetFlags();
        tcp.setFlagByte(flags.value().getHexa());
    }
    tcp.setAck(localAckNumber_);
    tcp.setSeq(localSeqNumber_);
    tcp.setWindow(localWindow_);

    tcp.calculateCheckSum();
}

bool TcpConnection::validateRemoteAck(const Tcp &tcp) const {
    const bool hasAckFlag{static_cast<bool>(tcp.getFlags() & Tcp::Flag::ACK)};
    const bool remoteAckNumberValidity{
        (sndUna_ < tcp.getAckNumber() && tcp.getAckNumber() <= localSeqNumber_) || tcp.getAckNumber() == localSeqNumber_
        || tcp.getAckNumber() == sndUna_
    };

    return hasAckFlag && remoteAckNumberValidity;
}

void TcpConnection::addToReceivedBuffer(std::span<uint8_t> buffer) {
    receivedBuffer_.insert(receivedBuffer_.end(), buffer.begin(), buffer.end());
}


IpConstructConfig TcpConnection::createIpBaseConfig() const {
    IpConstructConfig config{};
    config.versionNHl = (0x04) << 4 | 0x05;
    config.tos = 0;
    config.id = Ip::generateId();
    config.flagsFragment = Ip::Flags::DF << 13;
    config.ttl = 64;
    config.protocol = static_cast<uint8_t>(net::protocol::TCP);
    config.sourceAddr = connectionKey_.localIp;
    config.destAddr = connectionKey_.remoteIp;

    config.checkSum = 0;
    config.totalLength = 0;

    return config;
}

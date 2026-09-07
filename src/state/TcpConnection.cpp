#include "state/TcpConnection.h"
#include "state/FlagByte.h"
#include <limits>
#include <iostream>
#include <cmath>

std::optional<Tcp> TcpConnection::handlePacket(Tcp &tcp) {
    if (!tcp.validateCheckSum()) {
        return std::nullopt;
    }
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

            const TcpOptions receivedOptions{tcp.getParsedOptions()};
            setTcpOptions(receivedOptions);
            if (receivedOptions.tsVal.has_value()) {
                setTsEcr(receivedOptions.tsVal.value());
            }

            localSeqNumber_ = Tcp::generateIsn();
            sndUna_ = localSeqNumber_;
            localAckNumber_ = tcp.getSeqNumber();
            localWindow_ = std::numeric_limits<uint16_t>::max();
            remoteWindow_ = tcp.getWindow();

            synAckPending_ = true;

            localAckNumber_ += 1;

            state_ = State::SynReceived;

            if (auto segment{buildSegmentToSend()}) {
                return {segment.value()};
            }


            return std::nullopt;
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

            if (const TcpOptions receivedOptions{tcp.getParsedOptions()}; receivedOptions.tsVal.has_value()) {
                setTsEcr(receivedOptions.tsVal.value());
            }

            remoteWindow_ = tcp.getWindow();

            std::cout << "Connection established with : " << net::ipToString(connectionKey_.remoteIp) << "::" <<
                    connectionKey_.remotePort << "\n\n";
            state_ = State::Established;
            break;
        }
        case State::Established: {
            if (!validateRemoteAck(tcp))break;
            processAck(tcp);

            if (const TcpOptions receivedOptions{tcp.getParsedOptions()}; receivedOptions.tsVal.has_value()) {
                setTsEcr(receivedOptions.tsVal.value());
            }

            // printConnectionOptions();

            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }

            remoteWindow_ = tcp.getWindow();

            if (tcp.getPayloadSize() > 0) {
                ackPending_ = true;
                std::vector<uint8_t> buffer(tcp.getPayload().begin(), tcp.getPayload().end());
                addToReceivedBuffer(buffer);
                std::cout << "Data received from " << net::ipToString(connectionKey_.remoteIp) << "::" << connectionKey_
                        .remotePort << "\n";
                net::displayBytesAsText(buffer);

                // resend as test
                queueSend(buffer);

                localAckNumber_ += buffer.size();
            }

            if (tcp.getFlags() & Tcp::Flag::FIN) {
                finPending_ = true;

                localAckNumber_ += 1;

                state_ = State::TearingDown;
                // temp instant send fin when receive fin , change in future when have sending data
            }

            return buildSegmentToSend();

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

std::optional<Tcp> TcpConnection::buildSegmentToSend(const bool isRetransmit) {
    FlagByte<Tcp::Flag> flags;

    if (synAckPending_) {
        synAckPending_ = false;
        flags.setFlag(Tcp::Flag::SYN);
        flags.setFlag(Tcp::Flag::ACK);

        return makeSegment(flags,isRetransmit);
    }

    flags.setFlag(Tcp::Flag::ACK);

    if (auto payload{isRetransmit ? sendUnAcked() : sendNext()}) {
        flags.setFlag(Tcp::Flag::PSH);
        ackPending_ = false;
        Tcp seg {makeSegment(flags, isRetransmit,payload.value())};
        if (!isRetransmit) localSeqNumber_ += payload->size();

        return seg;
    }

    if (finPending_) {
        flags.setFlag(Tcp::Flag::FIN);
        ackPending_ = false;
        finPending_ = false;
        return makeSegment(flags,isRetransmit);
    }

    if (ackPending_) {
        ackPending_ = false;

        return makeSegment(flags,isRetransmit);
    }

    return std::nullopt;
}


Tcp TcpConnection::makeSegment(const FlagByte<Tcp::Flag> flags, const bool isRetransmit, const std::span<uint8_t> payload) {
    Tcp tcp(createTcpBaseConfig(isRetransmit));
    lastSentTime_ = std::chrono::steady_clock::now();
    setTsVal(getCurrentTimeStamp());

    std::vector<uint8_t> optionsBuffer{dumpTimeStampsOption()};
    tcp.setOptions(optionsBuffer);
    tcp.setFlagByte(flags.getHexa());

    if (!payload.empty()) {
        tcp.insertPayload(payload);

    }

    tcp.calculateCheckSum();
    return tcp;
}

std::optional<Tcp> TcpConnection::checkRetransmission() {

    const bool dataOutstanding = sndUna_ < localSeqNumber_;
    const bool controlOutstanding = (state_ == State::SynReceived || state_ == State::TearingDown);

    if (!dataOutstanding && !controlOutstanding) {
        return std::nullopt;
    }

    constexpr bool isRetransmit {true};
    const auto timeNow{std::chrono::steady_clock::now()};

    if (getSendElapsedTime() < rto_) {
        return std::nullopt;
    }

    rto_ = std::min(rto_ * 2, static_cast<uint32_t>(constants::RTT::rtoMax));

    lastSentTime_ = timeNow;


    if (state_ == TcpConnection::State::SynReceived) {
        synAckPending_ = true;
    }

    if (state_ == TcpConnection::State::TearingDown) {
        finPending_ = true;
    }

    if (auto segment{buildSegmentToSend(isRetransmit)}) {
        return {segment.value()};
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



    return buffer;
}

std::optional<std::vector<uint8_t>> TcpConnection::sendUnAcked() {
    if (sndUna_ >= localSeqNumber_) return std::nullopt;

    const uint8_t scale = tcpOptions_.windowScale.value_or(0);
    const uint32_t scaledWindow = remoteWindow_ << scale;
    const uint32_t mss = tcpOptions_.mss.value_or(536);

    const size_t sizeToSend = std::min({localSeqNumber_ - sndUna_, scaledWindow, mss});

    std::vector<uint8_t> buffer(sendBuffer_.begin(), sendBuffer_.begin() + static_cast<long>(sizeToSend) );

    return buffer;
}


void TcpConnection::printConnectionOptions() const {
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
}

uint32_t TcpConnection::getSendElapsedTime() const {
    const auto timeNow{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - lastSentTime_).count();

    return static_cast<uint32_t>(elapsed);
}

void TcpConnection::recalculateRto(const uint32_t rtt) {
    if (firstRttSample_) {
        srtt_ = rtt;
        rttvar_ = static_cast<double>(rtt) / 2;
        firstRttSample_ = false;
    } else {
        rttvar_ = (1 - constants::RTT::beta) * rttvar_ + constants::RTT::beta * std::abs(srtt_ - rtt);
        srtt_ = (1 - constants::RTT::alpha) * srtt_ + constants::RTT::alpha * rtt;
    }
    rto_ = static_cast<uint32_t>(std::clamp(srtt_ + constants::RTT::k * rttvar_, constants::RTT::rtoMin,
                                            constants::RTT::rtoMax));
}

uint32_t TcpConnection::getTsVal() const {
    return tcpOptions_.tsVal.value();
}

uint32_t TcpConnection::getTsEcr() const {
    return tcpOptions_.tsEcr.value();
}

void TcpConnection::setTsVal(uint32_t ts) {
    tcpOptions_.tsVal = ts;
}

void TcpConnection::setTsEcr(uint32_t ts) {
    tcpOptions_.tsEcr = ts;
}

std::vector<uint8_t> TcpConnection::dumpTimeStampsOption() const {
    using namespace net::bytes;

    std::vector<uint8_t> data;

    data.reserve(10);
    data.push_back(Tcp::Option::TS);
    data.push_back(10);

    appendBytes32(getTsVal(), data);
    appendBytes32(getTsEcr(), data);
    return data;
}

uint32_t TcpConnection::getCurrentTimeStamp() {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void TcpConnection::abortConnection(Tcp &tcp) {
    tcp.abort();
    state_ = State::Closed;
}

TcpConstructConfig TcpConnection::createTcpBaseConfig(bool isRetransmit) const {
    TcpConstructConfig config{};

    config.sourceIP = connectionKey_.localIp;
    config.destinationIP = connectionKey_.remoteIp;
    config.sourcePort = connectionKey_.localPort;
    config.destinationPort = connectionKey_.remotePort;
    config.seqNum = isRetransmit ? sndUna_:localSeqNumber_;
    config.ackNum = localAckNumber_;
    config.windowSize = localWindow_;
    config.urgentPtr = 0;

    config.dataOffset = 0;
    config.flags = 0;
    config.checkSum = 0;

    return config;
}

void TcpConnection::setTcpOptions(const TcpOptions &tcpOptions) {
    if (tcpOptions.mss.has_value()) {
        tcpOptions_.mss = *tcpOptions.mss;
    }

    if (tcpOptions.windowScale.has_value()) {
        tcpOptions_.windowScale = *tcpOptions.windowScale;
    }
}


void TcpConnection::processAck(const Tcp &tcp) {
    const uint32_t ack = tcp.getAckNumber();
    if (ack <= sndUna_) return;

    const size_t acked = ack - sndUna_;


    if (acked > sendBuffer_.size()) return;

    sndUna_ = ack;

    recalculateRto(getSendElapsedTime());

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

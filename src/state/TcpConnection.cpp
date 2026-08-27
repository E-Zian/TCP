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

            localSeqNumber_ = Tcp::randomIsn();
            localAckNumber_ = tcp.getSeqNumber();
            localWindow_ = std::numeric_limits<uint16_t>::max();
            remoteWindow_ = tcp.getWindow();

            // Syn-ack
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
            std::cout << "Connection established with : " << net::ipToString(connectionKey_.remoteIp) << "::" <<
                    connectionKey_.remotePort << "\n\n";
            state_ = State::Established;
            break;
        }
        case State::Established: {
            if (validateRemoteAck(tcp)) {
                break;
            }
            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }
            bool shouldReply{};

            if (tcp.getPayloadSize() > 0) {
                std::vector<uint8_t> buffer{tcp.getPayload()};
                addToReceivedBuffer(buffer);
                std::cout<< "Data received from " << net::ipToString(connectionKey_.remoteIp)  << "::" << connectionKey_.remotePort << "\n";
                net::displayBytesAsText(buffer);

                localAckNumber_ += buffer.size();

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
                Tcp tcpToSend{createTcpBase()};

                tcpToSend.setFlagByte(flags.getHexa());
                tcpToSend.setOptions({});
                tcpToSend.calculateCheckSum();
                return tcpToSend;
            }

            break;
        }
        case State::TearingDown: {
            if (tcp.getAckNumber() != localSeqNumber_ + 1) {
                break;
            }

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

std::vector<uint8_t> TcpConnection::recv() {
    return std::exchange(receivedBuffer_, {});
}

void TcpConnection::abortConnection(Tcp &tcp) {
    tcp.abort();
    state_ = State::Closed;
}

TcpConstructConfig TcpConnection::createTcpBase() const {
    TcpConstructConfig tcpConstructConfig{};

    tcpConstructConfig.sourceIP = connectionKey_.localIp;
    tcpConstructConfig.destinationIP = connectionKey_.remoteIp;
    tcpConstructConfig.sourcePort = connectionKey_.localPort;
    tcpConstructConfig.destinationPort = connectionKey_.remotePort;
    tcpConstructConfig.seqNum = localSeqNumber_;
    tcpConstructConfig.ackNum = localAckNumber_;

    tcpConstructConfig.dataOffset = 0;
    tcpConstructConfig.flags = 0;
    tcpConstructConfig.windowSize = localWindow_;
    tcpConstructConfig.checkSum = 0;
    tcpConstructConfig.urgentPtr = 0;

    return tcpConstructConfig;
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
    const bool remoteAckNumberValidity{tcp.getAckNumber() != localSeqNumber_};

    return hasAckFlag && remoteAckNumberValidity;
}

void TcpConnection::addToReceivedBuffer(std::span<uint8_t> buffer) {
    receivedBuffer_.insert(receivedBuffer_.end(), buffer.begin(), buffer.end());
}

#include "state/TcpConnection.h"
#include "state/FlagByte.h"
#include <limits>
#include <iostream>

void TcpConnection::handlePacket(Ip &ip, Tcp &tcp) {
    if (!tcp.validateCheckSum()) {
        return;
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
            formatToSend(tcp, flags);

            ip.reply();

            state_ = State::SynReceived;
            break;
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
            std::cout << "Connection established with : " << net::ip_to_string(connectionKey_.remoteIp) << "::" <<
                    connectionKey_.remotePort << "\n";
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


            if (tcp.getFlags() & Tcp::Flag::FIN) {
                // remoteFin_ = true;
                flags.setFlag(Tcp::Flag::FIN);
                flags.setFlag(Tcp::Flag::ACK);

                localAckNumber_ += 1;

                state_ = State::TearingDown;
                // temp instant send fin when receive fin change in future when have sending data
            }
            formatToSend(tcp, flags);

            ip.reply();

            break;
        }
        case State::TearingDown: {
            if (validateRemoteAck(tcp)) {
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
}

void TcpConnection::abortConnection(Ip &ip, Tcp &tcp) {
    tcp.abort();
    ip.reply();
    state_ = State::Closed;
}

void TcpConnection::formatToSend(Tcp &tcp, const std::optional<FlagByte<Tcp::Flag> > flags) const {
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
    return tcp.getAckNumber() != localSeqNumber_;
}

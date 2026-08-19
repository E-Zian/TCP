#include "state/TcpConnection.h"
#include <limits>
#include <cstdint>
#include <iostream>

void TcpConnection::handlePacket(Ip &ip, Tcp &tcp) {
    switch (state_) {
        case State::Listen: {
            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }
            if (tcp.getFlags() & (Tcp::Flag::FIN  | Tcp::Flag::PSH | Tcp::Flag::ACK |
                                  Tcp::Flag::URG) || !(tcp.getFlags() & (Tcp::Flag::SYN))) {
                break;
            }

            // Setting up the connection states
            connectionKey_.remoteIp = tcp.getSourceIP();
            connectionKey_.remotePort = tcp.getSourcePort();
            connectionKey_.localIp = tcp.getDestinationIP();
            connectionKey_.localPort = tcp.getDestPort();

            mySeqNumber_ = Tcp::randomIsn();
            myAckNumber_ = tcp.getSeqNumber();
            myWindow_ = std::numeric_limits<uint16_t>::max();
            theirWindow_ = tcp.getWindow();

            // Syn-ack
            tcp.swapSourceDestPort();
            myAckNumber_ += 1;
            tcp.setAck(myAckNumber_);
            tcp.setSeq(mySeqNumber_);
            tcp.setFlag(Tcp::Flag::ACK);
            tcp.setWindow(myWindow_);
            tcp.calculateCheckSum();

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
            if (tcp.getAckNumber() != mySeqNumber_ + 1) {
                break;
            }

            mySeqNumber_ += 1;
            std::cout << "Connection established with : "<< Net::ip_to_string(connectionKey_.remoteIp) <<"::"<<connectionKey_.remotePort << "\n";
            state_ = State::Established;
            break;
        }
        case State::Established: {
            if (tcp.getFlags() & Tcp::Flag::RST) {
                state_ = State::Closed;
                break;
            }

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

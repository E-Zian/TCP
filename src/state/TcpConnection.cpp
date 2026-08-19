#include "state/TcpConnection.h"
#include <limits>
#include <cstdint>


void TcpConnection::handlePacket(Ip& ip,Tcp& tcp) {
    switch (state_) {
        case State::Listen: {
            if (tcp.getFlags() & (Tcp::Flag::FIN | Tcp::Flag::RST | Tcp::Flag::PSH | Tcp::Flag::ACK |
                                  Tcp::Flag::URG) || !(tcp.getFlags() & (Tcp::Flag::SYN))) {
                tcp.reject();
                break;
            }

            // Setting up the connection states
            connectionKey_.remoteIp = tcp.getDestinationIP();
            connectionKey_.remotePort = tcp.getDestPort();
            connectionKey_.localIp = tcp.getSourceIP();
            connectionKey_.localPort = tcp.getSourcePort();

            mySeqNumber_ = Tcp::randomIsn();
            myAckNumber_ = tcp.getSeqNumber();
            myWindow_ = std::numeric_limits<uint16_t>::max();
            theirWindow_ = tcp.getWindow();

            // Syn-ack
            tcp.swapSourceDestPort();
            myAckNumber_+=1;
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
            break;
        }
        case State::Established: {
            break;
        }
        default:
            break;
    }
}

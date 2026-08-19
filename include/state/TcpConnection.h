#pragma once
#ifndef TCP_TCPCONNECTION_H
#define TCP_TCPCONNECTION_H
#include "protocol/Tcp.h"
#include "protocol/Ip.h"
#include "model/ConnectionKey.h"
#include <vector>
#include <cstdint>

class TcpConnection {
public:

    enum class State { Listen, SynReceived, Established };

    void handlePacket(Ip& ip,Tcp& tcp);

private:
    State state_{State::Listen};

    ConnectionKey connectionKey_{};

    uint32_t mySeqNumber_{};
    uint32_t myAckNumber_{};

    uint16_t myWindow_{};
    uint16_t theirWindow_{};

    std::vector<uint8_t> receivedBuffer_;
    std::vector<uint8_t> sendBuffer_;

};

#endif //TCP_TCPCONNECTION_H

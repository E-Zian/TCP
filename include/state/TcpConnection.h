#pragma once
#ifndef TCP_TCPCONNECTION_H
#define TCP_TCPCONNECTION_H
#include "protocol/Tcp.h"
#include <vector>
#include <cstdint>

class TcpConnection {
public:
    explicit TcpConnection(const Tcp& tcp);

    enum class State { Listen, SynReceived, Established };

    void handleTcpPacket(Tcp& tcp);

private:
    State state{State::Listen};

    uint32_t remoteIp_;
    uint16_t remotePort_;
    uint32_t localIp_;
    uint16_t localPort_;

    uint32_t myNextSequence_;
    uint32_t myNextExpectedSequence_;

    uint16_t myWindow_;
    uint16_t theirWindow_;

    std::vector<uint8_t> receivedBuffer_{};
    std::vector<uint8_t> sendBuffer_{};

};

#endif //TCP_TCPCONNECTION_H

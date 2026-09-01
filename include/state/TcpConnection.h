#pragma once
#ifndef TCP_TCPCONNECTION_H
#define TCP_TCPCONNECTION_H
#include "protocol/Tcp.h"
#include "protocol/Ip.h"
#include "model/ConnectionKey.h"
#include "state/FlagByte.h"
#include <vector>
#include <optional>

class TcpConnection {
public:

    enum class State { Listen, SynReceived, Established, TearingDown ,Closed };

    std::optional<Tcp> handlePacket(Tcp& tcp);

    [[nodiscard]] State getState() const {
        return state_;
    }
    
    // for application
    std::vector<uint8_t> recv();

    void queueSend(std::span<uint8_t> buffer);

    std::optional<std::vector<uint8_t>> flushSendBuffer();

    [[nodiscard]] IpConstructConfig createIpBaseConfig() const;

private:
    State state_{State::Listen};

    ConnectionKey connectionKey_{};

    uint32_t localSeqNumber_{};
    uint32_t localAckNumber_{};

    uint16_t localWindow_{};
    uint16_t remoteWindow_{};

    std::vector<uint8_t> receivedBuffer_;
    std::vector<uint8_t> sendBuffer_;

    // Sequence acknowledged by remote until
    uint32_t seqAckedUntil_{};

    TcpOptions tcpOptions_{};

    void reformatInboundPacket(Tcp& tcp,std::optional<FlagByte<Tcp::Flag>> flags = std::nullopt) const;

    [[nodiscard]] bool validateRemoteAck(const Tcp& tcp) const;

    void addToReceivedBuffer(std::span<uint8_t>buffer);

    void abortConnection(Tcp &tcp);

    [[nodiscard]] TcpConstructConfig createTcpBaseConfig() const;

    void setTcpOptions(const TcpOptions& tcpOptions);
};

#endif //TCP_TCPCONNECTION_H

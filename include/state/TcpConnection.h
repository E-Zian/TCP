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

    void queueSend(std::span<uint8_t> buffer);

    std::optional<Tcp> buildNextSegment();

    [[nodiscard]] IpConstructConfig createIpBaseConfig() const;

    std::optional<Tcp> checkRetransmission();

private:
    State state_{State::Listen};

    ConnectionKey connectionKey_{};

    uint32_t localSeqNumber_{};
    uint32_t localAckNumber_{};

    uint16_t localWindow_{};
    uint16_t remoteWindow_{};

    std::vector<uint8_t> receivedBuffer_;
    std::vector<uint8_t> sendBuffer_;

    // Unacknowledged bytes sent pos
    uint32_t sndUna_{};

    TcpOptions tcpOptions_{};

    bool synAckPending_{};

    bool ackOwed_{};

    bool finPending_{};

    void reformatInboundPacket(Tcp& tcp,std::optional<FlagByte<Tcp::Flag>> flags = std::nullopt) const;

    [[nodiscard]] bool validateRemoteAck(const Tcp& tcp) const;

    void addToReceivedBuffer(std::span<uint8_t>buffer);

    void abortConnection(Tcp &tcp);

    void setTcpOptions(const TcpOptions& tcpOptions);

    void updateTimeStamps(const TcpOptions& tcpOptions);

    void processAck(const Tcp& tcp);

    [[nodiscard]] Tcp makeSegment(FlagByte<Tcp::Flag> flags, std::span< uint8_t> payload = {}) const;

    [[nodiscard]] TcpConstructConfig createTcpBaseConfig() const;

    std::optional<std::vector<uint8_t>> sendNext();

    void printConnectionOptions() const;

};

#endif //TCP_TCPCONNECTION_H

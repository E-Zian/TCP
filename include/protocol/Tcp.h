#pragma once
#ifndef TCP_TCP_H
#define TCP_TCP_H

#include <cstdint>
#include <span>

struct TcpPseudoHeader {
    uint32_t sourceIP;
    uint32_t destinationIP;
    uint8_t zeroPadding;
    uint8_t protocol;
    uint16_t tcpLength;
};
class Tcp {

public:
    enum Offset : std::size_t {
        SourcePort  = 0,   // 2 bytes
        DestPort    = 2,   // 2 bytes
        SeqNum      = 4,   // 4 bytes
        AckNum      = 8,   // 4 bytes
        DataOffset  = 12,  // first 4 bits , last 4 is reserved
        Flags       = 13,  // 1 byte of control bits
        Window      = 14,  // 2 bytes
        Checksum    = 16,  // 2 bytes
        UrgentPtr   = 18,  // 2 bytes
        // Options / payload start at (DataOffset nibble * 4)
    };

    enum Flag : uint8_t {
        FIN = 0x01,
        SYN = 0x02,
        RST = 0x04,
        PSH = 0x08,
        ACK = 0x10,
        URG = 0x20,
    };

    Tcp(const std::span<uint8_t> data, const uint32_t sourceIP, const uint32_t destinationIP): data_(data),sourceIP_(sourceIP),destinationIP_(destinationIP) {

    };

    void swapSourceDestPort();
    void checkSum();
private:
    std::span<uint8_t> data_;
    uint32_t sourceIP_;
    uint32_t destinationIP_;
};
#endif //TCP_TCP_H

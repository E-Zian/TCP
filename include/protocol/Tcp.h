#pragma once
#ifndef TCP_TCP_H
#define TCP_TCP_H

#include "state/FlagByte.h"
#include "model/ConnectionKey.h"
#include <cstdint>
#include <format>
#include <span>
#include <vector>


struct TcpPseudoHeader {
    uint32_t sourceIP;
    uint32_t destinationIP;
    uint8_t zeroPadding;
    uint8_t protocol;
    uint16_t tcpLength;
};

struct TcpConstructConfig {
    uint32_t sourceIP;
    uint32_t destinationIP;

    uint16_t sourcePort;
    uint16_t destinationPort;
    uint32_t seqNum;
    uint32_t ackNum;
    uint8_t dataOffset;
    uint8_t flags;
    uint16_t windowSize;
    uint16_t checkSum;
    uint16_t urgentPtr;
};

class Tcp {
public:
    enum Offset : std::size_t {
        SourcePort = 0, // 2 bytes
        DestPort = 2, // 2 bytes
        SeqNum = 4, // 4 bytes
        AckNum = 8, // 4 bytes
        DataOffset = 12, // first 4 bits , last 4 is reserved
        Flags = 13, // 1 byte of control bits
        Window = 14, // 2 bytes
        CheckSum = 16, // 2 bytes
        UrgentPtr = 18, // 2 bytes
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

    enum Option : uint8_t {
        EOL = 0, // End of Option List, 1 byte, no length/value
        NOP = 1, // No-Operation 1 byte, no length/value (padding)
        MSS = 2, // Maximum Segment Size length 4, SYN only
        WScale = 3, // Window Scale length 3, SYN only
        SackOK = 4, //SACK Permitted length 2, SYN only
        Sack = 5, // variable length, data segments
        TS = 8, // Timestamps length 10, SYN + data
    };

    Tcp(std::span<uint8_t> data, uint32_t sourceIP, uint32_t destinationIP);

    explicit Tcp(const TcpConstructConfig &tcpConstructConfig);

    static uint32_t randomIsn();

    void swapSourceDestPort();

    uint16_t calculateCheckSum();

    void abort();

    bool validateCheckSum();

    [[nodiscard]] uint16_t getSourcePort() const {
        return sourcePort_;
    };

    [[nodiscard]] uint16_t getDestPort() const {
        return destinationPort_;
    }

    [[nodiscard]] uint32_t getSeqNumber() const {
        return seqNum_;
    };

    [[nodiscard]] uint32_t getAckNumber() const {
        return ackNum_;
    };

    [[nodiscard]] uint8_t getDataOffset() const {
        return (dataOffset_ >> 4) * 4;
    };

    [[nodiscard]] uint8_t getDataOffsetByte() const {
        return dataOffset_;
    };

    [[nodiscard]] uint8_t getFlags() const {
        return flags_.getHexa();
    };

    [[nodiscard]] uint16_t getWindow() const {
        return windowSize_;
    };

    [[nodiscard]] uint32_t getSourceIP() const {
        return sourceIP_;
    };

    [[nodiscard]] uint32_t getDestinationIP() const {
        return destinationIP_;
    };

    [[nodiscard]] uint16_t getCheckSum() const {
        return checkSum_;
    }

    [[nodiscard]] uint16_t getUrgentPtr() const {
        return urgentPtr_;
    }

    [[nodiscard]] std::vector<uint8_t> getOptions() const {
        return options_;
    }

    [[nodiscard]] std::vector<uint8_t> getPayload() const {
        return payload_;
    }

    [[nodiscard]] size_t getPayloadSize() const {
        return payload_.size();
    }

    [[nodiscard]] ConnectionKey getConnectionKey() const;

    void setSeq(const uint32_t sequenceNumber) {
        seqNum_ = sequenceNumber;
    }

    void setAck(const uint32_t acknowledgementNumber) {
        ackNum_ = acknowledgementNumber;
    }

    void setWindow(const uint16_t windowSize) {
        windowSize_ = windowSize;
    }

    void resetFlags() {
        flags_.reset();
    };

    void setFlag(const Flag flag) {
        flags_.setFlag(flag);
    };

    void setFlagByte(const uint8_t flagByte) {
        flags_.setFlagByte(flagByte);
    }

    bool setOptions(std::span<uint8_t> options);

    [[nodiscard]] std::vector<uint8_t> dump() const;


private:
    uint32_t sourceIP_;
    uint32_t destinationIP_;

    uint16_t sourcePort_;
    uint16_t destinationPort_;
    uint32_t seqNum_;
    uint32_t ackNum_;
    uint8_t dataOffset_;
    FlagByte<Flag> flags_;
    uint16_t windowSize_;
    uint16_t checkSum_;
    uint16_t urgentPtr_;

    std::vector<uint8_t> options_;
    std::vector<uint8_t> payload_;
};
#endif //TCP_TCP_H

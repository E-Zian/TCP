#pragma once
#ifndef TCP_TCP_H
#define TCP_TCP_H

#include <cstdint>
#include <span>
#include <vector>

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
        SourcePort = 0, // 2 bytes
        DestPort = 2, // 2 bytes
        SeqNum = 4, // 4 bytes
        AckNum = 8, // 4 bytes
        DataOffset = 12, // first 4 bits , last 4 is reserved
        Flags = 13, // 1 byte of control bits
        Window = 14, // 2 bytes
        Checksum = 16, // 2 bytes
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

    Tcp(std::span<uint8_t> data,uint32_t sourceIP,uint32_t destinationIP,int tunDeviceFD);

    void swapSourceDestPort();

    uint16_t calculateCheckSum();

    void reject();

    void resetFlags();

    void setFlag(Flag flag);

    bool validateCheckSum();

    [[nodiscard]] uint32_t getSeqNumber() const {
        return static_cast<uint32_t>(data_[SeqNum] << 24) | data_[SeqNum + 1] << 16 | data_[SeqNum + 2] << 8 | data_[SeqNum + 3];
    };

    [[nodiscard]] uint32_t getAckNumber() const {
        return static_cast<uint32_t>(data_[AckNum] << 24) | data_[AckNum + 1] << 16 | data_[AckNum + 2] << 8 | data_[AckNum + 3];
    };

    [[nodiscard]] uint8_t getDataOffset() const {
        return (data_[DataOffset] >> 4) * 4;
    };

    [[nodiscard]] uint8_t getFlags() const {
        return data_[Flags];
    };

    [[nodiscard]] uint16_t getWindow() const {
        return data_[Window] << 8 | data_[Window + 1];
    };

    void setSeq(const uint32_t sequenceNumber) {
        data_[SeqNum]     = sequenceNumber >> 24;
        data_[SeqNum + 1] = sequenceNumber >> 16;
        data_[SeqNum + 2] = sequenceNumber >> 8;
        data_[SeqNum + 3] = sequenceNumber;
    }

    void setAck(const uint32_t AcknowledgementNumber) {
        data_[AckNum]     = AcknowledgementNumber >> 24;
        data_[AckNum + 1] = AcknowledgementNumber >> 16;
        data_[AckNum + 2] = AcknowledgementNumber >> 8;
        data_[AckNum + 3] = AcknowledgementNumber;
    }

private:
    std::span<uint8_t> data_;
    uint32_t sourceIP_;
    uint32_t destinationIP_;
    const int tunDeviceFD_;
    uint32_t seqNum_;
    uint32_t ackNum_;
    std::vector<uint8_t> receivedData_{};
    uint32_t clientSeqNum_;
    uint32_t clientAckNum_;

    // void sendSynAck();
};
#endif //TCP_TCP_H

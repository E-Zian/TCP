#pragma once
#ifndef TCP_ICMP_H
#define TCP_ICMP_H

#include <span>
#include <cstdint>
#include <vector>

struct IcmpConfig {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
};

class Icmp {
public:
    explicit Icmp(std::span<uint8_t> data);

    explicit Icmp(const IcmpConfig& config);

    enum Offset : std::size_t {
        Type       = 0,   // 1 byte, 8 = echo request, 0 = echo reply
        Code       = 1,   // 1 byte
        CheckSum   = 2,   // 2 bytes
        Identifier = 4,   // 2 bytes
        Sequence   = 6,   // 2 bytes
        Data       = 8,   // payload starts here
    };

    enum RequestType : size_t {
        echo_reply  = 0,
        echo_request = 8,
    };

    [[nodiscard]] uint8_t getType() const { return type_; }
    [[nodiscard]] uint8_t getCode() const { return code_; }
    [[nodiscard]] uint16_t getCheckSum() const { return checkSum_; }
    [[nodiscard]] uint16_t getIdentifier() const { return identifier_; }
    [[nodiscard]] uint16_t getSequence() const { return sequence_; }

    void setType(const uint8_t type) { type_ = type; }
    void calculateChecksum();

    [[nodiscard]] std::vector<uint8_t> dump() const;

private:
    uint8_t type_;
    uint8_t code_;
    uint16_t checkSum_;
    uint16_t identifier_;
    uint16_t sequence_;

    std::vector<uint8_t> payload_;

};
#endif //TCP_ICMP_H

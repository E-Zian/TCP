#pragma once
#ifndef TCP_ICMP_H
#define TCP_ICMP_H

#include <span>
#include <cstdint>

class Icmp {
public:
    explicit Icmp(const std::span<uint8_t> data):data_(data) {}

    enum Offset : std::size_t {
        Type       = 0,   // 8 = echo request, 0 = echo reply
        Code       = 1,
        Checksum   = 2,   // 2 bytes
        Identifier = 4,   // 2 bytes
        Sequence   = 6,   // 2 bytes
        Data       = 8,   // payload starts here
    };

    enum RequestType : size_t {
        echo_reply  = 0,
        echo_request = 8,
    };

    [[nodiscard]] uint8_t type() const { return data_[Type]; }
    void calculateChecksum() const;
    std::span<uint8_t> data_;

private:

};
#endif //TCP_ICMP_H

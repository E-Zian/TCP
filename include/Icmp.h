#pragma once
#ifndef TCP_ICMP_H
#define TCP_ICMP_H

#include <span>
#include <cstdint>

class Icmp {
public:
    explicit Icmp(const std::span<uint8_t> data):data_(data) {}

private:
    std::span<uint8_t> data_;

};
#endif //TCP_ICMP_H

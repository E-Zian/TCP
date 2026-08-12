//
// Created by LeeEeZian on 12/8/2026.
//

#ifndef TCP_HELPER_H
#define TCP_HELPER_H

#include <span>
#include <cstdint>
#include <filesystem>
#include <bits/ios_base.h>

namespace Net {
    uint16_t checksum(std::span<const uint8_t> data);

    enum class protocol : uint8_t {
        ICMP   = 1,
        TCP    = 6,
        UDP    = 17,
    };

    void swapBytes(uint8_t* begin, uint8_t* target,size_t bytes);
}

#endif //TCP_HELPER_H

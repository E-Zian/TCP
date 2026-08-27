//
// Created by LeeEeZian on 12/8/2026.
//

#ifndef TCP_HELPER_H
#define TCP_HELPER_H

#include <span>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <bits/ios_base.h>

#include "protocol/Tcp.h"

namespace net {
    uint16_t checksum(std::span<const uint8_t> data);

    enum class protocol : uint8_t {
        ICMP   = 1,
        TCP    = 6,
        UDP    = 17,
    };

    void swapBytes(uint8_t* target1, uint8_t* target2,size_t bytes);
    void displayBytes(std::span<const uint8_t> bytes);
    std::string ipToString(uint32_t address);

    void displayBytesAsText(std::span<const uint8_t> bytes);
}

namespace net::bytes {
    uint8_t read8(std::span<uint8_t> bytes, size_t offset);
    uint16_t read16(std::span<uint8_t> bytes, size_t offset);
    uint32_t read32(std::span<uint8_t> bytes, size_t offset);

    std::vector<uint8_t> toBytes16(uint16_t data);
    std::vector<uint8_t> toBytes32(uint32_t data);

    void appendBytes16(uint16_t data,std::vector<uint8_t>& byteBuffer);
    void appendBytes32(uint32_t data,std::vector<uint8_t>& byteBuffer);

}

namespace constants {
    inline constexpr  size_t MAX_TRANSMISSION_UNIT {1500};
}

#endif //TCP_HELPER_H

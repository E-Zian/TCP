#include "Net.h"
#include <iostream>

uint16_t net::checksum(const std::span<const uint8_t> data) {
    uint32_t sum{};
    for (size_t i{}; i + 1 < data.size(); i += 2) {
        sum += (data[i] << 8 | data[i + 1]);
    }

    if (data.size() & 1) {
        sum += data[data.size() - 1] << 8;
    }

    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0xffff);
    }
    return static_cast<uint16_t>(~sum);
}

void net::swapBytes(uint8_t *target1, uint8_t *target2, const size_t bytes) {
    for (size_t i{}; i < bytes; ++i) {
        const uint8_t temp{target1[i]};
        target1[i] = target2[i];
        target2[i] = temp;
    }
}

void net::displayBytes(const std::span<const uint8_t> bytes) {
    std::cout << "Received packet of " << bytes.size() << " bytes:\n";

    for (ssize_t i = 0; i < bytes.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytes[i]) << ' ';
        if ((i + 1) % 16 == 0) std::cout << '\n'; // 16 bytes per row
    }
};

std::string net::ip_to_string(const uint32_t address) {
    return std::to_string((address >> 24) & 0xFF) + '.' +
           std::to_string((address >> 16) & 0xFF) + '.' +
           std::to_string((address >> 8) & 0xFF) + '.' +
           std::to_string(address & 0xFF);
}

uint8_t net::bytes::read8(const std::span<const uint8_t> bytes, const size_t offset) {
        return bytes[offset];
}

uint16_t net::bytes::read16(const std::span<const uint8_t> bytes, const size_t offset) {
    return bytes[offset] << 8 | bytes[offset + 1];
}

uint32_t net::bytes::read32(const std::span<const uint8_t> bytes, const size_t offset) {
    return bytes[offset] << 24 | bytes[offset + 1] << 16 | bytes[offset + 2] << 8 | bytes[offset + 3] ;
}
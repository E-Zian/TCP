#include "Net.h"
#include <iostream>
uint16_t Net::checksum(const std::span<const uint8_t> data) {
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
void Net::swapBytes(uint8_t* target1, uint8_t* target2, const size_t bytes) {
    for (size_t i{}; i < bytes; ++i) {
        const uint8_t temp{target1[i]};
        target1[i] = target2[i];
        target2[i] = temp;
    }
}
void Net::displayBytes(const std::span<const uint8_t> bytes) {
    for (ssize_t i = 0; i < bytes.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytes[i]) << ' ';
        if ((i + 1) % 16 == 0) std::cout << '\n'; // 16 bytes per row
    }

};

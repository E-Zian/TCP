#include "Net.h"
#include <iostream>
#include <arpa/inet.h>

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
    std::cout << std::dec << '\n'<<'\n';
};

std::string net::ipToString(const uint32_t address) {
    return std::to_string((address >> 24) & 0xFF) + '.' +
           std::to_string((address >> 16) & 0xFF) + '.' +
           std::to_string((address >> 8) & 0xFF) + '.' +
           std::to_string(address & 0xFF);
}

uint8_t net::bytes::read8(const std::span<const uint8_t> bytes, const size_t offset) {
        return bytes[offset];
}

uint16_t net::bytes::read16(const std::span<const uint8_t> bytes, const size_t offset) {
    return static_cast<uint16_t>(bytes[offset] << 8 | bytes[offset + 1]);
}

uint32_t net::bytes::read32(const std::span<const uint8_t> bytes, const size_t offset) {
    return static_cast<uint32_t>(bytes[offset] << 24 | bytes[offset + 1] << 16 | bytes[offset + 2] << 8 | bytes[offset + 3] );
}

uint64_t net::bytes::read64(const std::span<const uint8_t> bytes, const size_t offset) {
    return static_cast<uint64_t>(bytes[offset])     << 56 |
           static_cast<uint64_t>(bytes[offset + 1]) << 48 |
           static_cast<uint64_t>(bytes[offset + 2]) << 40 |
           static_cast<uint64_t>(bytes[offset + 3]) << 32 |
           static_cast<uint64_t>(bytes[offset + 4]) << 24 |
           static_cast<uint64_t>(bytes[offset + 5]) << 16 |
           static_cast<uint64_t>(bytes[offset + 6]) << 8  |
           static_cast<uint64_t>(bytes[offset + 7]);
}


std::vector<uint8_t> net::bytes::toBytes16(const uint16_t data) {
    std::vector<uint8_t> bytes;
    bytes.reserve(2);

    bytes.push_back(static_cast<uint8_t>(data >> 8));
    bytes.push_back(static_cast<uint8_t>(data & 0xFF));

    return bytes;
}

std::vector<uint8_t> net::bytes::toBytes32(const uint32_t data) {
    std::vector<uint8_t> bytes;
    bytes.reserve(4);

    bytes.push_back(static_cast<uint8_t>(data >> 24));
    bytes.push_back(static_cast<uint8_t>(data >> 16));
    bytes.push_back(static_cast<uint8_t>(data >> 8));
    bytes.push_back(static_cast<uint8_t>(data & 0xFF));

    return bytes;

};

void net::bytes::appendBytes16(const uint16_t data,std::vector<uint8_t>& byteBuffer) {
    byteBuffer.push_back(static_cast<uint8_t>(data >> 8));
    byteBuffer.push_back(static_cast<uint8_t>(data & 0xFF));

}

void net::bytes::appendBytes32(const uint32_t data,std::vector<uint8_t>& byteBuffer) {
    byteBuffer.push_back(static_cast<uint8_t>(data >> 24));
    byteBuffer.push_back(static_cast<uint8_t>(data >> 16));
    byteBuffer.push_back(static_cast<uint8_t>(data >> 8));
    byteBuffer.push_back(static_cast<uint8_t>(data & 0xFF));
}

void net::displayBytesAsText(const std::span<const uint8_t> bytes) {

    for (const unsigned char byte : bytes) {
        std::cout << std::dec<< byte;
    }
    std::cout<<'\n';
}
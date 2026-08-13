#include "Tcp.h"
#include "Net.h"
#include <span>
#include <vector>
#include <arpa/inet.h>
#include <random>

namespace {
    uint32_t randomIsn() {
        static std::mt19937 gen{std::random_device{}()};   // seeded once
        std::uniform_int_distribution<uint32_t> dist;       // default range = full uint32_t
        return dist(gen);
    }
}

void Tcp::swapSourceDestPort() {
    Net::swapBytes(&data_[SourcePort], &data_[DestPort], 2);
}

void Tcp::checkSum() {
    data_[Checksum] = 0;
    data_[Checksum + 1] = 0;
    const TcpPseudoHeader pseudoHeader{
        .sourceIP = htonl(sourceIP_),
        .destinationIP = htonl(destinationIP_),
        .zeroPadding = 0,
        .protocol = static_cast<uint8_t>(Net::protocol::TCP),
        .tcpLength = htons(static_cast<uint16_t>(data_.size_bytes()))
    };

    std::vector<uint8_t> checkSumTemp;
    checkSumTemp.reserve(sizeof(pseudoHeader) + data_.size_bytes());

    const auto ph{reinterpret_cast<const uint8_t *>(&pseudoHeader)};
    checkSumTemp.insert(checkSumTemp.end(), ph, ph + sizeof(pseudoHeader));

    checkSumTemp.insert(checkSumTemp.end(), data_.begin(), data_.end());

    const uint16_t checkSumValue{Net::checksum(checkSumTemp)};

    data_[Checksum] = checkSumValue >> 8;
    data_[Checksum + 1] = checkSumValue & 0xFF;
}

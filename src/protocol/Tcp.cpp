#include "protocol/Tcp.h"
#include "Net.h"
#include <span>
#include <vector>
#include <arpa/inet.h>
#include <random>
#include <unistd.h>

namespace {
    const std::string serverPayload{"Hi This is a test"};
}

uint32_t Tcp::randomIsn() {
    static std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist;
    return dist(gen);
}

ConnectionKey Tcp::getConnectionKey() const {
    ConnectionKey connectionKey{};
    connectionKey.remoteIp = getSourceIP();
    connectionKey.remotePort = getSourcePort();
    connectionKey.localIp = getDestinationIP();
    connectionKey.localPort = getDestPort();
    return connectionKey;
}

Tcp::Tcp(const std::span<uint8_t> data, const uint32_t sourceIP, const uint32_t destinationIP) : data_{data},
    sourceIP_{sourceIP},
    destinationIP_{destinationIP} {
};

void Tcp::swapSourceDestPort() {
    Net::swapBytes(&data_[SourcePort], &data_[DestPort], 2);
}

uint16_t Tcp::calculateCheckSum() {
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

    return checkSumValue;
}

void Tcp::reject() {
    swapSourceDestPort();
    resetFlags();
    setFlag(RST);
    calculateCheckSum();
}

bool Tcp::validateCheckSum() {
    const uint16_t checkSum{static_cast<uint16_t>(data_[Checksum] << 8 | data_[Checksum + 1])};
    const uint16_t calculatedCheckSum{calculateCheckSum()};

    return checkSum == calculatedCheckSum;
}

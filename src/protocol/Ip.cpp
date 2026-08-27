#include "protocol/Ip.h"
#include "Net.h"
#include <unistd.h>

void Ip::swapSourceDestination() {
    std::swap(sourceAddr_, destAddr_);
}

void Ip::resetCheckSum() {
    checkSum_ = 0;
}

uint16_t Ip::calculateCheckSum() {
    resetCheckSum();
    std::vector<uint8_t> ipDump{dumpHeader()};
    const uint16_t checkSum{net::checksum(ipDump)};
    checkSum_ = checkSum;
    return checkSum;
}

Ip::Ip(const std::span<uint8_t> data) : versionNHl_{net::bytes::read8(data, VersionIhl)},
                                        tos_{net::bytes::read8(data, Tos)},
                                        totalLength_{net::bytes::read16(data, TotalLength)},
                                        id_{net::bytes::read16(data, Id)},
                                        flagsFragment_{net::bytes::read16(data, FlagsFragment)},
                                        ttl_{net::bytes::read8(data, Ttl)},
                                        protocol_{net::bytes::read8(data, Protocol)},
                                        checkSum_{net::bytes::read16(data, Checksum)},
                                        sourceAddr_{net::bytes::read32(data, SourceAddr)},
                                        destAddr_{net::bytes::read32(data, DestAddr)},
                                        options_(data.begin() + IpDefaultHeaderSize,
                                                 data.begin() + getHeaderLength()) {
}

Ip::Ip(const IpConstructConfig &config) : versionNHl_{config.versionNHl},
                                          tos_{config.tos},
                                          totalLength_{config.totalLength},
                                          id_{config.id},
                                          flagsFragment_{config.flagsFragment},
                                          ttl_{config.ttl},
                                          protocol_{config.protocol},
                                          checkSum_{config.checkSum},
                                          sourceAddr_{config.sourceAddr},
                                          destAddr_{config.destAddr} {
}

uint16_t Ip::generateId() {
    static std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<uint16_t> dist;
    return dist(gen);
};

bool Ip::validateCheckSum() {
    const uint16_t checkSum{getChecksum()};
    const uint16_t calculatedCheckSum{calculateCheckSum()};
    return checkSum == calculatedCheckSum;
}

void Ip::appendInnerHeader(const std::span<uint8_t> data) {
    innerHeader_.reserve(data.size());
    innerHeader_.insert(innerHeader_.end(), data.begin(), data.end());

    setTotalLength(getHeaderLength() + data.size());
}

std::vector<uint8_t> Ip::dumpHeader() {
    using namespace net::bytes;

    std::vector<uint8_t> data;
    data.reserve(getHeaderLength());

    data.push_back(getVersionNHl());
    data.push_back(getTos());
    appendBytes16(getTotalLength(), data);
    appendBytes16(getId(), data);
    appendBytes16(getFlagsFragment(), data);
    data.push_back(getTtl());
    data.push_back(getProtocol());
    appendBytes16(getChecksum(), data);
    appendBytes32(getSourceAddr(), data);
    appendBytes32(getDestAddr(), data);

    data.insert(data.end(), options_.begin(), options_.end());

    return data;
}

std::vector<uint8_t> Ip::dump() {
    std::vector<uint8_t> data{dumpHeader()};
    data.insert(data.end(), innerHeader_.begin(), innerHeader_.end());

    return data;
}

std::ostream &operator<<(std::ostream &os, const Ip &h) {
    os << std::dec
            << "Version: " << static_cast<int>(h.getVersion()) << '\n'
            << "Header Length: " << static_cast<int>(h.getHeaderLength()) << '\n'
            << "Total Length: " << h.getTotalLength() << '\n'
            << "ID: " << h.getId() << '\n'
            << "TTL: " << static_cast<int>(h.getTtl()) << '\n'
            << "Protocol: " << static_cast<int>(h.getProtocol()) << '\n'
            << "Checksum: " << h.getChecksum() << '\n'
            << "Source Address: " << net::ipToString(h.getSourceAddr()) << '\n'
            << "Destination Address: " << net::ipToString(h.getDestAddr()) << '\n';

    return os;
}
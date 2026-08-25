#include "protocol/Ip.h"
#include "Net.h"
#include <unistd.h>

void Ip::swapSourceDestination() {
    std::swap(sourceAddr_, sourceAddr_);
}

void Ip::resetCheckSum() {
    checkSum_ = 0;
}

uint16_t Ip::calculateCheckSum() {
    resetCheckSum();
    std::vector<uint8_t> ipDump{dump()};
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
                                        options_(data.begin() + constants::ipHeaderSize,
                                                 data.begin() + getHeaderLength()) {
}

Ip::Ip(const IpConfig &config) : versionNHl_{config.versionNHl},
                                 tos_{config.tos},
                                 totalLength_{config.totalLength},
                                 id_{config.id},
                                 flagsFragment_{config.flagsFragment},
                                 ttl_{config.ttl},
                                 protocol_{config.protocol},
                                 checkSum_{config.checkSum},
                                 sourceAddr_{config.sourceAddr},
                                 destAddr_{config.destAddr} {
};


// void Ip::reply() {
//     swapSourceDestination();
//     calculateCheckSum();
//     ::write(tunFd_, data_.data(), data_.size_bytes());
// }

bool Ip::validateCheckSum() {
    const uint16_t checkSum{getChecksum()};
    const uint16_t calculatedCheckSum{calculateCheckSum()};
    return checkSum == calculatedCheckSum;
}

std::vector<uint8_t> Ip::dump() {
    std::vector<uint8_t> data;
    data.reserve(getHeaderLength());

    data.push_back(getVersionNHl());
    data.push_back(getTos());
    net::bytes::appendBytes16(getTotalLength(), data);
    net::bytes::appendBytes16(getId(), data);
    net::bytes::appendBytes16(getFlagsFragment(), data);
    data.push_back(getTtl());
    data.push_back(getProtocol());
    net::bytes::appendBytes16(getChecksum(), data);
    net::bytes::appendBytes32(getSourceAddr(), data);
    net::bytes::appendBytes32(getDestAddr(), data);

    data.insert(data.end(), options_.begin(), options_.end());

    return data;
}

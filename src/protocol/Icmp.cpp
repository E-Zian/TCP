#include "protocol/Icmp.h"
#include "Net.h"

Icmp::Icmp(const std::span<uint8_t> data) : type_{net::bytes::read8(data, Type)}, code_{net::bytes::read8(data, Code)},
                                            checkSum_{net::bytes::read16(data, CheckSum)},
                                            identifier_{net::bytes::read16(data, Identifier)},
                                            sequence_{net::bytes::read16(data, Sequence)},
                                            payload_{data.begin() + Data, data.end()} {
}


Icmp::Icmp(const IcmpConstructConfig &config) : type_{config.type}, code_{config.code}, checkSum_{config.checksum},
                                       identifier_{config.identifier}, sequence_{config.sequence} {
}

void Icmp::calculateChecksum() {
    checkSum_ = 0;

    const uint16_t checkSum{net::checksum(dump())};

    checkSum_ = checkSum;
}

std::vector<uint8_t> Icmp::dump() const {
    using namespace net::bytes;
    std::vector<uint8_t> data;
    data.push_back(getType());
    data.push_back(getCode());
    appendBytes16(getCheckSum(), data);
    appendBytes16(getIdentifier(), data);
    appendBytes16(getSequence(), data);

    data.insert(data.end(), payload_.begin(), payload_.end());

    return data;
}

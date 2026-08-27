#include "protocol/Tcp.h"
#include "Net.h"
#include <span>
#include <vector>
#include <arpa/inet.h>
#include <random>
#include <iostream>

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

bool Tcp::setOptions(const std::span<uint8_t> options)  {
    const uint8_t offset {static_cast<uint8_t>((options.size()+20)/4)};

    if (offset & 0xf0) {
        return false;
    }

    options_.assign(options.begin(), options.end());
    dataOffset_ = offset << 4;

    return true;
}

std::vector<uint8_t> Tcp::dump() const {
    using namespace net::bytes;
    std::vector<uint8_t> data{};

    // change in future to reserve the mss value
    data.reserve(1500);
    appendBytes16(getSourcePort(), data);
    appendBytes16(getDestPort(), data);
    appendBytes32(getSeqNumber(), data);
    appendBytes32(getAckNumber(), data);
    data.push_back(getDataOffsetByte());
    data.push_back(getFlags());
    appendBytes16(getWindow(), data);
    appendBytes16(getCheckSum(), data);
    appendBytes16(getUrgentPtr(), data);
    data.insert(data.end(), options_.begin(), options_.end());
    data.insert(data.end(), payload_.begin(), payload_.end());

    return data;
}


Tcp::Tcp(std::span<uint8_t> data, const uint32_t sourceIP, const uint32_t destinationIP) : sourceIP_{sourceIP},
                                                                                           destinationIP_{destinationIP},
                                                                                           sourcePort_{net::bytes::read16(data, SourcePort)},
                                                                                           destinationPort_{net::bytes::read16(data, DestPort)},
                                                                                           seqNum_{net::bytes::read32(data, SeqNum)},
                                                                                           ackNum_{net::bytes::read32(data, AckNum)},
                                                                                           dataOffset_{net::bytes::read8(data, DataOffset)},
                                                                                           flags_{data[Flags]},
                                                                                           windowSize_{net::bytes::read16(data, Window)},
                                                                                           checkSum_{net::bytes::read16(data, CheckSum)},
                                                                                           urgentPtr_{net::bytes::read16(data, UrgentPtr)},
                                                                                           options_{data.begin() + 20, data.begin() + getDataOffset()},
                                                                                           payload_(data.begin() + getDataOffset(), data.end()) {
}

Tcp::Tcp(const TcpConstructConfig &tcpConstructConfig) : sourceIP_{tcpConstructConfig.sourceIP},
                                                         destinationIP_{tcpConstructConfig.destinationIP},
                                                         sourcePort_{tcpConstructConfig.sourcePort},
                                                         destinationPort_{tcpConstructConfig.destinationPort},
                                                         seqNum_{tcpConstructConfig.seqNum},
                                                         ackNum_{tcpConstructConfig.ackNum},
                                                         dataOffset_{
                                                             static_cast<uint8_t>(tcpConstructConfig.dataOffset)
                                                         },
                                                         flags_{tcpConstructConfig.flags},
                                                         windowSize_{tcpConstructConfig.windowSize},
                                                         checkSum_{},
                                                         urgentPtr_{tcpConstructConfig.urgentPtr} {
};


void Tcp::swapSourceDestPort() {
    std::swap(sourcePort_, destinationPort_);
}

uint16_t Tcp::calculateCheckSum() {
    checkSum_ = 0;

    const size_t length{payload_.size() + getDataOffset()};

    const TcpPseudoHeader pseudoHeader{
        .sourceIP = htonl(sourceIP_),
        .destinationIP = htonl(destinationIP_),
        .zeroPadding = 0,
        .protocol = static_cast<uint8_t>(net::protocol::TCP),
        .tcpLength = htons(static_cast<uint16_t>(length))
    };

    std::vector<uint8_t> checkSumTemp;
    checkSumTemp.reserve(sizeof(pseudoHeader) + length);

    const auto ph{reinterpret_cast<const uint8_t *>(&pseudoHeader)};

    checkSumTemp.insert(checkSumTemp.end(), ph, ph + sizeof(pseudoHeader));

    std::vector<uint8_t> byteDump{dump()};

    checkSumTemp.insert(checkSumTemp.end(), byteDump.begin(), byteDump.end());

    const uint16_t checkSumValue{net::checksum(checkSumTemp)};

    checkSum_ = checkSumValue;

    return checkSumValue;
}

void Tcp::abort() {
    resetFlags();
    setFlag(RST);
    payload_.clear();
    options_.clear();
    calculateCheckSum();
}

bool Tcp::validateCheckSum() {
    const uint16_t checkSum{checkSum_};
    const uint16_t calculatedCheckSum{calculateCheckSum()};

    return checkSum == calculatedCheckSum;
}

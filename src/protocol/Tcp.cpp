#include "protocol/Tcp.h"
#include "Net.h"
#include <span>
#include <vector>
#include <arpa/inet.h>
#include <random>
#include <unistd.h>

namespace {
    uint32_t randomIsn() {
        static std::mt19937 gen{std::random_device{}()};
        std::uniform_int_distribution<uint32_t> dist;
        return dist(gen);
    }

    const std::string serverPayload{"Hi This is a test"};
}

Tcp::Tcp(const std::span<uint8_t> data, const uint32_t sourceIP, const uint32_t destinationIP,
         const int tunDeviceFD) : data_{data},
                                  sourceIP_{sourceIP},
                                  destinationIP_{destinationIP},
                                  tunDeviceFD_{tunDeviceFD},
                                  seqNum_{randomIsn()},
                                  ackNum_{getSeqNumber()},
                                  clientSeqNum_{getSeqNumber()},
                                  clientAckNum_{getAckNumber()} {
    if (getFlags() & (FIN | RST | PSH | ACK | URG) || !(getFlags() & (SYN))) {
        reject();
        return;
    }

    sendSynAck();
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

void Tcp::resetFlags() {
    data_[Flags] = 0;
}

void Tcp::setFlag(const Flag flag) {
    data_[Flags] = data_[Flags] | static_cast<uint8_t>(flag);
}

bool Tcp::validateCheckSum() {
    const uint16_t checkSum{static_cast<uint16_t>(data_[Checksum] << 8 | data_[Checksum + 1])};
    const uint16_t calculatedCheckSum{calculateCheckSum()};

    return checkSum == calculatedCheckSum;
}

// void Tcp::sendSynAck() {
//     ackNum_ += 1;
//     swapSourceDestPort();
//     setAck(ackNum_);
//     setSeq(seqNum_);
//     calculateCheckSum();
//
//     ::write(tunDeviceFD_, data_.data(), data_.size());
// }

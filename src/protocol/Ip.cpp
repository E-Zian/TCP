#include "protocol/Ip.h"
#include "Net.h"

void Ip::swapSourceDestination() {
    Net::swapBytes(&data_[SourceAddr], &data_[DestAddr],4);
}

void Ip::resetCheckSum() {
    data_[Checksum] = 0;
    data_[Checksum+1] = 0;
}

uint16_t Ip::calculateCheckSum() {
    resetCheckSum();
    const uint16_t checkSum{ Net::checksum(data_.first(header_len())) };
    data_[Checksum] = checkSum >> 8;
    data_[Checksum+1] = checkSum & 0xff;
    return checkSum;
}

bool Ip::validateCheckSum() {
    const uint16_t checkSum{static_cast<uint16_t>(data_[Checksum] << 8 | (data_[Checksum+1] & 0xff))};
    const uint16_t calculatedCheckSum{calculateCheckSum()};
    return checkSum == calculatedCheckSum;
}


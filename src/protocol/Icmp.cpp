#include "protocol/Icmp.h"
#include "Net.h"

void Icmp::calculateChecksum() const {
    data_[Checksum] = 0;
    data_[Checksum+1] = 0;

    const uint16_t checkSum{ Net::checksum(data_) };

    data_[Checksum] = checkSum >> 8;
    data_[Checksum+1] = checkSum & 0xff;
}
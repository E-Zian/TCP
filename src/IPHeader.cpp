#include "IPHeader.h"
#include "Net.h"

void IPHeader::swapSourceDestination() {
    Net::swapBytes(&data_[SourceAddr], &data_[DestAddr],4);
}

void IPHeader::resetChecksum() {
    data_[Checksum] = 0;
    data_[Checksum+1] = 0;
}

void IPHeader::calculateChecksum() {
    resetChecksum();
    const uint16_t checkSum{ Net::checksum(data_) };
    data_[Checksum] = checkSum >> 8;
    data_[Checksum+1] = checkSum & 0xff;
}


#include "IPHeader.h"
#include "Net.h"

void IPHeader::swapSourceDestination() {
    const uint32_t temp {source_addr_};
    source_addr_ = dest_addr_;
    dest_addr_ = temp;
}


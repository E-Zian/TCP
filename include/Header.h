#ifndef TCP_HEADER_H
#define TCP_HEADER_H
#include <cstdint>

struct IPv4Header {
    uint8_t  version_ihl;      // byte 0: version(4) + header length(4) packed
    uint8_t  tos;              // byte 1
    uint16_t total_length;     // bytes 2–3
    uint16_t id;               // bytes 4–5
    uint16_t flags_fragment;   // bytes 6–7
    uint8_t  ttl;              // byte 8
    uint8_t  protocol;         // byte 9
    uint16_t checksum;         // bytes 10–11
    uint32_t source_addr;      // bytes 12–15
    uint32_t dest_addr;        // bytes 16–19

    // small accessors for the packed first byte:
    uint8_t version()    const { return version_ihl >> 4; }
    uint8_t header_len() const { return (version_ihl & 0x0F) * 4; }
} __attribute__((packed));

static_assert(sizeof(IPv4Header) == 20);   // verify no padding snuck in

#endif //TCP_HEADER_H

#ifndef IPV4_H
#define IPV4_H
#include <cstdint>
#include <iosfwd>
#include <netinet/in.h>
#include <ostream>
#include <string>
class IPv4Header {
    public:
    [[nodiscard]] uint8_t  version()      const { return version_ihl_ >> 4; }
    [[nodiscard]] uint8_t  header_len()   const { return (version_ihl_ & 0x0F) * 4; }
    [[nodiscard]] uint16_t total_length() const { return ntohs(total_length_); }   // convert once, here
    [[nodiscard]] uint16_t id()           const { return ntohs(id_); }
    [[nodiscard]] uint8_t  ttl()          const { return ttl_; }        // 1 byte → no conversion
    [[nodiscard]] uint8_t  protocol()     const { return protocol_; }   // 1 byte → no conversion
    [[nodiscard]] uint16_t checksum()     const { return ntohs(checksum_); }
    [[nodiscard]] uint32_t source_addr()  const { return source_addr_; }  // kept raw (see note)
    [[nodiscard]] uint32_t dest_addr()    const { return dest_addr_; }

    private:
    uint8_t  version_ihl_{};      // byte 0: version(4) + header length(4) packed
    uint8_t  tos_{};              // byte 1
    uint16_t total_length_{};     // bytes 2–3
    uint16_t id_{};               // bytes 4–5
    uint16_t flags_fragment_{};   // bytes 6–7
    uint8_t  ttl_{};              // byte 8
    uint8_t  protocol_{};         // byte 9
    uint16_t checksum_{};         // bytes 10–11
    uint32_t source_addr_{};      // bytes 12–15
    uint32_t dest_addr_{};        // bytes 16–19

    // small accessors for the packed first byte:

} __attribute__((packed));

static std::string ip_to_string(const uint32_t net_order) {
    const uint32_t x = ntohl(net_order);
    return std::to_string((x >> 24) & 0xFF) + '.' +
           std::to_string((x >> 16) & 0xFF) + '.' +
           std::to_string((x >>  8) & 0xFF) + '.' +
           std::to_string( x        & 0xFF);
}

std::ostream& operator<<(std::ostream& os, const IPv4Header& h) {
    os << std::dec                                                   // ← force DECIMAL (std::hex is sticky!)
       << "Version: "             << static_cast<int>(h.version())    << '\n'   // cast uint8_t → int
       << "Header Length: "       << static_cast<int>(h.header_len()) << '\n'
       << "Total Length: "        << h.total_length()                 << '\n'
       << "ID: "                  << h.id()                           << '\n'
       << "TTL: "                 << static_cast<int>(h.ttl())         << '\n'
       << "Protocol: "            << static_cast<int>(h.protocol())    << '\n'
       << "Checksum: "            << h.checksum()                      << '\n'
       << "Source Address: "      << ip_to_string(h.source_addr())     << '\n'   // dotted-decimal
       << "Destination Address: " << ip_to_string(h.dest_addr())       << '\n';
    return os;
}
#endif

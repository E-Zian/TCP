#ifndef IPV4_H
#define IPV4_H
#include <cstdint>
#include <iosfwd>
#include <netinet/in.h>
#include <ostream>
#include <span>
#include <string>

class Ip {
public:
    explicit Ip(const std::span<uint8_t> data) {
        data_ = {data.begin(), data.begin()+(data[TotalLength] << 8 | data[TotalLength+1])};
    };

    enum Offset : std::size_t {
        VersionIhl    = 0, // byte 0 : (0-4 bit) version, (4-8 bit) header length
        Tos           = 1, // byte 1
        TotalLength   = 2, // bytes 2–3
        Id            = 4, // bytes 4–5
        FlagsFragment = 6, // bytes 6–7
        Ttl           = 8, // byte 8
        Protocol      = 9, // byte 9
        Checksum      = 10, // bytes 10–11
        SourceAddr    = 12, // bytes 12–15
        DestAddr      = 16, // bytes 16–19
    };

    [[nodiscard]] uint8_t version() const { return data_[VersionIhl] >> 4; }
    [[nodiscard]] uint8_t header_len() const { return (data_[VersionIhl] & 0x0F) * 4; }
    [[nodiscard]] uint8_t tos() const { return data_[Tos]; }
    [[nodiscard]] uint16_t total_length() const { return data_[TotalLength] << 8 | data_[TotalLength+1]; }
    [[nodiscard]] uint16_t id() const { return data_[Id] << 8 | data_[Id+1]; }
    [[nodiscard]] uint16_t flags_fragment() const { return data_[FlagsFragment] << 8 | data_[FlagsFragment+1]; }
    [[nodiscard]] uint8_t ttl() const { return data_[Ttl]; }
    [[nodiscard]] uint8_t protocol() const { return data_[Protocol]; }
    [[nodiscard]] uint16_t checksum() const { return data_[Checksum] << 8 | data_[Checksum+1]; }
    [[nodiscard]] uint32_t source_addr() const {
        return (static_cast<uint32_t>(data_[SourceAddr]) << 24)
             | (data_[SourceAddr+1] << 16) | (data_[SourceAddr+2] << 8) | data_[SourceAddr+3];
    }
    [[nodiscard]] uint32_t dest_addr() const {
        return (static_cast<uint32_t>(data_[DestAddr]) << 24)
             | (data_[DestAddr+1] << 16) | (data_[DestAddr+2] << 8) | data_[DestAddr+3];
    }
    [[nodiscard]] std::span<const uint8_t> get_data() const {
        return data_;
    }
    void swapSourceDestination();
    void calculateChecksum();

private:
    std::span<uint8_t> data_;


    void resetChecksum();

};

static std::string ip_to_string(const uint32_t address) {
    return std::to_string((address >> 24) & 0xFF) + '.' +
           std::to_string((address >> 16) & 0xFF) + '.' +
           std::to_string((address >> 8) & 0xFF) + '.' +
           std::to_string(address & 0xFF);
}

inline std::ostream &operator<<(std::ostream &os, const Ip &h) {
    os << std::dec // ← force DECIMAL (std::hex is sticky!)
            << "Version: " << static_cast<int>(h.version()) << '\n' // cast uint8_t → int
            << "Header Length: " << static_cast<int>(h.header_len()) << '\n'
            << "Total Length: " << h.total_length() << '\n'
            << "ID: " << h.id() << '\n'
            << "TTL: " << static_cast<int>(h.ttl()) << '\n'
            << "Protocol: " << static_cast<int>(h.protocol()) << '\n'
            << "Checksum: " << h.checksum() << '\n'
            << "Source Address: " << ip_to_string(h.source_addr()) << '\n' // dotted-decimal
            << "Destination Address: " << ip_to_string(h.dest_addr()) << '\n';

    return os;
}

#endif

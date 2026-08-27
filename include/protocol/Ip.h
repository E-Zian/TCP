#ifndef IPV4_H
#define IPV4_H
#include "Net.h"
#include <iosfwd>
#include <netinet/in.h>
#include <ostream>
#include <span>
#include <string>
namespace constants {
    inline constexpr size_t ipHeaderSize { 20 };
}

struct IpConstructConfig {
    uint8_t versionNHl;
    uint8_t tos;
    uint16_t totalLength;
    uint16_t id;
    uint16_t flagsFragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checkSum;
    uint32_t sourceAddr;
    uint32_t destAddr;
};

class Ip {
public:
    explicit Ip(std::span<uint8_t> data);

    explicit Ip(const IpConstructConfig &config);

    enum Offset : std::size_t {
        VersionIhl = 0, // byte 0 : (0-4 bit) version, (4-8 bit) header length
        Tos = 1, // byte 1
        TotalLength = 2, // bytes 2–3
        Id = 4, // bytes 4–5
        FlagsFragment = 6, // bytes 6–7
        Ttl = 8, // byte 8
        Protocol = 9, // byte 9
        Checksum = 10, // bytes 10–11
        SourceAddr = 12, // bytes 12–15
        DestAddr = 16, // bytes 16–19
    };

    [[nodiscard]] uint8_t getVersionNHl() const { return versionNHl_ ;}
    [[nodiscard]] uint8_t getVersion() const { return versionNHl_ >> 4; }
    [[nodiscard]] uint8_t getHeaderLength() const { return (versionNHl_ & 0x0F) * 4; }
    [[nodiscard]] uint8_t getTos() const { return tos_; }
    [[nodiscard]] uint16_t getTotalLength() const { return totalLength_; }
    [[nodiscard]] uint16_t getId() const { return id_; }
    [[nodiscard]] uint16_t getFlagsFragment() const { return flagsFragment_; }
    [[nodiscard]] uint8_t getTtl() const { return ttl_; }
    [[nodiscard]] uint8_t getProtocol() const { return protocol_; }
    [[nodiscard]] uint16_t getChecksum() const { return checkSum_; }
    [[nodiscard]] uint32_t getSourceAddr() const { return sourceAddr_; }
    [[nodiscard]] uint32_t getDestAddr() const { return destAddr_; }
    [[nodiscard]] std::vector<uint8_t> getOptions() const { return options_; }

    void setTotalLength(const uint16_t totalLength) { totalLength_ = totalLength; }

    bool validateCheckSum();

    void appendInnerHeader(std::span<uint8_t> data);

    std::vector<uint8_t> dump();

    std::vector<uint8_t> dumpAll();

    void swapSourceDestination();

    uint16_t calculateCheckSum();

private:
    uint8_t versionNHl_;
    uint8_t tos_;
    uint16_t totalLength_;
    uint16_t id_;
    uint16_t flagsFragment_;
    uint8_t ttl_;
    uint8_t protocol_;
    uint16_t checkSum_;
    uint32_t sourceAddr_;
    uint32_t destAddr_;

    std::vector<uint8_t> options_;

    std::vector<uint8_t> innerHeader_;

    void resetCheckSum();

};


inline std::ostream &operator<<(std::ostream &os, const Ip &h) {
    os << std::dec
            << "Version: " << static_cast<int>(h.getVersion()) << '\n'
            << "Header Length: " << static_cast<int>(h.getHeaderLength()) << '\n'
            << "Total Length: " << h.getTotalLength() << '\n'
            << "ID: " << h.getId() << '\n'
            << "TTL: " << static_cast<int>(h.getTtl()) << '\n'
            << "Protocol: " << static_cast<int>(h.getProtocol()) << '\n'
            << "Checksum: " << h.getChecksum() << '\n'
            << "Source Address: " << net::ipToString(h.getSourceAddr()) << '\n'
            << "Destination Address: " << net::ipToString(h.getDestAddr()) << '\n';

    return os;
}

#endif

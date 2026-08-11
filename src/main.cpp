#include "IPV4Header.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <ostream>
#include <string>
#include <system_error>
#include <span>

// Opens /dev/net/tun and attaches this program to the interface `dev`
// (e.g. "tun0"). Returns a file descriptor you read()/write() packets on.
// Throws std::system_error if anything goes wrong.

namespace {
    int tun_alloc(const std::string &dev) {
        const int fd = ::open("/dev/net/tun", O_RDWR);
        if (fd < 0)
            throw std::system_error(errno, std::generic_category(),
                                    "opening /dev/net/tun");

        ifreq ifr{};
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

        std::strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ - 1);

        if (::ioctl(fd, TUNSETIFF, &ifr) < 0) {
            ::close(fd);
            throw std::system_error(errno, std::generic_category(),
                                    "ioctl(TUNSETIFF) on " + dev);
        }

        return fd;
    }

    uint16_t checksum(const std::span<const uint8_t> data) {
        uint32_t sum{};
        for (int i{}; i + 1 < data.size(); i += 2) {
            sum += (data[i] << 8 | data[i + 1]);
        }

        if (data.size() & 1) {
            sum += data[data.size() - 1] << 8;
        }

        while (sum >> 16) {
            sum = (sum >> 16) + (sum & 0xffff);
        }
        return static_cast<uint16_t>(~sum);
    }
}

int main() {
    constexpr int MTU{1500};
    uint8_t buffer[MTU];

    const int tun0Fd{tun_alloc("tun0")};
    std::cout << "now listening for packets ... " << '\n';

    while (true) {
        IPv4Header header{};
        const ssize_t bytes{::read(tun0Fd, buffer, sizeof(buffer))};

        std::memcpy(&header, buffer, sizeof(header));
        if (header.version() != 4) {
            std::cout << "header version " << header.version() << " not supported, proceeding to next packet" << '\n';
            continue;
        }

        if (bytes < 0) {
            std::perror("read");
            return 1;
        }

        std::cout << "Received packet of " << bytes << " bytes:\n";
        for (ssize_t i = 0; i < bytes; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(buffer[i]) << ' ';
            if ((i + 1) % 16 == 0) std::cout << '\n'; // 16 bytes per row
        }


        std::cout << '\n';
        std::cout << header << '\n';

        if (header.protocol() == static_cast<uint8_t>(protocol::ICMP)) {
            std::span<uint8_t> icmp{buffer + header.header_len(), buffer + header.total_length()};
            if (icmp[0] != 8) continue;

            icmp[0] = 0;
            // Resetting check sum
            icmp[2] = 0;
            icmp[3] = 0;

            const uint16_t checkSumVal{checksum(icmp)};
            icmp[2] = checkSumVal >> 8;
            icmp[3] = checkSumVal & 0xff;

            // swap source and destination
            for (int i = 0; i < 4; ++i) {
                uint8_t t = buffer[12 + i];
                buffer[12 + i] = buffer[16 + i];
                buffer[16 + i] = t;
            }

            // Check sum for ip packet
            buffer[10] = 0; buffer[11] = 0;
            const uint16_t ipCk{ checksum({buffer, buffer + header.header_len()}) };
            buffer[10] = ipCk >> 8;
            buffer[11] = ipCk & 0xff;


            ::write(tun0Fd, buffer, bytes);
            std::cout << "Reply sent for ping" <<'\n';
        }
    }
}

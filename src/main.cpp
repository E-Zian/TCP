#include "IPHeader.h"
#include "Net.h"
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

}

int main() {
    constexpr int MTU{1500};
    uint8_t buffer[MTU];

    const int tun0Fd{tun_alloc("tun0")};
    std::cout << "now listening for packets ... " << '\n';

    while (true) {
        IPHeader ipHeader{};
        const ssize_t bytes{::read(tun0Fd, buffer, sizeof(buffer))};

        std::memcpy(&ipHeader, buffer, sizeof(ipHeader));

        if (ipHeader.version() != 4) {
            std::cout << "header version " << ipHeader.version() << " not supported, proceeding to next packet" << '\n';
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
        std::cout << ipHeader << '\n';

        if (ipHeader.protocol() == static_cast<uint8_t>(Net::protocol::ICMP)) {
            std::span<uint8_t> icmp{buffer + ipHeader.header_len(), buffer + ipHeader.total_length()};
            if (icmp[0] != 8) continue;

            icmp[0] = 0;
            // Resetting check sum
            icmp[2] = 0;
            icmp[3] = 0;

            const uint16_t checkSumVal{Net::checksum(icmp)};
            icmp[2] = checkSumVal >> 8;
            icmp[3] = checkSumVal & 0xff;

            // swap source and destination
            Net::swapBytes(buffer+12,buffer+16,4);

            // Check sum for ip packet
            // Resetting checksum
            buffer[10] = 0;
            buffer[11] = 0;
            const uint16_t ipCs{ Net::checksum({buffer, buffer + ipHeader.header_len()}) };
            buffer[10] = ipCs >> 8;
            buffer[11] = ipCs & 0xff;


            ::write(tun0Fd, buffer, bytes);
            std::cout << "Reply sent for ping" <<'\n';
        }
    }
}

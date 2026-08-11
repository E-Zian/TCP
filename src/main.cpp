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

// Opens /dev/net/tun and attaches this program to the interface `dev`
// (e.g. "tun0"). Returns a file descriptor you read()/write() packets on.
// Throws std::system_error if anything goes wrong.
int tun_alloc(const std::string& dev)
{
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

int main() {
    constexpr int MTU {1500};
    uint8_t buffer[MTU];

    const int tun0Fd {tun_alloc("tun0")};
    std::cout<< "now listening for packets ... " <<'\n';
    while (true) {
        IPv4Header header{};
        ssize_t bytes {::read(tun0Fd, buffer, sizeof(buffer))};

        std::memcpy(&header, buffer, sizeof(header));
        if (header.version() != 4) continue;
        
        if (bytes < 0) {
            std::perror("read");
            return 1;
        }

        std::cout << "Got a packet of " << bytes << " bytes:\n";
        for (ssize_t i = 0; i < bytes; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buffer[i]) << ' ';
            if ((i + 1) % 16 == 0) std::cout << '\n';   // 16 bytes per row
        }
        std::cout << '\n';
        std::cout <<"the total length is :"<< header.total_length() << '\n';
        std::cout <<header << '\n';
        std::cout << std::dec << '\n'<<'\n';
    }

}
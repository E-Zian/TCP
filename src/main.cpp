#include <fcntl.h>        // open(), O_RDWR
#include <unistd.h>       // close(), read(), write()
#include <sys/ioctl.h>    // ioctl()
#include <net/if.h>       // struct ifreq, IFNAMSIZ
#include <linux/if_tun.h> // IFF_TUN, IFF_NO_PI, TUNSETIFF
#include <iostream>
#include <iomanip>     // std::hex, std::setw, std::setfill
#include <cstdint>     // uint8_t
#include <cstring>        // std::strncpy
#include <cerrno>         // errno

#include <ostream>
#include <string>
#include <system_error>   // std::system_error

// Opens /dev/net/tun and attaches this program to the interface `dev`
// (e.g. "tun0"). Returns a file descriptor you read()/write() packets on.
// Throws std::system_error if anything goes wrong.
int tun_alloc(const std::string& dev)
{
    int fd = ::open("/dev/net/tun", O_RDWR);
    if (fd < 0)
        throw std::system_error(errno, std::generic_category(),
                                "opening /dev/net/tun");

    ifreq ifr{};                            // zero-initialised — no memset (your idea!)
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;    // layer-3 TUN, no 4-byte prefix

    std::strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ - 1);  // safe, bounded copy

    if (::ioctl(fd, TUNSETIFF, &ifr) < 0) {
        ::close(fd);
        throw std::system_error(errno, std::generic_category(),
                                "ioctl(TUNSETIFF) on " + dev);
    }

    return fd;
}

int main() {
    constexpr int MTU {1500};          // fixed size (see notes below)
    uint8_t buffer[MTU];               // bytes, not chars

    const int tun0Fd {tun_alloc("tun0")};
    while (true) {
        ssize_t bytes {::read(tun0Fd, buffer, sizeof(buffer))};
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
        std::cout << std::dec << '\n'<<'\n';
    }

}
//
// Created by LeeEeZian on 26/8/2026.
//

#include "io/TunDevice.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <cstring>
#include <cerrno>
#include <string>
#include <system_error>
#include <unistd.h>
#include <net/if.h>

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

TunDevice::TunDevice(const std::string &deviceName) : fd_{tun_alloc(deviceName)} {
}

ssize_t TunDevice::read(std::span<uint8_t> buffer) const {
    return ::read(fd_, buffer.data(), buffer.size_bytes());
}

void TunDevice::write(const std::span<uint8_t> buffer) const {
    ::write(fd_, buffer.data(), buffer.size_bytes());
}

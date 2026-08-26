//
// Created by LeeEeZian on 26/8/2026.
//

#ifndef TCP_TUNDEVICE_H
#define TCP_TUNDEVICE_H
#include <cstdint>
#include <string>
#include <span>

class TunDevice {
public:
    explicit TunDevice(const std::string &deviceName);

    [[nodiscard]] int getFd() const {
        return fd_;
    }

    [[nodiscard]] ssize_t read(std::span<uint8_t> buffer) const;

    void write(std::span<uint8_t> buffer) const;

private:
    const int fd_;
};

#endif //TCP_TUNDEVICE_H

//
// Created by LeeEeZian on 19/8/2026.
//
#pragma once
#ifndef TCP_CONNECTIONKEY_H
#define TCP_CONNECTIONKEY_H

#include <cstdint>
#include <functional>

struct ConnectionKey {
    uint32_t remoteIp;
    uint16_t remotePort;
    uint32_t localIp;
    uint16_t localPort;

    bool operator==(const ConnectionKey&) const = default;
};

template<>
struct std::hash<ConnectionKey> {
    std::size_t operator()(const ConnectionKey& k) const noexcept {
        std::size_t h = std::hash<uint32_t>{}(k.remoteIp);
        h = h * 31 + std::hash<uint16_t>{}(k.remotePort);
        h = h * 31 + std::hash<uint32_t>{}(k.localIp);
        h = h * 31 + std::hash<uint16_t>{}(k.localPort);
        return h;
    }
};


#endif //TCP_CONNECTIONKEY_H

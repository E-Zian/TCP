//
// Created by LeeEeZian on 11/8/2026.
//

#ifndef TCP_IPV4_H
#define TCP_IPV4_H
#include "IPV4Header.h"

class Ipv4 {
public:
    Ipv4(const IPv4Header &header) : header_{header} {
    }

private:
    IPv4Header header_{};
    uint8_t* payload_{nullptr};
};
#endif //TCP_IPV4_H

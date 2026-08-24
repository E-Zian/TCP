#include "protocol/Ip.h"
#include "Net.h"
#include "state/TcpConnectionTable.h"
#include "protocol/Tcp.h"
#include "protocol/Icmp.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <iostream>
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
    try {
        TcpConnectionTable tcpTable{};

        uint8_t buffer[constants::MAX_TRANSMISSION_UNIT];

        const int tun0Fd{tun_alloc("tun0")};

        std::cout << "now listening for packets ... " << '\n';

        while (true) {
            const ssize_t bytes{::read(tun0Fd, buffer, sizeof(buffer))};

            if (bytes < 0) {
                std::perror("read");
                return 1;
            }

            if (bytes < 20) {
                std::perror("read bytes too short");
                continue;
            };

            if (const uint8_t ipHeaderVersion{static_cast<uint8_t>(buffer[Ip::Offset::VersionIhl] >> 4)};
                ipHeaderVersion != 4) {
                std::cout << "Ip header version " << static_cast<int>(ipHeaderVersion) <<
                        " not supported, proceeding to next packet"
                        << '\n';
                continue;
            }

            Ip ipPacket{buffer, tun0Fd};

            net::displayBytes(ipPacket.get_data());

            std::cout << '\n';
            std::cout << ipPacket << '\n';

            const std::span<uint8_t> innerHeader{buffer + ipPacket.header_len(), buffer + ipPacket.total_length()};
            switch (static_cast<net::protocol>(ipPacket.protocol())) {
                case net::protocol::ICMP: {
                    Icmp icmp{innerHeader};
                    if (icmp.type() != Icmp::RequestType::echo_request) continue;

                    icmp.data_[Icmp::Offset::Type] = Icmp::RequestType::echo_reply;
                    icmp.calculateChecksum();

                    ipPacket.reply();

                    std::cout << "\nPing reply sent\n";
                    break;
                }
                case net::protocol::TCP: {
                    Tcp tcp{innerHeader, ipPacket.source_addr(), ipPacket.dest_addr()};
                    ConnectionKey connectionKey{tcp.getConnectionKey()};

                    if (!tcpTable.checkExistingConnection(connectionKey)) {
                        TcpConnection connection{};
                        tcpTable.addConnection(connectionKey, connection);
                    }

                    TcpConnection &connection{*tcpTable.getConnection(connectionKey)};
                    connection.handlePacket(ipPacket, tcp);

                    if (connection.getState() == TcpConnection::State::Closed) {
                        tcpTable.removeConnection(connectionKey);
                    }

                    break;
                }
                default:
                    std::cout << "Unrecognised Protocol" << '\n';
                    break;
            }
        }
    } catch (std::system_error &e) {
        std::perror(e.what());
    }
}

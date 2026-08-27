#include "protocol/Ip.h"
#include "Net.h"
#include "io/TunDevice.h"
#include "state/TcpConnectionTable.h"
#include "protocol/Tcp.h"
#include "protocol/Icmp.h"
#include <unistd.h>
#include <net/if.h>
#include <iostream>
#include <arpa/inet.h>
#include <ostream>
#include <string>
#include <system_error>
#include <span>


int main() {
    try {
        TcpConnectionTable tcpTable{};

        uint8_t buffer[constants::MAX_TRANSMISSION_UNIT];

        TunDevice tunDevice{"tun0"};

        std::cout << "now listening for packets ... " << '\n';

        while (true) {
            const ssize_t bytes{tunDevice.read(buffer)};

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

            // net::displayBytes(buffer);

            Ip ipPacket{buffer};
            // std::cout << "From Ip : " << '\n';
            // net::displayBytes(ipPacket.dump());

            const std::span<uint8_t> innerHeader{
                buffer + ipPacket.getHeaderLength(), buffer + ipPacket.getTotalLength()
            };
            switch (static_cast<net::protocol>(ipPacket.getProtocol())) {
                case net::protocol::ICMP: {
                    Icmp icmp{innerHeader};
                    if (icmp.getType() != Icmp::RequestType::echo_request) continue;

                    icmp.setType(Icmp::RequestType::echo_reply);
                    icmp.calculateChecksum();


                    std::vector<uint8_t> icmpByteBuffer{icmp.dump()};

                    ipPacket.swapSourceDestination();
                    ipPacket.appendInnerHeader(icmpByteBuffer);

                    ipPacket.calculateCheckSum();

                    std::vector<uint8_t> ipPacketBytes{ipPacket.dump()};

                    tunDevice.write(ipPacketBytes);

                    std::cout << "\nPing reply sent\n";
                    break;
                }
                case net::protocol::TCP: {
                    Tcp tcp{innerHeader, ipPacket.getSourceAddr(), ipPacket.getDestAddr()};
                    ConnectionKey connectionKey{tcp.getConnectionKey()};

                    TcpConnection &connection{tcpTable.getOrCreate(connectionKey)};

                    if (auto tcpToSend{connection.handlePacket(tcp)}; tcpToSend.has_value()) {

                        Ip ipToSend{connection.createIpBaseConfig()};

                        tunDevice.sendPacket(ipToSend,tcpToSend.value());
                    }

                    if (connection.getState() == TcpConnection::State::Closed) {
                        std::cout << "Connection closed for : "<< net::ipToString(connectionKey.remoteIp) << "::" << connectionKey.remotePort << "\n";
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

#include "state/TcpConnection.h"
#include <limits>
#include <cstdint>

TcpConnection::TcpConnection(const Tcp &tcp) : remoteIp_(tcp.getSourceIP()),
                                               remotePort_(tcp.getDestPort()),
                                               localIp_(tcp.getSourceIP()),
                                               localPort_(tcp.getSourcePort()),
                                               myNextSequence_{Tcp::randomIsn()},
                                               myNextExpectedSequence_{tcp.getSeqNumber()},
                                               myWindow_{std::numeric_limits<uint16_t>::max()},
                                               theirWindow_{tcp.getWindow()} {

}

void TcpConnection::handleTcpPacket(Tcp &tcp) {

}

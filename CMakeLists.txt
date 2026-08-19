cmake_minimum_required(VERSION 3.20)
project(tcp)

set(CMAKE_CXX_STANDARD 20)

set(sources
        src/main.cpp
        src/Net.cpp
        src/protocol/Ip.cpp
        src/protocol/Icmp.cpp
        src/protocol/Tcp.cpp
        src/state/TcpConnection.cpp
        src/state/TcpConnectionTable.cpp

)
set(includes
        include/protocol/Ip.h
        include/Net.h
        include/protocol/Icmp.h
        include/protocol/Tcp.h
        include/state/TcpConnection.h
        include/state/TcpConnectionTable.h
        include/model/ConnectionKey.h

)

add_executable(TCP ${sources} ${includes})

target_include_directories(TCP PUBLIC
        include

)
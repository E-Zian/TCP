cmake_minimum_required(VERSION 3.20)
project(tcp)

set(CMAKE_CXX_STANDARD 20)

set(sources
        src/main.cpp
)
set(includes
        include/protocol/Ip.h
        include/Net.h
        src/Net.cpp
        include/protocol/Icmp.h
        src/protocol/Ip.cpp
        src/protocol/Icmp.cpp
        src/protocol/Tcp.cpp
        include/protocol/Tcp.h
        src/state/TcpConnection.cpp
        include/state/TcpConnection.h

)

add_executable(TCP ${sources} ${includes})

target_include_directories(TCP PUBLIC
        include

)
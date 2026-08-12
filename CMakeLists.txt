cmake_minimum_required(VERSION 3.20)
project(tcp)

set(CMAKE_CXX_STANDARD 20)

set(sources
        src/main.cpp
)
set(includes
        include/IPHeader.h
        include/Net.h
        src/Net.cpp
        include/Icmp.h
        src/IPHeader.cpp
        src/Icmp.cpp

)

add_executable(TCP ${sources} ${includes})

target_include_directories(TCP PUBLIC include )
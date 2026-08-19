//
// Created by LeeEeZian on 19/8/2026.
//
#pragma once
#ifndef TCP_TCPCONNECTIONTABLE_H
#define TCP_TCPCONNECTIONTABLE_H

#include "TcpConnection.h"
#include "model/ConnectionKey.h"
#include <unordered_map>

class TcpConnectionTable {
    public:

    void addConnection(const ConnectionKey& connectionKey, TcpConnection& connection) ;

    bool checkExistingConnection(ConnectionKey connectionKey) const;

    TcpConnection* getConnection(const ConnectionKey& connectionKey);

    void removeConnection(const ConnectionKey& connectionKey);

    private:
    std::unordered_map<ConnectionKey, TcpConnection> connectionTable_;
};


#endif //TCP_TCPCONNECTIONTABLE_H

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

    void removeConnection(const ConnectionKey& connectionKey);

    TcpConnection& getOrCreate(const ConnectionKey& key) {
        return connectionTable_.try_emplace(key).first->second;
    }

    std::unordered_map<ConnectionKey, TcpConnection> getConnectionTable() const {
        return connectionTable_;
    }
    private:
    std::unordered_map<ConnectionKey, TcpConnection> connectionTable_;
};


#endif //TCP_TCPCONNECTIONTABLE_H

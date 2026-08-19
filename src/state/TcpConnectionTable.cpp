//
// Created by LeeEeZian on 19/8/2026.
//

#include "state/TcpConnectionTable.h"

void TcpConnectionTable::addConnection(const ConnectionKey& connectionKey, TcpConnection &connection) {
    connectionTable_.insert({connectionKey, std::move(connection)});
}

bool TcpConnectionTable::checkExistingConnection(const ConnectionKey connectionKey) const {
    return connectionTable_.contains(connectionKey);
}

TcpConnection * TcpConnectionTable::getConnection(const ConnectionKey &connectionKey) {
    const auto it = connectionTable_.find(connectionKey);
    if (it == connectionTable_.end()) {
        return nullptr;
    }
    return &(it->second);
}




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

void TcpConnectionTable::removeConnection(const ConnectionKey &connectionKey) {
    connectionTable_.erase(connectionKey);
}




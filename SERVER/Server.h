#pragma once
#ifndef NET_SERVER_CORE_H
#define NET_SERVER_CORE_H

#include "SocketManager.h"
#include <functional>
#include <atomic>

namespace net {
    using ClientHandler = std::function<void(DBSocket&&)>;

    class DBServer {
    public:
        explicit DBServer(uint16_t port);
        ~DBServer();

        void start(ClientHandler&& handler);
        void stop() noexcept;

    private:
        DBSocket listen_sock_;
        std::atomic<bool> running{ false };
        uint16_t port_;
    };
}

#endif // NET_SERVER_CORE_H
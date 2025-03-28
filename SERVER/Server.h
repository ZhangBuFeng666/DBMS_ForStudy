#pragma once
#ifndef NET_SERVER_CORE_H
#define NET_SERVER_CORE_H

#include <functional>
#include <atomic>

//namespace myServer {
//    using ClientHandler = std::function<void(mySocket::DBSocket&&)>;
//
//    class DBServer {
//    public:
//        explicit DBServer(uint16_t port);
//        ~DBServer();
//
//        void start(ClientHandler&& handler);
//        void stop() noexcept;
//
//    private:
//        mySocket::DBSocket listen_sock_;
//        std::atomic<bool> running{ false };
//        uint16_t port_;
//    };
//}
//
#include <mutex>
#include <unordered_map>
#include <thread>
#include <iostream>
#include "SocketManager.h"
#include "DiffPlat.h"


namespace myServer {
    class Server {
    private:
        std::mutex mutex_;
        std::unordered_map<int, std::thread> threads;
        std::unordered_map<int, mySocket::DBSocket> DBSockets;
        std::atomic<bool> running{ true };

    public:
        ~Server() {
            stop_all();
        }

        void add(int client_id, mySocket::DBSocket&& sock, std::thread&& thread);

        void remove(int client_id);

        void stop_all();

        bool is_running() const {
            return running;
        }
    };

    void client_handler(mySocket::DBSocket client_sock, int client_id, Server& my_server);
}
#endif // NET_SERVER_CORE_H
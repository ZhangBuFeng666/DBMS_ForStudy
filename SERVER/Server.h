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
//#include "SocketManager.h"
#include "Client.h"


namespace myServer {
    class Server {
    private:

        //std::mutex mtx_;

        std::mutex mutex_;
        std::unordered_map<int, std::thread> threads;
        std::unordered_map<int, std::unique_ptr<ClientSession>> clients;
        //std::unordered_map<int, mySocket::DBSocket> DBSockets;
        std::atomic<bool> running{ true };

    public:
        ~Server() {
            stop_all();
        }

        void main_controller();

        void add(int id, DBSocket&& sock, std::thread&& t);

        void remove(int client_id);

        void stop_all();

        bool is_running() const {
            return running;
        }
    };



}
#endif // NET_SERVER_CORE_H
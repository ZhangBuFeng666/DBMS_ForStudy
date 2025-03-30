#pragma once
#ifndef NET_SERVER_CORE_H
#define NET_SERVER_CORE_H

#include <functional>
#include <atomic>

#include <mutex>
#include <unordered_map>
#include <thread>
#include <iostream>
#include "Client.h"


namespace myServer {
    class Server {
    private:

        //std::mutex mtx_;
        static Server myServer;
        std::mutex mutex_s;
        std::unordered_map<int, std::thread> threads;
        std::unordered_map<int, std::unique_ptr<ClientSession>> clients;
        //std::unordered_map<int, mySocket::DBSocket> DBSockets;
        std::atomic<bool> running{ true };
        int client_id = 0;

    public:
        ~Server() {
            stop_all();
        }

        void main_controller();

        void add(int id, std::unique_ptr<ClientSession> client);

        void remove(int client_id);

        void stop_all();

        bool is_running() const {
            return running;
        }
    };



}
#endif // NET_SERVER_CORE_H
#pragma once
#ifndef NET_SERVER_CORE_H
#define NET_SERVER_CORE_H

#include <functional>
#include <atomic>

#include <mutex>
#include <unordered_map>
#include "Client.h"
#include"ThreadPool.h"



namespace myServer {
    class Server {
        friend class ClientSession;
    private:

        //std::mutex mtx_;
        std::mutex mutexes;
        //std::unordered_map<int, std::thread> threads;
        ThreadPool threadPool;
        std::unordered_map<int, std::unique_ptr<ClientSession>> clients;
        //std::unordered_map<int, mySocket::DBSocket> DBSockets;
        std::atomic<bool> running;
        int port;
        int client_id = 0;

    public:
        Server(int port, size_t threadCount)
            :port(port),
            threadPool(threadCount),
            running(true) {}

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
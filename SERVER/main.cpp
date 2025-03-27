// src/main.cpp
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include "SocketManager.h"
#include "DiffPlat.h"
#include"Tools.h"

using namespace net;

class ConnectionManager {
private:
    std::mutex mutex_;
    std::unordered_map<int, std::thread> threads;
    std::unordered_map<int, DBSocket> DBSockets;
    std::atomic<bool> running{ true };

public:
    ~ConnectionManager() {
        stop_all();
    }

    void add(int client_id, DBSocket&& sock, std::thread&& thread) {
        std::lock_guard<std::mutex> lock(mutex_);
        DBSockets.emplace(client_id, std::move(sock));
        threads.emplace(client_id, std::move(thread));
    }

    void remove(int client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (DBSockets.count(client_id)) {
            DBSockets.at(client_id).close();
            DBSockets.erase(client_id);
        }
        if (threads.count(client_id)) {
            if (threads.at(client_id).joinable()) {
                threads.at(client_id).detach();
            }
            threads.erase(client_id);
        }
    }

    void stop_all() {
        running = false;
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : DBSockets) {//pair[id, sock]
            pair.second.close();
        }
        DBSockets.clear();
        for (auto& pair : threads) {//pair[id, t]
            if (pair.second.joinable()) pair.second.detach();
        }
        threads.clear();
    }

    bool is_running() const {
        return running;
    }
};

void client_handler(DBSocket client_sock, int client_id, ConnectionManager& manager) {
    try {
        std::cout << "Client " << client_id << " connected\n";

        // 设置非阻塞模式
        client_sock.set_non_blocking(true);

        while (manager.is_running()) {
            // 接收数据
            char buffer[1024];
            int bytes_received = client_sock.recv(buffer, sizeof(buffer));

            if (bytes_received > 0) {
                // 处理数据
                std::string message(buffer, bytes_received);
                std::cout << "From client " << client_id << ": " << message << std::endl;

                // 发送响应
                std::string response = "Echo: " + message;
                client_sock.send(response.c_str(), response.size());
            }
            else if (bytes_received == 0) {
                // 连接关闭
                break;
            }
            else {
                // 非阻塞模式下的错误处理
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                break;
                }
            }
        }
    catch (const std::exception& e) {
        std::cerr << "Client " << client_id << " error: " << e.what() << std::endl;
    }

    std::cout << "Client " << client_id << " disconnected\n";
    manager.remove(client_id);
    }


int main() {;
try {
    platform::socket_lib_init();
    ConnectionManager manager;

    // 创建服务器DBSocket
    DBSocket server_sock(DBSocket::Protocol::TCP);
    server_sock.bind(12345);
    server_sock.listen();

    std::cout << "Server started on port 12345..." << std::endl;

    int client_counter = 0;
    //    while (manager.is_running()) {
    //        try {
    //            // 接受新连接
    //            DBSocket client_sock = server_sock.accept();
    //            int client_id = ++client_counter;

    //            // 启动客户端线程
    //            std::thread t([client_sock = std::move(client_sock), client_id, &manager]() mutable {
    //                client_handler(std::move(client_sock), client_id, manager);
    //                });

    //            // 添加连接管理
    //            manager.add(client_id, std::move(client_sock), std::move(t));
    //        }
    //        catch (const std::system_error& e) {
    //            if (!manager.is_running()) break;
    //            std::cerr << "Accept error: " << e.what() << std::endl;
    //        }
    //    }
    //    return 0;
    //}
        // 改进后的主循环
    while (manager.is_running()) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int server_fd = server_sock.get_fd();
        FD_SET(server_fd, &read_fds);

        // 设置1秒超时兼顾响应性和CPU效率
        timeval timeout{ 1, 0 };

        // 使用select等待可读事件
        int ready = select(server_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (ready < 0) {  // 错误处理
            if (errno == EINTR) continue;  // 被信号中断
            char buffer[256];  // 需要提供缓冲区
            std::cerr << "select error: " << strerror_s(buffer,errno) << std::endl;
            break;
        }

        if (ready == 0) {  // 超时
            // 执行定期维护任务（例如清理超时连接）
            //manager.cleanup_inactive();
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            // 批量接受连接的优化
            const int MAX_ACCEPT_PER_LOOP = 100;
            for (int i = 0; i < MAX_ACCEPT_PER_LOOP; ++i) {
                try {
                    DBSocket client_sock = server_sock.accept();
                    int client_id = ++client_counter;

                    // 创建线程并管理
                    std::thread t([client_sock = std::move(client_sock),
                        client_id, &manager]() mutable {
                            client_handler(std::move(client_sock), client_id, manager);
                        });
                    manager.add(client_id, std::move(client_sock), std::move(t));
                }
                catch (const std::system_error& e) {
                    if (e.code().value() == WSAEWOULDBLOCK ||
                        e.code().value() == EWOULDBLOCK) {
                        break;  // 无更多待接连接
                    }
                    std::cerr << "Accept error: " << e.what() << std::endl;
                }
            }
        }

        // 添加CPU让步避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    platform::socket_lib_cleanup();
}
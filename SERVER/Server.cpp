#include "Server.h"
#include "SocketManager.h" // 假设 DBSocket 在这里定义
#include "Client.h"       // 确保包含 ClientSession 定义 (虽然 Server.h 已包含)
#include <iostream>
#include <memory>         // for std::make_unique, std::move
#include <system_error>   // for std::system_error (socket 错误)

// Windows specific for select and WSAErrors if applicable
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace mySocket; // 假设 DBSocket 在这个命名空间
using namespace std;

namespace myServer {

    // --- add 方法 (保持不变) ---
    void Server::add(int id, std::unique_ptr<ClientSession> client) {
        std::lock_guard<std::mutex> lk(mutexes);
        clients[id] = std::move(client);
    }

    // --- stop_all 方法 (保持不变) ---
    void Server::stop_all() {
        running = false;  // 通知全局停止

        std::lock_guard<std::mutex> lock(mutexes);

        // 分两步操作确保线程安全
        for (auto& client : clients) {
            if (client.second) {
                // 保留你原来的析构调用，虽然有风险
                client.second->~ClientSession();
            }
        }

        clients.clear();  // unique_ptr 自动析构
    }

    // --- remove 方法 (保持不变) ---
    void Server::remove(int client_id) {
        std::lock_guard<std::mutex> lock(mutexes);

        if (auto it = clients.find(client_id); it != clients.end()) {
            // 保留你原来的析构调用
            it->second->~ClientSession();
            clients.erase(it);
        }
    }

    // --- main_controller 方法 (修改创建 ClientSession 部分) ---
    void Server::main_controller()
    {
        DBSocket server_sock(DBSocket::Protocol::TCP); // 创建监听套接字
        // 启动服务器的错误处理应更完善
        try {
            server_sock.bind(port);
            server_sock.listen();
        }
        catch (const std::exception& e) {
            std::cerr << "错误: 服务器启动失败 (bind/listen): " << e.what() << std::endl;
            running = false;
            return;
        }


        std::cout << "Server started on port " << port << " ..." << std::endl;


        while (this->is_running())
        {
            fd_set readSet;
            FD_ZERO(&readSet);
            // 检查监听套接字是否有效
            SOCKET listen_fd = server_sock.get_fd();
            if (listen_fd == INVALID_SOCKET) {
                if (running) { // 仅在预期运行时报错
                    std::cerr << "错误: 监听套接字无效，服务器停止。" << std::endl;
                }
                running = false;
                break;
            }
            FD_SET(listen_fd, &readSet);

            // 设置50ms超时检测
            timeval timeout{ 0, 50000 };  // 0秒+50000微秒

            int ready = ::select(0, &readSet, nullptr, nullptr, &timeout);

            if (!running) break; // select 返回后再次检查

            if (ready > 0) {
                if (FD_ISSET(listen_fd, &readSet)) { // 确认是监听套接字事件
                    try {
                        // 接受新连接
                        DBSocket client_sock = server_sock.accept();
                        int client_temp_id = ++client_id; // 注意线程安全（如果多线程 accept）

                        std::cout << "接受到新连接，分配 ID: " << client_temp_id << std::endl;

                        // --- **修改这里：创建 ClientSession 时传递 sqlInterface 引用** ---
                        auto new_session = std::make_unique<ClientSession>(
                            std::move(client_sock),     // 转移 socket 所有权
                            client_temp_id,             // 传递客户端 ID
                            this->sqlInterface          // <-- 将 Server 的 sqlInterface 成员引用传进去
                        );
                        // --- 结束修改 ---

                        // 添加连接管理
                        this->add(client_temp_id, std::move(new_session));

                        // 启动客户端会话 (访问 map 可能需要锁，取决于 add 的实现)
                        // 为了安全，这里也加上锁访问
                        {
                            std::lock_guard<std::mutex> lock(mutexes);
                            if (clients.count(client_temp_id)) {
                                std::cout << "启动客户端会话 ID: " << client_temp_id << std::endl;
                                clients[client_temp_id]->start(threadPool);
                            }
                            else {
                                std::cerr << "错误: 添加客户端会话 " << client_temp_id << " 后无法在 map 中找到，启动失败。" << std::endl;
                            }
                        }


                    }
                    catch (const std::system_error& e) {
                        if (!this->is_running()) break;
                        std::cerr << "Accept error: " << e.what() << std::endl;
                    }
                    catch (const std::exception& e) { // 捕获其他可能的异常
                        std::cerr << "处理新连接时发生错误: " << e.what() << std::endl;
                    }
                } // 结束 if FD_ISSET
            } // 结束 if ready > 0
            else if (ready == 0) {
                // 超时无连接请求，继续循环检测运行状态
                continue;
            }
            else { // ready < 0
                // 处理select错误
#ifdef _WIN32
                int error_code = WSAGetLastError();
                if (error_code != WSAEINTR && running) { // 忽略中断，运行时才报错
                    std::cerr << "Select error: " << error_code << std::endl;
                    running = false; // 发生严重错误，停止服务器
                }
#else
                if (errno != EINTR && running) {
                    std::cerr << "Select error: " << errno << " (" << strerror(errno) << ")" << std::endl;
                    running = false;
                }
#endif
            }
        } // 结束 while

        std::cout << "服务器主循环结束。" << std::endl;
        // 如果是因为错误退出循环，确保停止
        if (running) {
            stop_all();
        }

    } // 结束 main_controller

} // namespace myServer
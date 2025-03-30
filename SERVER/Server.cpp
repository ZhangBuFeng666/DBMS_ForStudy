
#include "Server.h"


using namespace mySocket;
using namespace std;

namespace myServer {


    void Server::add(int id, std::unique_ptr<ClientSession> client) {
        std::lock_guard<std::mutex> lk(mutexes);
        clients[id] = std::move(client);
    }

    // 关闭所有客户端连接并清理资源
    void Server::stop_all() {
        running = false;  // 通知全局停止

        std::lock_guard<std::mutex> lock(mutexes);

        // 分两步操作确保线程安全
        for (auto& client : clients) {
            if (client.second) {
                client.second->~ClientSession();  // 强制关闭套接字以中断阻塞操作
            }
        }

        clients.clear();  // unique_ptr 自动析构，触发 ClientSession 的析构函数
    }

    // 移除指定客户端
    void Server::remove(int client_id) {
        std::lock_guard<std::mutex> lock(mutexes);

        if (auto it = clients.find(client_id); it != clients.end()) {
            it->second->~ClientSession();
            clients.erase(it);
        }
    }

    void Server::main_controller()
    {// Server端核心逻辑
        DBSocket server_sock(DBSocket::Protocol::TCP);
        server_sock.bind(port);
        server_sock.listen();

        std::cout << "Server started on port "<< port<<" ..." << std::endl;


        while (this->is_running())
        {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(server_sock.get_fd(), &readSet);

            // 设置50ms超时检测
            timeval timeout{ 0, 50000 };  // 0秒+50000微秒

            int ready = ::select(0, &readSet, nullptr, nullptr, &timeout);

            if (ready > 0) {
                try {
                    // 接受新连接
                    DBSocket client_sock = server_sock.accept();
                    int client_temp_id = ++client_id;

                    // 启动客户端线程
                    //ClientSession client(move(client_sock), client_temp_id);
                   
                    // 添加连接管理
                    this->add(client_temp_id, make_unique<ClientSession>(move(client_sock),client_temp_id));

                    // 在线程池中执行客户端会话
                    threadPool.enqueueTask([this, client_temp_id] {
                        clients[client_temp_id]->start();
                        });

                }
                catch (const std::system_error& e) {
                    if (!this->is_running()) break;
                    std::cerr << "Accept error: " << e.what() << std::endl;
                }
            }
            else if (ready == 0) {
                // 超时无连接请求，继续循环检测运行状态
                continue;
            }
            else {
                // 处理select错误
                if (WSAGetLastError() != WSAEINTR) {
                    std::cerr << "Select error: " << WSAGetLastError() << std::endl;
                }
            }
        }

    }
}


#pragma once

#include "SocketManager.h"
#include<thread>

class ClientSession {
public:
    // 构造函数（接管socket和线程）
    ClientSession(mySocket::DBSocket&& sock, int id)
        :sock(std::move(sock)),
        client_id(id),
        last_active(std::chrono::steady_clock::now()) {}


    //std::unique_ptr<ClientSession> create_ptr_ClientSession() {
    //    return std::make_unique<ClientSession>(*this);  // 使用std::move返回所有权
    //}

    // 析构函数（确保线程安全退出）
    ~ClientSession();

    // 获取最后活动时间
    auto get_last_active() const { return last_active; }

    // 标记为需要关闭
    void mark_for_close() { is_active = false; }

    // 客户端对该链接处理逻辑
    void client_handle();
    // 其他方法（如数据写入接口等）...

private:
    mySocket::DBSocket sock;                // 客户端套接字
    int client_id;                     // 客户端ID
    std::thread worker_thread;    // 专属处理线程
    std::string recv_buffer;      // 接收缓冲区
    std::atomic<bool> is_active{ true }; // 连接状态
    std::chrono::steady_clock::time_point last_active; // 最后活动时间
};

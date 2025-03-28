#pragma once

#include "SocketManager.h"
#include<thread>

class ClientSession {
public:
    // 构造函数（接管socket和线程）
    ClientSession(mySocket::DBSocket&& sock, std::thread&& thread)
        : sock_(std::move(sock)),
        worker_thread_(std::move(thread)),
        last_active_(std::chrono::steady_clock::now()) {}

    // 析构函数（确保线程安全退出）
    ~ClientSession();

    // 获取最后活动时间
    auto get_last_active() const { return last_active_; }

    // 标记为需要关闭
    void mark_for_close() { is_active_ = false; }

    // 客户端对该链接处理逻辑
    void run();
    // 其他方法（如数据写入接口等）...

private:
    mySocket::DBSocket sock_;                // 客户端套接字
    int client_id_;                     // 客户端ID
    std::thread worker_thread_;    // 专属处理线程
    std::string recv_buffer_;      // 接收缓冲区
    std::atomic<bool> is_active_{ true }; // 连接状态
    std::chrono::steady_clock::time_point last_active_; // 最后活动时间
};

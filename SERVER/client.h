#pragma once

#include "SocketManager.h"
#include <nlohmann/json.hpp>
#include<thread>

class ClientSession {
public:
    // 构造函数（接管socket和线程）
    ClientSession(mySocket::DBSocket&& from_sock, int id)
        :in_sock(std::move(from_sock)),
        client_id(id),
        last_active(std::chrono::steady_clock::now()),
        is_active(true){}


    //std::unique_ptr<ClientSession> create_ptr_ClientSession() {
    //    return std::make_unique<ClientSession>(*this);  // 使用std::move返回所有权
    //}

    // 析构函数（确保线程安全退出）
    ~ClientSession();

    // 获取最后活动时间
    auto get_last_active() const { return last_active; }

    // 标记为需要关闭
    void mark_for_close() { is_active = false; }

    void start();
    void stop();

private:
    mySocket::DBSocket in_sock;                // 输入（接收）套接字
    mySocket::DBSocket out_sock;               // 输出（发送）套接字
    int client_id;                     // 客户端ID
    std::string user_name;              // 用户名
    std::thread worker_thread;    // 专属处理线程
    std::string recv_buffer;      // 接收缓冲区
    std::atomic<bool> is_active; // 连接状态
    std::chrono::steady_clock::time_point last_active; // 最后活动时间

    // 客户端对该链接处理逻辑
    void client_handle();
};

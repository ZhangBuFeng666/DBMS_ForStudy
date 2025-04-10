#pragma once

#include "SocketManager.h"
#include "ThreadPool.h"
#include <nlohmann/json.hpp>
#include<thread>

class ClientSession {
public:
    // 构造函数（接管socket和线程）
    ClientSession(mySocket::DBSocket&& sock, int id)
        :sock(std::move(sock)),
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

    void start(ThreadPool& pool);
    void stop(std::thread worker_thread);
    bool send_massage(const nlohmann::json& data);//外部接口

private:
    // 客户端对该链接处理逻辑
    void client_handle_recv();//
    void enqueue_message(const std::string& message);//（send_massage调用）将待发送信息保存到队列
    void client_handle_send();//（线程持有）处理发送队列

    mySocket::DBSocket sock;            // 套接字
    int client_id;                      // 客户端ID
    std::string user_name;              // 用户名

    std::queue<std::string> send_queue; // 发送队列
    std::mutex send_mutex;              // 发送队列互斥锁
    std::condition_variable send_cv;    // 发送队列条件变量

    std::string recv_buffer;            // 接收缓冲区

    std::atomic<bool> is_active;        // 连接状态(线程安全，硬件层强制唯一)

    std::chrono::steady_clock::time_point last_active; // 最后活动时间

    //std::thread worker_thread_recv;   // 专属处理接收线程
    //std::thread worker_thread_send;   // 专属处理发送线程
};

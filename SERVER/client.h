#pragma once

#include "SocketManager.h"
#include "ThreadPool.h"

#include "SQLInterface.h" // <-- 包含 SQLInterface
#include "SQLParser.h"   // <-- 包含 SQLCommand 和 SelectResult
#include <nlohmann/json.hpp>
#include<thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unordered_map> // 用于 CommandType 映射
#include <nlohmann/json.hpp>// 确保包含 json.hpp 头文件

class ClientSession {
public:
    // 构造函数（接管socket和线程,添加了一个）
    ClientSession(mySocket::DBSocket&& sock, int id, SQLInterface& sql_interface_ref)
        : sock(std::move(sock)),
        client_id(id),
        sql_interface(sql_interface_ref), // <-- 初始化引用成员
        last_active(std::chrono::steady_clock::now()),
        is_active(true),
        is_logged_in(false), // <-- 新增：登录状态
        current_database("default") // <-- 新增：当前数据库 (给个默认值)
    {
    }
  
    //std::unique_ptr<ClientSession> create_ptr_ClientSession() {
    //    return std::make_unique<ClientSession>(*this);  // 使用std::move返回所有权
    //}

    // 析构函数（确保线程安全退出）
    ~ClientSession();

    //（原始）外部接口
    bool send_massage(nlohmann::json& data);

    // 获取最后活动时间
    auto get_last_active() const { return last_active; }

    // 标记为需要关闭
    void mark_for_close() { is_active = false; }

    void start(ThreadPool& pool);
    void stop(std::thread worker_thread);

private:
    // 客户端对该链接处理逻辑
    void client_handle_recv();
    void enqueue_message(const std::string& message);//（send_massage调用）将待发送信息保存到队列
    void client_handle_send();//（线程持有）（内部调用）处理发送队列

    mySocket::DBSocket sock;            // 套接字
    int client_id;                      // 客户端ID
    std::string user_name;              // 用户名

    std::queue<std::string> send_queue; // 发送队列
    std::mutex send_mutex;              // 发送队列互斥锁
    std::condition_variable send_cv;    // 发送队列条件变量

    std::string recv_buffer;            // 接收缓冲区

    std::atomic<bool> is_active;        // 连接状态(线程安全，硬件层强制唯一)

    std::chrono::steady_clock::time_point last_active; // 最后活动时间

    // +++ 确保以下成员变量存在 +++
    SQLInterface& sql_interface;        // <-- 引用 SQLInterface 实例
    std::string current_database;       // <-- 当前使用的数据库名
    bool is_logged_in;                  // <-- 标记用户是否已登录
    // +++ 结束确保 +++


    enum class RecvStatusType {
        //REGISTER,
        Logout,
        Login,
        Structure,
        Order,
        Unknown
};
    RecvStatusType getRecvStatusType(const std::string& type);
    //std::thread worker_thread_recv;   // 专属处理接收线程
    //std::thread worker_thread_send;   // 专属处理发送线程
};

#pragma once
#ifndef NET_SERVER_CORE_H
#define NET_SERVER_CORE_H

#include <functional>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>           // 需要包含 <memory> for std::unique_ptr 和 std::make_unique
#include "Client.h"       // 包含 ClientSession 定义
#include "ThreadPool.h"
#include "SQLInterface.h"   // <-- 包含 SQLInterface 定义

// 假设 mySocket 命名空间已定义或相关头文件已包含
namespace mySocket { class DBSocket; }

namespace myServer {
    class Server {
        friend class ClientSession; // 保持你原来的友元声明
    private:
        //std::mutex mtx_; // 保持原来的注释状态
        std::mutex mutexes;     // 用于保护 clients map 的互斥锁
        //std::unordered_map<int, std::thread> threads; // 保持原来的注释状态
        ThreadPool threadPool;  // 线程池
        std::unordered_map<int, std::unique_ptr<ClientSession>> clients; // 客户端会话管理
        //std::unordered_map<int, mySocket::DBSocket> DBSockets; // 保持原来的注释状态
        std::atomic<bool> running; // 服务器运行状态
        int port;               // 监听端口
        int client_id = 0;      // 客户端 ID 计数器

        // --- 新增 SQLInterface 成员变量 ---
        SQLInterface sqlInterface; // <-- 数据库接口实例

    public:
        // 构造函数
        Server(int port, size_t threadCount)
            : port(port),
            threadPool(threadCount),          // 初始化线程池
            running(true),                    // 初始化运行状态
            sqlInterface(/* 构造参数? */)     // <-- 在初始化列表中构造 sqlInterface
            // clients map 会自动默认构造
        {
        }

        // 析构函数
        ~Server() {
            stop_all(); // 确保服务器停止时清理资源
        }

        // 服务器主循环
        void main_controller();

        // 添加客户端会话
        void add(int id, std::unique_ptr<ClientSession> client);

        // 移除客户端会话
        void remove(int client_id);

        // 停止所有服务和会话
        void stop_all();

        // 检查服务器是否运行
        bool is_running() const {
            return running; // 直接访问 atomic bool 也可以，但 load() 更标准
        }
    };

} // namespace myServer
#endif // NET_SERVER_CORE_H
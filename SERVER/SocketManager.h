#ifndef NET_SOCKET_WRAPPER_H
#define NET_SOCKET_WRAPPER_H

#include <string>
#include <system_error>
#include <stdexcept>
#include <winsock2.h>
#include<iostream>
#include <string>

namespace mySocket {
    /**
     * @brief 数据库Socket封装类，提供跨平台的TCP/UDP网络通信能力
     */
    class DBSocket {
    public:
        /// 协议类型枚举
        enum class Protocol { TCP, UDP };

        /**
         * @brief 构造函数，创建指定协议的Socket
         * @param proto 协议类型，默认TCP协议
         * @throw std::system_error 创建Socket失败时抛出
         */
        explicit DBSocket(Protocol proto = Protocol::TCP);

        /**
         * @brief 析构函数，自动关闭Socket连接
         */
        ~DBSocket();


        DBSocket(DBSocket&& other) noexcept
            : proto(other.proto),
            sockfd(other.sockfd),
            is_listening(other.is_listening) {
            other.sockfd = -1;  // 原对象放弃描述符所有权
        }

        DBSocket& operator=(DBSocket&& other) noexcept {
            if (this != &other) {
                close();  // 关闭当前持有的socket
                proto = other.proto;
                sockfd = other.sockfd;
                is_listening = other.is_listening;
                other.sockfd = -1;
            }
            return *this;
        }

        // 禁用拷贝
        DBSocket(const DBSocket&) = delete;
        DBSocket& operator=(const DBSocket&) = delete;


        bool is_open() const { return sockfd != -1; }

        /**
         * @brief 绑定Socket到指定端口
         * @param port 监听端口号（主机字节序）
         * @throw std::system_error 绑定失败时抛出
         */
        void bind(int port);

        /**
         * @brief 启动监听模式（仅用于TCP协议）
         * @param backlog 等待连接队列的最大长度，默认128
         * @throw std::system_error 监听失败时抛出
         */
        void listen(int backlog = 128);

        /**
         * @brief 接受TCP连接（阻塞模式）
         * @return 新连接的客户端Socket对象
         * @throw std::system_error 接受连接失败时抛出
         */
        DBSocket accept();


        //void connect(const std::string& host, uint16_t port);

        size_t send(const char* data, size_t length);
        size_t recv(std::string& buffer);

        void set_non_blocking();
        void set_blocking();


        /**
         * @brief 安全关闭Socket连接（noexcept保证）
         */
        void close() noexcept;

        SOCKET get_fd() const { return sockfd; } ///< 获取底层Socket文件描述符, 供epoll/kqueue使用

    private:
        SOCKET sockfd = -1;       ///< 底层Socket文件描述符
        Protocol proto;        ///< 协议类型
        bool is_listening = false; ///< 是否处于监听模式
    };


}
#endif // NET_SOCKET_WRAPPER_H
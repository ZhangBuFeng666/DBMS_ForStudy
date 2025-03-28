#ifndef NET_SOCKET_WRAPPER_H
#define NET_SOCKET_WRAPPER_H

#include <string>
#include <system_error>
#include <stdexcept>
#include <winsock2.h>

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

        /**
         * @brief 连接到目标主机（TCP客户端/UDP设置目标地址）
         * @param host 目标主机IP地址或域名
         * @param port 目标端口号（主机字节序）
         * @throw std::system_error 连接失败时抛出
         */
        void connect(const std::string& host, uint16_t port);

        /**
         * @brief 发送数据（模板方法支持任意数据类型）
         * @tparam T 数据类型（需满足连续内存布局）
         * @param data 数据指针
         * @param length 数据字节长度
         * @return 实际发送的字节数
         * @throw std::system_error 发送失败时抛出
         */
        template <typename T>
        size_t send(const T* data, size_t length) { return 0; }

        /**
         * @brief 接收数据（模板方法支持任意数据类型）
         * @tparam T 数据类型（需满足连续内存布局）
         * @param buffer 接收缓冲区指针
         * @param buf_size 缓冲区最大容量
         * @return 实际接收的字节数（0表示连接关闭）
         * @throw std::system_error 接收失败时抛出
         */
        template <typename T>
        size_t recv(T* buffer, size_t buf_size) { return 0; }

        /**
         * @brief 设置非阻塞模式
         * @param enable true-非阻塞模式/false-阻塞模式
         * @throw std::system_error 设置失败时抛出
         */
        void set_non_blocking(bool enable);

        /**
         * @brief 安全关闭Socket连接（noexcept保证）
         */
        void close() noexcept;

        int get_fd() const { return sockfd; } ///< 获取底层Socket文件描述符

    private:
        int sockfd = -1;       ///< 底层Socket文件描述符
        Protocol proto;        ///< 协议类型
        bool is_listening = false; ///< 是否处于监听模式
    };
}
#endif // NET_SOCKET_WRAPPER_H
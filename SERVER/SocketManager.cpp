#include "SocketManager.h"
#include "DiffPlat.h"
#include<iostream>

namespace mySocket {

    DBSocket::DBSocket(Protocol proto) : proto(proto) {
        int type = (proto == Protocol::TCP) ? SOCK_STREAM : SOCK_DGRAM;
        sockfd = socket(AF_INET, type, 0);
        if (sockfd == -1) {
            throw std::system_error(
                platform::get_last_error(),
                std::system_category(),
                "socket creation failed"
            );
        }
    }

    DBSocket::~DBSocket() {
        if (sockfd != -1) {
            platform::close_socket(sockfd);
        }
    }



    void DBSocket::bind(int port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (::bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
#ifdef _WIN32
            throw std::system_error(WSAGetLastError(),
                std::system_category(), "bind failed");
#else
            throw std::system_error(errno,
                std::system_category(), "bind failed");
#endif
        }
    }

    void DBSocket::listen(int backlog) {
        if (::listen(sockfd, backlog) == -1) {
            throw std::system_error(WSAGetLastError(),
                std::system_category(), "listen failed");
        }
        is_listening = true;
    }

    DBSocket DBSocket::accept() {
        if (!is_listening) {
            throw std::logic_error("Socket is not in listening state");
        }

        // 轮询检查可读事件
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeval timeout{ 0, 0 }; // 非阻塞立即返回
        int ready = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);

        if (ready > 0) {

            sockaddr_in client_addr{};
            int len = sizeof(client_addr);
            SOCKET client_fd = ::accept(sockfd, (sockaddr*)&client_addr, &len);

            if (client_fd == -1) { /* 错误处理 */ }

            // 根据当前协议创建新DBSocket
            DBSocket client_socket(proto);
            client_socket.sockfd = client_fd;  // 需要添加sockfd的setter或友元访问

            return client_socket;
        }
        else if (ready == 0) { // 无连接
            throw std::system_error(WSAEWOULDBLOCK,
                std::system_category(), "No pending connections");
        }
        else { // select错误
            throw std::system_error(WSAGetLastError(),
                std::system_category(), "select failed");
        }
    }

    void DBSocket::set_non_blocking(bool enable) {
        unsigned long mode = enable ? 1 : 0;
        if (ioctlsocket(sockfd, FIONBIO, &mode) != 0) {
            throw std::system_error(WSAGetLastError(),
                std::system_category(), "ioctlsocket failed");
        }

    }

    void DBSocket::connect(const std::string& host, uint16_t port) {}


    void DBSocket::close() noexcept {

        if (sockfd != -1) {
            // 1. 尝试关闭双向通信
            ::shutdown(sockfd, 2);  // 同时关闭读写通道

            // 2. 执行平台相关关闭操作
            const int close_result = platform::close_socket(sockfd);

            // 3. 重置描述符并记录日志
            sockfd = -1;
            ///////////////////////////////////////////////////////记录日志

            // 4. 调试模式下的错误检测（生产环境应使用日志库）
            if (close_result != 0) {
#ifdef _DEBUG
                const int err = platform::get_last_error();
                std::cerr << "Socket关闭错误: " << err << std::endl;
#endif
            }
        }

    }
}
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

    //void DBSocket::set_non_blocking(bool enable) {
    //    unsigned long mode = enable ? 1 : 0;
    //    if (ioctlsocket(sockfd, FIONBIO, &mode) != 0) {
    //        throw std::system_error(WSAGetLastError(),
    //            std::system_category(), "ioctlsocket failed");
    //    }

    //}

    void DBSocket::connect(const std::string& host, uint16_t port) {}

    //void DBSocket::send(const std::string& data) {
    //    try {
    //        while (client.isActive()) {
    //            std::string response = "服务器回应数据";
    //            client.sock.send(response.c_str(), response.size());  // 发送响应数据给客户端
    //            std::this_thread::sleep_for(std::chrono::seconds(1));  // 每秒发送一次
    //        }
    //    }
    //    catch (...) {
    //        std::cerr << "发送数据时发生错误" << std::endl;
    //    }
    //}

    size_t DBSocket::DBSocket::recv(char* buffer, size_t buf_size) {
        // 检查缓冲区是否有效
        if (buffer == nullptr || buf_size == 0) {
            throw std::invalid_argument("Invalid buffer or buffer size");
        }

        // 步骤1: 先接收固定大小的头部，假设头部是4字节，表示数据的大小
        uint32_t dataSize = 0;
        int bytesReceived = 0;
        while (bytesReceived < sizeof(dataSize)) {
            int result = ::recv(sockfd, reinterpret_cast<char*>(&dataSize) + bytesReceived, sizeof(dataSize) - bytesReceived, 0);
            if (result == SOCKET_ERROR) {
                throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to receive data size");
            }
            if (result == 0) {
                throw std::runtime_error("Connection closed while waiting for data size");
            }
            bytesReceived += result;
        }

        // 步骤2: 如果数据的实际大小超过缓冲区大小，则抛出异常
        if (dataSize > buf_size) {
            throw std::overflow_error("Received data size exceeds buffer size");
        }

        // 步骤3: 接收实际的数据，确保数据接收完
        bytesReceived = 0;
        while (bytesReceived < dataSize) {
            int result = ::recv(sockfd, buffer + bytesReceived, dataSize - bytesReceived, 0);
            if (result == SOCKET_ERROR) {
                throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to receive actual data");
            }
            if (result == 0) {
                throw std::runtime_error("Connection closed while receiving data");
            }
            bytesReceived += result;
        }

        // 返回实际接收到的数据字节数
        return bytesReceived;
    
    }
    bool DBSocket::send(char* data, size_t length) {
        // 通过底层套接字实现数据发送
        int result = ::send(sockfd, data, static_cast<int>(length) + 4, 0);//约定前4字节为数据长度
        if (result == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            return 0;
        }
        return result > 0;  // 返回发送的字节数
    }



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
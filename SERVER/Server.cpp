//#include <iostream>
//#include <fstream>
//#include <winsock2.h>
//#include <ws2tcpip.h> // for inet_pton
//#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库
//
//const int CHUNK_SIZE = 1024; // 1KB
//
//// 发送消息头（长度）
//void send_header(SOCKET sock, uint32_t length) {
//    uint32_t net_length = htonl(length);
//    send(sock, reinterpret_cast<const char*>(&net_length), sizeof(net_length), 0);
//}
//
//// 发送数据块
//void send_chunk(SOCKET sock, const char* data, size_t size) {
//    send_header(sock, size + 1);  // +1 用于消息类型字节
//    char type = 0x02;             // 数据类型标记
//    send(sock, &type, 1, 0);
//    send(sock, data, static_cast<int>(size), 0);
//}
//
//int main() {
//    // 初始化 Winsock
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::cerr << "WSAStartup failed!" << std::endl;
//        return 1;
//    }
//
//    // 创建 Socket
//    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//    if (server_socket == INVALID_SOCKET) {
//        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
//        WSACleanup();
//        return 1;
//    }
//
//    sockaddr_in addr{};
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(12345);
//    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
//
//    // 绑定 & 监听
//    if (bind(server_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
//        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
//        closesocket(server_socket);
//        WSACleanup();
//        return 1;
//    }
//
//    if (listen(server_socket, 5) == SOCKET_ERROR) {
//        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
//        closesocket(server_socket);
//        WSACleanup();
//        return 1;
//    }
//
//    std::cout << "Server listening on port 12345..." << std::endl;
//
//    while (true) {
//        // 接受客户端连接
//        SOCKET client_sock = accept(server_socket, nullptr, nullptr);
//        if (client_sock == INVALID_SOCKET) {
//            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
//            continue;
//        }
//
//        char buffer[1024];
//
//        // 接收客户端请求
//        recv(client_sock, buffer, 4, MSG_WAITALL);
//        uint32_t body_len = ntohl(*reinterpret_cast<uint32_t*>(buffer));
//        recv(client_sock, buffer, body_len, MSG_WAITALL);
//
//        if (buffer[0] == 0x01) { // 处理请求
//            std::string filename(buffer + 1, body_len - 1);
//            std::ifstream file(filename, std::ios::binary);
//
//            if (!file) {
//                // 发送错误
//                send_header(client_sock, 1 + 12);
//                char msg[] = "\xFF File not found"; // 0xFF + 错误信息
//                send(client_sock, msg, sizeof(msg), 0);
//            }
//            else {
//                char chunk[CHUNK_SIZE];
//                while (!file.eof()) {
//                    file.read(chunk, CHUNK_SIZE);
//                    send_chunk(client_sock, chunk, file.gcount());
//
//                    // 等待ACK
//                    char ack;
//                    recv(client_sock, &ack, 1, 0); // 简化ACK逻辑
//                }
//            }
//        }
//        closesocket(client_sock);
//    }
//
//    closesocket(server_socket);
//    WSACleanup();
//    return 0;
//}

#include "Server.h"
#include "DiffPlat.h"
#include <thread>

namespace net {
    DBServer::DBServer(uint16_t port) : port_(port) {
        platform::socket_lib_init();
    }

    DBServer::~DBServer() {
        stop();
        platform::socket_lib_cleanup();
    }

    void DBServer::start(ClientHandler&& handler) {
        listen_sock_.bind(port_);
        listen_sock_.listen();
        running = true;

        while (running) {
            try {
                auto client = listen_sock_.accept();
                std::thread(handler, std::move(client)).detach();
            }
            catch (const std::system_error& e) {
                if (!running) break;
                // 处理异常
            }
        }
    }

    void DBServer::stop() noexcept {
        running = false;
        listen_sock_.close();
    }
}
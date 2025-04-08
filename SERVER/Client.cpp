#include "Client.h"
#include "Tools.h"
#include <iostream>

ClientSession::~ClientSession() {
        try {
            if (in_sock.is_open()) {  // 需在DBSocket中添加is_open()方法
                in_sock.close();
            }
            if (out_sock.is_open()) {
                out_sock.close();
            }
        }
        catch (std::exception& e) {
            // 记录日志但禁止异常传播
            std::cerr << "人Exception in ClientSession::~ClientSession(): " << e.what() << "\n";
        }
    }
    


void ClientSession::start() {
    worker_thread = std::thread(&ClientSession::client_handle, this);
    std::cout<<"线程启动，本次链接ID:"<< client_id <<"\n";
}
void ClientSession::stop() {
    is_active = false;
    if (worker_thread.joinable()) {
        if (worker_thread.get_id() != std::this_thread::get_id()) {
            worker_thread.join();  // 等待线程结束
        }
        else {
            worker_thread.detach(); // 避免自我死锁
        }
    }
}

void ClientSession::client_handle() {
    try {
        while (is_active) {
            char buffer[1024];
            size_t n = in_sock.recv(buffer, sizeof(buffer));

            if (n > 0) {
                recv_buffer.append(buffer, n); // 累积到缓冲区

                // 循环处理所有完整JSON
                while (true) {
                    size_t json_end = tools::find_json_end(recv_buffer);
                    if (json_end == 0) break; // 无完整JSON，退出等待更多数据

                    // 提取并解析JSON
                    std::string json_str = recv_buffer.substr(0, json_end);
                    try {
                        nlohmann::json json = nlohmann::json::parse(json_str);
                        //////////////////////////////process_data(json); // 替换为实际处理函数
                        recv_buffer.erase(0, json_end); // 移除已处理数据
                    }
                    catch (const nlohmann::json::parse_error& e) {
                        std::cerr << "JSON解析失败: " << e.what() << std::endl;
                        recv_buffer.clear(); // 异常时清空缓冲区
                        break;
                    }
                }
            }
            else if (n == 0) {
                std::cout << "连接正常关闭" << std::endl;
                break;
            }
            else {
                //////////////////////////////handle_error(); // 处理错误（如连接重置）
                break;
            }
        }
    }
    catch (...) {
        // 全局异常处理（如记录日志）
    }
}
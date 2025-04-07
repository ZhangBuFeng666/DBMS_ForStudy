#include "Client.h"
#include <iostream>

ClientSession::~ClientSession() {
    if (sock.is_open()) {  // 需在DBSocket中添加is_open()方法
        try {
            sock.close();
        }
        catch (std::exception& e) {
            // 记录日志但禁止异常传播
            std::cerr << "人Exception in ClientSession::~ClientSession(): " << e.what() << "\n";
        }
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
            // 数据接收与处理
            char buffer[1024];
            size_t n = sock.recv(buffer, sizeof(buffer));

            if (n > 0) {
                std::cout << "有东西！！\n";
                ////////////////////////////process_data(buffer, n);
            }
            else if (n == 0) {
                std::cout<<"没东西，连接关闭\n";
                break; // 连接关闭
            }
            else {
                ////////////////////////////handle_error();
            }
        }
    }
    catch (...) {
        // 异常处理
    }
}
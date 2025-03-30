#include "Client.h"

ClientSession::~ClientSession() {
    is_active = false;
    if (sock.is_open()) {  // 需在DBSocket中添加is_open()方法
        try {
            sock.close();
        }
        catch (...) {
            // 记录日志但禁止异常传播
        }
    }
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
                ////////////////////////////process_data(buffer, n);
            }
            else if (n == 0) {
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
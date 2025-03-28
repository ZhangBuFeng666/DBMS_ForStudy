#include "Client.h"

ClientSession::~ClientSession() {
    if (sock_.is_open()) {  // 需在DBSocket中添加is_open()方法
        try {
            sock_.close();
        }
        catch (...) {
            // 记录日志但禁止异常传播
        }
    }
    if (worker_thread_.joinable()) {
        if (worker_thread_.get_id() != std::this_thread::get_id()) {
            worker_thread_.join();  // 等待线程结束
        }
        else {
            worker_thread_.detach(); // 避免自我死锁
        }
    }
}

void ClientSession::run() {
    try {
        while (is_active_) {
            // 数据接收与处理
            char buffer[1024];
            int n = sock_.recv(buffer, sizeof(buffer));

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
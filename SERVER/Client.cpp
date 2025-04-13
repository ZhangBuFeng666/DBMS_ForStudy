#include "Client.h"
#include "Tools.h"
#include <iostream>

using json = nlohmann::json;
ClientSession::~ClientSession() {
        try {
            if (sock.is_open()) {  // 需在DBSocket中添加is_open()方法
                sock.close();
            }
        }
        catch (std::exception& e) {
            // 记录日志但禁止异常传播
            std::cerr << "人Exception in ClientSession::~ClientSession(): " << e.what() << "\n";
        }
    }
    


void ClientSession::start(ThreadPool& pool) {
    pool.enqueueTask([this] {
        this->client_handle_recv();
        });
    std::cout << "接收线程启动，本次链接ID:" << client_id << "\n";

    pool.enqueueTask([this] {
        this->client_handle_send();
        });
    //worker_thread_send = std::thread(&ClientSession::client_handle_send, this);
    std::cout << "发送线程启动，本次链接ID:" << client_id << "\n";
}
void ClientSession::stop(std::thread worker_thread) {
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


void ClientSession::client_handle_recv() {

    try {
        while (is_active) {
            std::string buffer;
            size_t n = sock.recv(buffer);
            std::cout<<"本次链接ID:" << client_id << "接收数据长度:" << n << "\n";
            //if (buffer.size() > 10 * 1024) { // 限制10KB
            //    std::cerr << "缓冲区溢出，强制关闭连接" << std::endl;
            //    //break;
            //}
            if (n > 0) {
                recv_buffer+=buffer; // 累积到缓冲区

                // 循环处理所有完整JSON
                while (true) {
                    size_t json_end = tools::find_json_end(recv_buffer);
                    if (json_end == 0) break; // 无完整JSON，退出等待更多数据

                    // 提取并解析JSON
                    std::string json_str = recv_buffer.substr(0, json_end);
                    try {
                        json json_mas = json::parse(json_str);
                        CommandType cmd = getCommandType(json_mas["Type"]);
                        json return_json = { {"Type", "Unkno"},
                            {"Status", "Err"},
                            {"Mass", "Null"}
                        };
                        switch (cmd) {
                        case CommandType::Login:
                            /////////////////////////////////////////////////////////////////id+password
                            return_json["Type"] = "Login";
                            return_json["Status"] = "Success";
                            break;
                        case CommandType::Order:
                            return_json["Type"] = "Order";
                            return_json["Status"] = "Success";
                            return_json["Mass"] = "a...b";
                            ////////////////////////////////////
                            break;
                        case CommandType::Unknown:
                            /////////////////////////////////////
                            break;
                        default:
                            ////////////////////////////////////////////////
                            break;
                        }
                        send_massage(return_json);
                        //////////////////////////////process_data(json); // 替换为实际处理函数
                        recv_buffer.erase(0, json_end); // 移除已处理数据
                    }
                    catch (const nlohmann::json::parse_error& e) {
                        std::cerr << "JSON解析失败: " << e.what() << std::endl;
                        recv_buffer.clear(); // 异常时清空缓冲区
                        //break;
                    }
                }
            }
            else if (n == 0) {
                std::cout << "ClientSession:连接中断" << std::endl;
                break;
            }
            else {
                //////////////////////////////handle_error(); // 处理错误（如连接重置）
                //break;
            }
        }
    }
    catch (...) {
        // 全局异常处理（如记录日志）
    }
}
void ClientSession::enqueue_message(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(send_mutex);
        send_queue.push(message);
    }
    send_cv.notify_one(); // 唤醒发送线程
}

bool ClientSession::send_massage(const json& data){
    try {
        // 序列化JSON并加入发送队列
        std::string json_str = data.dump();
        enqueue_message(json_str);
    }
    catch (std::exception& e) {
        std::cerr << "Exception in ClientSession::send_massage(): " << e.what() << "\n";
        return false;
    }

    return true;

}

void ClientSession::client_handle_send() {
    try {
        while (is_active) {
            std::unique_lock<std::mutex> lock(send_mutex);
            send_cv.wait(lock, [this] {
                return !send_queue.empty() || !is_active;
                });

            if (!is_active) break;

            // 取出队列中所有待发送消息
            std::queue<std::string> temp_queue;
            temp_queue.swap(send_queue);
            lock.unlock(); // 提前释放锁，减少临界区时间

            // 发送所有消息
            while (!temp_queue.empty()) {
                const std::string& data = temp_queue.front();
                size_t total_sent = 0;

                while (total_sent < data.size()) {//底层send会根据情况切片发送数据，要确保所有数据都发送成功
                    size_t sent = sock.send(data.c_str() + total_sent,
                        data.size() - total_sent);
                    if (sent <= 0) {
                        throw std::runtime_error("发送失败");
                    }
                    total_sent += sent;
                }
                temp_queue.pop();
            }
        }
    }
    catch (...) {
        /////////////////////////////////////handle_error();
        is_active = false;
    }

}
ClientSession::CommandType ClientSession::getCommandType(const std::string& type) {
    static const std::unordered_map<std::string, CommandType> typeMap = {
        {"Login", CommandType::Login},
        {"Order", CommandType::Order},
        {"Unknown", CommandType::Unknown}
    };

    auto it = typeMap.find(type);
    if (it != typeMap.end())
        return it->second;
    else
        return CommandType::Unknown;
}
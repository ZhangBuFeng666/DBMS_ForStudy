#include "Client.h"
#include "Tools.h"
#include "SQLParser.h"   // 包含 SQLCommand 和 SelectResult
#include <iostream>
#include <sstream>       // 用于解析注册/登录字符串

using namespace std;
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


// --- 修改 client_handle_recv ---
void ClientSession::client_handle_recv() {
    try {
        while (is_active) {
            std::string buffer;
            size_t n = sock.recv(buffer); // 接收数据
            last_active = std::chrono::steady_clock::now(); // 更新活动时间

            if (n > 0) {
                recv_buffer += buffer; // 追加到接收缓冲区

                // 循环处理缓冲区中所有完整的 JSON 消息
                while (is_active) { // 内层循环也检查 is_active
                    size_t json_end = tools::find_json_end(recv_buffer); // 查找完整 JSON 的结尾
                    if (json_end == 0) break; // 没有完整 JSON，跳出内层循环等待更多数据

                    std::string json_str = recv_buffer.substr(0, json_end); // 提取 JSON 字符串
                    recv_buffer.erase(0, json_end); // 从缓冲区移除已提取的部分

                    cout << "链接ID:" << client_id << " 收到完整JSON: " << json_str << endl;

                    json return_json = { {"Type", "Unknown"}, {"Status", "Failure"}, {"Mass", ""}}; // 初始化响应 JSON

                    try {
                        json received_json = json::parse(json_str); // 解析收到的 JSON
                        if (!received_json.contains("Type")) {
                            cerr << "错误: 收到的 JSON 格式无效 (缺少 Type)" << endl;
                            return_json["Mass"] = "错误: 无效的请求格式。";
                            send_massage(return_json);
                            continue; // 处理下一个 JSON
                        }

                        string type_str = received_json["Type"];
                        RecvStatusType cmd = getRecvStatusType(type_str); // 获取命令类型
                        json mass = received_json["Mass"]; // 获取 Mass 数据

                        return_json["Type"] = type_str; // 回复相同的类型

                        // --- 根据命令类型处理 ---
                        switch (cmd) {
                        //case RecvStatusType::REGISTER: {
                        //    if (!mass.is_string()) {
                        //        return_json["Mass"] = "错误: REGISTER 的 Mass 必须是字符串 '用户名 密码 [权限]'。";
                        //        break;
                        //    }
                        //    string reg_info = mass.get<string>();
                        //    stringstream ss(reg_info);
                        //    string username, password, privilege = "admin"; // 默认权限为 admin
                        //    if (ss >> username >> password) { // 至少需要用户名和密码
                        //        ss >> privilege; // 尝试读取权限，如果失败则使用默认值
                        //        if (username.empty() || password.empty()) {
                        //            return_json["Mass"] = "错误: 用户名或密码不能为空。";
                        //        }
                        //        else {
                        //            cout << "处理 REGISTER: 用户=" << username << ", 密码=***, 权限=" << privilege << endl;
                        //            if (sql_interface.create_user(username, password, privilege)) {
                        //                return_json["Status"] = "Success";
                        //                return_json["Mass"] = "";
                        //            }
                        //            else {
                        //                return_json["Status"] = "Failure";
                        //                return_json["Mass"] = "错误: 用户名可能已存在或注册失败。";
                        //            }
                        //        }
                        //    }
                        //    else {
                        //        return_json["Mass"] = "错误: REGISTER 的 Mass 格式应为 ‘用户名 密码 [权限]’!";
                        //    }
                        //    break;
                        //} // 结束 REGISTER

                        case RecvStatusType::Login: {
                            if (is_logged_in) { // 如果已登录，则不允许重复登录（虽然没必要）
                                return_json["Status"] = "Failure";
                                return_json["Mass"] = "错误: 用户已登录!";
                                break;
                            }
                            string username, password;

                            if (mass.contains("ID") && mass.contains("Password")) {
                                username = mass["ID"];
                                password = mass["Password"];
                            }
                            else {
                                std::cerr << "mass字段不存在或格式错误\n";
                                break;
                            }
                            //string login_info = mass.get<string>();
                            //stringstream ss(login_info);
                            if (username.empty() || password.empty()) {
                                return_json["Mass"] = "错误: 用户名或密码不能为空。";
                                break;
                            }
                            else {
                                cout << "处理 Login: 用户=" << username << ", 密码=***" << endl;
                                if (sql_interface.check_login(username, password)) {
                                    return_json["Status"] = "Success";
                                    return_json["Mass"] = "登录成功。";
                                    this->user_name = username; // 保存用户名
                                    this->is_logged_in = true; // 设置登录状态
                                    this->current_database = username; // 登录后默认使用用户同名数据库
                                    cout << "用户 '" << username << "' 登录，当前数据库设置为 '" << this->current_database << "'" << endl;
                                }
                                else {
                                    return_json["Status"] = "Failure";
                                    return_json["Mass"] = "错误: 用户名或密码错误。";
                                }
                            }
                            
                            break;
                        } // 结束 Login

                        case RecvStatusType::Order: { // 处理 SQL 命令
                            if (!is_logged_in) { // 要求必须先登录才能执行 SQL
                                return_json["Status"] = "Failure";
                                return_json["Mass"] = "错误: 请先登录再执行 SQL 命令。";
                                break;
                            }
                            if (!mass.is_string()) {
                                return_json["Mass"] = "错误: Order 的 Mass 必须是 SQL 语句字符串。"; 
                                break;
                            }
                            string sql_statement = mass.get<string>();
                            if (sql_statement.empty()) {
                                return_json["Mass"] = "错误: SQL 语句不能为空。";
                                break;
                            }

                            cout << "处理 Order (SQL): " << sql_statement << ", 当前数据库: " << this->current_database << endl;

                            string result_message;
                            SelectResult select_result;
                            // 调用 SQLInterface 处理命令
                            bool cmd_success = sql_interface.process_sql_command(sql_statement, this->current_database, result_message, select_result);

                            return_json["Status"] = cmd_success ? "Success" : "Failure";

                            // --- 处理返回结果 ---
                            if (select_result.success && !select_result.header.empty()) { // 如果是成功的 SELECT 查询
                                // 将 SelectResult 格式化为 JSON (或字符串)
                                json result_data;
                                result_data["Header"] = select_result.header;
                                result_data["Data"] = select_result.data;
                                result_data["Message"] = result_message; // 添加行数信息
                                return_json["Mass"] = result_data;
                            }
                            else {
                                // 对于非 SELECT 或失败的 SELECT，只返回消息
                                return_json["Mass"] = result_message;
                            }
                            break;
                        } // 结束 Order

                        case RecvStatusType::Unknown:
                        default:
                            cerr << "错误: 未知的命令类型 '" << type_str << "'" << endl;
                            return_json["Mass"] = "错误: 不支持的命令类型。";
                            break;
                        } // 结束 switch(cmd)

                    }
                    catch (const json::parse_error& e) {
                        cerr << "错误: JSON 解析失败: " << e.what() << " 对于字符串: " << json_str << endl;
                        return_json["Type"] = "Error";
                        return_json["Mass"] = "错误: 请求的 JSON 格式无效。";
                        // 不清空缓冲区，因为可能只是部分 JSON 错误
                    }
                    catch (const std::exception& e) {
                        cerr << "错误: 处理命令时发生异常: " << e.what() << endl;
                        return_json["Type"] = "Error";
                        return_json["Mass"] = "错误: 服务器内部错误。";
                    }

                    // 发送响应给客户端
                    send_massage(return_json);

                } // 结束内层 while (处理缓冲区中的 JSON)

            }
            else if (n == 0) {
                cout << "链接ID:" << client_id << " 客户端断开连接。" << endl;
                is_active = false; // 标记为非活动
                break; // 退出外层循环
            }
            else { // n < 0，发生错误
                cerr << "链接ID:" << client_id << " 接收数据时发生错误 (返回值 " << n << ")。" << endl;
                is_active = false; // 标记为非活动
                break; // 退出外层循环
            }
        } // 结束外层 while(is_active)

    }
    catch (const std::exception& e) {
        cerr << "链接ID:" << client_id << " client_handle_recv 发生未捕获异常: " << e.what() << endl;
    }
    catch (...) {
        cerr << "链接ID:" << client_id << " client_handle_recv 发生未知类型异常。" << endl;
    }

    // 线程即将退出，进行清理
    cout << "链接ID:" << client_id << " 接收处理线程退出。" << endl;
    is_active = false; // 确保标记为非活动
    // 可能需要通知发送线程也退出
    send_cv.notify_one();
    // 关闭 socket (可选，析构函数会做)
    // sock.close();
}
void ClientSession::enqueue_message(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(send_mutex);
        send_queue.push(message);
    }
    send_cv.notify_one(); // 唤醒发送线程
}
std::string gbk_to_utf8(const std::string& gbk) {
    int wlen = MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), -1, nullptr, 0);
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), -1, &wstr[0], wlen);

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);

    // 去掉末尾的 '\0'
    utf8.pop_back();
    return utf8;
}

void convert_json_strings_to_utf8(nlohmann::json& j) {
    if (j.is_object()) {
        for (auto& [key, value] : j.items()) {
            convert_json_strings_to_utf8(value);
        }
    }
    else if (j.is_array()) {
        for (auto& item : j) {
            convert_json_strings_to_utf8(item);
        }
    }
    else if (j.is_string()) {
        j = gbk_to_utf8(j.get<std::string>());
    }
}
bool ClientSession::send_massage(json& data){
    try {
        convert_json_strings_to_utf8(data);
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
ClientSession::RecvStatusType ClientSession::getRecvStatusType(const std::string& type) {
    static const std::unordered_map<std::string, RecvStatusType> typeMap = {
        {"Login", RecvStatusType::Login},
        {"Order", RecvStatusType::Order},
		//{"Register", RecvStatusType::REGISTER}, 
        {"Unknown", RecvStatusType::Unknown}
    };

    auto it = typeMap.find(type);
    if (it != typeMap.end())
        return it->second;
    else
        return RecvStatusType::Unknown;
}
#include <vector>
#include <atomic>
#include "DiffPlat.h"
#include"Tools.h"
#include"Server.h"

using namespace myServer;
using namespace mySocket;


int main() {;
try {
    platform::socket_lib_init();
    Server my_server;

    // 创建服务器DBSocket
    DBSocket server_sock(DBSocket::Protocol::TCP);
    server_sock.bind(12345);
    server_sock.listen();

    std::cout << "Server started on port 12345..." << std::endl;

    int client_counter = 0;
    
    while (my_server.is_running()) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int server_fd = server_sock.get_fd();
        FD_SET(server_fd, &read_fds);

        // 设置1秒超时兼顾响应性和CPU效率
        timeval timeout{ 1, 0 };

        // 使用select等待可读事件
        int ready = select(server_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (ready < 0) {  // 错误处理
            if (errno == EINTR) continue;  // 被信号中断
            char buffer[256];  // 需要提供缓冲区
            std::cerr << "select error: " << strerror_s(buffer,errno) << std::endl;
            break;
        }

        if (ready == 0) {  // 超时
            // 执行定期维护任务（例如清理超时连接）
            //my_server.cleanup_inactive();
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            // 批量接受连接的优化
            const int MAX_ACCEPT_PER_LOOP = 100;
            for (int i = 0; i < MAX_ACCEPT_PER_LOOP; ++i) {
                try {
                    DBSocket client_sock = server_sock.accept();
                    int client_id = ++client_counter;

                    // 创建线程并管理
                    std::thread t([client_sock = std::move(client_sock),
                        client_id, &my_server]() mutable {
                            client_handler(std::move(client_sock), client_id, my_server);
                        });
                    my_server.add(client_id, std::move(client_sock), std::move(t));
                }
                catch (const std::system_error& e) {
                    if (e.code().value() == WSAEWOULDBLOCK ||
                        e.code().value() == EWOULDBLOCK) {
                        break;  // 无更多待接连接
                    }
                    std::cerr << "Accept error: " << e.what() << std::endl;
                }
            }
        }

        // 添加CPU让步避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    platform::socket_lib_cleanup();
}


//    while (my_server.is_running()) {
    //        try {
    //            // 接受新连接
    //            DBSocket client_sock = server_sock.accept();
    //            int client_id = ++client_counter;

    //            // 启动客户端线程
    //            std::thread t([client_sock = std::move(client_sock), client_id, &my_server]() mutable {
    //                client_handler(std::move(client_sock), client_id, my_server);
    //                });

    //            // 添加连接管理
    //            my_server.add(client_id, std::move(client_sock), std::move(t));
    //        }
    //        catch (const std::system_error& e) {
    //            if (!my_server.is_running()) break;
    //            std::cerr << "Accept error: " << e.what() << std::endl;
    //        }
    //    }
    //    return 0;
    //}
        // 改进后的主循环
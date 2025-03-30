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
    Server my_server(6666,10);

    // 创建服务器
    my_server.main_controller();
}
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    platform::socket_lib_cleanup();
}


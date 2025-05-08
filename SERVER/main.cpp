#include <vector>
#include <atomic>
#include "DiffPlat.h"
#include"Tools.h"
#include"Server.h"
#include"logs.h"

using namespace myServer;
using namespace mySocket;
using namespace logs;

int main() {;
try {
    platform::socket_lib_init();
    Server my_server(6666,10);

    // 创建服务器
    my_server.main_controller();
}
    catch (const std::exception& e) {
        std::string serr = "Fatal error: ";
        serr+=e.what();
        std::cerr << serr << std::endl;
        Logger::log(serr);
        return 1;
    }
    platform::socket_lib_cleanup();
}
#ifndef PLATFORM_SOCKET_UTILS_H
#define PLATFORM_SOCKET_UTILS_H
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif
#include <cstdint>

namespace platform {
    void socket_lib_init();
    void socket_lib_cleanup();
    int close_socket(int sockfd) noexcept;
    int get_last_error();
}

#endif // PLATFORM_SOCKET_UTILS_H
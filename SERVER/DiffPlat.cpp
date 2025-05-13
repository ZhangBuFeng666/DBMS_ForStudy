#include "DiffPlat.h"

namespace platform {
    void socket_lib_init() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    void socket_lib_cleanup() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    int close_socket(int sockfd) noexcept {
#ifdef _WIN32
        return closesocket(sockfd);
#else
        return close(sockfd);
#endif
    }

    int get_last_error() {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }
}
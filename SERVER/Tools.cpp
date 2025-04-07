#include"Tools.h"

std::string tools::Timer::getCurrentTime() {
    // 获取当前时间
    std::time_t now = std::time(0);

    // 格式化当前时间
    char buffer[20];  // 确保缓冲区足够大
    struct tm time_info;

    // 将 time_t 转换为 tm 结构
    localtime_s(&time_info, &now);

    // 使用 strftime 格式化时间，精确到分钟
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info);

    return std::string(buffer);
}
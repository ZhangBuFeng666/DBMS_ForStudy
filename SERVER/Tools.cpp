#include"Tools.h"
namespace tools {
    std::string Timer::getCurrentTime() {
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


    // 辅助函数：查找第一个完整JSON的结束位置（基于括号匹配和转义处理）
    size_t find_json_end(const std::string& buffer) {
        int brace_count = 0;
        bool in_string = false;
        bool escape = false;

        for (size_t i = 0; i < buffer.size(); ++i) {
            char c = buffer[i];
            if (escape) {
                escape = false; // 转义符只影响下一个字符
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '"') {
                in_string = !in_string; // 进入/退出字符串状态
            }
            else if (!in_string) {
                if (c == '{' || c == '[') brace_count++;
                else if (c == '}' || c == ']') brace_count--;

                if (brace_count == 0) return i + 1; // 找到完整JSON结束位置
            }
        }
        return 0; // 未找到完整JSON
    }
}
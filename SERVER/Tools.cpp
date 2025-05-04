#include"Tools.h"
using json = nlohmann::json;

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


    // GBK → UTF-8 编码转换
    std::string gbk_to_utf8(const std::string& gbk_str) {
        int wide_len = MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, nullptr, 0);
        if (wide_len == 0) return {};

        std::wstring wstr(wide_len, 0);
        MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, &wstr[0], wide_len);

        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8_len == 0) return {};

        std::string utf8_str(utf8_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);

        return utf8_str;
    }

    // 递归处理 json，将所有字符串转为 UTF-8 编码
    void convert_json_strings_to_utf8(json& j) {
        if (j.is_string()) {
            std::string gbk_str = j.get<std::string>();
            j = gbk_to_utf8(gbk_str);
        }
        else if (j.is_array()) {
            for (auto& item : j) {
                convert_json_strings_to_utf8(item);
            }
        }
        else if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                convert_json_strings_to_utf8(value);
            }
        }
    }


    std::string utf8_to_gbk(const std::string& utf8_str) {
        // 先将 UTF-8 转为 UTF-16（wstring）
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
        if (wide_len == 0) return {};

        std::wstring wstr(wide_len, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], wide_len);

        // 再将 UTF-16 转为 GBK
        int gbk_len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (gbk_len == 0) return {};

        std::string gbk_str(gbk_len, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &gbk_str[0], gbk_len, nullptr, nullptr);

        return gbk_str;
    }

}
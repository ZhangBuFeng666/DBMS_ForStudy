#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <windows.h>
#include <nlohmann/json.hpp>

//#include <winsock2.h>

using json = nlohmann::json;

namespace tools
{
	class Timer {
    public:
        // 获取当前时间（精确到秒钟）
        //使用说明 tools::Timer::getCurrentTime();
        static std::string getCurrentTime();
    };

    // 辅助函数：查找第一个完整JSON的结束位置（基于括号匹配和转义处理）
    size_t find_json_end(const std::string& buffer);


    // GBK → UTF-8 编码转换
    std::string gbk_to_utf8(const std::string& gbk_str);
    // 递归处理 json，将所有字符串转为 UTF-8 编码
    void convert_json_strings_to_utf8(json& j);
    // UTF-8 → GBK 编码转换

    std::string utf8_to_gbk(const std::string& utf8_str);

}
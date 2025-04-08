#pragma once
#include <iostream>
#include <chrono>
#include <ctime>

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
}
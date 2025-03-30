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
}
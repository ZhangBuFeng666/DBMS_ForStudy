#include "Logs.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <filesystem> // C++17 及以上

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace fs = std::filesystem;
namespace logs {
    void Logger::log(const std::string& message) {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
#ifdef _WIN32
        localtime_s(&now_tm, &now_c);
#else
        localtime_r(&now_c, &now_tm);
#endif
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        std::string timestamp = ss.str();

        // 构建日志文件名
        std::stringstream filename_ss;
        filename_ss << std::put_time(&now_tm, "%Y_%m_%d") << ".log";
        std::string filename = filename_ss.str();

        // 构建日志文件夹路径
        std::string log_dir = "logs";

        // 创建日志文件夹（如果不存在）
        fs::path log_path(log_dir);
        if (!fs::exists(log_path)) {
            fs::create_directories(log_path);
        }

        // 构建完整的文件路径
        std::string full_path = log_dir + "/" + filename;

        // 写入日志内容到文件
        std::ofstream log_file(full_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << timestamp << " - " << message << std::endl;
            log_file.close();
        }
        else {
            std::cerr << "Error opening log file: " << full_path << std::endl;
        }
    }
}
#pragma once

#ifndef LOGS_H
#define LOGS_H
#include<string>
namespace logs {
	class Logger {
	public:
		static void log(const std::string& message);
	};

}

#endif // !LOGS_H
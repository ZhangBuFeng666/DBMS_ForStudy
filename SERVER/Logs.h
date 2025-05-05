#pragma once

#ifndef LOGS_H
#define LOGS_H

namespace logs {
	class Logger {
	public:
		static void log(const std::string& message);
	};

}

#endif // !LOGS_H
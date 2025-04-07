#pragma once

#ifndef LOGS_H
#define LOGS_H

namespace logs {
	class changeLog {
	public:
        	changeLog();
        	~changeLog();

			bool write_log(const char* log);
			bool read_log(const char* log);
	};

}

#endif // !LOGS_H
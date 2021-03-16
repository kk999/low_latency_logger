#ifndef _nplog_h_
#define _nplog_h_

#include "Logger.hpp"
#include <string>

namespace compatibility_nplog_nqlog {
	struct nplog_t {
		std::string logFilename;
	};
	void npLogOpen(nplog_t *nplog, char *logFilename, int) { nplog->logFilename = logFilename; }
	void npLogClose(nplog_t *)                             {}
}

#endif
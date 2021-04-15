#ifndef _nplog_h_
#define _nplog_h_

#include "Logger.hpp"
#include <string>

namespace compatibility_nplog_nqlog {
    struct nplog_t {
        std::string logFilename;
    };
    inline void npLogClose(nplog_t *&nplog) {
        if (nplog != nullptr) {
            delete nplog;
            nplog = nullptr;
        }
    }
    inline void npLogOpen(nplog_t *&nplog, char *logFilename, int) {
        npLogClose(nplog);
        nplog = new nplog_t();
        assert(nplog != nullptr);
        nplog->logFilename = logFilename;
    }
}

#endif
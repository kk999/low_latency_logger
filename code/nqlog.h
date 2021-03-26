#ifndef _nqlog_h_
#define _nqlog_h_

#include "Logger.hpp"
#include "nplog.h"

namespace compatibility_nplog_nqlog {
	#define NQLOG_CHR  Logger::TypeID::TypeID_char          // %c           : char
	#define NQLOG_INT  Logger::TypeID::TypeID_int           // %d           : int
	#define NQLOG_DBL  Logger::TypeID::TypeID_double        // %lf          : double
	#define NQLOG_STR  Logger::TypeID::TypeID_char_star     // %s           : null-terminated string
	#define NQLOG_DAT  Logger::TypeID::TypeID_data          // %.*s         : non-null-terminated string. But it's internally transformed to NQLOG_STR.
	#define NQLOG_FLT  Logger::TypeID::TypeID_float         // %f           : float
	#define NQLOG_LNG  Logger::TypeID::TypeID_long          // %ld          : long
	#define NQLOG_UINT Logger::TypeID::TypeID_unsigned_int  // %u           : unsigned int
	#define NQLOG_ULNG Logger::TypeID::TypeID_unsigned_long // %lu          : unsigned long
	#define NQLOG_UCHR Logger::TypeID::TypeID_unsigned_char // %02X or %02x : unsigned char
	#define NQLOG_ADR  Logger::TypeID::TypeID_void_star     // %p           : (char *)
	#define NQLOG_END  Logger::TypeID::TypeID_end           // end of arguments -- for assertion !!!
	#define nqlog_chr    NQLOG_CHR
	#define nqlog_char   NQLOG_CHR
	#define nq_chr(a)    NQLOG_CHR,a
	#define nq_char(a)   NQLOG_CHR,a
	#define nqlog_int    NQLOG_INT
	#define nq_int(a)    NQLOG_INT,a
	#define nqlog_dbl    NQLOG_DBL
	#define nqlog_double NQLOG_DBL
	#define nq_dbl(a)    NQLOG_DBL,a
	#define nq_double(a) NQLOG_DBL,a
	#define nqlog_str    NQLOG_STR
	#define nq_str(a)    NQLOG_STR,a
	#define nqlog_dat    NQLOG_DAT
	#define nqlog_data   NQLOG_DAT
	#define nq_dat(a,b)  NQLOG_DAT,a,b
	#define nq_data(a,b) NQLOG_DAT,a,b
	#define nqlog_flt    NQLOG_FLT
	#define nqlog_float  NQLOG_FLT
	#define nq_flt(a)    NQLOG_FLT,a
	#define nq_float(a)  NQLOG_FLT,a
	#define nqlog_lng    NQLOG_LNG
	#define nqlog_long   NQLOG_LNG
	#define nq_lng(a)    NQLOG_LNG,a
	#define nq_long(a)   NQLOG_LNG,a
	#define nqlog_uint   NQLOG_UINT
	#define nq_uint(a)   NQLOG_UINT,a
	#define nqlog_ulng   NQLOG_ULNG
	#define nqlog_ulong  NQLOG_ULNG
	#define nq_ulng(a)   NQLOG_ULNG,a
	#define nq_ulong(a)  NQLOG_ULNG,a
	#define nqlog_02X    NQLOG_UCHR
	#define nqlog_02x    NQLOG_UCHR
	#define nq_02X(a)    NQLOG_UCHR,a
	#define nq_02x(a)    NQLOG_UCHR,a
	#define nqlog_uchr   NQLOG_UCHR
	#define nqlog_uchar  NQLOG_UCHR
	#define nq_uchr(a)   NQLOG_UCHR,a
	#define nq_uchar(a)  NQLOG_UCHR,a
	#define nqlog_adr    NQLOG_ADR
	#define nqlog_PTR    NQLOG_ADR
	#define nqlog_ptr    NQLOG_ADR
	#define nq_adr(a)    NQLOG_ADR,a
	#define nq_PTR(a)    NQLOG_ADR,a
	#define nq_ptr(a)    NQLOG_ADR,a
	#define nqlog_end    NQLOG_END
	#define nq_end       NQLOG_END
}

namespace compatibility_nplog_nqlog {
	struct nqlog_t {
		Logger::LoggerCore<Logger::PrintfInfomation,19> core;
		Logger::LoggerConsumer<decltype(core)> logger;
		nqlog_t(): logger(core) {}
	};
	using write_log_func = void (*)(void *phlog, const char *msgFmt, ...);
	using flush_func = void (*)(void *phlog);
	write_log_func nplog_write_log_func = nullptr;
	flush_func     nplog_flush_func     = nullptr;
}

namespace compatibility_nplog_nqlog {
	int nqlog_open(nqlog_t *nqlog, char *, void *nplog, write_log_func, flush_func, int, int) { nqlog->logger.setFilename(static_cast<nplog_t*>(nplog)->logFilename); return 0; }
	void nqlog_close(nqlog_t *nqlog, int)                                                      { nqlog->logger.close(); }
	template<typename...Targs> inline void nqlog_write(nqlog_t *nqlog, Targs...args)           { (Logger::LoggerProducer<decltype(nqlog_t::core)>(nqlog->core))(args...); }
}

using namespace compatibility_nplog_nqlog;

#endif
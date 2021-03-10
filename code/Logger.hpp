#include <atomic>
#include <cassert>  // assert
#include <cstring>  // strlen strcpy memcpy
#include <fstream>  // fstream
#include <iomanip>  // stringstream put_time setfill
#include <iostream>  // std::cerr
#include <thread>

namespace Logger {
	inline std::string getTime(const char *format, std::chrono::high_resolution_clock::time_point theTime) {
		std::time_t rawTime = std::chrono::system_clock::to_time_t(theTime);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&rawTime), format)
			<< std::setfill('0') << std::setw(6) << std::chrono::duration_cast<std::chrono::microseconds>(theTime.time_since_epoch()).count() % 1000000
		;
		return ss.str();
	}
	inline std::string getTime() {
		return getTime("%Y%m%d_%H%M%S_", std::chrono::system_clock::now());
	}
	template <class T, std::size_t N> constexpr std::size_t size(const T (&array)[N]) noexcept {
		return N;
	}
}

namespace Logger {
	enum TypeID: int {
		TypeID_init = 0,
		TypeID_char = 0X77AABB00,
		TypeID_int,
		TypeID_double,
		TypeID_char_star,
		TypeID_data,
		TypeID_float,
		TypeID_long,
		TypeID_unsigned_int,
		TypeID_unsigned_long,
		TypeID_unsigned_char,
		TypeID_void_star,
		TypeID_end = 0X77AABBFF
	};
	struct PrintfInfomation {
		enum {
			maxPrintfInfomation = 1<<11,
			maxArgCnt           = 20,
			maxArgSize          = 64,
			maxFormatStrLen     = maxPrintfInfomation - maxArgCnt * maxArgSize,
		};
		char formatting[maxFormatStrLen];
		struct ArgumentInfo {
			int length;
			TypeID type;
			union Argument {
				char c;                // TypeID_char
				int i;                 // TypeID_int
				double d;              // TypeID_double
				char s[maxArgSize];    // TypeID_char_star
				char data[maxArgSize]; // TypeID_data
				float f;               // TypeID_float
				long l;                // TypeID_long
				unsigned int ui;       // TypeID_unsigned_int
				unsigned long ul;      // TypeID_unsigned_long
				unsigned char uc;      // TypeID_unsigned_char
				void *p;               // TypeID_void_star
			} value;
		} args[maxArgCnt];
	};
}

namespace Logger {
	template<typename T, unsigned int poolSizeInLog2> class LoggerCore {
		private:
			constexpr static unsigned int poolSize = 1 << poolSizeInLog2;
			constexpr static unsigned int poolMask = poolSize-1;
			char memPool[poolSize][sizeof(T)];
			std::atomic_uint idxProducer;
			unsigned int idxConsumer;
		public:
			LoggerCore(): idxProducer(-1), idxConsumer{0} {
				for (auto memoryData: memPool) reinterpret_cast<PrintfInfomation*>(memoryData)->formatting[0] = '\0';
			}
			T* getAvailableSpaceForProducer() {
				return reinterpret_cast<PrintfInfomation*>(memPool[++idxProducer & poolMask]);
			}
			T* getAvailableSpaceForConsumer() {
				PrintfInfomation *printfInfo = reinterpret_cast<PrintfInfomation*>(memPool[idxConsumer]);
				if (printfInfo->formatting[0] != '\0') {
					++idxConsumer &= poolMask;
					return printfInfo;
				}
				return nullptr;
			}
	};
	template<typename LoggerCore = LoggerCore<PrintfInfomation,19>> class LoggerConsumer {
		private:
			LoggerCore &core;
			const std::string logPath;
			std::ofstream logFile;
			std::thread thread;
			std::atomic_bool running;
		protected:
			void main() { while (running) consume(); consume(); }
			void consume() {
				for (PrintfInfomation *printfInfo; (printfInfo = core.getAvailableSpaceForConsumer()) != nullptr; ) {
					int passcnt = 0;
					logFile << getTime("%H:%M:%S.", std::chrono::system_clock::now()) << " ";
					for (auto ch: printfInfo->formatting) {
						if (passcnt > 0) {
							--passcnt;
							continue;
						}
						if (ch == '%') {
							for (auto &arg: printfInfo->args) {
								if (arg.length == 0) continue;
								switch (arg.type) {
									case TypeID::TypeID_init: break;
									case TypeID::TypeID_char          : { const char fmt[]{   "%c"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.c    )); logFile <<               arg.value.c    ;} break;
									case TypeID::TypeID_unsigned_char : { const char fmt[]{ "%02x"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.uc   )); logFile <<               arg.value.uc   ;} break;
									case TypeID::TypeID_int           : { const char fmt[]{   "%d"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.i    )); logFile <<               arg.value.i    ;} break;
									case TypeID::TypeID_unsigned_int  : { const char fmt[]{   "%u"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.ui   )); logFile <<               arg.value.ui   ;} break;
									case TypeID::TypeID_long          : { const char fmt[]{  "%ld"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.l    )); logFile <<               arg.value.l    ;} break;
									case TypeID::TypeID_unsigned_long : { const char fmt[]{  "%lu"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.ul   )); logFile <<               arg.value.ul   ;} break;
									case TypeID::TypeID_float         : { const char fmt[]{   "%f"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.f    )); logFile << std::fixed << arg.value.f    ;} break;
									case TypeID::TypeID_double        : { const char fmt[]{  "%lf"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.d    )); logFile << std::fixed << arg.value.d    ;} break;
									case TypeID::TypeID_char_star     : { const char fmt[]{   "%s"}; passcnt += sizeof(fmt)-2; assert(arg.length == strlen(arg.value.s    )); logFile <<               arg.value.s    ;} break;
									case TypeID::TypeID_data          : { const char fmt[]{ "%.*s"}; passcnt += sizeof(fmt)-2; assert(arg.length == strlen(arg.value.data )); logFile <<               arg.value.data ;} break;
									case TypeID::TypeID_void_star     : { const char fmt[]{   "%p"}; passcnt += sizeof(fmt)-2; assert(arg.length == sizeof(arg.value.p    )); logFile <<               arg.value.p    ;} break;
									default: logFile << "!unknow"; break;
								}
								arg.length = 0;
								break;
							}
							continue;
						} else if (ch == '\0') {
							break;
						}
						logFile << ch;
					}
					printfInfo->formatting[0] = '\0';
				}
			}
		public:
			~LoggerConsumer() { close(); }
			LoggerConsumer(LoggerCore &core):core(core), logPath("go."+getTime()+".log"), logFile(logPath, std::ofstream::out), thread(&LoggerConsumer::main, this), running(true) {}
			void close() { running = false; if (thread.joinable()) thread.join(); }
			void flush() {
				if (!!logFile) logFile << std::flush;
				else           logFile.clear();
			}
	};
	template<typename LoggerCore = LoggerCore<PrintfInfomation,19>> class LoggerProducer {
		private:
			PrintfInfomation *printfInfo;
		public:
			LoggerProducer(LoggerCore &core) {
				do printfInfo = core.getAvailableSpaceForProducer();
				while (printfInfo->formatting[0] != '\0');
			}
			template<typename...Targs> inline void operator()(const char *format, Targs&&... args) {
				strcpy(printfInfo->formatting, format);
				processParameters(std::forward<Targs>(args)...);
			}
			template<int idxArgument=0> inline void processParameters(TypeID type) {
				assert(type == TypeID_end);
			}
			template<int idxArgument=0, typename...Targs> inline void processParameters(TypeID type, int dataSize, const char *data, Targs&&...args) {
				PrintfInfomation::ArgumentInfo &arg = printfInfo->args[idxArgument];
				arg.type = type;
				arg.length = dataSize;
				memcpy(arg.value.data, data, arg.length);
				processParameters<idxArgument+1>(std::forward<Targs>(args)...);
			}
			template<int idxArgument=0, typename...Targs, typename T> inline void processParameters(TypeID type, T &&value, Targs&&...args) {
				PrintfInfomation::ArgumentInfo &arg = printfInfo->args[idxArgument];
				arg.type = type;
				using TYPE = std::remove_cv_t<std::remove_reference_t<std::decay_t<T>>>;
				if constexpr (std::is_same_v<TYPE, const char*> or std::is_same_v<TYPE, char*>) {
					arg.length = strlen(value);
					strcpy(arg.value.s, value);
				} else if constexpr (std::is_same_v<TYPE, std::string>) {
					arg.length = value.size();
					strcpy(arg.value.s, value.c_str());
				} else {
					arg.length = sizeof(T);
					arg.value = *reinterpret_cast<PrintfInfomation::ArgumentInfo::Argument*>(&std::forward<T>(value));
				}
				processParameters<idxArgument+1>(std::forward<Targs>(args)...);
			}
	};
}

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
	struct nplog_t {};
	struct nqlog_t {};
	using write_log_func = void (*)(void *phlog, const char *msgFmt, ...);
	using flush_func = void (*)(void *phlog);
	write_log_func nplog_write_log_func = nullptr;
	flush_func     nplog_flush_func     = nullptr;
}

namespace compatibility_nplog_nqlog {
	static Logger::LoggerCore<Logger::PrintfInfomation,19> core;
	static Logger::LoggerConsumer<decltype(core)> logger(core);
}

namespace compatibility_nplog_nqlog {
	void npLogOpen(nplog_t *, char *, int) {}
	void npLogClose(nplog_t *) {}
	void nqlog_open(nqlog_t *, char *, void *, write_log_func, flush_func, int, int) { }
	void nqlog_close(nqlog_t *, int) { logger.close(); }
	template<typename...Targs> inline void nqlog_write(nqlog_t *, Targs...args) { (Logger::LoggerProducer<decltype(core)>(core))(args...); }
}

using namespace compatibility_nplog_nqlog;

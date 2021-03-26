#ifndef _Logger_hpp_
#define _Logger_hpp_

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
			std::string logPath;
			std::ofstream logFile;
			std::thread thread;
			std::atomic_bool running;
		protected:
			void main() { while (running) consume(); consume(); }
			void consume() {
				// std::this_thread::yield();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (!logFile.is_open()) return;
				for (PrintfInfomation *printfInfo; (printfInfo = core.getAvailableSpaceForConsumer()) != nullptr; ) {
					logFile << getTime("%H:%M:%S.", std::chrono::system_clock::now()) << " ";
					for (auto *itr=printfInfo->formatting, *itr_end=printfInfo->formatting+sizeof(PrintfInfomation::formatting); itr < itr_end; ++itr ) {
						if (*itr == '%') {
							for (auto &arg: printfInfo->args) {
								switch (arg.type) {
									case TypeID::TypeID_init: continue;
									case TypeID::TypeID_char          : itr += sizeof(  "%c")-2; logFile <<               arg.value.c    ; break;
									case TypeID::TypeID_unsigned_char : itr += sizeof("%02x")-2; logFile <<               arg.value.uc   ; break;
									case TypeID::TypeID_int           : itr += sizeof(  "%d")-2; logFile <<               arg.value.i    ; break;
									case TypeID::TypeID_unsigned_int  : itr += sizeof(  "%u")-2; logFile <<               arg.value.ui   ; break;
									case TypeID::TypeID_long          : itr += sizeof( "%ld")-2; logFile <<               arg.value.l    ; break;
									case TypeID::TypeID_unsigned_long : itr += sizeof( "%lu")-2; logFile <<               arg.value.ul   ; break;
									case TypeID::TypeID_float         : itr += sizeof(  "%f")-2; logFile << std::fixed << arg.value.f    ; break;
									case TypeID::TypeID_double        : itr += sizeof( "%lf")-2; logFile << std::fixed << arg.value.d    ; break;
									case TypeID::TypeID_char_star     : itr += sizeof(  "%s")-2; logFile <<               arg.value.s    ; break;
									case TypeID::TypeID_data          : itr += sizeof("%.*s")-2; logFile <<               arg.value.data ; break;
									case TypeID::TypeID_void_star     : itr += sizeof(  "%p")-2; logFile <<               arg.value.p    ; break;
									default: logFile << "!unknow"; break;
								}
								arg.type = TypeID::TypeID_init;
								break;
							}
							continue;
						} else if (*itr == '\0') {
							break;
						}
						logFile << *itr;
					}
					printfInfo->formatting[0] = '\0';
				}
			}
		public:
			~LoggerConsumer() { close(); }
			LoggerConsumer(LoggerCore &core):core(core), thread(&LoggerConsumer::main, this), running(true) {}
			void close() { running = false; if (thread.joinable()) thread.join(); }
			bool setFilename(const std::string &logFilename) {
				logPath = logFilename;
				logFile.close();
				logFile.open(logPath, std::ofstream::out);
				return logFile.is_open();
			}
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
			template<int idxArgument=0
				, typename T
				, typename TEnable = std::enable_if_t<std::is_arithmetic<T>::value && !std::is_same<T, char>::value, T>
				, typename U
				, typename UEnable =
					std::enable_if_t<
						std::is_same<U, char>::value
							|| std::is_same<U, const char>::value
							|| std::is_same<U, signed char>::value
							|| std::is_same<U, const signed char>::value
							|| std::is_same<U, unsigned char>::value
							|| std::is_same<U, const unsigned char>::value
						, U
					>
				, typename...Targs
			>
			inline void processParameters(TypeID type, T dataSize, U *data, Targs&&...args) {
				PrintfInfomation::ArgumentInfo &arg = printfInfo->args[idxArgument];
				arg.type = type;
				memcpy(arg.value.data, data, dataSize);
				arg.value.data[dataSize] = '\0';
				processParameters<idxArgument+1>(std::forward<Targs>(args)...);
			}
			template<int idxArgument=0, typename...Targs, typename T> inline void processParameters(TypeID type, T &&value, Targs&&...args) {
				PrintfInfomation::ArgumentInfo &arg = printfInfo->args[idxArgument];
				arg.type = type;
				using TYPE = std::remove_cv_t<std::remove_reference_t<std::decay_t<T>>>;
				if constexpr (std::is_same_v<TYPE, const char*> or std::is_same_v<TYPE, char*>) {
					strcpy(arg.value.s, value);
				} else if constexpr (std::is_same_v<TYPE, std::string>) {
					strcpy(arg.value.s, value.c_str());
				} else {
					arg.value = *reinterpret_cast<PrintfInfomation::ArgumentInfo::Argument*>(&std::forward<T>(value));
				}
				processParameters<idxArgument+1>(std::forward<Targs>(args)...);
			}
	};
}

#endif
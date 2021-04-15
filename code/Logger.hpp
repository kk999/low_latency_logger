#ifndef _Logger_hpp_
#define _Logger_hpp_

#include <atomic>
#include <cassert>  // assert
#include <cstring>  // strlen strcpy memcpy
#include <cstdio>  // FILE fopen fclose fprint fflush
#include <iomanip>  // stringstream put_time setfill
#include <iostream>  // std::cerr
#include <sys/time.h>  // gettimeofday
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
    struct PrintfInformation {
        enum {
            maxPrintfInformation = 1<<13,
            maxArgCnt           = 66,
            maxArgSize          = 108,
            maxFormatStrLen     = maxPrintfInformation - maxArgCnt * maxArgSize,
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
        std::atomic_bool dirty;
    };
}

namespace Logger {
    template<typename T> class LoggerCore {
        private:
            const unsigned int poolSize;
            const unsigned int poolMask;
            char (*memPool)[sizeof(T)];
            std::atomic_uint idxProducer;
            unsigned int idxConsumer;
        public:
            ~LoggerCore() { if (memPool != nullptr) delete [] memPool; }
            LoggerCore(const unsigned int poolSizeInLog2): poolSize(1 << poolSizeInLog2), poolMask(poolSize-1), memPool(nullptr), idxProducer(-1), idxConsumer{0} {
                try {
                    memPool = reinterpret_cast<decltype(memPool)>(new T[poolSize]);
                } catch (...) {
                    memPool = nullptr;
                }
                if (memPool != nullptr) {
                    for (auto itr = memPool, end = memPool+poolSize; itr < end; ++itr)
                        reinterpret_cast<PrintfInformation*>(itr)->dirty.store(false, std::memory_order_relaxed);
                }
            }
            T* getAvailableSpaceForProducer() {
                if (memPool == nullptr) return nullptr;
                PrintfInformation *printfInfo = reinterpret_cast<PrintfInformation*>(memPool[++idxProducer & poolMask]);
                if (printfInfo->dirty.exchange(true, std::memory_order_acq_rel)) return nullptr;
                return printfInfo;
            }
            T* getAvailableSpaceForConsumer() {
                if (memPool == nullptr) return nullptr;
                if (PrintfInformation *printfInfo = reinterpret_cast<PrintfInformation*>(memPool[idxConsumer]); printfInfo->dirty.load(std::memory_order_acquire)) {
                    ++idxConsumer &= poolMask;
                    return printfInfo;
                }
                return nullptr;
            }
    };
    template<typename LoggerCore> class LoggerConsumer {
        private:
            LoggerCore &core;
            std::string logPath;
            FILE *logFHandle;
            std::thread thread;
            std::atomic_bool running;
            int autoFlushTimeInt;
            std::chrono::high_resolution_clock::time_point lastFlushTime;
        protected:
            void main() { while (running) consume(); consume(); }
            void consume() {
                // std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (logFHandle == nullptr) return;
                for (PrintfInformation *printfInfo; (printfInfo = core.getAvailableSpaceForConsumer()) != nullptr; ) {
                    fprintf(logFHandle, "%s ", getTime("%H:%M:%S.", std::chrono::system_clock::now()).c_str());
                    for (auto *itr=printfInfo->formatting, *itr_end=printfInfo->formatting+sizeof(PrintfInformation::formatting); itr < itr_end; ++itr ) {
                        if (*itr == '%') {
                            for (auto &arg: printfInfo->args) {
                                switch (arg.type) {
                                    case TypeID::TypeID_init: continue;
                                    case TypeID::TypeID_char          : itr += sizeof(  "%c")-2; if (*itr=='c') fprintf(logFHandle,   "%c", arg.value.c    ); else {fprintf(logFHandle, "!consume: type(char"          ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_unsigned_char : itr += sizeof("%02x")-2; if (*itr=='x') fprintf(logFHandle, "%02x", arg.value.uc   ); else {fprintf(logFHandle, "!consume: type(unsigned char" ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_int           : itr += sizeof(  "%d")-2; if (*itr=='d') fprintf(logFHandle,   "%d", arg.value.i    ); else {fprintf(logFHandle, "!consume: type(int"           ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_unsigned_int  : itr += sizeof(  "%u")-2; if (*itr=='u') fprintf(logFHandle,   "%u", arg.value.ui   ); else {fprintf(logFHandle, "!consume: type(unsigned int"  ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_long          : itr += sizeof( "%ld")-2; if (*itr=='d') fprintf(logFHandle,  "%ld", arg.value.l    ); else {fprintf(logFHandle, "!consume: type(long"          ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_unsigned_long : itr += sizeof( "%lu")-2; if (*itr=='u') fprintf(logFHandle,  "%lu", arg.value.ul   ); else {fprintf(logFHandle, "!consume: type(unsigned long" ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_float         : itr += sizeof(  "%f")-2; if (*itr=='f') fprintf(logFHandle,   "%f", arg.value.f    ); else {fprintf(logFHandle, "!consume: type(float"         ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_double        : itr += sizeof( "%lf")-2; if (*itr=='f') fprintf(logFHandle,  "%lf", arg.value.d    ); else {fprintf(logFHandle, "!consume: type(double"        ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_char_star     : itr += sizeof(  "%s")-2; if (*itr=='s') fprintf(logFHandle,   "%s", arg.value.s    ); else {fprintf(logFHandle, "!consume: type(char*"         ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_data          : itr += sizeof("%.*s")-2; if (*itr=='s') fprintf(logFHandle,   "%s", arg.value.data ); else {fprintf(logFHandle, "!consume: type(data"          ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    case TypeID::TypeID_void_star     : itr += sizeof(  "%p")-2; if (*itr=='p') fprintf(logFHandle,   "%p", arg.value.p    ); else {fprintf(logFHandle, "!consume: type(void*"         ")%c\n",*itr); arg.type = TypeID::TypeID_init; goto nextlog;} break;
                                    default: fprintf(logFHandle, "!unknow"); break;
                                }
                                arg.type = TypeID::TypeID_init;
                                break;
                            }
                            continue;
                        } else if (*itr == '\0') {
                            break;
                        }
                        fprintf(logFHandle, "%c", *itr);
                    }
                    nextlog:
                    flush();
                    printfInfo->dirty.store(false);
                }
                flush();
            }
        public:
            ~LoggerConsumer() { close(); }
            LoggerConsumer(LoggerCore &core, const int autoFlushTimeInt):core(core), logFHandle(nullptr), thread(&LoggerConsumer::main, this), running(true), autoFlushTimeInt(autoFlushTimeInt), lastFlushTime(std::chrono::high_resolution_clock::now()) {}
            void close() { running = false; if (thread.joinable()) thread.join(); }
            bool setFilename(const std::string &logFilename) {
                logPath = logFilename;
                if (logFHandle != nullptr) fclose(logFHandle);
                logFHandle = fopen(logPath.c_str(), "at");
                if (logFHandle == nullptr) {
                    return false;
                }
                return true;
            }
            void flush() {
                const auto current = std::chrono::high_resolution_clock::now();
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current - lastFlushTime).count();
                if (ms >= autoFlushTimeInt) {
                    fflush(logFHandle);
                    lastFlushTime = current;
                }
            }
    };
    template<typename LoggerCore> class LoggerProducer {
        private:
            PrintfInformation *printfInfo;
        public:
            LoggerProducer(LoggerCore &core) {
                printfInfo = core.getAvailableSpaceForProducer();
            }
            template<typename...Targs> inline void operator()(const char *format, Targs&&... args) {
                if (printfInfo == nullptr) return;
                strcpy(printfInfo->formatting, format);
                processParameters(std::forward<Targs>(args)...);
            }
            template<int idxArgument=0> inline void processParameters() {
                static_assert(idxArgument <= PrintfInformation::maxArgCnt, "!Logger::processParameters : Incorrect number of parameters.");
            }
            template<int idxArgument=0> inline void processParameters(TypeID type) {
                assert(type == TypeID_end);
                processParameters<idxArgument>();
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
                PrintfInformation::ArgumentInfo &arg = printfInfo->args[idxArgument];
                arg.type = type;
                memcpy(arg.value.data, data, dataSize);
                arg.value.data[dataSize] = '\0';
                processParameters<idxArgument+1>(std::forward<Targs>(args)...);
            }
            template<int idxArgument=0, typename...Targs, typename T> inline void processParameters(TypeID type, T &&value, Targs&&...args) {
                PrintfInformation::ArgumentInfo &arg = printfInfo->args[idxArgument];
                arg.type = type;
                using TYPE = std::remove_cv_t<std::remove_reference_t<std::decay_t<T>>>;
                if constexpr (std::is_same_v<TYPE, const char*> or std::is_same_v<TYPE, char*>) {
                    strcpy(arg.value.s, value);
                } else if constexpr (std::is_same_v<TYPE, std::string>) {
                    strcpy(arg.value.s, value.c_str());
                } else {
                    arg.value = *reinterpret_cast<PrintfInformation::ArgumentInfo::Argument*>(&std::forward<T>(value));
                }
                processParameters<idxArgument+1>(std::forward<Targs>(args)...);
            }
    };
    template<typename LoggerCore> LoggerProducer(LoggerCore &core) -> LoggerProducer<LoggerCore>;
}

#endif
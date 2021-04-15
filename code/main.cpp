/*
舊版nplog與nqlog使用的編譯指令：
g++ -I. -I/home/tradetron/libs/include -g -pthread -O2 -Wall -DDEBUG -DUDEBUG -D_VR_LINUX -DHAVE_SYS_RESOURCE_H -DHAVE_STRUCT_RLIMIT -std=gnu++11 -I../../../boost_1_60_0/ -c main.cpp && g++ -pthread -L. -lut -lrt -o go.old main.o uutil.o prof2.o prof.o dlist.o lru.o hash.o dec.o dset.o heap.o heapdbg.o hexdump.o plog.o uassert.o dbgprint.o nstring.o prime.o dirutil.o md5.o strtoken.o utgetopt.o tokenizer.o tcputil.o 1024.o umail.o ummap.o TimeUtil.o argtoken.o consio.o utthread.o fdlock.o uumutex.o utmem.o utmon.o dynq.o hi_util.o utthread_rwlock.o mtb_hash.o txtutil.o rbfq.o qlog.o mmq.o rbfq1.o mmq1.o mmq_mgr.o uu_assort.o uupwd.o uuthrdsafe.o udputil.o uubase64.o crc32c.o endian.o uu_cache_line.o nplog.o nqlog.o ebcdic.o utbcdmap.o dynq2.o mtb2_hash.o && ll go* && rm go.old.log
及其執行指令：
rm -f go.*.log && echo ---go.old--- && go.old 100 go.old.a go.old.b && echo ---go.new--- && go.new 100 go.new.a go.new.b && ll go*
*/

#include "nplog.h"
#include "nqlog.h"

#include <thread>
#include <time.h>

struct timespec diff(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

static nplog_t *tnplog1 = nullptr;
static nqlog_t *tnqlog1 = nullptr;
static nplog_t *tnplog2 = nullptr;
static nqlog_t *tnqlog2 = nullptr;

void nplog_write_log_func_empty(void *, const char *, ...){}
void nplog_flush_func_empty(void *){}

template<unsigned repeat> class TimeDiff {
    private:
        struct timespec time[repeat+1];
        unsigned int cnt;
        unsigned int times;
    public:
        TimeDiff(const unsigned int times): cnt(0), times(times) {}
        ~TimeDiff() { report(); }
        void stamp() { timespec_get(&time[cnt++], TIME_UTC); }
        void report() {
            struct timespec time_diff[sizeof(time)/sizeof(*time)-1];
            for (std::size_t t = 0; t < repeat; ++t)
                time_diff[t] = diff(time[t], time[t+1]);
            long time_diff_ns[sizeof(time_diff)/sizeof(*time_diff)];
            for (std::size_t t = 0; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t)
                time_diff_ns[t] = time_diff[t].tv_sec*1000000000 + time_diff[t].tv_nsec;
            printf("(%10ld", time_diff_ns[0]);
            for (std::size_t t = 1; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t) printf(",%10ld", time_diff_ns[t]);
            printf(")/%d=avg(%6ld", times, time_diff_ns[0]/times);
            for (std::size_t t = 1; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t) printf(",%6ld", time_diff_ns[t]/times);
            printf(")ns\n");
        }
};

#include <functional>
#include <random>
template<unsigned int repeat> void case1(const int idxThread, const unsigned int times) {
    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1,10);
    auto randfun = std::bind(dis, gen);
    const unsigned int delay = randfun();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    TimeDiff<repeat> timediff(times);
    timediff.stamp();
    char datatype[16] { "data-type" };
    decltype(sizeof(datatype)) datatypesize = 6;
    for (unsigned int t = 1; t <= repeat; ++t) {
        for (unsigned int i = 0; i < times; ++i) {
            nqlog_write(tnqlog1, "%p 0123456789 %d %d %d %lf %s %.*s %f %c 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n"
                , nqlog_ptr    , &i
                , nqlog_int    , idxThread
                , nqlog_uint   , t
                , nqlog_uint   , i
                , nqlog_double , i*1.6
                , nqlog_str    , "c-string-"
                , nqlog_data   , datatypesize, datatype
                , nqlog_float  , i/1.1f
                , nqlog_char   , static_cast<char>('$'+i%6)
                , nqlog_end
            );
            nqlog_write(tnqlog2, "%p 9876543210 %d %d %d %lf %s %.*s %f %c 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n"
                , nqlog_ptr    , &i
                , nqlog_int    , idxThread
                , nqlog_uint   , t
                , nqlog_uint   , i
                , nqlog_double , i*1.6
                , nqlog_str    , "c-string-"
                , nqlog_data   , datatypesize, datatype
                , nqlog_float  , i/1.1f
                , nqlog_char   , static_cast<char>('$'+i%6)
                , nqlog_end
            );
        }
        timediff.stamp();
    }
    printf("thread%3d(%2dms): ", idxThread, delay);
}

int main(int argc, char **argv) {
    if (argc < 4) return 0;
    int times = atoi(argv[1]);
    char logFName1[100];
    char logFName2[100];
    snprintf(logFName1, sizeof(logFName1), "%s.log", argv[2]);
    snprintf(logFName2, sizeof(logFName2), "%s.log", argv[3]);
    npLogOpen(tnplog1, logFName1, 0 /* autoFlushFlg */ );
    npLogOpen(tnplog2, logFName2, 0 /* autoFlushFlg */ );
    nqlog_open(tnqlog1, (char *) "ftt_nqlog" /* nqlog_id */,
                    tnplog1,
                    nplog_write_log_func,
                    nplog_flush_func, 20 /* autoFlushTimeInt */ ,
                    50); /* preAllocLogMsgNodeCount */
    nqlog_open(tnqlog2, (char *) "ftt_nqlog" /* nqlog_id */,
                    tnplog2,
                    nplog_write_log_func,
                    nplog_flush_func, 20 /* autoFlushTimeInt */ ,
                    50); /* preAllocLogMsgNodeCount */
    struct timespec time[3];
    timespec_get(&time[0], TIME_UTC);
    std::thread threads[4];
    for (int idxThread = sizeof(threads)/sizeof(*threads); --idxThread >= 0; ) {
        threads[idxThread] = std::thread(case1<4>, idxThread, times);
    }
    for (auto &thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    timespec_get(&time[1], TIME_UTC);
    nqlog_close(tnqlog1, -1 /* wait for flush all possible queued log msg */ );
    nqlog_close(tnqlog2, -1 /* wait for flush all possible queued log msg */ );
    npLogClose(tnplog1);
    npLogClose(tnplog2);
    timespec_get(&time[2], TIME_UTC);
    struct timespec time_ab = diff(time[0], time[1]);
    struct timespec time_bc = diff(time[1], time[2]);
    long time_used_ab = time_ab.tv_sec*1000000000 + time_ab.tv_nsec;
    long time_used_bc = time_bc.tv_sec*1000000000 + time_bc.tv_nsec;
    printf("total: sum(%6ld,%6ld) = %6ldms\n", time_used_ab/1000, time_used_bc/1000, (time_used_ab+time_used_bc)/1000000);
}
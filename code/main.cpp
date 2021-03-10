#include "Logger.hpp"

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

nplog_t tnplog;
nqlog_t tnqlog;

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
			for (int t = 0; t < sizeof(time)/sizeof(*time); ++t)
				time_diff[t] = diff(time[t], time[t+1]);
			long time_diff_ns[sizeof(time_diff)/sizeof(*time_diff)];
			for (int t = 0; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t)
				time_diff_ns[t] = time_diff[t].tv_sec*1000000000 + time_diff[t].tv_nsec;
			printf("(%10ld", time_diff_ns[0]);
			for (int t = 1; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t) printf(",%10ld", time_diff_ns[t]);
			printf(")/%d=avg(%6ld", times, time_diff_ns[0]/times);
			for (int t = 1; t < sizeof(time_diff_ns)/sizeof(*time_diff_ns); ++t) printf(",%6ld", time_diff_ns[t]/times);
			printf(")ns\n");
		}
};

volatile static bool start_case1 = false;
template<unsigned repeat> void case1(const int idxThread, const unsigned int times) {
	while (!start_case1) {}
	TimeDiff<repeat> timediff(times);
	timediff.stamp();
	for (int t = 1; t <= repeat; ++t) {
		for (int i = 0; i < times; ++i) {
			nqlog_write(&tnqlog, "0123456789 %d %d %d %p %lf %s %.*s %f %c 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n"
				, nqlog_int    , idxThread
				, nqlog_int    , t
				, nqlog_int    , i
				, nqlog_ptr    , &i
				, nqlog_double , i*1.6
				, nqlog_str    , "c-string-"
				, nqlog_data   , 6, "data-type"
				, nqlog_float  , i/1.1f
				, nqlog_char   , static_cast<char>('$'+i%6)
				, nqlog_end
			);
		}
		timediff.stamp();
	}
	printf("thread%3d: ", idxThread);
}

int main(int argc, char **argv) {
	if (argc < 2) return 0;
	int times = atoi(argv[1]);
	char logFName[100];
	const char tbase[] = "go.old";
	snprintf(logFName, sizeof(logFName), "%s.log", tbase);
	npLogOpen( &tnplog, logFName, 0 /* autoFlushFlg */ );
	nqlog_open( &tnqlog, (char *) "ftt_nqlog" /* nqlog_id */,
					&tnplog,
					nplog_write_log_func,
					nplog_flush_func, 20 /* autoFlushTimeInt */ ,
					50); /* preAllocLogMsgNodeCount */
	struct timespec time[3];
	timespec_get(&time[0], TIME_UTC);
	std::thread threads[4];
	for (int idxThread = sizeof(threads)/sizeof(*threads); --idxThread >= 0; ) {
		threads[idxThread] = std::thread(case1<4>, idxThread, times);
	}
	start_case1 = true;
	for (auto &thread: threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	timespec_get(&time[1], TIME_UTC);
	nqlog_close(&tnqlog, -1 /* wait for flush all possible queued log msg */ );
	npLogClose(&tnplog);
	timespec_get(&time[2], TIME_UTC);
	struct timespec time_ab = diff(time[0], time[1]);
	struct timespec time_bc = diff(time[1], time[2]);
	long time_used_ab = time_ab.tv_sec*1000000000 + time_ab.tv_nsec;
	long time_used_bc = time_bc.tv_sec*1000000000 + time_bc.tv_nsec;
	printf("total: sum(%6ld,%6ld) = %6ldms\n", time_used_ab/1000, time_used_bc/1000, (time_used_ab+time_used_bc)/1000000);
}
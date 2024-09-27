#include<malloc.h>
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string.h>
#include<time.h>
#include<limits>
#include<omp.h>
#include<thread>
#include<vector>
#include<sstream>
#include<algorithm>
#include<cmath>
#include<numeric>
#include<iomanip>
#include<map>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

using namespace std;

#define PRECISION (4)

#define BUFFER_SIZE 1024*1024*1024
char BUFFER[BUFFER_SIZE];

std::stringstream nullbuff;

typedef struct {
    timespec start;
    timespec stop;
    timespec sum;
} t_timemes;

#define t_init(t) t.sum.tv_nsec = 0; t.sum.tv_sec = 0
#define t_start(t) clock_gettime(CLOCK_MONOTONIC, &(t.start))
#define t_stop(t) clock_gettime(CLOCK_MONOTONIC, &(t.stop)); \
    t.sum.tv_sec += t.stop.tv_sec - t.start.tv_sec; \
    t.sum.tv_nsec += t.stop.tv_nsec - t.start.tv_nsec
#define t_get_seconds(t) (double)t.sum.tv_sec + (double)t.sum.tv_nsec / (double)1000000000

inline double test_memset() {
    t_timemes time = {0};
    t_init(time);
    t_start(time);
    
    #pragma omp parallel for
    for (long long i = 0; i < BUFFER_SIZE; i += 1048576) {
        memset(BUFFER + i, 0, 1048576);
    }
    t_stop(time);
    
    return t_get_seconds(time);
}

inline double test_memchr() {
    t_timemes time = {0};
    t_init(time);
    t_start(time);

    #pragma omp parallel for    
    for (long long i = 0; i < BUFFER_SIZE; i += 1048576) {
        char *c = NULL;
        c = (char *)memchr(BUFFER + i, 'Q', 1048576);
        if (c != NULL) {
            nullbuff << "Char on returned position: " << c[0] << endl;
            nullbuff << "Position                 : " << (long long)(c - BUFFER) << endl;
        }
    }

    t_stop(time);
    
    return t_get_seconds(time);
}

map<int, vector<double>> run_test(double (*fun)(), int nloops, int threads) {
    map<int, vector<double>> bench_times;
    for (int th = 1; th <= threads; th++) {
        omp_set_num_threads(th);
        bench_times.insert(pair<int, vector<double>>(th, vector<double>()));
        for (int i = 0; i < nloops; i++) {
            bench_times[th].push_back(fun());
        }
    }
    return bench_times;
}

double calculate_speed(double time_seconds) {
    // Speed in MB/s = (buffer size in MB) / time in seconds
    return ((double)(BUFFER_SIZE / 1000 / 1000)) / time_seconds;
}

double get_average_speed(map<int, vector<double>> res) {
    vector<double> all_speeds;
    for (auto &entry : res) {
        for (double time : entry.second) {
            all_speeds.push_back(calculate_speed(time));  // Convert time to speed (MB/s)
        }
    }
    double sum = accumulate(all_speeds.begin(), all_speeds.end(), 0.0);
    return sum / all_speeds.size();
}

void export_average_speed_to_env(double average_speed) {
    std::stringstream ss;
    ss << fixed << setprecision(PRECISION) << average_speed;
    string avg_str = ss.str();
    cout << "MEMTESTRESULT=" << avg_str << endl;  // Print the result for the shell script
}

void perform_benchmark() {

    cout << "======================================================================" << endl;
    cout << "BENCHMARKING RAM WITH MULTI THREADS" << endl;
    cout << "(...please wait...)" << endl;
    cout << endl;

    unsigned int nth = std::thread::hardware_concurrency();
    cout << nth << " concurrent threads are supported." << endl << endl;

    double overall_speed_sum = 0.0;
    int total_tests = 0;

    // MEMSET TEST
    cout << "----------------------------------------------------------------------" << endl;
    cout << "MEMSET TEST" << endl;

    map<int, vector<double>> memset_res = run_test(test_memset, 10, nth);
    double memset_avg_speed = get_average_speed(memset_res);
    overall_speed_sum += memset_avg_speed;
    total_tests++;

    cout << "Average speed for MEMSET: " << fixed << setprecision(PRECISION) << memset_avg_speed << " MB/s" << endl << endl;

    // MEMCHR TEST
    cout << "----------------------------------------------------------------------" << endl;
    cout << "MEMCHR TEST" << endl;
    
    nullbuff << "Last char                : " << BUFFER[BUFFER_SIZE - 1] << endl;
    BUFFER[BUFFER_SIZE - 1] = 'Q';

    map<int, vector<double>> memchr_res = run_test(test_memchr, 10, nth);
    double memchr_avg_speed = get_average_speed(memchr_res);
    overall_speed_sum += memchr_avg_speed;
    total_tests++;

    cout << "Average speed for MEMCHR: " << fixed << setprecision(PRECISION) << memchr_avg_speed << " MB/s" << endl << endl;

    // Calculate overall average speed across all tests
    double final_average_speed = overall_speed_sum / total_tests;
    cout << "----------------------------------------------------------------------" << endl;
    cout << "Overall Average Speed for all tests: " << fixed << setprecision(PRECISION) << final_average_speed << " MB/s" << endl;

    // Export the average speed result to the environment variable
    export_average_speed_to_env(final_average_speed);

    cout << "======================================================================" << endl;
}

int main() {
    perform_benchmark();
    return 0;
}

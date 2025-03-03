
#include <stdio.h>

typedef unsigned long long u64;
#if _WIN32
#include <intrin.h>
#include <windows.h>

static u_int64 GetOsTimeFreq(void) {
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);

    return Freq.QuadPart;
}

static u_int64 ReadOSTimer(void) {
    LARGE_INTEGER Value;
    QueryPerformanceCounter(&Value);

    return Value.QuadPart;
}

static u_int64 ReadCpuTimer(void) {
    return _rdtsc();
}

static u_int64 GetCpuFreq(u_int64 cpuElapsed, u_int64 osElapsed) {
    u_int64 osFreq = GetOsTimeFreq();
    return osFreq * cpuElapsed / osElapsed;
}
#endif

#if __APPLE__
#include <sys/time.h>
static u64 GetOsTimeFreq(void) {
    return 1000000;
}

static u64 ReadOSTimer(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

static u64 ReadCpuTimer(void) {
    u64 cnt;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));

    return cnt;
}

static u64 GetCpuFreq(void) {
    u64 cnt;
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(cnt));

    return cnt;
}

#endif


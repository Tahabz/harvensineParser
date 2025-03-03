
#include <windows.h>
#include <intrin.h>
#include <stdio.h>

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
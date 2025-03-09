#include <stdio.h>
#include <string.h>
typedef unsigned long long u64;
#if _WIN32
#include <intrin.h>
#include <windows.h>

static u_int64 GetOsTimeFreq(void)
{
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);

    return Freq.QuadPart;
}

static u_int64 ReadOSTimer(void)
{
    LARGE_INTEGER Value;
    QueryPerformanceCounter(&Value);

    return Value.QuadPart;
}

static u_int64 ReadCpuTimer(void)
{
    return _rdtsc();
}

static u_int64 GetCpuFreq(u_int64 cpuElapsed, u_int64 osElapsed)
{
    u_int64 osFreq = GetOsTimeFreq();
    return osFreq * cpuElapsed / osElapsed;
}
#endif

#if __APPLE__
#include <sys/time.h>
static u64 GetOsTimeFreq(void)
{
    return 1000000;
}

static u64 ReadOSTimer(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

static u64 ReadCpuTimer(void)
{
    u64 cnt;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));

    return cnt;
}

static u64 GetCpuFreq(void)
{
    u64 cnt;
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(cnt));

    return cnt;
}

#endif

typedef struct
{
    const char *label;
    u64 inclusive;
    u64 exclusive;
    unsigned char hit;
    u64 bytesProcessed;
} anchor;

typedef struct
{
    u64 start;
    u64 parentIndex;
    u64 oldTSCElapsed;
    unsigned char anchorIndex;
} blockData;

typedef struct
{
    anchor anchors[20];
    u64 start;
    u64 elapsed;
} profilerData;

static profilerData data;
u64 globalParentIndex = 0;

static int getPercentile(u64 max, u64 var)
{
    return var * 100 / max;
}

static void printData(anchor *anchor)
{
    float mb = (float)anchor->bytesProcessed / (1024*1024);
    float gb = mb / 1024;
    printf("%s[%d] = %llu (%d%%)",
           anchor->label,
           anchor->hit,
           anchor->inclusive,
           getPercentile(data.elapsed, anchor->exclusive));
    if (anchor->bytesProcessed) {
        printf(" %.3fmb at %.3fgb/s",
            mb,
            gb * GetCpuFreq() / anchor->inclusive
            );
    }
    if (anchor->exclusive != anchor->inclusive)
    {
        printf("  w/children %d%%\n", getPercentile(data.elapsed, anchor->inclusive));
    }
    else
        printf("\n");
}


static void init_profile()
{
    data.start = ReadCpuTimer();
}

static void _end_profile(unsigned char counter)
{
    u64 end = ReadCpuTimer();
    u64 elapsed = end - data.start;
    data.elapsed = elapsed;
    unsigned char i = 1;
    printf("CLOCK FREQ: %lluHz\n", GetCpuFreq());
    printf("TOTAL = %llu (%.2fms) \n\n\n", data.elapsed, (float)data.elapsed * 1000 / GetCpuFreq());
    while (i < counter)
    {
        printData(&data.anchors[i]);
        i += 1;
    }
}

#define END_PROFILE _end_profile(__COUNTER__ + 1)
#define INIT_PROFILE init_profile()

#ifdef PROFILER
static void init_block(blockData *block, int anchorIndex, const char *label, u64 bytes)
{
    block->parentIndex = globalParentIndex;
    block->anchorIndex = anchorIndex;
    data.anchors[anchorIndex].label = label;
    data.anchors[anchorIndex].bytesProcessed += bytes;
    block->start = ReadCpuTimer();
    block->oldTSCElapsed = data.anchors[anchorIndex].inclusive;
    globalParentIndex = anchorIndex;
}

#define PROFILE_FUNCTION_WB(bytes) PROFILE_BLOCK_WB(__func__, bytes)

#define PROFILE_BLOCK_WB(label, bytes) \
    int anchorIndex = __COUNTER__ + 1;   \
    blockData block; \
    init_block(&block, anchorIndex, label, bytes);  \
    __attribute__((cleanup(_block_cleanup))) blockData blockD = block;

static void _block_cleanup(blockData *block)
{
    anchor *anchor = &data.anchors[block->anchorIndex];
    globalParentIndex = block->parentIndex;
    u64 end = ReadCpuTimer();
    u64 elapsed = end - block->start;
    data.anchors[block->parentIndex].exclusive -= elapsed;
    anchor->exclusive += elapsed;
    anchor->inclusive = elapsed + block->oldTSCElapsed;
    anchor->hit += 1;
}

#define PROFILE_FUNCTION PROFILE_FUNCTION_WB(0)
#define PROFILE_BLOCK(label) PROFILE_BLOCK_WB(label, 0)
#else
    #define PROFILE_BLOCK(...)
    #define PROFILE_BLOCK_WB(...)
    #define PROFILE_FUNCTION
    #define PROFILE_FUNCTION_WB(...)
#endif
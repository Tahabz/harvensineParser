
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
    u64 cpuElapsed;
    u64 childrenElapsed;
    u64 rootElapsed;
    unsigned char hit;
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

static void init_block(blockData *block, int anchorIndex, const char *label)
{
    block->parentIndex = globalParentIndex;
    block->anchorIndex = anchorIndex;
    data.anchors[anchorIndex].label = label;
    block->start = ReadCpuTimer();
    block->oldTSCElapsed = data.anchors[anchorIndex].rootElapsed;
    globalParentIndex = anchorIndex;
}

#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

#define PROFILE_BLOCK(label) \
    int anchorIndex = __COUNTER__ + 1;   \
    blockData block; \
    init_block(&block, anchorIndex, label);  \
    __attribute__((cleanup(_block_cleanup))) blockData blockD = block;

static void _block_cleanup(blockData *block)
{
    anchor *anchor = &data.anchors[block->anchorIndex];
    globalParentIndex = block->parentIndex;
    u64 end = ReadCpuTimer();
    u64 elapsed = end - block->start;
    anchor->cpuElapsed += elapsed;
    data.anchors[block->parentIndex].childrenElapsed += elapsed;
    anchor->rootElapsed = elapsed + block->oldTSCElapsed;
    anchor->hit += 1;
}

static void init_profile()
{
    data.start = ReadCpuTimer();
}

static int getPercentile(u64 max, u64 var)
{
    return var * 100 / max;
}

static void printData(anchor *anchor)
{
    printf("%s[%d] = %llu (%d%%)",
           anchor->label,
           anchor->hit,
           anchor->rootElapsed,
           getPercentile(data.elapsed, anchor->cpuElapsed - anchor->childrenElapsed));
    if (anchor->childrenElapsed)
    {
        printf("  w/children %d%%\n", getPercentile(data.elapsed, anchor->rootElapsed));
    }
    else
        printf("\n");
}


#define END_PROFILE _end_profile(__COUNTER__ + 1)
#define INIT_PROFILE init_profile()

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
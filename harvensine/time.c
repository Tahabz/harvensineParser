
#include <stdio.h>
#include <string.h>
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


typedef struct {
    const char *blockName;
    u64 start;
    u64 cpuElapsed;
} blockData;

typedef struct {
    blockData blocks[20];
    unsigned char count;
} profilerData;

profilerData data;

int cmp(const char *str1, const char *str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);

    if (len1 != len2) return 0;
    int i = 0;
    while (i < len1) {
        if (str1[i] != str2[i]) return 0;
        i += 1;
    }

    return 1;
}

int blockPosByName(const char *name) {
    unsigned char i = 0;
    while (i < data.count) {
        if (cmp(data.blocks[i].blockName, name)) {
            return i;
        }
        i += 1;
    }
    return -1;
}

static int init_block(const char *blockName) {
    blockData *block = &data.blocks[data.count];
    int pos = blockPosByName(blockName);
    if (pos != -1)  block = &data.blocks[pos];
    else
    {
        block->blockName = blockName;
        pos = data.count;
        data.count += 1;
        block->cpuElapsed = 0;
    }
    block->start = ReadCpuTimer();
    return pos;
}

#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

#define PROFILE_BLOCK(blockName) \
    int pos = init_block(blockName); \
    __attribute__((cleanup(_block_cleanup))) int blockPos = pos;

static void _block_cleanup(int *blockPos) {
    blockData *block = &data.blocks[*blockPos];
    u64 end = ReadCpuTimer();
    u64 elapsed = end - block->start;
    block->cpuElapsed += elapsed;
}

static void INIT_PROFILE() {
    data.count = 0;
    data.blocks[0].start = ReadCpuTimer();
    data.blocks[0].blockName = "TOTAL";
    data.count += 1;
}

static int getPercentile(u64 max, u64 var) {
    return var * 100 / max;
}

static void printData (const char *name, u64 elapsed, u64 total) {
    printf("%s = %llu (%d%%)\n", name, elapsed, getPercentile(total, elapsed));   
}

static void END_PROFILE()
{
    u64 end = ReadCpuTimer();
    u64 elapsed = end - data.blocks[0].start;
    data.blocks[0].cpuElapsed = elapsed;
    unsigned char i = 0;
    while (i < data.count) {
        printData(data.blocks[i].blockName, data.blocks[i].cpuElapsed, elapsed);
        i += 1;
    }
}

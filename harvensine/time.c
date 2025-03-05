
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
    u64 childrenElapsed;
    u64 parentIndex;
    unsigned char hit;
} blockData;

typedef struct {
    blockData blocks[20];
} profilerData;

static profilerData data;
static u64 globalParentIndex;

static void init_block(const char *blockName, int pos) {
    blockData *block = &data.blocks[pos];
    block->parentIndex = globalParentIndex;
    block->blockName = blockName;
    block->start = ReadCpuTimer();
    globalParentIndex = pos;
}

#define PROFILE_FUNCTION PROFILE_BLOCK(__func__)

#define PROFILE_BLOCK(blockName) \
    int pos = __COUNTER__ + 1; \
    init_block(blockName, pos); \
    __attribute__((cleanup(_block_cleanup))) int blockPos = pos;

static void _block_cleanup(int *blockPos) {
    blockData *block = &data.blocks[*blockPos];
    globalParentIndex = block->parentIndex;
    u64 end = ReadCpuTimer();
    u64 elapsed = end - block->start;
    block->cpuElapsed += elapsed;
    data.blocks[block->parentIndex].childrenElapsed += elapsed;
    block->hit += 1;
}

static void INIT_PROFILE() {
    data.blocks[0].start = ReadCpuTimer();
    data.blocks[0].blockName = "TOTAL";
}

static int getPercentile(u64 max, u64 var) {
    return var * 100 / max;
}

static void printData (blockData block, u64 total) {
    printf("%s[%d] = %llu (%d%%)",
        block.blockName,
        block.hit,
        block.cpuElapsed,
        getPercentile(total, block.cpuElapsed - block.childrenElapsed)
    );
    if (block.childrenElapsed) {
        printf("  w/children %d%%\n", getPercentile(total, block.cpuElapsed));
    } else printf("\n");
}


#define END_PROFILE _end_profile(__COUNTER__ + 1)
#define INIT_PROFILE INIT_PROFILE()

static void _end_profile(unsigned char counter)
{
    u64 end = ReadCpuTimer();
    u64 elapsed = end - data.blocks[0].start;
    data.blocks[0].cpuElapsed = elapsed;
    unsigned char i = 0;
    while (i < counter) {
        printData(data.blocks[i], elapsed);
        i += 1;
    }
}

#include "parser.h"
#include "referenceHarvensince.c"

int main(int argc, char **argv) {

    int fd = open("./harvensine.json", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dprintf(fd, "[");
    int i = 0;
    float count = 10000;
    float res = 0;
    if (!argv[1]) {
        printf("ENTER THE FUCKING SEED!!\n");
        return -1;
    }
    unsigned int seed = atoi(argv[1]); // Change this value to get different sequences
    srand(seed);

    while (i < count)
    {
        float x0 = random_float(0, 0);
        float x1 = random_float(0, 0);
        float y0 = random_float(0, 0);
        float y1 = random_float(0, 0);
        
        // printf("x0=%d, y0=%d, x1=%d, y1=%d\n", x0, y0, x1, y1);
        res += ReferenceHaversine(x0, x1, y0, y1, 6372.8);
        
        dprintf(fd, "{\"x0\":%f,\"y0\":%f,\"x1\":%f,\"y1\":%f}",
            x0,
            y0,
            x1,
            y1
        );
        if ((i + 1) != count)
           dprintf(fd, ",");
        i+=1;
    }
    printf("%d\n", i);
    dprintf(fd, "]\n");
    printf("pi=%d, total=%f, average=%f\n", i, res, res/i);
    close(fd);
    return 0;
}
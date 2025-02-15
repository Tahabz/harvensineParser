#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    unsigned char w : 1;
    unsigned char d : 1;
    unsigned char op : 6;
} Byte1;

typedef struct {
    unsigned char rm : 3;
    unsigned char reg : 3;
    unsigned char mod : 2;
} Byte2;

typedef struct {
    unsigned char reg : 3;
    unsigned char w : 1;
    unsigned char op : 4;
} ImByte1;


char *RMOD_table[8][2] = {
    {"AL", "AX"},
    {"CL", "CX"},
    {"DL", "DX"},
    {"BL", "BX"},
    {"AH", "SP"},
    {"CH", "BP"},
    {"DH", "SI"},
    {"BH", "DI"}};

char *MMOD_table[8] = {
    "BX + SI",
    "BX + DI",
    "BP + SI",
    "BP + DI",
    "SI",
    "DI",
    "BP",
    "BX"
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("please enter the file name!");
        return EXIT_FAILURE;
    }

    unsigned char buffer[2];
    char *file_name = argv[1];
    FILE *output;
    int fd = open(file_name, O_RDONLY);

    if (fd < 0)
    {
        printf("source does not exist or can't be read");
        return EXIT_FAILURE;
    }

    if (!(output = fopen("output.asm", "w")))
    {
        printf("output can't be created");
        return EXIT_FAILURE;
    }
    fprintf(output, "BITS 16\n\n");
    while (read(fd, &buffer, 2) > 0)
    {
        if (buffer[0] >> 1 == 0b01100011) {
            Byte1 *byte1 = (Byte1 *)&buffer[0];
            Byte2 *byte2 = (Byte2 *)&buffer[1];
            if (byte2->mod == 0b00) {
                if (byte2->rm == 0b00000110) {
                    unsigned short dh;
                    read(fd, &dh, 2);
                    if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("MOV [%d], %d\n", dh, hdata);
                    } else {
                        unsigned char ldata;
                        read(fd, &ldata, 1);
                        printf("MOV [%d], %d\n", dh, ldata);
                    }
                }
                else if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("MOV [%s], %d\n", MMOD_table[byte2->rm], hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("MOV [%s], %d\n", MMOD_table[byte2->rm], ldata);
                }
            } else if (byte2->mod == 0b01) {
                unsigned char dl;
                read(fd, &dl, 1);
                if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("MOV [%s + %d], %d\n", MMOD_table[byte2->rm], dl, hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("MOV [%s + %d], %d\n", MMOD_table[byte2->rm], dl, ldata);
                }
            }
        }
        else if (buffer[0] >> 4 == 0b00001011) {
            ImByte1 *imByte1 = (ImByte1 *)&buffer[0];
            unsigned char d[2];
            d[0] = buffer[1];
            if (imByte1->w) {
                read(fd, d + 1, 1);
                printf("MOV %s, %d\n", RMOD_table[imByte1->reg][imByte1->w], *(unsigned short *)d);
            } else {
                printf("MOV %s, %d\n", RMOD_table[imByte1->reg][imByte1->w], d[0]);
            }
        } else {




            Byte1 *byte1 = (Byte1 *)&buffer[0];
            Byte2 *byte2 = (Byte2 *)&buffer[1];
        
            // register to register
            if (byte2->mod == 0b11) {
                printf("MOV %s, %s\n", RMOD_table[byte2->rm][byte1->w], RMOD_table[byte2->reg][byte1->w]);
            // no displacement
            } else if (byte2->mod == 0b00) {
                unsigned char dh[2];
                if (byte1->d == 0b0) {
                    if (byte2->rm == 0b00000110) {
                        read(fd, &dh, 2);
                        printf("MOV [%d], %s\n", *((unsigned short *)dh), RMOD_table[byte2->reg][byte1->w]);
                    }
                    else {
                        printf("MOV [%s], %s\n", MMOD_table[byte2->rm], RMOD_table[byte2->reg][byte1->w]);
                    }
                }
                else {
                    if (byte2->rm == 0b00000110) {
                        read(fd, &dh, 2);
                        printf("MOV %s, [%d]\n", RMOD_table[byte2->reg][byte1->w], *((unsigned short *)dh));
                    } else {
                        printf("MOV %s, [%s]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm]);
                    }
                }
            } else if (byte2->mod == 0b01) {
                unsigned char dl;
                read(fd, &dl, 1);
                if (byte1->d == 0b0) {
                    //TODO: 110 is a special case
                    printf("MOV [%s + %d], %s\n", MMOD_table[byte2->rm], dl, RMOD_table[byte2->reg][byte1->w]);
                } else {
                    printf("MOV %s, [%s + %d]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm], dl);
                }
            } else if (byte2->mod == 0b10) {
                unsigned char dh[2];
                read(fd, &dh, 2);
                if (byte1->d == 0b0) {
                    //TODO: 110 is a special case
                    printf("MOV [%s + %d], %s\n", MMOD_table[byte2->rm], *((unsigned short *)dh), RMOD_table[byte2->reg][byte1->w]);
                } else {
                    printf("MOV %s, [%s + %d]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm], *((unsigned short *)dh));
                }
            }
        }

    }

    close(fd);
    fclose(output);

    return 0;
}
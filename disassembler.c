#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

unsigned short mem[8] = {0};

// enum REGS {
//   a,
//   b,
//   c,
//   d,
//   sp,
//   bp,
//   si,
//   di
// };

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
        Byte1 *byte1 = (Byte1 *)&buffer[0];
        Byte2 *byte2 = (Byte2 *)&buffer[1];
        int regMem = 0;
        int imRegMem = 0;
        int imReg = 0;

        if (buffer[0] >> 1 == 0b01100011) {
            printf("MOV ");
            imRegMem = 1;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b000) {
            printf("ADD ");
            imRegMem = 1;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b101) {
            printf("SUB ");
            imRegMem = 1;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b111) {
            printf("CMP ");
            imRegMem = 1;
        } else if (buffer[0] >> 4 == 0b00001011) {
            printf("MOV ");
            imReg = 1;
        } else if (buffer[0] >> 1 == 0b00000010) {
            printf("ADD ");
            imReg = 1;
        } else if (buffer[0] >> 1 == 0b00010110) {
            printf("SUB ");
            imReg = 1;
        } else if (buffer[0] >> 1 == 0b00011110) {
            printf("CMP ");
            imReg = 1;
        } else if (buffer[0] >> 2 == 0b00100010) {
            printf("MOV ");
            regMem = 1;
        } else if (buffer[0] >> 2 == 0b0) {
            printf("ADD ");
            regMem = 1;
        } else if (buffer[0] >> 2 == 0b00001010) {
            printf("SUB ");
            regMem = 1;
        } else if (buffer[0] >> 2 == 0b00001110) {
            printf("CMP ");
            regMem = 1;
        }

        if (imRegMem) {
            if (byte2->mod == 0b00) {
                if (byte2->rm == 0b00000110) {
                    unsigned short dh;
                    read(fd, &dh, 2);
                    if (byte1->w) {
                        unsigned short hdata;
                        read(fd, &hdata, 2);
                        printf("[%d], %d\n", dh, hdata);
                    } else {
                        unsigned char ldata;
                        read(fd, &ldata, 1);
                        printf("[%d], %d\n", dh, ldata);
                    }
                }
                else if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("[%s], %d\n", MMOD_table[byte2->rm], hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("[%s], %d\n", MMOD_table[byte2->rm], ldata);
                }
            } else if (byte2->mod == 0b01) {
                unsigned char dl;
                read(fd, &dl, 1);
                if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dl, hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dl, ldata);
                }
            } else if (byte2->mod == 0b10) {
                unsigned short dh;
                read(fd, &dh, 2);
                if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dh, hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dh, ldata);
                }
            } else {
                if (byte1->w) {
                    unsigned short hdata;
                    read(fd, &hdata, 2);
                    printf("%s, %d\n", RMOD_table[byte2->rm][byte1->w], hdata);
                } else {
                    unsigned char ldata;
                    read(fd, &ldata, 1);
                    printf("%s, %d\n", RMOD_table[byte2->rm][byte1->w], ldata);
                }
            }
        }
        //[]
        else if (imReg) {
            ImByte1 *imByte1 = (ImByte1 *)&buffer[0];
            unsigned char d[2];
            d[0] = buffer[1];
            if (imByte1->w) {
                read(fd, d + 1, 1);
                printf("%s, %d; %d -> %d\n",
                    RMOD_table[imByte1->reg][imByte1->w],
                    *(unsigned short *)d,
                    mem[imByte1->reg],
                    *(unsigned short *)d
                );
                mem[imByte1->reg] = *(unsigned short *)d;
            }
            else {
                unsigned char *reg;
                if (imByte1->reg < 0b100 && !imByte1->w) {
                    reg = (unsigned char *)&mem[imByte1->reg];
                    printf("%s, %d; %d -> %d\n",
                        RMOD_table[imByte1->reg][imByte1->w],
                        d[0],
                        *(reg + 1),
                        d[0]
                    );
                    *(reg + 1) = d[0];
                } else {
                    reg = (unsigned char *)&mem[imByte1->reg - 4];
                    printf("%s, %d; %d -> %d\n",
                        RMOD_table[imByte1->reg][imByte1->w],
                        d[0],
                        *(reg),
                        d[0]
                    );
                    *reg = d[0];
                }
            }
        } else {
        
            // register to register
            if (byte2->mod == 0b11) {
                printf("%s, %s; ", RMOD_table[byte2->rm][byte1->w], RMOD_table[byte2->reg][byte1->w]);
                if (byte1->w) {
                    printf("%d -> %d\n", mem[byte2->rm], mem[byte2->reg]);
                    mem[byte2->rm] = mem[byte2->reg];
                }
                else {
                    unsigned char *dd;
                    unsigned char *sd;
                    if (byte2->rm > 0b100) {
                        dd = ((unsigned char *)&mem[byte2->rm - 4]);
                    }
                    else {
                        dd = ((unsigned char *)&mem[byte2->rm] + 1);
                    }
                    if (byte2->reg > 0b100) {
                        sd = ((unsigned char *)&mem[byte2->reg - 4]);
                    }
                    else {
                        sd = ((unsigned char *)&mem[byte2->reg] + 1);
                    }
                    printf("%d -> ", *dd);
                    *dd = *sd;
                    printf("%d\n", *dd);
                }
            // no displacement
            } else if (byte2->mod == 0b00) {
                unsigned char dh[2];
                if (byte1->d == 0b0) {
                    if (byte2->rm == 0b00000110) {
                        read(fd, &dh, 2);
                        printf("[%d], %s\n", *((unsigned short *)dh), RMOD_table[byte2->reg][byte1->w]);
                    }
                    else {
                        printf("[%s], %s\n", MMOD_table[byte2->rm], RMOD_table[byte2->reg][byte1->w]);
                    }
                }
                else {
                    if (byte2->rm == 0b00000110) {
                        read(fd, &dh, 2);
                        printf("%s, [%d]\n", RMOD_table[byte2->reg][byte1->w], *((unsigned short *)dh));
                    } else {
                        printf("%s, [%s]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm]);
                    }
                }
            } else if (byte2->mod == 0b01) {
                unsigned char dl;
                read(fd, &dl, 1);
                if (byte1->d == 0b0) {
                    //TODO: 110 is a special case
                    printf("[%s + %d], %s\n", MMOD_table[byte2->rm], dl, RMOD_table[byte2->reg][byte1->w]);
                } else {
                    printf("%s, [%s + %d]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm], dl);
                }
            } else if (byte2->mod == 0b10) {
                unsigned char dh[2];
                read(fd, &dh, 2);
                if (byte1->d == 0b0) {
                    //TODO: 110 is a special case
                    printf("[%s + %d], %s\n", MMOD_table[byte2->rm], *((unsigned short *)dh), RMOD_table[byte2->reg][byte1->w]);
                } else {
                    printf("%s, [%s + %d]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm], *((unsigned short *)dh));
                }
            }
        }

    }

    printf("----------RESULT-----------\n");
    printf("AX=%d\n", mem[0]);
    // printf("AH=%d\n", mem[0] & 0xFF);
    // printf("AL=%d\n", (mem[0] >> 8) & 0xFF);
    printf("CX=%d\n", mem[1]);
    // printf("CH=%d\n", mem[1] & 0xFF);
    // printf("CL=%d\n", (mem[1] >> 8) & 0xFF);
    printf("DX=%d\n", mem[2]);
    // printf("DH=%d\n", mem[2] & 0xFF);
    // printf("DL=%d\n", (mem[2] >> 8) & 0xFF);
    printf("BX=%d\n", mem[3]);
    // printf("BH=%d\n", *((unsigned char *)&mem[3]));
    // printf("BL=%d\n", *((unsigned char *)&mem[3] + 1));
    printf("SP=%d\n", mem[4]);
    printf("BP=%d\n", mem[5]);
    printf("SI=%d\n", mem[6]);
    printf("DI=%d\n", mem[7]);

    close(fd);
    fclose(output);

    return 0;
}
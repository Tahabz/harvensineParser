#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

unsigned short RegMem[8] = {0};
unsigned char flags[2] = {0};
unsigned char memory[1024];
int i = 0;

enum ops {
  MOV,
  ADD,
  SUB,
  CMP,
};

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

unsigned char MMOD_map[8][2] = {
    {3, 6},
    {3, 2},
    {5, 6},
    {5, 7},
    {6, 0},
    {7, 0},
    {5, 0},
    {3, 0}
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("please enter the file name!");
        return EXIT_FAILURE;
    }

    unsigned char buffer[1000];
    char *file_name = argv[1];
    FILE *output;
    int fd = open(file_name, O_RDONLY);
    enum ops op;
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
    read(fd, &buffer, 1000);
    while (buffer[i])
    {
        unsigned char opBuffer[2] = { buffer[i], buffer[i + 1] };
        Byte1 *byte1 = (Byte1 *)&buffer[i];
        ImByte1 *imByte1 = (ImByte1 *)&buffer[i];
        i += 1;
        Byte2 *byte2 = (Byte2 *)&buffer[i];
        i += 1;
        int regMem = 0;
        int imRegMem = 0;
        int imReg = 0;
    //[, , , b0]
        if (opBuffer[0] == 0b01110101) {
            if (!flags[1]) {
                i += *((char *)&buffer[i - 1]);
                printf("IP=%d\n", i);
                continue;
            }
            printf("IP=%d\n", i);
        } else if (opBuffer[0] >> 1 == 0b01100011) {
            printf("MOV ");
            imRegMem = 1;
            op = MOV;
        } else if (opBuffer[0] >> 2 == 0b00100000 && byte2->reg == 0b000) {
            printf("ADD ");
            imRegMem = 1;
            op = ADD;
        } else if (opBuffer[0] >> 2 == 0b00100000 && byte2->reg == 0b101) {
            printf("SUB ");
            imRegMem = 1;
            op = SUB;
        } else if (opBuffer[0] >> 2 == 0b00100000 && byte2->reg == 0b111) {
            printf("CMP ");
            imRegMem = 1;
            op = CMP;
        } else if (opBuffer[0] >> 4 == 0b00001011) {
            printf("MOV ");
            imReg = 1;
            op = MOV;
        } else if (opBuffer[0] >> 1 == 0b00000010) {
            printf("ADD ");
            imReg = 1;
            op = ADD;
        } else if (opBuffer[0] >> 1 == 0b00010110) {
            printf("SUB ");
            imReg = 1;
            op = SUB;
        } else if (opBuffer[0] >> 1 == 0b00011110) {
            printf("CMP ");
            imReg = 1;
            op = CMP;
        } else if (opBuffer[0] >> 2 == 0b00100010) {
            printf("MOV ");
            regMem = 1;
            op = MOV;
        } else if (opBuffer[0] >> 2 == 0b0) {
            printf("ADD ");
            regMem = 1;
            op = ADD;
        } else if (opBuffer[0] >> 2 == 0b00001010) {
            printf("SUB ");
            regMem = 1;
            op = SUB;
        } else if (opBuffer[0] >> 2 == 0b00001110) {
            printf("CMP ");
            regMem = 1;
            op = CMP;
        }

        if (imRegMem) {
            if (byte2->mod == 0b00) {
                if (byte2->rm == 0b00000110) {
                    // unsigned short dh;
                    // read(fd, &dh, 2);
                    unsigned char dh[2];
                    dh[0] = buffer[i];
                    i += 1;
                    dh[1] = opBuffer[i];
                    i += 1;
                    if (byte1->w) {
                        unsigned char hdata[2];
                        hdata[0] = buffer[0];
                        i += 1;
                        hdata[1] = buffer[0];
                        i += 1;
                        // unsigned short hdata;
                        // read(fd, &hdata, 2);
                        printf("[%d], %d\n", *((unsigned short *)dh), *((unsigned short *)hdata));
                         *((unsigned short *)&memory[*((unsigned short *)dh)]) = *((unsigned short *)hdata);
                    } else {
                        unsigned char ldata;
                        ldata = buffer[i];
                        i += 1;
                        // unsigned char ldata;
                        // read(fd, &ldata, 1);
                        printf("[%d], %d\n", *((unsigned short *)dh), ldata);
                    }
                }
                else if (byte1->w) {
                    // unsigned short hdata;
                    // read(fd, &hdata, 2);
                    unsigned char hdata[2];
                    hdata[0] = buffer[i];
                    i += 1;
                    hdata[1] = buffer[i];
                    i += 1;
                    printf("[%s], %d\n", MMOD_table[byte2->rm], *((unsigned short *)hdata));
                    unsigned int regsValue;
                    if (byte2->rm > 0b11) {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]];
                    } else {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]] + RegMem[MMOD_map[byte2->rm][1]];
                    }
                    *((unsigned short *)&memory[regsValue]) =  *((unsigned short *)hdata);
                } else {
                    // unsigned char ldata;
                    // read(fd, &ldata, 1);
                    unsigned char ldata;
                    ldata = buffer[i];
                    i += 1;
                    printf("[%s], %d\n", MMOD_table[byte2->rm], ldata);
                }
            } else if (byte2->mod == 0b01) {
                // unsigned char dl;
                // read(fd, &dl, 1);
                unsigned char dl;
                dl = buffer[i];
                i += 1;
                if (byte1->w) {
                    // unsigned short hdata;
                    // read(fd, &hdata, 2);
                    unsigned char hdata[2];
                    hdata[0] = buffer[i];
                    i += 1;
                    hdata[1] = buffer[i];
                    i += 1;
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dl, *((unsigned short *)hdata));
                    unsigned int regsValue;
                    if (byte2->rm > 0b11) {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]];
                    } else {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]] + RegMem[MMOD_map[byte2->rm][1]];
                    }
                    *((unsigned short *)&memory[regsValue + dl]) =  *((unsigned short *)hdata);
                } else {
                    // unsigned char ldata;
                    // read(fd, &ldata, 1);
                    unsigned char ldata;
                    ldata = buffer[i];
                    i += 1;
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], dl, ldata);
                }
            } else if (byte2->mod == 0b10) {
                // unsigned short dh;
                // read(fd, &dh, 2);
                unsigned char dh[2];
                dh[0] = buffer[i];
                i += 1;
                dh[1] = buffer[i];
                i += 1;
                if (byte1->w) {
                    // unsigned short hdata;
                    // read(fd, &hdata, 2);
                    unsigned char hdata[2];
                    hdata[0] = buffer[i];
                    i += 1;
                    hdata[1] = buffer[i];
                    i += 1;
                    unsigned int regsValue;
                    if (byte2->rm > 0b11) {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]];
                    } else {
                        regsValue = RegMem[MMOD_map[byte2->rm][0]] + RegMem[MMOD_map[byte2->rm][1]];
                    }
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], *((unsigned short *)dh), *((unsigned short *)hdata));
                    *((unsigned short *)&memory[regsValue]) = *((unsigned short *)hdata);
                } else {
                    // unsigned char ldata;
                    // read(fd, &ldata, 1);
                    unsigned char ldata;
                    ldata = buffer[i];
                    i += 1;
                    printf("[%s + %d], %d\n", MMOD_table[byte2->rm], *((unsigned short *)dh), ldata);
                }
            } else {
                if (byte1->w) {
                    // unsigned short hdata;
                    // read(fd, &hdata, 2);
                    unsigned char hdata[2];
                    hdata[0] = buffer[i];
                    i += 1;
                    hdata[1] = buffer[i];
                    i += 1;
                    int res = *((unsigned short *)hdata);
                    if (op == ADD) {
                        res = RegMem[byte2->rm] + *((unsigned short *)hdata);
                    } else if (op == SUB) {
                        res = RegMem[byte2->rm] - *((unsigned short *)hdata);
                    } 
                    else if (op == CMP) {
                        res = RegMem[byte2->rm] - *((unsigned short *)hdata);
                    }

                    if (res < 0) {
                        flags[0] = 1;
                        flags[1] = 0;
                    } else if (res > 0) {
                        flags[0] = 0;
                        flags[1] = 0;
                    } else {
                        flags[0] = 0;
                        flags[1] = 1;
                    }
                    printf("%s, %d; ",
                        RMOD_table[byte2->rm][byte1->w],
                        *((unsigned short *)hdata)
                    );
                    printf("%d --> %d\n",
                        RegMem[byte2->rm],
                        res
                    );
                    printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
                    if (op != CMP)  RegMem[byte2->rm] = res;
                } else {
                    // unsigned char ldata;
                    // read(fd, &ldata, 1);
                    unsigned char ldata;
                    ldata = buffer[i];
                    i += 1;
                    printf("%s, %d\n", RMOD_table[byte2->rm][byte1->w], ldata);
                }
            }
        }
        //[]
        else if (imReg) {
            unsigned char opReg = 0; // accumulator
            unsigned char w = opBuffer[0] & 1;
            if (op == MOV) {
                opReg = imByte1->reg;
                w = imByte1->w;
            }
            unsigned char d[2];
            d[0] = opBuffer[1];
            if (w) {
                *(d + 1) = buffer[i];
                i += 1;
                // read(fd, d + 1, 1);
                int res = *(unsigned short *)d;
                if (op == ADD) {
                    res = res + *(unsigned short *)d;
                } else if (op == SUB) {
                    res = RegMem[opReg] - *(unsigned short *)d;
                } 
                else if (op == CMP) {
                    res = RegMem[opReg] - *(unsigned short *)d;
                }

                if (res < 0) {
                    flags[0] = 1;
                    flags[1] = 0;
                } else if (res > 0) {
                    flags[0] = 0;
                    flags[1] = 0;
                } else {
                    flags[0] = 0;
                    flags[1] = 1;
                }
                printf("%s, %d; %d -> %d\n",
                    RMOD_table[opReg][w],
                    *(unsigned short *)d,
                    RegMem[opReg],
                    res
                );
                printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
                if (op != CMP)  RegMem[opReg] = res;
            }
            else {
                unsigned char *reg;
                if (opReg < 0b100 && !w) {
                    reg = (unsigned char *)&RegMem[opReg];
                    printf("%s, %d; %d -> %d\n",
                        RMOD_table[opReg][w],
                        d[0],
                        *(reg + 1),
                        d[0]
                    );
                    *(reg + 1) = d[0];
                } else {
                    reg = (unsigned char *)&RegMem[opReg - 4];
                    printf("%s, %d; %d -> %d\n",
                        RMOD_table[opReg][w],
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

                    int res = RegMem[byte2->reg];
                    if (op == ADD) {
                        res = RegMem[byte2->rm] + RegMem[byte2->reg];
                    } else if (op == SUB) {
                        res = RegMem[byte2->rm] - RegMem[byte2->reg];
                    } 
                    else if (op == CMP) {
                        res = RegMem[byte2->rm] - RegMem[byte2->reg];
                    }

                    if (res < 0) {
                        flags[0] = 1;
                        flags[1] = 0;
                    } else if (res > 0) {
                        flags[0] = 0;
                        flags[1] = 0;
                    } else {
                        flags[0] = 0;
                        flags[1] = 1;
                    }

                    printf("%d -> %d\n", RegMem[byte2->rm], res);
                    printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
                    if (op != CMP) RegMem[byte2->rm] = res;
                }
                else {
                    unsigned char *dd;
                    unsigned char *sd;
                    if (byte2->rm > 0b100) {
                        dd = ((unsigned char *)&RegMem[byte2->rm - 4]);
                    }
                    else {
                        dd = ((unsigned char *)&RegMem[byte2->rm] + 1);
                    }
                    if (byte2->reg > 0b100) {
                        sd = ((unsigned char *)&RegMem[byte2->reg - 4]);
                    }
                    else {
                        sd = ((unsigned char *)&RegMem[byte2->reg] + 1);
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
                        // read(fd, &dh, 2);
                        dh[0] = buffer[i];
                        i += 1;
                        dh[1] = buffer[i];
                        i += 1;
                        printf("[%d], %s\n", *((unsigned short *)dh), RMOD_table[byte2->reg][byte1->w]);
                    }
                    else {
                        printf("[%s], %s\n", MMOD_table[byte2->rm], RMOD_table[byte2->reg][byte1->w]);
                        if (byte1->w) {
                            unsigned int regsValue;
                            if (byte2->rm > 0b11) {
                                regsValue = RegMem[MMOD_map[byte2->rm][0]];
                            } else {
                                regsValue = RegMem[MMOD_map[byte2->rm][0]] + RegMem[MMOD_map[byte2->rm][1]];
                            }
                            *((unsigned short *)&memory[regsValue]) = RegMem[byte2->reg];
                        }
                    }
                }
                else {
                    if (byte2->rm == 0b00000110) {
                        // read(fd, &dh, 2);
                        dh[0] = buffer[i];
                        i += 1;
                        dh[1] = buffer[i];
                        i += 1;
                        printf("%s, [%d]\n", RMOD_table[byte2->reg][byte1->w], *((unsigned short *)dh));
                    } else {
                        printf("%s, [%s]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm]);
                    }
                }
            } else if (byte2->mod == 0b01) {
                // unsigned char dl;
                // read(fd, &dl, 1);
                i += 1;
                unsigned char dl;
                dl = buffer[i];
                if (byte1->d == 0b0) {
                    //TODO: 110 is a special case
                    printf("[%s + %d], %s\n", MMOD_table[byte2->rm], dl, RMOD_table[byte2->reg][byte1->w]);
                } else {

                    printf("%s, [%s + %d]\n", RMOD_table[byte2->reg][byte1->w], MMOD_table[byte2->rm], dl);
                }
            } else if (byte2->mod == 0b10) {
                unsigned char dh[2];
                dh[0] = buffer[i];
                i += 1;
                dh[1] = buffer[i];
                i += 1;
                // read(fd, &dh, 2);
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
    // printf("AX=%d\n", mem[0]);
    // printf("AH=%d\n", mem[0] & 0xFF);
    // printf("AL=%d\n", (mem[0] >> 8) & 0xFF);
    printf("BX=%d\n", RegMem[3]);
    // printf("CX=%d\n", mem[1]);
    // printf("CH=%d\n", mem[1] & 0xFF);
    // printf("CL=%d\n", (mem[1] >> 8) & 0xFF);
    printf("DX=%d\n", RegMem[2]);
    // printf("DH=%d\n", mem[2] & 0xFF);
    // printf("DL=%d\n", (mem[2] >> 8) & 0xFF);
    // printf("BH=%d\n", *((unsigned char *)&mem[3]));
    // printf("BL=%d\n", *((unsigned char *)&mem[3] + 1));
    printf("SP=%d\n", RegMem[4]);
    printf("BP=%d\n", RegMem[5]);
    // printf("SI=%d\n", mem[6]);
    printf("DI=%d\n", RegMem[7]);

    printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
    printf("IP=%d\n", i);

    printf("memory[bp + di]=%d\n", *((unsigned short *)(&memory[RegMem[5] + RegMem[7]])));
    printf("memory[bp]=%d\n", *((unsigned short *)(&memory[RegMem[5]])));
    printf("memory[bx]=%d\n", *((unsigned short *)(&memory[RegMem[3]])));
    close(fd);
    fclose(output);

    return 0;
}
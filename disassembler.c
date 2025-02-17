#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

unsigned short mem[8] = {0};
unsigned char flags[2] = {0};


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
            op = MOV;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b000) {
            printf("ADD ");
            imRegMem = 1;
            op = ADD;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b101) {
            printf("SUB ");
            imRegMem = 1;
            op = SUB;
        } else if (buffer[0] >> 2 == 0b00100000 && byte2->reg == 0b111) {
            printf("CMP ");
            imRegMem = 1;
            op = CMP;
        } else if (buffer[0] >> 4 == 0b00001011) {
            printf("MOV ");
            imReg = 1;
            op = MOV;
        } else if (buffer[0] >> 1 == 0b00000010) {
            printf("ADD ");
            imReg = 1;
            op = ADD;
        } else if (buffer[0] >> 1 == 0b00010110) {
            printf("SUB ");
            imReg = 1;
            op = SUB;
        } else if (buffer[0] >> 1 == 0b00011110) {
            printf("CMP ");
            imReg = 1;
            op = CMP;
        } else if (buffer[0] >> 2 == 0b00100010) {
            printf("MOV ");
            regMem = 1;
            op = MOV;
        } else if (buffer[0] >> 2 == 0b0) {
            printf("ADD ");
            regMem = 1;
            op = ADD;
        } else if (buffer[0] >> 2 == 0b00001010) {
            printf("SUB ");
            regMem = 1;
            op = SUB;
        } else if (buffer[0] >> 2 == 0b00001110) {
            printf("CMP ");
            regMem = 1;
            op = CMP;
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
            unsigned char opReg = 0; // accumulator
            unsigned char w = buffer[0] & 1;
            if (op == MOV) {
                ImByte1 *imByte1 = (ImByte1 *)&buffer[0];
                opReg = imByte1->reg;
                w = imByte1->w;
            }
            unsigned char d[2];
            d[0] = buffer[1];
            if (w) {
                read(fd, d + 1, 1);
                int res = *(unsigned short *)d;
                if (op == ADD) {
                    res = res + *(unsigned short *)d;
                } else if (op == SUB) {
                    res = mem[opReg] - *(unsigned short *)d;
                } 
                else if (op == CMP) {
                    res = mem[opReg] - *(unsigned short *)d;
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
                    mem[opReg],
                    res
                );
                printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
                if (op != CMP)  mem[opReg] = res;
            }
            else {
                unsigned char *reg;
                if (opReg < 0b100 && !w) {
                    reg = (unsigned char *)&mem[opReg];
                    printf("%s, %d; %d -> %d\n",
                        RMOD_table[opReg][w],
                        d[0],
                        *(reg + 1),
                        d[0]
                    );
                    *(reg + 1) = d[0];
                } else {
                    reg = (unsigned char *)&mem[opReg - 4];
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

                    int res = mem[byte2->reg];
                    if (op == ADD) {
                        res = mem[byte2->rm] + mem[byte2->reg];
                    } else if (op == SUB) {
                        res = mem[byte2->rm] - mem[byte2->reg];
                    } 
                    else if (op == CMP) {
                        res = mem[byte2->rm] - mem[byte2->reg];
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

                    printf("%d -> %d\n", mem[byte2->rm], res);
                    printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
                    if (op != CMP) mem[byte2->rm] = res;
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
    // printf("AX=%d\n", mem[0]);
    // printf("AH=%d\n", mem[0] & 0xFF);
    // printf("AL=%d\n", (mem[0] >> 8) & 0xFF);
    printf("BX=%d\n", mem[3]);
    printf("CX=%d\n", mem[1]);
    // printf("CH=%d\n", mem[1] & 0xFF);
    // printf("CL=%d\n", (mem[1] >> 8) & 0xFF);
    // printf("DX=%d\n", mem[2]);
    // printf("DH=%d\n", mem[2] & 0xFF);
    // printf("DL=%d\n", (mem[2] >> 8) & 0xFF);
    // printf("BH=%d\n", *((unsigned char *)&mem[3]));
    // printf("BL=%d\n", *((unsigned char *)&mem[3] + 1));
    printf("SP=%d\n", mem[4]);
    // printf("BP=%d\n", mem[5]);
    // printf("SI=%d\n", mem[6]);
    // printf("DI=%d\n", mem[7]);

    printf("SF=%d, ZF=%d\n", flags[0], flags[1]);
    close(fd);
    fclose(output);

    return 0;
}
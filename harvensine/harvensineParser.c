#include "parser.h"
#include "referenceHarvensince.c"
#include <sys/time.h> // For gettimeofday()
#include "time.c"

bool cmp_key(unsigned char *str1, unsigned char *str2) {
    return str1[0] == str2[0] && str1[1] == str2[1];
}

bool is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void skip_white_space(Lexer *l) {
    while (l->buffer.data[l->i] == ' ' || l->buffer.data[l->i] == '\t' || l->buffer.data[l->i] == '\n') {
        l->i += 1;
    }
}

token getToken(Lexer* l) {
    token token;
    int length = 0;
    bool point = false;
    skip_white_space(l);
    token.pos = l->i;
    token.buffer.data = &l->buffer.data[l->i];

    switch (l->buffer.data[l->i])
    {
    case '\"':
        l->i += 1;
        token.buffer.data = &l->buffer.data[l->i];
        while(isprint(l->buffer.data[l->i]) &&
            l->buffer.data[l->i] != '"')
        {
            l->i += 1;
            length += 1;
        }
        if (l->buffer.data[l->i] != '"') {
            printf("SYNTAX ERROR, EXPECTED ENCLOSING QUOTE\n");
            exit(-1);
        }
        l->i += 1;
        token.type = IDENTIFIER;
        token.buffer.length = length;
        break;
    case '{':
        token.type = OPBRACE;
        token.buffer.length = 1;
        l->i += 1;
        break;
    case '}':
        token.type = CLBRACE;
        token.buffer.length = 1;
        l->i += 1;
        break;
    case '[':
        token.type = OPBRACKET;
        token.buffer.length = 1;
        l->i += 1;
        break;
    case ']':
        token.type = CLBRACKET;
        token.buffer.length = 1;
        l->i += 1;
        break;
    case ',':
        token.type = COMMA;
        token.buffer.length = 1;
        l->i += 1;
        break;
    case ':':
        token.type = COLON;
        token.buffer.length = 1;
        l->i += 1;
        break;
    default:
        if (l->buffer.data[l->i] == '-') {
            l->i += 1;
            length += 1;
        }
        while(l->buffer.data[l->i] != '"' &&
                l->buffer.data[l->i] != ',' &&
                l->buffer.data[l->i] != '}' &&
                !is_white_space(l->buffer.data[l->i])
        )
        {
            if (!isdigit(l->buffer.data[l->i])) {
                if (l->buffer.data[l->i] == '.') {
                    if (point) {
                        // printf("here?\n");
                        printf("INVALID JSON, Number expected\n");
                        exit(-1);
                    }
                    point = true;
                } else {
                    // printf("here???\n");
                    printf("INVALID JSON, Number expected\n");
                    exit(-1);
                }
            }
            l->i += 1;
            length += 1;
        }
        skip_white_space(l);
        token.type = IDENTIFIER;
        token.buffer.length = length;
        break;
    }

    return token;
}

void init_lexer(Lexer *l, Buffer buffer) {
    l->buffer = buffer;
    l->i = 0;
}

void init_parser(Parser *p, Lexer *l) {
    p->l = l;
    p->pair_length = 0;
}


float get_value(Lexer *l) {
    token t;
    t = getToken(l);
    if (t.type != COLON) {
        printf("INVALID JSON, COLON EXPECTED\n");
        exit(-1);
    }
    t = getToken(l);
    if (t.type != IDENTIFIER) {
        printf("INVALID JSON, INT VALUE EXPECTED\n");
        exit(-1);
    }
    char v[5] = {0};
    unsigned char sign = 1;
    int i = 0;
    if (t.buffer.data[0] == '-') {
        v[0] = '-';
        i += 1;
    }
    while(i < t.buffer.length) {
        v[i] = t.buffer.data[i];
        i += 1;
    }
    float point = atof(v) * sign;
    if (point < -180.0 || point > 180.0) {
        printf("INVALID value %c%c, should be between [-180, 180]\n", t.buffer.data[0], t.buffer.data[1]);
        exit(-1);
    }
    
    return point;
}

Pair parse(Parser *p) {
    Lexer *l = p->l;
    token token = getToken(l);
    if (token.type == OPBRACKET) {
        if (p->pair_length == 0 ) {
            token = getToken(l);
        } else {
            printf("INVALID JSON, SHOULD START WITH AN OPENING BRACKET!!! \n");
            exit(-1);
        }
    }
    bool x0 = false;
    bool x1 = false;
    bool y0 = false;
    bool y1 = false;
    Pair pair;
    if (token.type != OPBRACE) {
        printf("pair count = %d\n", p->pair_length);
        printf("type=%d\n", token.type);
        printf("i=%d\n", l->i);
        write(1, token.buffer.data, token.buffer.length);
        printf("\nINVALID JSON, EVERY PAIR SHOULD START WITH AN OPENING BRACE!!! \n");
        exit(-1);
    }
    while (true)
    {
        token = getToken(l);
        if (token.type != IDENTIFIER) {
            printf("INVALID JSON, KEY EXPECTED\n");
            exit(-1);
        }
        if (cmp_key(token.buffer.data, (unsigned char *)"x0"))
        {
            pair.x0 = get_value(l);
            x0 = true;
        } else if (cmp_key(token.buffer.data, (unsigned char *)"x1")) {
            pair.x1 = get_value(l);
            x1 = true;
        } else if (cmp_key(token.buffer.data, (unsigned char *)"y0")) {
            pair.y0 = get_value(l);
            y0 = true;
        } else if (cmp_key(token.buffer.data, (unsigned char *)"y1")) {
            pair.y1 = get_value(l);
            y1 = true;
        } else {
            printf("INVALID KEY DETECTED\n");
            exit(-1);
        }
        if (x0 && x1 && y0 && y1) break;
        token = getToken(l);
        if (token.type != COMMA) {
            printf("INVALID JSON, COMMA VALUE EXPECTED\n");
            exit(-1);
        }
    }
    p->pair_length += 1;
    token = getToken(l);
    if (token.type != CLBRACE) {
            printf("INVALID JSON, CLOSING BRACE EXPECTED\n");
            exit(-1);
    }
    token = getToken(l);
    if (token.type == CLBRACKET) {
            return pair;
    } else if (token.type != COMMA) {
        printf("INVALID JSON, COMMA EXPECTED\n");
        exit(-1);
    }
    return pair;
}

unsigned int file_size(const char *filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

Buffer allocate_buffer(unsigned long long size) {
    Buffer buffer = {};
    buffer.data = malloc(size);
    buffer.length = size;
    
    return buffer;
}

int main(int argc, const char **argv) {
    u64 start = ReadOSTimer();
    u64 startUpStart = ReadCpuTimer();
        if (argc < 2)
        {
            printf("please enter the file name!");
            return -1;
        }

        const char *file_name = argv[1];
    u64 startUpEnd = ReadCpuTimer();

    
    u64 memSetupStart = ReadCpuTimer();
        int fd = open(file_name, O_RDONLY);
        unsigned int size = file_size(file_name);
        Buffer buffer = allocate_buffer(size);
        Lexer l;
        init_lexer(&l, buffer);
        Parser p;
        init_parser(&p, &l);
        unsigned char min_pair_encoding = 6*4+6; // {"x0":0,"y0":0,"x1":0,"y1":0}
        unsigned long long max_pair_count = (buffer.length + 1) / min_pair_encoding;
        Buffer parsed_pairs = allocate_buffer(max_pair_count * sizeof(Pair));
        Pair *pairs = (Pair *)parsed_pairs.data;
    u64 memSetupEnd = ReadCpuTimer();

    u64 readStart = ReadCpuTimer();
        read(fd, buffer.data, buffer.length);
    u64 readEnd = ReadCpuTimer();

    u64 parseStart = ReadCpuTimer();
        while(l.i < size - 1) {
            pairs[p.pair_length] = parse(&p);
        }
    u64 parseEnd = ReadCpuTimer();

    u64 sumStart = ReadCpuTimer();
        unsigned int i = 0;
        int res = 0;
        while (i < p.pair_length) {
            res += ReferenceHaversine(
                pairs[i].x0,
                pairs[i].x1,
                pairs[i].y0,
                pairs[i].y1,
                6372.8
            );
            i += 1;
        }
    u64 sumEnd = ReadCpuTimer();
    
    u64 printStart = ReadCpuTimer();
        printf("Input Size: %lu\n", sizeof(pairs));
        printf("Pair Count: %d\n", p.pair_length);
        printf("Harvensine Average Sum: %f\n", (float)res / i);
    u64 printEnd = ReadCpuTimer();

    u64 end = ReadOSTimer();

    printf("\n------------------------------------------------\n");
    float totalTime = (end - start);
    u64 startup =  startUpEnd - startUpStart;
    u64 read = readEnd - readStart;
    u64 memorySetup = memSetupEnd - memSetupStart;
    u64 parse = parseEnd - parseStart;
    u64 sum = sumEnd - sumStart;
    u64 print = printEnd - printStart;

    u64 totalCpu = startup + read + memorySetup + parse + sum + print;
    u64 cpuFreq = GetCpuFreq();

    printf("Total time: %.3fs (CPU freq %llu)\n", totalTime/1000000, cpuFreq);
    printf("Start up: %llu (%.4f\%%)\n", startup, (float)startup * 100 / totalCpu);
    printf("Read: %llu (%.4f\%%)\n", read, (float)read * 100 / totalCpu);
    printf("Mem setup: %llu (%.4f\%%)\n", memorySetup, (float)memorySetup * 100 / totalCpu);
    printf("Parse: %llu (%.4f\%%)\n", parse, (float)parse * 100 / totalCpu);
    printf("Sum: %llu (%.4f\%%)\n", sum, (float)sum * 100 / totalCpu);
    printf("Print: %llu (%.4f\%%)\n", print, (float)print * 100 / totalCpu);

    return 0;
}
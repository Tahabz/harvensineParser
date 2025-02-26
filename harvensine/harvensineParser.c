#include "parser.h"
#include "referenceHarvensince.c"

bool cmp_key(unsigned char *str1, unsigned char *str2) {
    return str1[0] == str2[0] && str1[1] == str2[1];
}

bool is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void skip_white_space(Lexer *l) {
    while (l->buffer[l->i] == ' ' || l->buffer[l->i] == '\t' || l->buffer[l->i] == '\n') {
        l->i += 1;
    }
}

token getToken(Lexer* l) {
    token token;
    int length = 0;
    bool point = false;
    skip_white_space(l);
    token.pos = l->i;
    token.value = &l->buffer[l->i];
    switch (l->buffer[l->i])
    {
    case '\"':
        l->i += 1;
        token.value = &l->buffer[l->i];
        while(isprint(l->buffer[l->i]) &&
            l->buffer[l->i] != '"')
        {
            l->i += 1;
            length += 1;
        }
        if (l->buffer[l->i] != '"') {
            printf("SYNTAX ERROR, EXPECTED ENCLOSING QUOTE\n");
            exit(-1);
        }
        l->i += 1;
        token.type = IDENTIFIER;
        token.length = length;
        break;
    case '{':
        token.type = OPBRACE;
        token.length = 1;
        l->i += 1;
        break;
    case '}':
        token.type = CLBRACE;
        token.length = 1;
        l->i += 1;
        break;
    case '[':
        token.type = OPBRACKET;
        token.length = 1;
        l->i += 1;
        break;
    case ']':
        token.type = CLBRACKET;
        token.length = 1;
        l->i += 1;
        break;
    case ',':
        token.type = COMMA;
        token.length = 1;
        l->i += 1;
        break;
    case ':':
        token.type = COLON;
        token.length = 1;
        l->i += 1;
        break;
    default:
        if (l->buffer[l->i] == '-') {
            l->i += 1;
            length += 1;
        }
        while(l->buffer[l->i] != '"' &&
                l->buffer[l->i] != ',' &&
                l->buffer[l->i] != '}' &&
                !is_white_space(l->buffer[l->i])
        )
        {
            if (!isdigit(l->buffer[l->i])) {
                if (l->buffer[l->i] == '.') {
                    if (point) {
                        printf("INVALID JSON, Number expected\n");
                        exit(-1);
                    }
                    point = true;
                } else {
                    printf("INVALID JSON, Number expected\n");
                    exit(-1);
                }
            }
            l->i += 1;
            length += 1;
        }
        skip_white_space(l);
        token.type = IDENTIFIER;
        token.length = length;
        break;
    }

    return token;
}

void init_lexer(Lexer *l, unsigned char *buffer) {
    l->buffer = buffer;
    l->i = 0;
}

void init_parser(Parser *p, Lexer *l) {
    p->l = l;
    p->pair_length = 0;
}


int get_value(Lexer *l) {
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
    if (t.length > 4) {
        printf("INVALID value %c%c, should be between [-180, 180]\n", t.value[0], t.value[1]);
        exit(-1);
    }
    char v[5] = {0};
    unsigned char sign = 1;
    int i = 0;
    if (t.value[0] == '-') {
        v[0] = '-';
        i += 1;
    }
    while(i < t.length) {
        v[i] = t.value[i];
        i += 1;
    }
    return atof(v) * sign;
}

void parse(Parser *p) {
    Lexer *l = p->l;
    char val[2];
    token token = getToken(l);


    if (token.type != OPBRACKET) {
        printf("INVALID JSON, SHOULD START WITH AN OPENING BRACKET!!! \n");
        exit(-1);
    }
    while (l->buffer[l->i])
    {
        bool x0 = false;
        bool x1 = false;
        bool y0 = false;
        bool y1 = false;
        token = getToken(l);
        if (token.type != OPBRACE) {
            printf("INVALID JSON, EVERY PAIR SHOULD START WITH AN OPENING BRACE!!! \n");
            exit(-1);
        }
        int pos = 0;
        while (true)
        {
            token = getToken(l);
            if (token.type != IDENTIFIER) {
                printf("INVALID JSON, KEY EXPECTED\n");
                exit(-1);
            }
            if (cmp_key(token.value, (unsigned char *)"x0"))
            {
                p->pairs[p->pair_length].x0 = get_value(l);
                x0 = true;
            } else if (cmp_key(token.value, (unsigned char *)"x1")) {
                p->pairs[p->pair_length].x1 = get_value(l);
                x1 = true;
            } else if (cmp_key(token.value, (unsigned char *)"y0")) {
                p->pairs[p->pair_length].y0 = get_value(l);
                y0 = true;
            } else if (cmp_key(token.value, (unsigned char *)"y1")) {
                p->pairs[p->pair_length].y1 = get_value(l);
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
                return;
        } else if (token.type != COMMA) {
            printf("INVALID JSON, COMMA EXPECTED\n");
            exit(-1);
        }
    }
}


int main(int argc, char **argv) {
    printf("hi\n");
    if (argc < 2)
    {
        printf("please enter the file name!");
        return -1;
    }

    unsigned char buffer[875741];
    char *file_name = argv[1];
    FILE *output;
    int fd = open(file_name, O_RDONLY);
    read(fd, buffer, 875741);
    Lexer l;
    init_lexer(&l, buffer);
    Parser p;
    init_parser(&p, &l);
    parse(&p);
    int pi = 0;
    float res = 0;
    while(pi < p.pair_length) {
        res += ReferenceHaversine(
            p.pairs[pi].x0,
            p.pairs[pi].x1,
            p.pairs[pi].y0,
            p.pairs[pi].y1,
            6372.8);
        pi += 1;
    }
    printf("pi=%d, total=%f, average=%f\n", pi, res, res/pi);
    return 0;
}
 
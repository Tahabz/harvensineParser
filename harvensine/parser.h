#pragma once
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

typedef unsigned long long u64;
typedef double f64;
static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius);
typedef enum {
    OPBRACE,
    CLBRACE,
    OPBRACKET,
    CLBRACKET,
    IDENTIFIER,
    COMMA,
    COLON
} types;

typedef struct {
    unsigned char *data;
    int length;
} Buffer;

typedef struct {
    float x0;
    float x1;
    float y0;
    float y1;
} Pair;

typedef struct {
    types type;
    Buffer buffer;
    int pos;
} token;

typedef struct {
    unsigned int i;
    Buffer buffer;
} Lexer;

typedef struct {
    Lexer *l;
    unsigned int pair_length;
    unsigned int curr_pair;
} Parser;
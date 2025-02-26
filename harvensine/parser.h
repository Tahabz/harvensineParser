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
    float x0;
    float x1;
    float y0;
    float y1;
} Pair;

typedef struct {
    types type;
    unsigned char *value;
    int pos;
    unsigned char length;
} token;

typedef struct {
    int i;
    unsigned char *buffer;
} Lexer;

typedef struct {
    Lexer *l;
    int pair_length;
    Pair pairs[875741];
} Parser;
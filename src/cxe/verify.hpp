#pragma once
#include <stdio.h>
#include <stdlib.h>

static inline void fatal(const char* file, const int line, const char* msg) {
    printf("%s:%i: %s\n", file, line, msg);
    exit(1);
}

#define fatal(msg) fatal(__FILE__, __LINE__, msg)

#define verify(expr) ((expr)||(fatal("verify("#expr") failed"),0))

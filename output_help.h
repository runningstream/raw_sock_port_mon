#ifndef OUTPUT_HELP_H
#define OUTPUT_HELP_H

#include <stdio.h>

extern int out_help_verbose_level;

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define verbose_printf_1(...) { if(out_help_verbose_level >= 1) { printf(__VA_ARGS__); } }
#define verbose_printf_2(...) { if(out_help_verbose_level >= 2) { printf(__VA_ARGS__); } }

#ifndef NDEBUG
#define DEBUG_PRINT(...) eprintf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

void hex_print_frame(char *buf, ssize_t amt);

void set_verbose_level(int level);

#endif

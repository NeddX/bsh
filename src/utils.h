#ifndef BSH_UTILITIES_H
#define BSH_UTILITIES_H

#include "stdbsh.h"

char* file_read_all_text(const char* restrict filepath);
bool str_starts_with(const char* restrict str, const char* restrict pattern);
char* str_replace(const char* restrict str, const char* restrict find, const char* restrict replacement);
void io_readline(char** restrict buffer, usize* restrict buffer_size);

#endif // BSH_UTILITIES_H

#include "utils.h"

char* file_read_all_text(const char* restrict filepath) {
    FILE* fs = fopen(filepath, "r");
    if (!fs)
        return NULL;

    fseek(fs, 0, SEEK_END);
    usize file_size = ftell(fs);
    fseek(fs, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    fread(buffer, file_size, 1, fs);
    fclose(fs);
    buffer[file_size] = 0;
    return buffer;
}

bool str_starts_with(const char* restrict str, const char* restrict pattern) {
    return !strncmp(str, pattern, strlen(pattern));
}

char* str_replace(const char* restrict str, const char* restrict find, const char* restrict replacement) {
    usize str_len = strlen(str);
    usize repl_len = strlen(replacement);
    usize find_len = strlen(find);
    usize new_len = str_len;

    for (usize i = 0; i < str_len - find_len; ++i) {
        if (strcmp(str + i, find) == 0) {
            new_len += repl_len - find_len;
            i += find_len - 1;
        }
    }

    char* new_str = (char*)calloc(sizeof(char), new_len + 1);

    i32 new_index = 0;
    for (usize i = 0; i < str_len; ++i) {
        if (strncmp(str + i, find, find_len) == 0) {
            strncpy(new_str + new_index, replacement, repl_len);
            new_index += repl_len;
            i += find_len - 1;
        } else {
            new_str[new_index++] = str[i];
        }
    }
    return new_str;
}

void io_readline(char** restrict buffer, usize* restrict buffer_size) {
    usize i = 0;
    char c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (i >= *buffer_size * sizeof(char)) {
            *buffer_size = *buffer_size * 2 + 1;
            *buffer = (char*)realloc(*buffer, *buffer_size);
        }
        *(*buffer + i++) = c;
    }
    *(*buffer + i) = 0;
}

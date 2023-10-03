#ifndef BSH_SHELL_INTERFACE_H
#define BSH_SHELL_INTERFACE_H

#include "pjob_handler.h"
#include "stdbsh.h"
#include "utils.h"

#define DEF_INPUT_COUNT 256
#define MAX_COMMAND_LEN 1024

typedef struct _shell {
    i32 status;
    const char* prompt;
    char cwd[PATH_MAX];
    char prev_cwd[PATH_MAX];
    char* buffer;
    usize buffer_size;
    char** args;
    usize args_size;
    usize current_arg_count;
    char bin_dir[PATH_MAX];
    PJobHandler* phnd;
} Shell;

void Shell_Init(Shell* restrict shell, const char* local_dir_path);
void Shell_ParseCommand(Shell* restrict shell);
void Shell_HandleCommand(Shell* restrict shell);
i32 Shell_Run(Shell* restrict shell);
void Shell_Destroy(Shell* restrict shell);

i32 execute_command(char** args);

#endif // BSH_SHELL_INTERFACE_H

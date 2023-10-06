#ifndef BSH_SHELL_INTERFACE_H
#define BSH_SHELL_INTERFACE_H

#include "pjob_handler.h" // For the job handling system.
#include "stdbsh.h"       // Common include files.

#define DEF_INPUT_COUNT 256  // Default starting input character length.

// Shell state struct.
typedef struct _shell {
    i32 status;
    const char* prompt;
    char cwd[PATH_MAX];
    char pwd[PATH_MAX];
    char* buffer;
    usize buffer_size;
    char** args;
    usize args_size;
    usize current_arg_count;
    char bin_dir[PATH_MAX];
    PJobHandler* phnd;
} Shell;

// Methods of Shell struct.
void Shell_Init(Shell* restrict shell, const char* local_dir_path);
void Shell_ParseCommand(Shell* restrict shell);
void Shell_HandleCommand(Shell* restrict shell);
i32 Shell_Run(Shell* restrict shell);
void Shell_Destroy(Shell* restrict shell);

#endif // BSH_SHELL_INTERFACE_H

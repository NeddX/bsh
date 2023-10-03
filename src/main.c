#include <string.h>

#include "shell_interface.h"

Shell shell;

i32 main(const i32 argc, const char* argv[]) {
    Shell_Init(&shell, argv[0]);
    Shell_Run(&shell);
    Shell_Destroy(&shell);
    return shell.status;
}

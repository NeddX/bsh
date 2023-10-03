#include <string.h>

#include "shell_interface.h"

i32 main(const i32 argc, const char* argv[]) {
    Shell shell;
    Shell_Init(&shell, argv[0]);
    Shell_Run(&shell);
    Shell_Destroy(&shell);
    return shell.status;
}

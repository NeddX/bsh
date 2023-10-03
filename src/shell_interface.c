#include "shell_interface.h"
#include "pjob_handler.h"

void Shell_Init(Shell* restrict shell, const char* local_dir_path) {
    shell->status = 1;
    shell->prompt = "(%s@%s)-[%s]\n$: ";
    shell->buffer_size = DEF_INPUT_COUNT * sizeof(char);
    shell->buffer = (char*)malloc(shell->buffer_size);
    shell->args_size = 1 * sizeof(char*);
    shell->args = (char**)malloc(shell->args_size);
    shell->phnd = PJobHandler_Get();

    realpath(local_dir_path, shell->bin_dir);
    dirname(shell->bin_dir);

    if (!getcwd(shell->cwd, sizeof(shell->cwd))) {
        perror("bsh: Failed resolving current working directory");
    } else {
        strcpy(shell->prev_cwd, shell->cwd);
    }
}

void Shell_ParseCommand(Shell* restrict shell) {
    i32 i = 0;
    *(shell->args) = NULL;
    *(shell->args) = strtok(shell->buffer, " \t\n");
    while (*(shell->args + i)) {
        if (i >= shell->args_size / sizeof(char*) - 1) {
            shell->args_size *= 2;
            shell->args = (char**)realloc(shell->args, shell->args_size);
        }
        *(shell->args + ++i) = strtok(NULL, " \t\n");
        shell->current_arg_count = i;
    }
}

void Shell_HandleCommand(Shell* restrict shell) {
    char path[PATH_MAX];
    if (!strcmp(shell->args[0], "help")) {
        strcpy(path, shell->bin_dir);
        strcat(path, "/help.txt");
        char* const content = file_read_all_text(path);
        if (content) {
            printf("%s\n", content);
            free(content);
        } else {
            perror("bsh: Couldn't open help.txt");
        }
    } else if (!strcmp(shell->args[0], "cd")) {
        strcpy(shell->prev_cwd, shell->cwd);

        if (!strcmp(shell->args[1], "~")) {
            chdir(getenv("HOME"));
        } else if (!strcmp(shell->args[1], "-")) {
            chdir(shell->prev_cwd);
        } else {
            chdir(shell->args[1]);
        }

        if (getcwd(path, sizeof(path))) {
            strcpy(shell->cwd, path);
        } else {
            perror("bsh");
            strcpy(shell->cwd, shell->args[0]);
        }
    } else if (!strcmp(shell->args[0], "exit")) {
        shell->status = 0;
    } else if (!strcmp(shell->args[0], "pwd")) {
        printf("%s\n", shell->cwd);
    } else if (!strcmp(shell->args[0], "about")) {
        strcpy(path, shell->bin_dir);
        strcat(path, "/about.txt");
        char* const content = file_read_all_text(path);
        if (content) {
            printf("%s\n", content);
            free(content);
        } else {
            perror("bsh: Couldn't open about.txt");
        }
    } else if (!strcmp(shell->args[0], "fg")) {
        /*
        i32 id = atoi(shell->args[1]);
        pid_t fg_pid = shell->phnd->jobs[id - 1].pid;
        tcsetpgrp(STDIN_FILENO, fg_pid);
        kill(fg_pid, SIGCONT);
        i32 status;
        waitpid(fg_pid, &status, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        */
    } else if (!strcmp(shell->args[0], "bg")) {
        /*
        pid_t bg_pid = fork();
        if (bg_pid == 0) {
            setpgid(0, 0);
            signal(SIGTTOU, SIG_IGN);
            signal(SIGTTIN, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            execvp(shell->args[1], shell->args + 1);
            perror("exec");
            exit(EXIT_FAILURE);
        } else if (bg_pid > 0) {
            printf("[%u]: BG %d.", shell->phnd->job_count, bg_pid);
            PJobHandler_AddJob(bg_pid, shell->args[1]);
        } else {
            perror("fork");
        }
        */
    } else if (!strcmp(shell->args[0], "joblist")) {
        if (shell->phnd->job_count > 0) {
            for (usize i = 0; i < shell->phnd->job_count; ++i) {
                printf("[%zu] (%d): %s\n", i + 1, shell->phnd->jobs[i].pid, shell->phnd->jobs[i].command);
            }
        } else {
            puts("No currently active jobs.\n");
        }
    } else {
        pid_t pid, wpid;
        i32 status;
        bool background = false;

        // Check if this command is supposed to be ran at the background.
        if (!strcmp(shell->args[shell->current_arg_count - 1], "&")) {
            // If yes then set the background flag and remove the ampersand symbol from the arg list.
            background = true;
            shell->args[shell->current_arg_count - 1] = NULL;
        }

        pid = fork();

        if (pid == 0) {
            // Successfully created child process for execvp to take over.

            // Check for background mode.
            if (background) {
                // If yes then redirect stdout and stdin to the null device.

                // Disable stdin and stdout for the child process
                //close(STDIN_FILENO);
                //close(STDOUT_FILENO);

                // alt
                freopen("/dev/null", "r", stdin);
                freopen("/dev/null", "w", stdout);
            }

            if (execvp(shell->args[0], shell->args) == -1) {
                perror("bsh");
            }

            printf("Am I always intact?\n");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Failed to fork.
            perror("bsh");
        } else {
            // Parent process.

            // Add the process as a job to the job handler.
            PJobHandler_AddJob(pid, shell->args[0]);

            // Check if the command is supposed to be ran in the background.
            if (background) {
                // Wait for the command to finish if not.
                do {
                    wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));

                // Remove the job responsible for this command because the command finished executing.
                PJobHandler_RemoveJob(pid);
            }

            printf("[+] (%u): %s\n", shell->phnd->job_count - 1, shell->args[0]);
        }
    }
}

i32 Shell_Run(Shell* restrict shell) {
    const char* user_name = getenv("USER");
    const char* home_dir = getenv("HOME");
    char host_name[256];
    gethostname(host_name, sizeof(host_name));

    while (shell->status) {
        if  (str_starts_with(shell->cwd, home_dir)) {
            char* fmt = str_replace(shell->cwd, home_dir, "~");
            printf(shell->prompt, user_name, host_name, fmt);
            free(fmt);
        } else {
            printf(shell->prompt, user_name, host_name, shell->cwd);
        }

        io_readline(&shell->buffer, &shell->buffer_size);

        if (strlen(shell->buffer)) {
            Shell_ParseCommand(shell);
            Shell_HandleCommand(shell);
        }
    }

    return shell->status;
}

void Shell_Destroy(Shell* restrict shell) {
    free(shell->buffer);
    free(shell->args);
}

i32 execute_command(char** args) {
    pid_t pid, wpid;
    i32 status;

    pid = fork();

    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("bsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("bsh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

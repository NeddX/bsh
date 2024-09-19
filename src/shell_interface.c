#include "shell_interface.h"
#include "pjob_handler.h"
#include "utils.h"

void Shell_Init(Shell* restrict shell, const char* local_dir_path) {
    shell->status = 1;
    shell->prompt = "(%s@%s)-[%s]\n$: ";
    shell->buffer_size = DEF_INPUT_COUNT * sizeof(char);
    shell->buffer = (char*)malloc(shell->buffer_size);
    shell->args_size = 8 * sizeof(char*);
    shell->args = (char**)malloc(shell->args_size);
    shell->phnd = PJobHandler_Get();

    // Resolve the absolute path of the executable binary directory.
    realpath(local_dir_path, shell->bin_dir);
    dirname(shell->bin_dir);

    // Resolving current working directory.
    if (!getcwd(shell->cwd, sizeof(shell->cwd))) {
        perror("bsh: Failed resolving current working directory");
    } else {
        strcpy(shell->pwd, shell->cwd);
    }
}

void Shell_ParseCommand(Shell* restrict shell) {
    // Parse the command by splitting it into tokens seperated by space, tab and new line characters.
    i32 i = 0;
    *(shell->args) = strtok(shell->buffer, " \t\n");
    while (*(shell->args + i)) {
        if (i >= shell->args_size / sizeof(char*) - 1) {
            shell->args_size *= 2;
            shell->args = (char**)realloc(shell->args, shell->args_size);
        }
        *(shell->args + ++i) = strtok(NULL, " \t\n");
    }
    shell->current_arg_count = i;
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
        strcpy(shell->pwd, shell->cwd);

        if (!strcmp(shell->args[1], "~")) {
            chdir(getenv("HOME"));
        } else if (!strcmp(shell->args[1], "-")) {
            chdir(shell->pwd);
        } else {
            chdir(shell->args[1]);
        }

        // Update current working directory.
        if (getcwd(path, sizeof(path))) {
            strcpy(shell->cwd, path);
        } else {
            perror("bsh");
            strcpy(shell->cwd, shell->args[1]);
        }
    } else if (!strcmp(shell->args[0], "exit")) {
        shell->status = 0;

        // Terminate all running jobs.
        for (usize i = 0; i < shell->phnd->job_count; ++i) {
            if (shell->phnd->jobs[i].state != PJOB_STATE_TERMINATED) {
                kill(shell->phnd->jobs[i].pid, SIGKILL);
                shell->phnd->jobs[i].state = PJOB_STATE_TERMINATED;
            }
        }
    } else if (!strcmp(shell->args[0], "pwd")) {
        printf("%s\n\n", shell->cwd);
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
        if (!shell->args[1] && shell->args[1][0] != '%') {
            puts("Usage: fg %[ jod_id ]\nBrings a job back to the foreground.");
        }

        // String to integer conversion with checks.
        char* ptr;
        usize id = strtol(shell->args[1] + 1, &ptr, 10);
        if (ptr - strlen(shell->args[1] + 1) != shell->args[1] + 1) {
            perror("fg");
        }

        PJob* job = PJobHandler_GetJobById(id);
        if (!job) {
            perror("kill: Job doesn't exist");
            return;
        }

        // Set our current active foreground to be, well the current job.
        shell->phnd->current_fg_job = job;

        // Redirect stdin, stdout and stderr back.
        tcsetpgrp(STDIN_FILENO, getpid());
        tcsetpgrp(STDOUT_FILENO, getpid());
        tcsetpgrp(STDERR_FILENO, getpid());

        // Send SIGCONT signal to the child process for it to continue.
        kill(job->pid, SIGCONT);

        // Wait for the child process and retrieve its status.
        i32 status;
        pid_t wpid;
        wpid = waitpid(job->pid, &status, WUNTRACED);

        // Remove the job if the child process finished, otherwise it was suspended so keep it.
        if (!WIFSTOPPED(status)) {
            PJobHandler_RemoveJob(job->pid);
        }
    } else if (!strcmp(shell->args[0], "bg")) {
        if (!shell->args[1] && shell->args[1][0] != '%') {
            puts("Usage: fg %[ jod_id ]\nBrings a job back to the foreground.");
        }

        // String to integer conversion with checks.
        char* ptr;
        usize id = strtol(shell->args[1] + 1, &ptr, 10);
        if (ptr - strlen(shell->args[1] + 1) != shell->args[1] + 1) {
            perror("fg");
        }

        PJob* job = PJobHandler_GetJobById(id);
        if (!job) {
            perror("kill: Job doesn't exist");
            return;
        }

        // Send SIGCONT signal to the child process for it to continue.
        kill(job->pid, SIGCONT);

        // Same as fg except we don't wait for the child process on the main thread.
    } else if (!strcmp(shell->args[0], "joblist")) {
        if (shell->phnd->active_job_count > 0) {
            for (usize i = 0; i < shell->phnd->job_count; ++i) {
                if (shell->phnd->jobs[i].state != PJOB_STATE_TERMINATED) {
                    printf("[%s] (%zu) %d:\t%s\n",
                           PJOB_GET_STATE_STR(shell->phnd->jobs[i].state),
                           shell->phnd->jobs[i].id,
                           shell->phnd->jobs[i].pid,
                           shell->phnd->jobs[i].command);
                }
            }
        } else {
            puts("No currently active jobs.\n");
        }
    } else if (!strcmp(shell->args[0], "kill")) {
        if (!shell->args[1]) {
            puts("Usage: kill -[ signal ] %[ jod_id ] or [ pid ]");
            return;
        }

        i32 signal = SIGKILL;
        pid_t pid = -1;
        usize id = 0;
        PJob* job = NULL;
        char** args = shell->args + 1;

        while (*args) {
            if (**args == '-') {
                // It's a signal.
                if (!strcmp(*args + 1, "SIGINT")) {
                    signal = SIGINT;
                } else if (!strcmp(*args + 1, "SIGQUIT")) {
                    signal = SIGQUIT;
                } else if (!strcmp(*args + 1, "SIGKILL")) {
                    signal = SIGKILL;
                } else if (!strcmp(*args + 1, "SIGTERM")) {
                    signal = SIGTERM;
                } else if (!strcmp(*args + 1, "SIGCONT")) {
                    signal = SIGCONT;
                } else if (!strcmp(*args + 1, "SIGSTOP")) {
                    signal = SIGSTOP;
                } else {
                    fprintf(stderr, "kill: Invalid signal: %s\n", *args + 1);
                    return;
                }
            } else if (isdigit(**args)) {
                // It's a pid.
                char* ptr;
                pid = strtol(*args, &ptr, 10);
                if (ptr - strlen(*args) != *args) {
                    perror("kill: Invalid PID");
                    return;
                }
            } else if (**args == '%') {
                // It's a job id.
                char* ptr;
                id = strtol(*args + 1, &ptr, 10);
                if (ptr - strlen(*args + 1) != *args + 1) {
                    perror("kill: Invalid Job ID");
                }

                job = PJobHandler_GetJobById(id);
                if (!job) {
                    perror("kill: Job doesn't exist");
                    return;
                }
            } else {
                fputs("kill: Invalid argument.", stderr);
                return;
            }
            ++args;
        }

        if (pid == -1) {
            // Then we're working with a job.
            kill(job->pid, signal);
            job->state = PJOB_STATE_RUNNING;
        } else {
            // Then we're working with an external process.
            kill(pid, signal);

            PJob* job = PJobHandler_GetJobByPid(pid);
            if (job)
                job->state = PJOB_STATE_RUNNING;
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

        // Create a child process.
        pid = fork();

        // Check if we are the child or main process, or if the process creation failed entirley.
        if (pid == 0) {
            // This is the child process.

            // Execute our command with its arguments.
            if (execvp(shell->args[0], shell->args) == -1) {
                perror("bsh");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Failed to create the child process.
            perror("bsh");
        } else {
            // This is the main process.
            // Add the child process as a job to the job handler.
            PJob* job = PJobHandler_AddJob(pid, shell->args[0], background);

            // Check if the command is supposed to be ran in the background.
            if (!background) {
                // Wait for the child proccess.
                wpid = waitpid(pid, &status, WUNTRACED);

                // If the child process wasn't suspended then most likley it finished its job or
                // got terminated so remove it from the job list.
                if (!WIFSTOPPED(status)) {
                    PJobHandler_RemoveJob(pid);
                }
            } else {
                printf("[+] (%zu) %d:\t%s\n", job->id, pid, shell->args[0]);
            }
        }
    }
}

i32 Shell_Run(Shell* restrict shell) {
    // Fetch some envrionment variables for the prompt.
    const char* user_name = getenv("USER");
    const char* home_dir = getenv("HOME");
    char host_name[256];
    gethostname(host_name, sizeof(host_name));

    // Our prompt loop.
    while (shell->status) {
        // Fancy prompt stuff.
        if  (str_starts_with(shell->cwd, home_dir)) {
            char* fmt = str_replace(shell->cwd, home_dir, "~");
            printf(shell->prompt, user_name, host_name, fmt);
            free(fmt);
        } else {
            printf(shell->prompt, user_name, host_name, shell->cwd);
        }

        // Dynamically consume input.
        io_readline(&shell->buffer, &shell->buffer_size);

        // Parse and execute our command.
        if (strlen(shell->buffer)) {
            Shell_ParseCommand(shell);
            Shell_HandleCommand(shell);
        }
    }

    return shell->status;
}

void Shell_Dispose(Shell* restrict shell) {
    free(shell->buffer);
    free(shell->args);
}

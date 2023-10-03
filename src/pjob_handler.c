#include "pjob_handler.h"
#include "shell_interface.h"

PJobHandler* g_instance = NULL;

void PJobHandler_Ctor() {
    g_instance->job_count = 0;
    g_instance->sa.sa_handler = signal_handler;
    g_instance->sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    for (usize i = 0; i < MAX_JOBS; ++i) {
        g_instance->jobs[i].command = NULL;
        g_instance->jobs[i].pid = -1;
    }

    sigaction(SIGCHLD, &g_instance->sa, NULL);
}

PJobHandler* PJobHandler_Get() {
    // Return the instance, otherwise create it, call the constructor and return it.
    if (!g_instance) {
        g_instance = (PJobHandler*)malloc(sizeof(PJobHandler));
        PJobHandler_Ctor();
    }
    return g_instance;
}


void PJobHandler_AddJob(const pid_t pid, const char* cmd) {
    PJobHandler* phnd = PJobHandler_Get();
    if (phnd->job_count < MAX_JOBS) {
        phnd->jobs[phnd->job_count].pid = pid;
        phnd->jobs[phnd->job_count].command = (char*)calloc(sizeof(char), strlen(cmd) + 1);
        strcpy(phnd->jobs[phnd->job_count++].command, cmd);
    } else {
        fprintf(stderr, "bsh: Cannot add job, capacity exceeded.\n");
    }
}

void PJobHandler_RemoveJob(const pid_t pid) {
    PJobHandler* phnd = PJobHandler_Get();
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].pid == pid) {
            free(phnd->jobs[i].command);
            phnd->jobs[i].command = NULL;
            for (usize j = i; j < phnd->job_count - 1; ++j) {
                phnd->jobs[j] = phnd->jobs[j + 1];
            }
            --phnd->job_count;
            break;
        }
    }
}

void PJobHandler_Destroy() {
    // Detach our signal handler and free.
    g_instance->sa.sa_handler = SIG_DFL;
    free(g_instance);
    g_instance = NULL;
}

void signal_handler(const i32 signo) {
    PJobHandler* phnd = PJobHandler_Get();
    if (signo == SIGCHLD) {
        i32 status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // Find the job with the given PID and remove it from the jobs list
            for (i32 i = 0; i < phnd->job_count; i++) {
                if (phnd->jobs[i].pid == pid) {
                    printf("[%d] Done\t%s\n", i + 1, phnd->jobs[i].command);
                    for (int j = i; j < phnd->job_count - 1; j++) {
                        phnd->jobs[j] = phnd->jobs[j + 1];
                    }
                    phnd->job_count--;
                    break;
                }
            }
        }
    }
}

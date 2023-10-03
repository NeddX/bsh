#include "pjob_handler.h"

void PJobHandler_Init(PJobHandler* restrict phnd) {
    phnd->job_count = 0;
    phnd->sa.sa_handler = signal_handler;
    phnd->sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &phnd->sa, NULL);
}

void PJobHandler_AddJob(PJobHandler* restrict phnd, const pid_t pid, const char* cmd) {
    if (phnd->job_count < MAX_JOBS) {
        phnd->jobs[phnd->job_count].pid = pid;
        strcpy(phnd->jobs[phnd->job_count++].command, cmd);
    } else {
        fprintf(stderr, "bsh: Cannot add job, capacity exceeded.\n");
    }
}

void PJobHandler_RemoveJob(PJobHandler* restrict phnd, const pid_t pid) {
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].pid == pid) {
            for (usize j = i; j < phnd->job_count - 1; ++j) {
                phnd->jobs[j] = phnd->jobs[j + 1];
            }
            --phnd->job_count;
            break;
        }
    }
}

void signal_handler(const i32 signo) {
    if (signo == SIGCHLD) {
        i32 status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        }
    }
}

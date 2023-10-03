#ifndef BSH_POSIX_JOB_HANDLER_H
#define BSH_POSIX_JOB_HANDLER_H

#include "stdbsh.h"

#define MAX_JOBS 1024

typedef struct _posix_job {
    pid_t pid;
    char* command;
} PJob;

typedef struct _posix_job_handler {
    PJob jobs[MAX_JOBS];
    u32 job_count;
    struct sigaction sa;
} PJobHandler;

PJobHandler* PJobHandler_Get();
void PJobHandler_AddJob(const pid_t pid, const char* cmd);
void PJobHandler_RemoveJob(const pid_t pid);
void PJobHandler_SignalHandler();
void PJobHandler_Destroy();

void signal_handler(const i32 signo);

#endif // BSH_POSIX_JOB_HANDLER_H

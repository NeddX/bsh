#ifndef BSH_POSIX_JOB_HANDLER_H
#define BSH_POSIX_JOB_HANDLER_H

#define PJOB_GET_STATE_STR(state) g_pjob_state_str[(usize)state]

#include "stdbsh.h"

#define DEF_JOB_COUNT 1

typedef enum _pjob_state : u8 {
    PJOB_STATE_SUSPENDED,
    PJOB_STATE_RUNNING,
    PJOB_STATE_TERMINATED
} PJobState;

extern const char* const g_pjob_state_str[];

typedef struct _posix_job {
    pid_t pid;
    usize id;
    char* command;
    bool background;
    PJobState state;
} PJob;

typedef struct _posix_job_handler {
    PJob* jobs;
    usize job_count;
    usize active_job_count;
    PJob* current_fg_job;
    struct sigaction sa;
} PJobHandler;

PJobHandler* PJobHandler_Get();
PJob* PJobHandler_AddJob(const pid_t pid, const char* cmd, const bool background);
void PJobHandler_RemoveJob(const pid_t pid);
PJob* PJobHandler_GetJobById(const usize id);
PJob* PJobHandler_GetJobByPid(const pid_t pid);
void PJobHandler_SignalHandler();
void PJobHandler_Destroy();

void signal_handler(i32 signo, siginfo_t* info, void* context);

#endif // BSH_POSIX_JOB_HANDLER_H

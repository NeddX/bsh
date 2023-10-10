#include "pjob_handler.h"

#include <signal.h>

PJobHandler* g_instance = NULL;

const char* const g_pjob_state_str[] = {
    "suspended",
    "running",
    "terminated"
};

void PJobHandler_Ctor() {
    g_instance->active_job_count = 0;
    g_instance->current_fg_job = NULL;
    g_instance->sa.sa_sigaction = signal_handler;
    g_instance->sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_NOCLDSTOP;
    sigemptyset(&g_instance->sa.sa_mask);

    // Set our desired signals to be handled by our signal handler.
    if (sigaction(SIGCHLD, &g_instance->sa, NULL) == -1) {
        perror("Failed to set up SIGCHLD handler.");
    }
    if (sigaction(SIGTSTP, &g_instance->sa, NULL) == -1) {
        perror("Failed to set up SIGTSTP handler.");
    }
    if (sigaction(SIGCONT, &g_instance->sa, NULL) == -1) {
        perror("Failed to set up SIGCONT handler.");
    }

    g_instance->jobs = (PJob*)malloc(sizeof(PJob) * DEF_JOB_COUNT);
    g_instance->job_count = DEF_JOB_COUNT;

    for (usize i = 0; i < g_instance->job_count; ++i) {
        g_instance->jobs[i].pid = -1;
        g_instance->jobs[i].id = 0;
        g_instance->jobs[i].command = NULL;
        g_instance->jobs[i].background = false;
        g_instance->jobs[i].state = PJOB_STATE_TERMINATED;
    }
}

PJobHandler* PJobHandler_Get() {
    // Return the instance, otherwise create it, call the constructor and return it.
    if (!g_instance) {
        g_instance = (PJobHandler*)malloc(sizeof(PJobHandler));
        PJobHandler_Ctor();
    }
    return g_instance;
}


PJob* PJobHandler_AddJob(const pid_t pid, const char* cmd, const bool background) {
    PJobHandler* phnd = PJobHandler_Get();

    // Grow the job list if capacity exceeded.
    if (phnd->active_job_count >= phnd->job_count) {
        phnd->job_count *= 2;
        phnd->jobs = (PJob*)realloc(phnd->jobs, phnd->job_count * sizeof(PJob));

        for (usize i = phnd->active_job_count; i < phnd->job_count; ++i) {
            phnd->jobs[i].pid = -1;
            phnd->jobs[i].id = 0;
            phnd->jobs[i].command = NULL;
            phnd->jobs[i].background = false;
            phnd->jobs[i].state = PJOB_STATE_TERMINATED;
        }
    }

    // Find an available slot to occupy, we do this by checking if the pid of the current slot is -1 because we know that
    // uninitialized or freed slot is always going to have pid of -1.
    PJob* job = NULL;
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].pid == -1) {
            job = &phnd->jobs[i];
            job->id = i;
            break;
        }
    }

    job->pid = pid;
    job->state = PJOB_STATE_RUNNING;
    job->command = (char*)calloc(sizeof(char), strlen(cmd) + 1);
    job->background = background;
    strcpy(job->command, cmd);
    ++phnd->active_job_count;

    if (!background)
        phnd->current_fg_job = job;

    return job;
}

void PJobHandler_RemoveJob(const pid_t pid) {
    PJobHandler* phnd = PJobHandler_Get();
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].pid == pid) {
            if (!phnd->jobs[i].background && &phnd->jobs[i] == phnd->current_fg_job)
                phnd->current_fg_job = NULL;

            phnd->jobs[i].id = 0;
            phnd->jobs[i].pid = -1;
            phnd->jobs[i].background = false;
            free(phnd->jobs[i].command);
            phnd->jobs[i].command = NULL;
            phnd->jobs[i].state = PJOB_STATE_TERMINATED;

            --phnd->active_job_count;
            break;
        }
    }
}

PJob* PJobHandler_GetJobById(const usize id) {
    PJobHandler* phnd = PJobHandler_Get();
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].id == id)
            return &phnd->jobs[i];
    }
    return NULL;
}

PJob* PJobHandler_GetJobByPid(const pid_t pid) {
    PJobHandler* phnd = PJobHandler_Get();
    for (usize i = 0; i < phnd->job_count; ++i) {
        if (phnd->jobs[i].pid == pid)
            return &phnd->jobs[i];
    }
    return NULL;
}

void PJobHandler_Destroy() {
    // Detach our signal handler and free.
    g_instance->sa.sa_handler = SIG_DFL;
    free(g_instance->jobs);
    free(g_instance);
    g_instance = NULL;
}

void signal_handler(i32 signo, siginfo_t* info, void* context) {
    PJobHandler* phnd = PJobHandler_Get();
    switch (signo) {
        case SIGCHLD: {
            i32 status;
            pid_t pid;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                // Find the job with the given PID and remove it from the jobs list.
                for (i32 i = 0; i < phnd->active_job_count; i++) {
                    if (phnd->jobs[i].pid == pid) {
						// For OSX.
						if (phnd->jobs[i].background)	
							printf("\n[-] (%u) %d:\t%s\n", i, phnd->jobs[i].pid, phnd->jobs[i].command);
                        PJobHandler_RemoveJob(phnd->jobs[i].pid);
                        break;
                    }
                }
            }
            break;
        }
        case SIGTSTP: {
            kill(phnd->current_fg_job->pid, SIGTSTP);
            phnd->current_fg_job->state = PJOB_STATE_SUSPENDED;
			phnd->current_fg_job->background = true;
            printf("[suspended] (%zu) %d:\t%s\n", phnd->current_fg_job->id, phnd->current_fg_job->pid, phnd->current_fg_job->command);
			phnd->current_fg_job = NULL;
            break;
        }
        case SIGCONT: {
            puts("someoe was continued...");
        }
    }
}

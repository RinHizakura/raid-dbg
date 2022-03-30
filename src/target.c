#include "target.h"
#include <stdio.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int target_lauch(target_t *t, char *cmd)
{
    int wstatus;
    pid_t pid = fork();
    /* for child process */
    if (pid == 0) {
        // disable address space randomization
        personality(ADDR_NO_RANDOMIZE);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(cmd, cmd, NULL);
    }

    /* for parent process */
    if (waitpid(pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return -1;
    }

    t->pid = pid;
    int options = PTRACE_O_EXITKILL;
    ptrace(PTRACE_SETOPTIONS, pid, NULL, options);
    printf("PID(%d)\n", t->pid);
    return 0;
}

int target_conti(target_t *t)
{
    int wstatus;
    ptrace(PTRACE_CONT, t->pid, NULL, NULL);

    if (waitpid(t->pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return -1;
    }

    return 0;
}

int target_set_breakpoint(target_t *t, size_t addr)
{
    return bp_set(&t->bp[0], t->pid, addr);
}

#include "target.h"
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int target_lauch(target_t *t, char *cmd)
{
    int wstatus;
    pid_t pid = fork();
    /* for child process */
    if (pid == 0) {
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

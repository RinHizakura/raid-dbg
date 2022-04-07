#include "target.h"
#include <stdio.h>
#include <string.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include "arch.h"

bool target_lauch(target_t *t, char *cmd)
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
        return false;
    }

    t->pid = pid;
    int options = PTRACE_O_EXITKILL;
    ptrace(PTRACE_SETOPTIONS, pid, NULL, options);
    printf("PID(%d)\n", t->pid);
    return true;
}

bool target_conti(target_t *t)
{
    int wstatus;
    ptrace(PTRACE_CONT, t->pid, NULL, NULL);

    if (waitpid(t->pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return false;
    }

    return true;
}

bool target_set_breakpoint(target_t *t, size_t addr)
{
    return bp_set(&t->bp[0], t->pid, addr);
}

bool target_get_reg(target_t *t, char *name, size_t *value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    /* TODO: choose the corresponding register by name */
    bool found = false;
    int idx = 0;
    for (; idx < regs_cnt; idx++) {
        if (strcmp(name, reg_desc_array[idx].name) == 0) {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    *value = *(((size_t *) &regs) + idx);
    return true;
}

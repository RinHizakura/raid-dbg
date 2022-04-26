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
    t->hit_bp = NULL;

    /* we should guarantee the initial value of breakpoint array */
    memset(t->bp, 0, sizeof(bp_t) * MAX_BP);
    hashtbl_create(&t->tbl, MAX_BP);

    int options = PTRACE_O_EXITKILL;
    ptrace(PTRACE_SETOPTIONS, pid, NULL, options);
    printf("PID(%d)\n", t->pid);
    return true;
}

static bool target_handle_bp(target_t *t)
{
    if (!t->hit_bp) {
        return true;
    }

    size_t addr;
    if (!target_get_reg(t, RIP, &addr))
        return false;

    /* If the address isn't at the last breakpoint we hit, it means
     * the user may change pc during this period. We don't execute an extra step
     * of the original instruction in that case. */
    if (addr == t->hit_bp->addr) {
        if (!target_step(t))
            return false;
    }

    /* restore the trap instruction before we do cont command */
    if (!bp_set(t->hit_bp))
        return false;

    t->hit_bp = NULL;
    return true;
}

bool target_step(target_t *t)
{
    int wstatus;
    ptrace(PTRACE_SINGLESTEP, t->pid, NULL, NULL);
    if (waitpid(t->pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return false;
    }
    return true;
}

bool target_conti(target_t *t)
{
    if (!target_handle_bp(t))
        return false;

    int wstatus;
    ptrace(PTRACE_CONT, t->pid, NULL, NULL);

    if (waitpid(t->pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return false;
    }

    if (WIFSTOPPED(wstatus) && (WSTOPSIG(wstatus) == SIGTRAP)) {
        /* When a breakpoint is hit previously, to keep executing instead of
         * hanging on the trap instruction latter, we first rollback pc to the
         * previous instruction and restore the original instruction
         * temporarily. */
        size_t addr = 0;
        if (!target_get_reg(t, RIP, &addr))
            return false;
        addr -= 1;

        char str[17];
        snprintf(str, 17, "%lx", addr);

        bp_t *bp;
        if (hashtbl_fetch(&t->tbl, str, (void **) &bp)) {
            t->hit_bp = bp;
            if (!bp_unset(bp))
                return false;

            if (!target_set_reg(t, RIP, addr))
                return false;
        }
    }
    return true;
}

bool target_set_breakpoint(target_t *t, size_t addr)
{
    /* FIXME: We have to enable more break point and also be
     * awared to set two breakpoint on the same address */
    bp_init(&t->bp[0], t->pid, addr);
    if (!bp_set(&t->bp[0]))
        return false;

    if (!hashtbl_add(&t->tbl, t->bp[0].addr_key, &t->bp[0]))
        return false;

    return true;
}


bool target_set_reg(target_t *t, size_t idx, size_t value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    *(((size_t *) &regs) + idx) = value;

    ptrace(PTRACE_SETREGS, t->pid, NULL, &regs);
    return true;
}

bool target_get_reg(target_t *t, size_t idx, size_t *value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    *value = *(((size_t *) &regs) + idx);
    return true;
}

bool target_get_reg_by_name(target_t *t, char *name, size_t *value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    /* TODO: choose the corresponding register by name */
    bool found = false;
    int idx = 0;
    for (; idx < REGS_CNT; idx++) {
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

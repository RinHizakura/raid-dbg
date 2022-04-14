#include "bp.h"
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include "arch.h"

void bp_init(bp_t *bp, pid_t pid, size_t addr)
{
    bp->pid = pid;
    bp->addr = addr;
    bp->is_set = false;
    snprintf(bp->addr_key, 17, "%lx", bp->addr);
}

bool bp_set(bp_t *bp)
{
    if (bp->is_set)
        return false;

    /* TODO: explicitly prevent calling this function twice on the same addr */
    size_t instr = ptrace(PTRACE_PEEKDATA, bp->pid, (void *) bp->addr, NULL);
    if (instr == (size_t) -1) {
        perror("ptrace_peek");
        return false;
    }

    bp->orig_instr = instr;
    bp->is_set = true;

    memcpy(&instr, INT3, sizeof(INT3));

    int ret = ptrace(PTRACE_POKEDATA, bp->pid, (void *) bp->addr, instr);
    if (ret == -1) {
        perror("ptrace_poke");
        return false;
    }
    return true;
}

bool bp_unset(bp_t *bp)
{
    if (!bp->is_set)
        return false;

    int ret =
        ptrace(PTRACE_POKEDATA, bp->pid, (void *) bp->addr, bp->orig_instr);
    if (ret == -1) {
        perror("ptrace_poke");
        return false;
    }

    bp->is_set = false;
    return true;
}

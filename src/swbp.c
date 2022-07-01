#include "swbp.h"
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include "arch.h"

void swbp_init(swbp_t *bp, pid_t pid, size_t addr)
{
    bp->pid = pid;
    bp->addr = addr;
    bp->is_set = false;
    snprintf(bp->addr_key, 17, "%lx", bp->addr);
}

bool swbp_set(swbp_t *bp)
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

    memcpy(&instr, INT3, sizeof(INT3));

    int ret = ptrace(PTRACE_POKEDATA, bp->pid, (void *) bp->addr, instr);
    if (ret == -1) {
        perror("ptrace_poke");
        return false;
    }

    /* FIXME: We should handle error if we fail on the middle process */
    bp->is_set = true;
    return true;
}

bool swbp_unset(swbp_t *bp)
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

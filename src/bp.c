#include "bp.h"
#include <string.h>
#include <sys/ptrace.h>
#include "arch.h"

int bp_set(bp_t *bp, pid_t pid, size_t addr)
{
    /* TODO: explicitly prevent calling this function twice on the same addr */
    size_t instr = ptrace(PTRACE_PEEKDATA, pid, (void *) addr, NULL);
    bp->orig_instr = instr;
    bp->addr = addr;
    memcpy(&instr, INT3, sizeof(INT3));
    ptrace(PTRACE_POKEDATA, pid, (void *) addr, instr);
    return 0;
}

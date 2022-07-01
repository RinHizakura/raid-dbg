#include "hwbp.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>

/* Reference:
 * - https://www.sandpile.org/x86/drx.htm
 * - https://en.wikipedia.org/wiki/X86_debug_register */

/* FIXME: support more than one debug register */
void hwbp_init(hwbp_t *bp, pid_t pid, size_t addr, HWBP_LEN len, HWBP_RW rw)
{
    bp->pid = pid;
    bp->addr = addr;
    bp->is_set = false;
    bp->index = 1;
    bp->len = len;
    bp->rw = rw;
    snprintf(bp->addr_key, 17, "%lx", bp->addr);
}

#ifdef __x86_64__

/* hardware breakpoint is supported only for x86_64 architecture */

bool hwbp_set(hwbp_t *bp)
{
    if (bp->is_set)
        return false;

    // take the debug controll register(DR7)
    size_t dr7;
    if (ptrace(PTRACE_PEEKUSER, bp->pid, offsetof(struct user, u_debugreg[7]),
               &dr7))
        return false;

    int index = bp->index;
    unsigned int enable_bit = 1 << (2 * index);
    unsigned int rw_bit = bp->rw << (16 + index * 4);
    unsigned int len_bit = bp->len << (18 + index * 4);

    // check if the local enable bit has been raised already
    if (dr7 & enable_bit)
        return false;

    dr7 = dr7 | enable_bit | rw_bit | len_bit;

    // set address to breakpoint register according to index
    if (ptrace(PTRACE_POKEUSER, bp->pid,
               offsetof(struct user, u_debugreg[index]), bp->addr))
        return false;


    // update the debug controll register
    if (ptrace(PTRACE_POKEUSER, bp->pid, offsetof(struct user, u_debugreg[7]),
               dr7))
        return false;

    // clear DR6 immediately before returning.
    if (ptrace(PTRACE_POKEUSER, bp->pid, offsetof(struct user, u_debugreg[6]),
               0))
        return false;

    /* FIXME: We should handle error if we fail on the middle process */
    bp->is_set = true;
    return true;
}

bool hwbp_handle(hwbp_t *bp)
{
    size_t dr6;
    if (ptrace(PTRACE_PEEKUSER, bp->pid, offsetof(struct user, u_debugreg[6]),
               &dr6))
        return false;

    dr6 &= ~(1 << bp->index);

    // clear DR6 for the next hit of watchpoint
    if (ptrace(PTRACE_POKEUSER, bp->pid, offsetof(struct user, u_debugreg[6]),
               dr6))
        return false;

    return true;
}
#else
#error "unsupported architecture"
#endif

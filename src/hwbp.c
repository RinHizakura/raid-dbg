#include "hwbp.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>

/* Reference:
 * - https://www.sandpile.org/x86/drx.htm
 * - https://en.wikipedia.org/wiki/X86_debug_register */

/* FIXME: support more than one debug register */
void hwbp_init(hwbp_t *bp, pid_t pid, size_t addr)
{
    bp->pid = pid;
    bp->addr = addr;
    bp->is_set = false;
    bp->index = 0;
    snprintf(bp->addr_key, 17, "%lx", bp->addr);
}

#ifdef __x86_64__

/* hardware breakpoint is supported only for x86_64 architecture */
enum hwbp_len {
    ONE_BYTE = 0b00,
    TWO_BYTE = 0b01,
    FOUR_BYTE = 0b11,
    EIGHT_BYTE = 0b10,
};

enum hwbp_rw {
    Execute = 0b00,
    Read = 0b11,
    ReadWrite = 0b11,
    Write = 0b01,
};

bool hwbp_set(hwbp_t *bp)
{
    // take the debug controll register(DR7)
    size_t dr7;
    if (ptrace(PTRACE_PEEKUSER, bp->pid, offsetof(struct user, u_debugreg[7]),
               &dr7))
        return false;

    int index = bp->index;
    unsigned int enable_bit = 1 << (2 * index);
    unsigned int rw_bit = ReadWrite << (16 + index * 4);
    unsigned int len_bit = FOUR_BYTE << (18 + index * 4);
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
               dr7))
        return false;

    return true;
}
#else
#error "unsupported architecture"
#endif

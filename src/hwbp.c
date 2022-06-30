#include "hwbp.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>

// https://www.sandpile.org/x86/drx.htm

void hwbp_init(hwbp_t *bp, pid_t pid, size_t addr)
{
    bp->pid = pid;
    bp->addr = addr;
    bp->is_set = false;
    snprintf(bp->addr_key, 17, "%lx", bp->addr);
}

enum hwbp_len {
    ONE_BYTE = 0b00,
    TWO_BYTE = 0b01,
    FOUR_BYTE = 0b11,
    EIGHT_BYTE = 0b10,
};

bool hwbp_set(hwbp_t *bp)
{
    // take the debug controll register(DR7)
    size_t dr7;
    if (ptrace(PTRACE_PEEKUSER, bp->pid, offsetof(struct user, u_debugreg[7]),
               &dr7))
        return false;

    int index = 0;
    unsigned int enable_bit = 1 << (2 * index);
    unsigned int rw_bit = 0b11 << (16 + index * 4);
    unsigned int len_bit = FOUR_BYTE << (18 + index * 4);
    // check if the local enable bit has been raised already
    if (dr7 & enable_bit)
        return false;

    dr7 = dr7 | enable_bit;

    return true;
}

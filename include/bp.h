#ifndef BP_H
#define BP_H

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

typedef struct {
    /* set to true to imply that the breakpoint is used for
     * trap instruction. */
    bool is_set;
    size_t orig_instr;
    size_t addr;
} bp_t;

bool bp_set(bp_t *bp, pid_t pid, size_t addr);

#endif

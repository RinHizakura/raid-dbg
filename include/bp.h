#ifndef BP_H
#define BP_H

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

typedef struct {
    /* set to true to imply that the breakpoint is used for
     * trap instruction. */
    pid_t pid;
    bool is_set;
    size_t orig_instr;
    size_t addr;
} bp_t;

void bp_init(bp_t *bp, pid_t pid, size_t addr);
bool bp_set(bp_t *bp);
bool bp_unset(bp_t *bp);

#endif

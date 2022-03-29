#ifndef BP_H
#define BP_H

#include <stddef.h>
#include <unistd.h>

typedef struct {
    size_t orig_instr;
    size_t addr;
} bp_t;

int bp_set(bp_t *bp, pid_t pid, size_t addr);

#endif

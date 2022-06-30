#ifndef SW_BP_H
#define SW_BP_H

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
    /* The string representation of addr, which is used as
     * key of hash table. */
    char addr_key[17];
} swbp_t;

void swbp_init(swbp_t *bp, pid_t pid, size_t addr);
bool swbp_set(swbp_t *bp);
bool swbp_unset(swbp_t *bp);

#endif

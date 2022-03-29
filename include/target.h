#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <sys/types.h>
#include "bp.h"

#define MAX_BP 16
typedef struct {
    pid_t pid;
    /* TODO: maintain a better data structure to store multiple
     * breakpoints and access them efficiently.  */
    bp_t bp[MAX_BP];
} target_t;

int target_lauch(target_t *t, char *cmd);
int target_conti(target_t *t);
int target_set_breakpoint(target_t *t, size_t addr);

#endif

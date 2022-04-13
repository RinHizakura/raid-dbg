#ifndef TARGET_H
#define TARGET_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "bp.h"
#include "hashtbl.h"

#define MAX_BP 16
typedef struct {
    pid_t pid;
    /* TODO: maintain a better data structure to store multiple
     * breakpoints and access them efficiently.  */
    hashtbl_t tbl;
    bp_t bp[MAX_BP];
    bp_t *hit_bp;
} target_t;

bool target_lauch(target_t *t, char *cmd);
bool target_step(target_t *t);
bool target_conti(target_t *t);
bool target_set_breakpoint(target_t *t, size_t addr);
bool target_set_reg(target_t *t, size_t idx, size_t value);
bool target_get_reg(target_t *t, size_t idx, size_t *value);
bool target_get_reg_by_name(target_t *t, char *name, size_t *value);
#endif

#ifndef TARGET_H
#define TARGET_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "hashtbl.h"
#include "hwbp.h"
#include "swbp.h"

#define MAX_BP 16
typedef struct {
    bool run;
    char *cmd;
    pid_t pid;

    /* TODO: maintain a better data structure to store multiple
     * breakpoints and access them efficiently.  */
    hashtbl_t tbl;
    swbp_t bp[MAX_BP];
    swbp_t *hit_bp;
    uint16_t bp_bitmap;

    /* FIXME: raid will only use this breakpoint as watchpoint now, but
     * it's actually fine to use it for pausing tracee. */
    hwbp_t hwbp;
} target_t;

bool target_lauch(target_t *t, char *cmd);
bool target_runnable(target_t *t);
bool target_step(target_t *t);
bool target_conti(target_t *t);
bool target_set_breakpoint(target_t *t, size_t addr);
bool target_set_watchpoint(target_t *t, size_t addr, size_t len);
bool target_set_reg(target_t *t, size_t idx, size_t value);
bool target_get_reg(target_t *t, size_t idx, size_t *value);
bool target_get_reg_by_name(target_t *t, char *name, size_t *value);
bool target_read_mem(target_t *t, void *buf, size_t len, size_t target_addr);
bool target_write_mem(target_t *t, size_t *buf, size_t len, size_t target_addr);
void target_close(target_t *t);
#endif

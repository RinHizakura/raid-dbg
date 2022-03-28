#ifndef DBG_H
#define DBG_H

#include "target.h"
#include "utils/list.h"

typedef struct {
    target_t target;
    struct list_head list;
} dbg_t;

int dbg_init(dbg_t *dbg, char *cmd);
void dbg_run(dbg_t *dbg);

#endif

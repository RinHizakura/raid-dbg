#ifndef DBG_H
#define DBG_H

#include "console.h"
#include "target.h"

typedef struct {
    target_t target;
    console_t console;
} dbg_t;

int dbg_init(dbg_t *dbg, char *cmd);
void dbg_run(dbg_t *dbg);

#endif

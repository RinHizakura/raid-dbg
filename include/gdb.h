#ifndef GDB_H
#define GDB_H

#include "console.h"
#include "target.h"

typedef struct {
    target_t target;
    console_t console;
} gdb_t;

int gdb_init(gdb_t *gdb, char *CMD);
void gdb_run(gdb_t *gdb);

#endif

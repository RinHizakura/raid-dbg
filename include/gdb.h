#ifndef GDB_H
#define GDB_H

#include "target.h"

typedef struct {
    target_t target;
} gdb_t;

int gdb_init(gdb_t *gdb, char *CMD);
#endif

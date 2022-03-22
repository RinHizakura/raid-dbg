#include "gdb.h"

int gdb_init(gdb_t *gdb, char *CMD)
{
    return target_lauch(&gdb->target, CMD);
}

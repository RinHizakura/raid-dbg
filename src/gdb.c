#include "gdb.h"

int gdb_init(gdb_t *gdb, char *cmd)
{
    int ret = target_lauch(&gdb->target, cmd);
    if (ret)
        return ret;

    ret = console_init(&gdb->console);
    if (ret)
        return ret;

    return 0;
}

void gdb_run(gdb_t *gdb)
{
    console_run(&gdb->console);
}

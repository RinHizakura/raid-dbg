#include "dbg.h"

int dbg_init(dbg_t *dbg, char *cmd)
{
    int ret = target_lauch(&dbg->target, cmd);
    if (ret)
        return ret;

    ret = console_init(&dbg->console);
    if (ret)
        return ret;

    return 0;
}

void dbg_run(dbg_t *dbg)
{
    console_run(&dbg->console);
}

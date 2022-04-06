#include <stdio.h>
#include "dbg.h"
#define CMD "./bin/hello"

int main()
{
    dbg_t dbg;
    int ret = dbg_init(&dbg, CMD);
    if (ret) {
        perror("dbg_init");
        return ret;
    }

    dbg_run(&dbg);
    return 0;
}

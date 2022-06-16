#include <stdio.h>
#include "dbg.h"

static char *cmd;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: %s executable\n", argv[0]);
        return 1;
    }

    cmd = argv[1];

    dbg_t dbg;
    if (!dbg_init(&dbg, cmd)) {
        perror("dbg_init");
        return -1;
    }

    dbg_run(&dbg);
    dbg_close(&dbg);
    return 0;
}

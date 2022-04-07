#include <stdio.h>
#include "dbg.h"
#define CMD "./bin/hello"

int main()
{
    dbg_t dbg;
    if (!dbg_init(&dbg, CMD)) {
        perror("dbg_init");
        return -1;
    }

    dbg_run(&dbg);
    return 0;
}

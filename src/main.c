#include "dbg.h"
#define CMD "./bin/hello"

int main()
{
    dbg_t dbg;
    dbg_init(&dbg, CMD);
    dbg_run(&dbg);
    return 0;
}

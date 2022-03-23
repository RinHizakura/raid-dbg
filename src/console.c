#include "console.h"
#include <stddef.h>
#include <stdio.h>
#include "linenoise.h"

int console_init(console_t *console)
{
    // TODO
    return 0;
}

void console_run(console_t *console)
{
    char *line;
    while ((line = linenoise("(raid)")) != NULL) {
        printf("echo %s\n", line);
        linenoiseFree(line);
    }
}

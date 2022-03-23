#ifndef CONSOLE_H
#define CONSOLE_H

#include "utils/list.h"

typedef struct {
    struct list_head list;
} console_t;

int console_init(console_t *console);
void console_run(console_t *console);

#endif

#ifndef CONSOLE_H
#define CONSOLE_H

typedef struct {
    int option;
} console_t;

int console_init(console_t *console);
void console_run(console_t *console);

#endif

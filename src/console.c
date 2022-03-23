#include "console.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

static console_t *gConsole;

typedef bool (*cmd_func)(int argc, char *argv[]);

struct cmd_entry {
    struct list_head node;
    char *name;
    char *description;
    cmd_func op;
};

static int console_add_cmd(console_t *console,
                           char *name,
                           cmd_func op,
                           char *description)
{
    struct cmd_entry *entry = malloc(sizeof(struct cmd_entry));
    if (!entry)
        return -1;
    INIT_LIST_HEAD(&entry->node);

    entry->name = name;
    entry->description = description;
    entry->op = op;

    list_add_tail(&entry->node, &console->list);
    return 0;
}

static bool do_help(int argc, char *argv[])
{
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gConsole->list, node)
    {
        printf("%s | %s\n", item->name, item->description);
        ;
    }
    return true;
}

int console_init(console_t *console)
{
    gConsole = console;
    INIT_LIST_HEAD(&console->list);

    console_add_cmd(console, "help", do_help, " print me!");
    return 0;
}

static bool console_match_cmd(int argc, char *argv[])
{
    bool ret = false;
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gConsole->list, node)
    {
        if (!strcmp(item->name, "help")) {
            ret = item->op(argc, argv);
            break;
        }
    }

    return ret;
}

void console_run(console_t *console)
{
    bool ret;
    int argc;
    char **argv;

    char *line;
    while ((line = linenoise("(raid)")) != NULL) {
        if (!strcmp(line, "help"))
            ret = console_match_cmd(argc, argv);
        else
            printf("echo %s\n", line);
        linenoiseFree(line);
    }
}

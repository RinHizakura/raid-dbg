#include "dbg.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

static dbg_t *gDbg;

typedef bool (*cmd_func)(int argc, char *argv[]);

struct cmd_entry {
    struct list_head node;
    char *name;
    char *description;
    cmd_func op;
};

static int dbg_add_cmd(dbg_t *dbg, char *name, cmd_func op, char *description)
{
    struct cmd_entry *entry = malloc(sizeof(struct cmd_entry));
    if (!entry)
        return -1;
    INIT_LIST_HEAD(&entry->node);

    entry->name = name;
    entry->description = description;
    entry->op = op;

    list_add_tail(&entry->node, &dbg->list);
    return 0;
}

static bool do_help(int argc, char *argv[])
{
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gDbg->list, node)
    {
        printf("%s | %s\n", item->name, item->description);
        ;
    }
    return true;
}

static bool do_cont(int argc, char *argv[])
{
    return !target_conti(&gDbg->target);
}

int dbg_init(dbg_t *dbg, char *cmd)
{
    int ret = target_lauch(&dbg->target, cmd);
    if (ret)
        return ret;

    gDbg = dbg;
    INIT_LIST_HEAD(&dbg->list);

    dbg_add_cmd(dbg, "help", do_help, " print me!");
    dbg_add_cmd(dbg, "cont", do_cont, " restart the stopped tracee process.");
    return 0;
}

static bool dbg_match_cmd(int argc, char *argv[])
{
    bool ret = false;
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gDbg->list, node)
    {
        if (!strcmp(item->name, argv[0])) {
            ret = item->op(argc, argv);
            goto cmd_found;
        }
    }

    fprintf(stderr, "Command %s not found\n", argv[0]);
    return false;

cmd_found:
    return ret;
}

void dbg_run(dbg_t *dbg)
{
    bool ret;
    int argc;
    char **argv;

    char *line;
    while ((line = linenoise("(raid)")) != NULL) {
        ret = dbg_match_cmd(1, &line);
        linenoiseFree(line);
    }
}

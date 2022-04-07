#include "dbg.h"
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

static bool dbg_add_cmd(dbg_t *dbg, char *name, cmd_func op, char *description)
{
    struct cmd_entry *entry = malloc(sizeof(struct cmd_entry));
    if (!entry)
        return false;
    INIT_LIST_HEAD(&entry->node);

    entry->name = name;
    entry->description = description;
    entry->op = op;

    list_add_tail(&entry->node, &dbg->list);
    return true;
}

static bool do_help(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gDbg->list, node)
    {
        printf("%10s | %s\n", item->name, item->description);
    }
    return true;
}

static bool do_cont(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    return target_conti(&gDbg->target);
}

static bool do_break(int argc, char *argv[])
{
    if (argc != 2)
        return false;

    size_t addr;
    sscanf(argv[1], "%lx", &addr);
    return target_set_breakpoint(&gDbg->target, addr);
}

static bool do_regs(int argc, char *argv[])
{
    if (argc != 3)
        return false;

    bool ret = false;
    char *sub_cmd = argv[1];
    char *reg_name = argv[2];

    if (strcmp(sub_cmd, "read") == 0) {
        size_t value;
        ret = target_get_reg(&gDbg->target, reg_name, &value);
        if (!ret) {
            printf("Unknown register name '%s'\n", reg_name);
        } else {
            printf("reg %s = %lx\n", reg_name, value);
        }
    } else {
        printf("Unknown subcommand '%s' of regs\n", sub_cmd);
    }

    return ret;
}

static bool cmd_maybe(const char *target, const char *src)
{
    for (size_t i = 0; i < strlen(src); i++) {
        if (target[i] == '\0')
            return false;
        if (src[i] != target[i])
            return false;
    }
    return true;
}

static void completion(const char *buf, linenoiseCompletions *lc)
{
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &gDbg->list, node)
    {
        if (cmd_maybe(item->name, buf))
            linenoiseAddCompletion(lc, item->name);
    }
}

bool dbg_init(dbg_t *dbg, char *cmd)
{
    int ret = target_lauch(&dbg->target, cmd);
    if (!ret)
        return false;

    gDbg = dbg;
    INIT_LIST_HEAD(&dbg->list);

    dbg_add_cmd(dbg, "help", do_help, "print me!");
    dbg_add_cmd(dbg, "cont", do_cont, "restart the stopped tracee process.");
    dbg_add_cmd(dbg, "break", do_break, "set breakpoint on tracee process.");
    dbg_add_cmd(dbg, "regs", do_regs, "dump registers.");

    linenoiseSetCompletionCallback(completion);
    return true;
}

static char **dbg_parse_cmd(char *line, int *argc)
{
    /* FIXME: A stupid command line parser: take the first token as
     * command and use everything left as one arguments. Also note that the
     * returned pointer array only valid when the input parameter line is
     * valid. */

    int _argc = 1;
    char *src = line;
    while (*src++ != '\0') {
        if (*src == ' ') {
            *src = '\0';
            src++;
            _argc++;
        }
    }

    char **argv = calloc(sizeof(char *), _argc);
    for (int i = 0; i < _argc; i++) {
        argv[i] = line;
        line += strlen(argv[i]) + 1;
    }

    *argc = _argc;
    return argv;
}

static bool dbg_match_cmd(dbg_t *dbg, int argc, char *argv[])
{
    bool ret = false;
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &dbg->list, node)
    {
        if (!strcmp(item->name, argv[0])) {
            ret = item->op(argc, argv);
            goto cmd_found;
        }
    }

    return false;

cmd_found:
    return ret;
}

void dbg_run(dbg_t *dbg)
{
    bool ret;
    char *line;
    int argc;
    char **argv;

    while ((line = linenoise("(raid)")) != NULL) {
        linenoiseHistoryAdd(line);
        argv = dbg_parse_cmd(line, &argc);
        ret = dbg_match_cmd(dbg, argc, argv);
        if (!ret)
            fprintf(stderr, "Command '%s' not found or failed\n", line);
        linenoiseFree(line);
    }
}

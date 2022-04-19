#include "dbg.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

static dbg_t *gDbg;

typedef bool (*cmd_func)(int argc, char *argv[]);

struct opt_entry {
    struct list_head node;
    char *name;
    cmd_func op;
};

struct cmd_entry {
    struct list_head node;  // the node for pending on dbg_t
    struct list_head list;  // the list of option parameters
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
    INIT_LIST_HEAD(&entry->list);

    entry->name = name;
    entry->description = description;
    entry->op = op;

    list_add_tail(&entry->node, &dbg->list);
    return true;
}

static bool __dbg_add_option(struct cmd_entry *cmd, char *name, cmd_func op)
{
    struct opt_entry *entry = malloc(sizeof(struct opt_entry));
    if (!entry)
        return false;
    INIT_LIST_HEAD(&entry->node);

    entry->name = name;
    entry->op = op;

    list_add_tail(&entry->node, &cmd->list);
    return true;
}

static bool dgb_add_option(dbg_t *dbg,
                           char *cmd_name,
                           char *opt_name,
                           cmd_func op)
{
    struct cmd_entry *entry = malloc(sizeof(struct cmd_entry));
    if (!entry)
        return false;

    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &dbg->list, node)
    {
        if (!strcmp(item->name, cmd_name)) {
            __dbg_add_option(item, opt_name, op);
            return true;
        }
    }

    return false;
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

static bool do_regs_read(int argc, char *argv[])
{
    size_t value;
    char *reg_name = argv[2];

    if (argc != 3)
        return false;

    bool ret = target_get_reg_by_name(&gDbg->target, reg_name, &value);
    if (!ret) {
        fprintf(stderr, "Unknown register name '%s'\n", reg_name);
    } else {
        printf("reg %s = %lx\n", reg_name, value);
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
    if (!target_lauch(&dbg->target, cmd))
        return false;

    if (!dwarf_init(&dbg->dwarf, cmd))
        return false;

    gDbg = dbg;
    INIT_LIST_HEAD(&dbg->list);

    dbg_add_cmd(dbg, "help", do_help, "print me!");
    dbg_add_cmd(dbg, "cont", do_cont, "restart the stopped tracee process.");
    dbg_add_cmd(dbg, "break", do_break, "set breakpoint on tracee process.");

    dbg_add_cmd(dbg, "regs", NULL, "dump registers.");
    dgb_add_option(dbg, "regs", "read", do_regs_read);
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

static bool dbg_match_subcmd(struct cmd_entry *item, int argc, char *argv[])
{
    bool ret = false;
    char *sub_cmd = argv[1];
    struct opt_entry *opt_item, *tmp;

    if (argc < 2)
        return false;

    list_for_each_entry_safe(opt_item, tmp, &item->list, node)
    {
        if (!strcmp(opt_item->name, sub_cmd)) {
            if (opt_item->op) {
                ret = opt_item->op(argc, argv);
                break;
            }
        }
    }

    if (!ret)
        fprintf(stderr, "Sub-command '%s' not found or failed\n", sub_cmd);

    return ret;
}

static bool dbg_match_cmd(dbg_t *dbg, int argc, char *argv[])
{
    bool ret = false;
    struct cmd_entry *item, *tmp;

    list_for_each_entry_safe(item, tmp, &dbg->list, node)
    {
        if (!strcmp(item->name, argv[0])) {
            if (item->op) {
                ret = item->op(argc, argv);
            } else {
                ret = dbg_match_subcmd(item, argc, argv);
            }
            break;
        }
    }

    if (!ret)
        fprintf(stderr, "Command '%s' not found or failed\n", argv[0]);

    return ret;
}

void dbg_run(dbg_t *dbg)
{
    char *line;
    int argc;
    char **argv;

    while ((line = linenoise("(raid)")) != NULL) {
        linenoiseHistoryAdd(line);
        argv = dbg_parse_cmd(line, &argc);
        dbg_match_cmd(dbg, argc, argv);
        linenoiseFree(line);
    }
}

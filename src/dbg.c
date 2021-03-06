#include "dbg.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "linenoise.h"

static dbg_t *gDbg;
static bool gExec = true;

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

static bool do_quit(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    gExec = false;
    return true;
}

static bool dbg_print_source_line(dbg_t *dbg, size_t addr)
{
    /* check target status first to avoid to query the invalid address */
    if (!target_runnable(&dbg->target))
        return false;

    const char *file_name;
    int linep;
    if (!dwarf_get_addr_src(&dbg->dwarf, addr - gDbg->base_addr, &file_name,
                            &linep))
        return false;

    /* FIXME: We can't assume that we are always hitting a breakpoint if we
     * called dbg_print_source_line. */
    printf("\nHit breakpoint at addr %lx file %s: line %d.\n", addr, file_name,
           linep);

    char *line = NULL;
    size_t len = 0;

    FILE *stream = fopen(file_name, "r");
    if (stream == NULL) {
        perror("fopen");
        return false;
    }

    for (int i = 0; i < linep; i++) {
        if (getline(&line, &len, stream) == -1) {
            perror("getline");
            return false;
        }
    }

    printf("%d\t%s", linep, line);
    free(line);
    fclose(stream);

    return true;
}

static bool do_cont(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    if (!target_conti(&gDbg->target))
        return false;

    size_t addr;
    target_get_reg(&gDbg->target, RIP, &addr);
    dbg_print_source_line(gDbg, addr);

    return true;
}

static bool do_step(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    size_t addr;
    int prev_linep = 0;
    int linep = 0;

    target_get_reg(&gDbg->target, RIP, &addr);
    /* FIXME: The actually behavior of 'step' now is keeping going until the
     * next line of the input executable. In other words, we won't stop at any
     * line under shared library. To implement this, we intentionally ignore the
     * return error of dwarf_get_addr_src and use linep = 0 to represent when
     * the address is at unknown line (i.e. shared library in our assumption).
     */
    dwarf_get_addr_src(&gDbg->dwarf, addr - gDbg->base_addr, NULL, &prev_linep);

    /* FIXME: This is a also a very naive implementation: we keep doing stepi
     * until meeting the next line. We should have to try other better
     * algorithms for this. */
    do {
        if (!target_step(&gDbg->target))
            return false;
        target_get_reg(&gDbg->target, RIP, &addr);
        dwarf_get_addr_src(&gDbg->dwarf, addr - gDbg->base_addr, NULL, &linep);
    } while ((linep == 0) || (prev_linep == linep));

    dbg_print_source_line(gDbg, addr);

    return true;
}

static bool dbg_get_ra(dbg_t *dbg, size_t addr, size_t *ra)
{
    int reg_no, offset;
    if (!dwarf_get_frame_reg(&gDbg->dwarf, addr - dbg->base_addr,
                             DWARF_RA_REGNO, &reg_no, &offset))
        return false;

    size_t ra_addr;
    target_get_reg(&gDbg->target, regno_map[reg_no], &ra_addr);
    target_read_mem(&gDbg->target, ra, sizeof(size_t), ra_addr + offset);

    return true;
}

static bool do_next(__attribute__((unused)) int argc,
                    __attribute__((unused)) char *argv[])
{
    func_t func;
    size_t addr;

    target_get_reg(&gDbg->target, RIP, &addr);
    if (!dwarf_get_addr_func(&gDbg->dwarf, addr - gDbg->base_addr, &func))
        return false;

    size_t len = func.high_pc - func.low_pc;
    size_t *buf = malloc(len);

    /* Backup the whole function block first. Then, we'll inject INT3 in every
     * line except the current one */
    target_read_mem(&gDbg->target, buf, len, func.low_pc + gDbg->base_addr);

    /* The function should be placed in the same file, so we just get it at the
     * first time we call dwarf_get_addr_src */
    const char *file_name;
    int linep, start_linep, end_linep;
    if (!dwarf_get_addr_src(&gDbg->dwarf, addr - gDbg->base_addr, &file_name,
                            &linep))
        return false;
    if (!dwarf_get_addr_src(&gDbg->dwarf, func.low_pc, NULL, &start_linep))
        return false;
    if (!dwarf_get_addr_src(&gDbg->dwarf, func.high_pc, NULL, &end_linep))
        return false;

    size_t int3 = INT3[0];
    for (int l = start_linep; l < end_linep; l++) {
        if (l == linep)
            continue;

        size_t bp_addr;
        if (!dwarf_get_line_addr(&gDbg->dwarf, file_name, l, &bp_addr))
            return false;

        target_write_mem(&gDbg->target, &int3, sizeof(int3),
                         bp_addr + gDbg->base_addr);
    }

    /* We also need to set breakpoint at return address */
    size_t ra, ra_instr;
    if (!dbg_get_ra(gDbg, addr, &ra))
        return false;
    target_read_mem(&gDbg->target, &ra_instr, sizeof(size_t), ra);
    target_write_mem(&gDbg->target, &int3, sizeof(int3), ra);

    if (!target_conti(&gDbg->target))
        return false;
    target_write_mem(&gDbg->target, buf, len, func.low_pc + gDbg->base_addr);
    target_write_mem(&gDbg->target, &ra_instr, sizeof(size_t), ra);
    free(buf);

    target_get_reg(&gDbg->target, RIP, &addr);
    dbg_print_source_line(gDbg, addr);

    return true;
}

static bool dbg_set_func_symbol_bp(dbg_t *dbg, char *bp_name, size_t *addr)
{
    if (!dwarf_get_func_symbol_addr(&dbg->dwarf, bp_name, addr))
        return false;

    *addr += dbg->base_addr;
    return true;
}

static bool dbg_set_addr_bp(__attribute__((unused)) dbg_t *dbg,
                            char *bp_name,
                            size_t *addr)
{
    int pos, ret;
    ret = sscanf(bp_name, "0x%lx%n", addr, &pos);
    if ((ret == 0) || ((size_t) pos != strlen(bp_name)))
        return false;
    return true;
}

static bool dbg_set_src_line_bp(dbg_t *dbg, char *bp_name_base, size_t *addr)
{
    /* FIXME: This is a very naive implementation. It could be unsafe! */
    bool ret = false;
    char *bp_name = strdup(bp_name_base);

    char *file_name = strtok(bp_name, ":");
    if (file_name == NULL)
        goto get_src_line_bp_end;

    char *line_str = strtok(NULL, ":");
    if (line_str == NULL)
        return false;

    int ret2, pos, line;
    ret2 = sscanf(line_str, "%d%n", &line, &pos);
    if ((ret2 == 0) || ((size_t) pos != strlen(line_str)))
        goto get_src_line_bp_end;

    if (!dwarf_get_line_addr(&dbg->dwarf, file_name, line, addr))
        goto get_src_line_bp_end;

    *addr += dbg->base_addr;
    ret = true;

get_src_line_bp_end:
    free(bp_name);
    return ret;
}

static bool do_break(int argc, char *argv[])
{
    if (argc != 2)
        return false;

    size_t addr;
    char *bp_name = argv[1];

    if (!(dbg_set_addr_bp(gDbg, bp_name, &addr)) &&
        !(dbg_set_src_line_bp(gDbg, bp_name, &addr)) &&
        !(dbg_set_func_symbol_bp(gDbg, bp_name, &addr))) {
        fprintf(stderr, "Invalid breakpoint name '%s'\n", bp_name);
        return false;
    }

    if (!target_set_breakpoint(&gDbg->target, addr))
        return false;

    return true;
}

static bool dbg_set_var_symbol_bp(dbg_t *dbg, char *bp_name, var_t *var)
{
    size_t scope_pc;
    target_get_reg(&dbg->target, RIP, &scope_pc);

    if (!dwarf_get_var_symbol_addr(&dbg->dwarf, scope_pc - dbg->base_addr,
                                   bp_name, var)) {
        fprintf(stderr, "No symbol \"%s\" in current context.\n", bp_name);
        return false;
    }

    if (var->type != VAR_TYPE_ADDR) {
        fprintf(stderr, "\"%s\" can't be watched", bp_name);
        return false;
    }

    return true;
}

static bool do_watch(int argc, char *argv[])
{
    if (argc != 2)
        return false;

    char *bp_name = argv[1];
    var_t var;
    if (!(dbg_set_var_symbol_bp(gDbg, bp_name, &var))) {
        fprintf(stderr, "Invalid watch point name '%s'\n", bp_name);
        return false;
    }

    if (!target_set_watchpoint(&gDbg->target, var.addr + gDbg->base_addr,
                               var.bytes)) {
        fprintf(stderr, "Set watch point on '%s' fail\n", bp_name);
        return false;
    }

    return true;
}

static bool do_backtrace(__attribute__((unused)) int argc,
                         __attribute__((unused)) char *argv[])
{
    size_t addr;
    func_t f;
    int frame_no = 0;

    target_get_reg(&gDbg->target, RIP, &addr);

    do {
        size_t ra;
        if (!dbg_get_ra(gDbg, addr, &ra))
            return false;

        if (!dwarf_get_addr_func(&gDbg->dwarf, ra - gDbg->base_addr, &f))
            return false;

        printf("frame #%d: %s\n", frame_no, f.name);
        addr = ra;
        frame_no++;
    } while (strcmp(f.name, "main") != 0 && frame_no < 5);
    return true;
}

static bool dbg_print_reg(dbg_t *dbg, char *reg_name)
{
    size_t value;

    bool ret = target_get_reg_by_name(&dbg->target, reg_name, &value);
    if (!ret) {
        fprintf(stderr, "Unknown register name '%s'\n", reg_name);
    } else {
        printf("$%ld = %lx\n", ++dbg->print_cnt, value);
    }

    return ret;
}


static bool dbg_print_var(dbg_t *dbg, char *var_name)
{
    size_t scope_pc;

    target_get_reg(&dbg->target, RIP, &scope_pc);

    var_t var;
    if (!dwarf_get_var_symbol_addr(&dbg->dwarf, scope_pc - dbg->base_addr,
                                   var_name, &var)) {
        fprintf(stderr, "No symbol \"%s\" in current context.\n", var_name);
        return false;
    }

    size_t addr;
    size_t value = 0;
    switch (var.type) {
    case VAR_TYPE_REG_OFF:
        target_get_reg(&dbg->target, regno_map[var.reg_no], &addr);
        target_read_mem(&dbg->target, &value, var.bytes, addr + var.offset);
        break;
    case VAR_TYPE_ADDR:
        target_read_mem(&dbg->target, &value, var.bytes,
                        dbg->base_addr + var.addr);
        break;
    default:
        break;
    }

    printf("$%ld = %ld\n", ++dbg->print_cnt, value);
    return true;
}

static bool do_print(__attribute__((unused)) int argc,
                     __attribute__((unused)) char *argv[])
{
    if (argc != 2)
        return false;

    bool ret;
    char *str = argv[1];
    /* A register name should be prefixed with '$' */
    if (str[0] == '$') {
        ret = dbg_print_reg(gDbg, str + 1);
    } else {
        ret = dbg_print_var(gDbg, str);
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

static bool dbg_init_debuggee_base(dbg_t *dbg)
{
    /* FIXME: This is a very naive implementation for the base address of
     * debuggee, which assume it is the first line of address on proc
     * filesystem. Refine this if we have better solution. */
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", dbg->target.pid);

    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    stream = fopen(path, "r");
    if (stream == NULL) {
        perror("fopen");
        return false;
    }

    nread = getdelim(&line, &len, '-', stream);
    if (nread == -1) {
        perror("getdelim");
        return false;
    }

    sscanf(line, "%lx", &dbg->base_addr);
    free(line);
    fclose(stream);

    return true;
}

bool dbg_init(dbg_t *dbg, char *cmd)
{
    memset(dbg, 0, sizeof(dbg_t));

    if (!target_lauch(&dbg->target, cmd))
        return false;

    if (!dwarf_init(&dbg->dwarf, cmd))
        return false;

    gDbg = dbg;
    if (!dbg_init_debuggee_base(dbg))
        return false;
    printf("@ %lx\n", dbg->base_addr);
    INIT_LIST_HEAD(&dbg->list);

    dbg_add_cmd(dbg, "help", do_help, "print me!");
    dbg_add_cmd(dbg, "cont", do_cont, "restart the stopped tracee process.");
    dbg_add_cmd(dbg, "break", do_break, "set breakpoint on tracee process.");
    dbg_add_cmd(dbg, "watch", do_watch, "set watchpoint on tracee process.");
    dbg_add_cmd(dbg, "quit", do_quit, "exit from raid debugger.");
    dbg_add_cmd(dbg, "step", do_step, "step in to the next line.");
    dbg_add_cmd(dbg, "next", do_next, "step over to the next line.");
    dbg_add_cmd(dbg, "backtrace", do_backtrace, "backtrace the call frame.");
    dbg_add_cmd(dbg, "print", do_print, "dump the request variables.");

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
        free(argv);

        if (!gExec)
            break;
    }
}

void dbg_close(dbg_t *dbg)
{
    struct cmd_entry *item, *tmp;
    struct opt_entry *opt_item, *opt_tmp;

    list_for_each_entry_safe(item, tmp, &dbg->list, node)
    {
        if (!item->op) {
            list_for_each_entry_safe(opt_item, opt_tmp, &item->list, node)
            {
                list_del(&opt_item->node);
                free(opt_item);
            }
        }

        list_del(&item->node);
        free(item);
    }

    target_close(&dbg->target);
    dwarf_close(&dbg->dwarf);
}

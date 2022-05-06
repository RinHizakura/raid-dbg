#include "dwarf_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "dwarf.h"

static bool dwarf_cu_get_die(dwarf_t *dwarf, cu_t *cu, Dwarf_Die *die_result)
{
    Dwarf_Off die_offset = cu->off + cu->header_sizep;
    if (!dwarf_offdie(dwarf->inner, die_offset, die_result))
        return false;
    return true;
}

static void dwarf_iter_start(dwarf_iter_t *iter, dwarf_t *dwarf)
{
    iter->finish = false;
    iter->next_off = 0;
    iter->dwarf = dwarf->inner;
}

static bool dwarf_iter_next(dwarf_iter_t *iter, cu_t *cu)
{
    if (iter->finish)
        return false;

    cu->off = iter->next_off;

    int ret =
        dwarf_next_unit(iter->dwarf, cu->off, &iter->next_off,
                        &cu->header_sizep, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ret != 0) {
        iter->finish = true;
        return false;
    }

    return true;
}

static void die_iter_child_start(die_iter_t *iter, Dwarf_Die *die)
{
    iter->start = true;
    iter->finish = false;
    memcpy(&iter->die, die, sizeof(Dwarf_Die));
}

static bool die_iter_child_next(die_iter_t *iter, Dwarf_Die *die_result)
{
    int ret;

    if (iter->finish)
        return false;

    if (iter->start) {
        iter->start = false;
        ret = dwarf_child(&iter->die, &iter->die);
    } else {
        ret = dwarf_siblingof(&iter->die, &iter->die);
    }

    if (ret != 0) {
        iter->finish = false;
        return false;
    }

    memcpy(die_result, &iter->die, sizeof(Dwarf_Die));
    return true;
}

static int test_callback(Dwarf_Die *die, __attribute__((unused)) void *arg)
{
    Dwarf_Die die_result;
    die_iter_t die_iter;
    Dwarf_Attribute attr_result;
    Dwarf_Word line;
    Dwarf_Addr addr;

    if (dwarf_tag(die) != DW_TAG_subprogram) {
        return DWARF_CB_OK;
    }

    const char *file = dwarf_decl_file(die);
    if (file != NULL)
        printf("Die of file %s\n", file);

    if (dwarf_attr(die, DW_AT_name, &attr_result)) {
        const char *str = dwarf_formstring(&attr_result);
        if (str != NULL)
            printf("\tfunction %s\n", str);
    }

    if (dwarf_attr(die, DW_AT_decl_line, &attr_result)) {
        if (!dwarf_formudata(&attr_result, &line))
            printf("\t@ line %lx\n", line);
    }

    if (dwarf_attr(die, DW_AT_low_pc, &attr_result)) {
        if (!dwarf_formaddr(&attr_result, &addr))
            printf("\t@ pc %lx\n", addr);
    }

    die_iter_child_start(&die_iter, die);
    while (die_iter_child_next(&die_iter, &die_result)) {
        if (dwarf_tag(&die_result) != DW_TAG_formal_parameter)
            continue;

        if (!dwarf_attr(&die_result, DW_AT_name, &attr_result))
            continue;

        const char *str = dwarf_formstring(&attr_result);

        if (str != NULL)
            printf("\t\tparam %s\n", str);
    }

    return DWARF_CB_OK;
}

static void test1(dwarf_t *dwarf)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Die die_result;

    dwarf_iter_start(&iter, dwarf);

    while (dwarf_iter_next(&iter, &cu)) {
        dwarf_cu_get_die(dwarf, &cu, &die_result);

        if (dwarf_tag(&die_result) == DW_TAG_compile_unit) {
            dwarf_getfuncs(&die_result, test_callback, NULL, 0);
        }
    }
}

bool dwarf_init(dwarf_t *dwarf, char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    dwarf->inner = dwarf_begin(fd, DWARF_C_READ);

    /* FIXME: This is only used for testing the libdw api, which
     * will be removed in the future. */
    test1(dwarf);

    return true;
}

struct dwarf_get_symbol_addr_args {
    char *symbol;
    size_t *addr;
};

static int dwarf_get_symbol_addr_cb(Dwarf_Die *die, void *arg)
{
    Dwarf_Attribute attr_result;
    Dwarf_Addr addr;
    char *symbol = ((struct dwarf_get_symbol_addr_args *) arg)->symbol;
    size_t *ret_addr = ((struct dwarf_get_symbol_addr_args *) arg)->addr;

    if (dwarf_tag(die) != DW_TAG_subprogram)
        return DWARF_CB_OK;

    if (!dwarf_attr(die, DW_AT_name, &attr_result))
        return DWARF_CB_OK;

    const char *str = dwarf_formstring(&attr_result);
    if (str == NULL || strcmp(str, symbol) != 0) {
        return DWARF_CB_OK;
    }

    if (!dwarf_attr(die, DW_AT_low_pc, &attr_result))
        return DWARF_CB_OK;

    if (dwarf_formaddr(&attr_result, &addr))
        return DWARF_CB_OK;

    *ret_addr = addr;
    return DWARF_CB_ABORT;
}

bool dwarf_get_symbol_addr(dwarf_t *dwarf, char *sym, size_t *addr)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Die die_result;
    struct dwarf_get_symbol_addr_args args = {.symbol = sym, .addr = addr};
    dwarf_iter_start(&iter, dwarf);

    while (dwarf_iter_next(&iter, &cu)) {
        dwarf_cu_get_die(dwarf, &cu, &die_result);

        if (dwarf_tag(&die_result) == DW_TAG_compile_unit) {
            ptrdiff_t ret =
                dwarf_getfuncs(&die_result, dwarf_get_symbol_addr_cb, &args, 0);
            if (ret != 0) {
                return true;
            }
        }
    }

    return false;
}


bool dwarf_get_addr_src(dwarf_t *dwarf,
                        Dwarf_Addr addr,
                        const char **name,
                        int *linep)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Line *line;
    Dwarf_Die cudie;

    if (dwarf_addrdie(dwarf->inner, addr, &cudie)) {
        line = dwarf_getsrc_die(&cudie, addr);
        if (line != NULL) {
            if (name != NULL)
                *name = dwarf_linesrc(line, NULL, NULL);
            if (linep != NULL)
                dwarf_lineno(line, linep);
        }
        return true;
    }

    return false;
}

void dwarf_close(dwarf_t *dwarf)
{
    dwarf_end(dwarf->inner);
}

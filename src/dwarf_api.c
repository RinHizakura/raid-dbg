#include "dwarf_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "dwarf.h"

bool dwarf_cu_get_die(dwarf_t *dwarf, cu_t *cu, Dwarf_Die *die_result)
{
    Dwarf_Off die_offset = cu->off + cu->header_sizep;
    if (!dwarf_offdie(dwarf->inner, die_offset, die_result))
        return false;
    return true;
}

void dwarf_iter_start(dwarf_iter_t *iter, dwarf_t *dwarf)
{
    iter->finish = false;
    iter->next_off = 0;
    iter->dwarf = dwarf->inner;
}

bool dwarf_iter_next(dwarf_iter_t *iter, cu_t *cu)
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

void die_iter_child_start(die_iter_t *iter, Dwarf_Die *die)
{
    iter->start = true;
    iter->finish = false;
    memcpy(&iter->die, die, sizeof(Dwarf_Die));
}

bool die_iter_child_next(die_iter_t *iter, Dwarf_Die *die_result)
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

static int callback(Dwarf_Die *die, void *arg)
{
    Dwarf_Die die_result;
    die_iter_t die_iter;
    Dwarf_Attribute attr_result;

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

static void test(dwarf_t *dwarf)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Die die_result;
    Dwarf_Attribute attr_result;

    dwarf_iter_start(&iter, dwarf);

    while (dwarf_iter_next(&iter, &cu)) {
        dwarf_cu_get_die(dwarf, &cu, &die_result);

        if (dwarf_tag(&die_result) == DW_TAG_compile_unit) {
            dwarf_getfuncs(&die_result, callback, NULL, 0);
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
    test(dwarf);

    return true;
}

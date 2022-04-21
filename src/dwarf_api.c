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
    iter->next_off = 0;
    iter->dwarf = dwarf->inner;
}

bool dwarf_iter_next(dwarf_iter_t *iter, cu_t *cu)
{
    cu->off = iter->next_off;

    int ret =
        dwarf_next_unit(iter->dwarf, cu->off, &iter->next_off,
                        &cu->header_sizep, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ret != 0)
        return false;

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
    if (iter->start) {
        iter->start = false;
        ret = dwarf_child(&iter->die, &iter->die);
    } else {
        ret = dwarf_siblingof(&iter->die, &iter->die);
    }

    if (ret != 0)
        return false;

    memcpy(die_result, &iter->die, sizeof(Dwarf_Die));
    return true;
}

static int callback(Dwarf_Die *die, void *arg)
{
    const char *file = dwarf_decl_file(die);
    if (file != NULL)
        printf("file %s\n", file);

    Dwarf_Die die_result;
    die_iter_t die_iter;
    Dwarf_Attribute attr_result;

    die_iter_child_start(&die_iter, die);

    if (!die_iter_child_next(&die_iter, &die_result))
        return DWARF_CB_OK;

    if (!dwarf_attr_integrate(&die_result, DW_AT_linkage_name, &attr_result))
        return DWARF_CB_OK;
    const char *str = dwarf_formstring(&attr_result);

    if (str != NULL)
        printf("dwarf %s\n", str);

    return DWARF_CB_OK;
}

static void test(dwarf_t *dwarf)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Die die_result;
    Dwarf_Attribute attr_result;

    dwarf_iter_start(&iter, dwarf);
    dwarf_iter_next(&iter, &cu);
    dwarf_cu_get_die(dwarf, &cu, &die_result);

    if (dwarf_tag(&die_result) == DW_TAG_compile_unit) {
        dwarf_getfuncs(&die_result, callback, NULL, 0);
    }

    if (!dwarf_attr(&die_result, DW_AT_producer, &attr_result))
        return;

    const char *str = dwarf_formstring(&attr_result);

    if (str != NULL)
        printf("dwarf %s\n", str);
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

#include "dwarf_api.h"
#include <fcntl.h>
#include <stdio.h>
#include "dwarf.h"

bool dwarf_cu_get_die(dwarf_t *dwarf, cu_t *cu, Dwarf_Die *die_result)
{
    Dwarf_Off die_offset = cu->off + cu->header_sizep;
    if (!dwarf_offdie(dwarf->inner, die_offset, die_result))
        return false;
    return true;
}

void dwarf_iter_start(dwarf_t *dwarf)
{
    dwarf->iter_next_off = 0;
}

bool dwarf_iter_next(dwarf_t *dwarf, cu_t *cu)
{
    cu->off = dwarf->iter_next_off;

    int ret =
        dwarf_next_unit(dwarf->inner, cu->off, &dwarf->iter_next_off,
                        &cu->header_sizep, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ret != 0)
        return false;

    return true;
}

static int callback(Dwarf_Die *die, void *arg)
{
    printf("1\n");
    return DWARF_CB_OK;
}

static void test(dwarf_t *dwarf)
{
    cu_t cu;
    Dwarf_Die die_result;
    Dwarf_Attribute attr_result;

    dwarf_iter_start(dwarf);
    dwarf_iter_next(dwarf, &cu);
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

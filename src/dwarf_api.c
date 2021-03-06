#include "dwarf_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "arch.h"
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

bool dwarf_init(dwarf_t *dwarf, char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    dwarf->inner = dwarf_begin(fd, DWARF_C_READ);

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

/* FIXME: consider if there are many static functions with same
 * names, what will happend? */
bool dwarf_get_func_symbol_addr(dwarf_t *dwarf, char *sym, size_t *addr)
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
    Dwarf_Line *line;
    Dwarf_Die die;

    if (!dwarf_addrdie(dwarf->inner, addr, &die))
        return false;

    line = dwarf_getsrc_die(&die, addr);
    if (line != NULL) {
        if (name != NULL)
            *name = dwarf_linesrc(line, NULL, NULL);
        if (linep != NULL)
            dwarf_lineno(line, linep);
    }

    return true;
}

static bool dwarf_get_scope_die(dwarf_t *dwarf,
                                Dwarf_Addr addr,
                                Dwarf_Die **func_die)
{
    Dwarf_Die die;

    if (!dwarf_addrdie(dwarf->inner, addr, &die))
        return false;

    Dwarf_Die *scopes;
    int scopes_cnt = dwarf_getscopes(&die, addr, &scopes);
    if (scopes_cnt < 1)
        return false;

    *func_die = &scopes[0];
    return true;
}

static bool dwarf_get_die_var(Dwarf_Die *func_die,
                              char *name,
                              Dwarf_Die *die_result)
{
    Dwarf_Attribute attr_result;
    die_iter_t die_iter;
    bool found = false;

    die_iter_child_start(&die_iter, func_die);

    while (die_iter_child_next(&die_iter, die_result)) {
        int tag = dwarf_tag(die_result);
        if (tag != DW_TAG_formal_parameter && tag != DW_TAG_variable)
            continue;

        if (!dwarf_attr(die_result, DW_AT_name, &attr_result))
            continue;

        const char *str = dwarf_formstring(&attr_result);

        if (strcmp(str, name) == 0) {
            found = true;
            break;
        }
    }

    return found;
}

static bool dwarf_get_var_type(Dwarf_Die *var_die, Dwarf_Word *bytes)
{
    Dwarf_Die type_die;
    Dwarf_Attribute attr_result;

    if (!dwarf_attr(var_die, DW_AT_type, &attr_result) ||
        !dwarf_formref_die(&attr_result, &type_die) ||
        (dwarf_tag(&type_die) != DW_TAG_base_type) ||
        !dwarf_attr(&type_die, DW_AT_byte_size, &attr_result) ||
        dwarf_formudata(&attr_result, bytes))
        return false;

    return true;
}

static bool dwarf_get_global_var_symbol_addr(dwarf_t *dwarf,
                                             char *name,
                                             Dwarf_Die *die_result)
{
    cu_t cu;
    dwarf_iter_t iter;
    Dwarf_Die cu_die;

    dwarf_iter_start(&iter, dwarf);
    while (dwarf_iter_next(&iter, &cu)) {
        dwarf_cu_get_die(dwarf, &cu, &cu_die);

        if (dwarf_tag(&cu_die) == DW_TAG_compile_unit) {
            if (dwarf_get_die_var(&cu_die, name, die_result)) {
                return true;
            }
        }
    }

    return false;
}

static bool dwarf_get_local_var_symbol_addr(dwarf_t *dwarf,
                                            Dwarf_Addr scope_pc,
                                            char *name,
                                            Dwarf_Die *die_result)
{
    Dwarf_Die *func_die;
    if (!dwarf_get_scope_die(dwarf, scope_pc, &func_die))
        return false;

    if (!dwarf_get_die_var(func_die, name, die_result))
        return false;

    return true;
}

static bool dwarf_expr_fbreg(dwarf_t *dwarf,
                             Dwarf_Op *ops,
                             Dwarf_Addr scope_pc,
                             var_t *var)
{
    if (!dwarf_get_frame_cfa(dwarf, scope_pc, &var->reg_no, &var->offset))
        return false;
    /* Add extra offset to find the address of variable */
    var->offset += ops->number;
    var->type = VAR_TYPE_REG_OFF;

    return true;
}

static void dwarf_expr_addr(Dwarf_Op *ops, var_t *var)
{
    var->addr = ops->number;
    var->type = VAR_TYPE_ADDR;
}

bool dwarf_get_var_symbol_addr(dwarf_t *dwarf,
                               Dwarf_Addr scope_pc,
                               char *name,
                               var_t *var)
{
    bool found = false;
    Dwarf_Die die_result;
    Dwarf_Attribute attr_result;

    /* We need to consider the current scope to pick only
     * the visible variable, so the input parameters including scope_pc. */
    found = dwarf_get_local_var_symbol_addr(dwarf, scope_pc, name, &die_result);

    if (!found && !dwarf_get_global_var_symbol_addr(dwarf, name, &die_result))
        return false;

    if (!dwarf_attr(&die_result, DW_AT_location, &attr_result))
        return false;

    Dwarf_Op *ops;
    size_t nops = 0;
    if (dwarf_getlocation(&attr_result, &ops, &nops))
        return false;

    /* FIXME: This is the only dwarf location operation we consider now. */
    if ((ops->atom != DW_OP_fbreg && ops->atom != DW_OP_addr) || nops != 1)
        return false;

    switch (ops->atom) {
    case DW_OP_fbreg:
        dwarf_expr_fbreg(dwarf, ops, scope_pc, var);
        break;
    case DW_OP_addr:
        dwarf_expr_addr(ops, var);
        break;
    default:
        break;
    }

    if (!dwarf_get_var_type(&die_result, &var->bytes))
        return false;

    return true;
}

bool dwarf_get_addr_func(dwarf_t *dwarf, Dwarf_Addr addr, func_t *func)
{
    Dwarf_Sword offset;
    Dwarf_Attribute attr_result;
    Dwarf_Die *func_die;
    if (!dwarf_get_scope_die(dwarf, addr, &func_die))
        return false;

    if (!dwarf_attr(func_die, DW_AT_low_pc, &attr_result) ||
        dwarf_formaddr(&attr_result, &func->low_pc))
        return false;

    if (!dwarf_attr(func_die, DW_AT_high_pc, &attr_result) ||
        dwarf_formsdata(&attr_result, &offset))
        return false;
    func->high_pc = func->low_pc + offset - 1;

    if (!dwarf_attr(func_die, DW_AT_name, &attr_result))
        return false;
    func->name = dwarf_formstring(&attr_result);
    if (func->name == NULL)
        return false;

    return true;
}

bool dwarf_get_line_addr(dwarf_t *dwarf,
                         const char *fname,
                         int line,
                         size_t *addr)
{
    Dwarf_Line **line_info;
    size_t nsrcs = 0;
    dwarf_getsrc_file(dwarf->inner, fname, line, 0, &line_info, &nsrcs);

    /* FIXME: In what scenario will the nsrcs > 1? */
    if (nsrcs != 1)
        return false;

    if (dwarf_lineaddr(line_info[0], addr))
        return false;

    return true;
}

static bool dwarf_get_frame(dwarf_t *dwarf, size_t addr, Dwarf_Frame **frame)
{
    /* FIXME: Only the CFI in the ELF file(.eh_frame) is supported now. */
    Elf *elf = dwarf_getelf(dwarf->inner);
    Dwarf_CFI *cfi = dwarf_getcfi_elf(elf);
    if (cfi == NULL)
        return false;

    if (dwarf_cfi_addrframe(cfi, addr, frame))
        return false;

    return true;
}

static bool __dwarf_get_frame_cfa(Dwarf_Frame *frame, int *reg_no, int *offset)
{
    Dwarf_Op *ops;
    size_t nops = 0;
    if (dwarf_frame_cfa(frame, &ops, &nops) != 0)
        return false;

    /* FIXME: This is the only dwarf location operation we consider now. */
    if (ops->atom != DW_OP_bregx || nops != 1)
        return false;

    *reg_no = ops->number;
    *offset = ops->number2;

    return true;
}

bool dwarf_get_frame_cfa(dwarf_t *dwarf, size_t addr, int *reg_no, int *offset)
{
    Dwarf_Frame *frame;
    if (!dwarf_get_frame(dwarf, addr, &frame))
        return false;

    if (!__dwarf_get_frame_cfa(frame, reg_no, offset))
        return false;

    return true;
}

bool dwarf_get_frame_reg(dwarf_t *dwarf,
                         size_t addr,
                         int req_reg,
                         int *reg_no,
                         int *offset)
{
    Dwarf_Frame *frame;
    if (!dwarf_get_frame(dwarf, addr, &frame))
        return false;

    Dwarf_Op *ops;
    Dwarf_Op ops_mem[3];
    size_t nops = 0;
    if (dwarf_frame_register(frame, req_reg, ops_mem, &ops, &nops) != 0)
        return false;

    /* FIXME: The following is the only case we will handle now */
    if (nops != 2 || ops[0].atom != DW_OP_call_frame_cfa ||
        ops[1].atom != DW_OP_plus_uconst)
        return false;


    if (!__dwarf_get_frame_cfa(frame, reg_no, offset))
        return false;

    *offset = *offset + ops[1].number;

    return true;
}

void dwarf_close(dwarf_t *dwarf)
{
    dwarf_end(dwarf->inner);
}

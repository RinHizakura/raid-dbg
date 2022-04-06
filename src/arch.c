#include "arch.h"

const uint8_t INT3[1] = {0xcc};

/* clang-format off */
const struct reg_desc reg_desc_array[regs_cnt] = {
    [r15]      = {"r15"},
    [r14]      = {"r14"},
    [r13]      = {"r13"},
    [r12]      = {"r12"},
    [rbp]      = {"rbp"},
    [rbx]      = {"rbx"},
    [r11]      = {"r11"},
    [r10]      = {"r10"},
    [r9]       = {"r9"},
    [r8]       = {"r8"},
    [rax]      = {"rax"},
    [rcx]      = {"rcx"},
    [rdx]      = {"rdx"},
    [rsi]      = {"rsi"},
    [rdi]      = {"rdi"},
    [orig_rax] = {"orig_rax"},
    [rip]      = {"rip"},
    [cs]       = {"cs"},
    [eflags]   = {"eflags"},
    [rsp]      = {"rsp"},
    [ss]       = {"ss"},
    [fs_base]  = {"fs_base"},
    [gs_base]  = {"gs_base"},
    [ds]       = {"ds"},
    [es]       = {"es"},
    [fs]       = {"fs"},
    [gs]       = {"gs"},
};
/* clang-format on */

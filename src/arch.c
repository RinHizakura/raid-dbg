#include "arch.h"

const uint8_t INT3[1] = {0xcc};

/* clang-format off */
const struct reg_desc reg_desc_array[REGS_CNT] = {
    [REG_R15]      = {"r15"},
    [REG_R14]      = {"r14"},
    [REG_R13]      = {"r13"},
    [REG_R12]      = {"r12"},
    [REG_RBP]      = {"rbp"},
    [REG_RBX]      = {"rbx"},
    [REG_R11]      = {"r11"},
    [REG_R10]      = {"r10"},
    [REG_R9]       = {"r9"},
    [REG_R8]       = {"r8"},
    [REG_RAX]      = {"rax"},
    [REG_RCX]      = {"rcx"},
    [REG_RDX]      = {"rdx"},
    [REG_RSI]      = {"rsi"},
    [REG_RDI]      = {"rdi"},
    [REG_ORIG_RAX] = {"orig_rax"},
    [REG_RIP]      = {"rip"},
    [REG_CS]       = {"cs"},
    [REG_EFLAGS]   = {"eflags"},
    [REG_RSP]      = {"rsp"},
    [REG_SS]       = {"ss"},
    [REG_FS_BASE]  = {"fs_base"},
    [REG_GS_BASE]  = {"gs_base"},
    [REG_DS]       = {"ds"},
    [REG_ES]       = {"es"},
    [REG_FS]       = {"fs"},
    [REG_GS]       = {"gs"},
};
/* clang-format on */

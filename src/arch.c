#include "arch.h"

const uint8_t INT3[1] = {0xcc};

/* clang-format off */
const struct reg_desc reg_desc_array[REGS_CNT] = {
    [R15]      = {"r15"},
    [R14]      = {"r14"},
    [R13]      = {"r13"},
    [R12]      = {"r12"},
    [RBP]      = {"rbp"},
    [RBX]      = {"rbx"},
    [R11]      = {"r11"},
    [R10]      = {"r10"},
    [R9]       = {"r9"},
    [R8]       = {"r8"},
    [RAX]      = {"rax"},
    [RCX]      = {"rcx"},
    [RDX]      = {"rdx"},
    [RSI]      = {"rsi"},
    [RDI]      = {"rdi"},
    [ORIG_RAX] = {"orig_rax"},
    [RIP]      = {"rip"},
    [CS]       = {"cs"},
    [EFLAGS]   = {"eflags"},
    [RSP]      = {"rsp"},
    [SS]       = {"ss"},
    [FS_BASE]  = {"fs_base"},
    [GS_BASE]  = {"gs_base"},
    [DS]       = {"ds"},
    [ES]       = {"es"},
    [FS]       = {"fs"},
    [GS]       = {"gs"},
};

/* FIXME: Only use temporarily for the lazy guy RinHizakura.
 * We should fill all mapping value for him. */
const int regno_map[REGS_CNT] = {
    [7] = RSP,
};
/* clang-format on */

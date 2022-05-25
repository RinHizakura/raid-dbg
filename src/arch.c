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

/* Reference:
 * [x86_64_abi](https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.95.pdf)
 * Figure 3.36: DWARF Register Number Mapping */

#define R_PAIR(n) PAIR(n, R##n)
#define REG_PAIR      \
    PAIR(0, RAX),     \
    PAIR(1, RDX),     \
    PAIR(2, RCX),     \
    PAIR(3, RBX),     \
    PAIR(4, RSI),     \
    PAIR(5, RDI),     \
    PAIR(6, RBP),     \
    PAIR(7, RSP),     \
    R_PAIR(8),        \
    R_PAIR(9),        \
    R_PAIR(10),       \
    R_PAIR(11),       \
    R_PAIR(12),       \
    R_PAIR(13),       \
    R_PAIR(14),       \
    R_PAIR(15),

/* clang-format on */

/* FIXME: There's some register index which aren't map */
const int regno_map[16] = {
#define PAIR(a, b) [a] = b
    REG_PAIR
#undef PAIR
};

const int reverse_regno_map[REGS_CNT] = {
#define PAIR(a, b) [b] = a
    REG_PAIR
#undef PAIR
};

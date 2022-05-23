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

/* Reference: [x86_64_abi](https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.95.pdf)
 * Figure 3.36: DWARF Register Number Mapping */
const int regno_map[16] = {
    [0] = RAX,
    [1] = RDX,
    [2] = RCX,
    [3] = RBX,
    [4] = RSI,
    [5] = RDI,
    [6] = RBP,
    [7] = RSP,
#define X(n) [n] = R##n
    X(8), X(9), X(10), X(11), X(12), X(13), X(14), X(15),
#undef X
};
/* clang-format on */

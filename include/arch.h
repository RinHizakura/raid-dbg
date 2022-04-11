#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

#ifdef __x86_64__
struct reg_desc {
    const char *name;
};

/* The layout follow usr/include/sys/user.h */
typedef struct {
    enum {
        REG_R15,
        REG_R14,
        REG_R13,
        REG_R12,
        REG_RBP,
        REG_RBX,
        REG_R11,
        REG_R10,
        REG_R9,
        REG_R8,
        REG_RAX,
        REG_RCX,
        REG_RDX,
        REG_RSI,
        REG_RDI,
        REG_ORIG_RAX,
        REG_RIP,
        REG_CS,
        REG_EFLAGS,
        REG_RSP,
        REG_SS,
        REG_FS_BASE,
        REG_GS_BASE,
        REG_DS,
        REG_ES,
        REG_FS,
        REG_GS,
        REGS_CNT,
    } idx;
} regs_struct_idx;

extern const uint8_t INT3[1];
extern const struct reg_desc reg_desc_array[REGS_CNT];
#else
#error "unsupported architecture"
#endif

#endif

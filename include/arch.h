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
        R15,
        R14,
        R13,
        R12,
        RBP,
        RBX,
        R11,
        R10,
        R9,
        R8,
        RAX,
        RCX,
        RDX,
        RSI,
        RDI,
        ORIG_RAX,
        RIP,
        CS,
        EFLAGS,
        RSP,
        SS,
        FS_BASE,
        GS_BASE,
        DS,
        ES,
        FS,
        GS,
        REGS_CNT,
    } idx;
} regs_struct_idx;

extern const uint8_t INT3[1];
extern const struct reg_desc reg_desc_array[REGS_CNT];
#else
#error "unsupported architecture"
#endif

#endif

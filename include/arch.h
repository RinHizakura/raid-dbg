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
        r15,
        r14,
        r13,
        r12,
        rbp,
        rbx,
        r11,
        r10,
        r9,
        r8,
        rax,
        rcx,
        rdx,
        rsi,
        rdi,
        orig_rax,
        rip,
        cs,
        eflags,
        rsp,
        ss,
        fs_base,
        gs_base,
        ds,
        es,
        fs,
        gs,
        regs_cnt,
    } idx;
} regs_struct_idx;

extern const uint8_t INT3[1];
extern const struct reg_desc reg_desc_array[regs_cnt];
#else
#error "unsupported architecture"
#endif

#endif

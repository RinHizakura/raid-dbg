#ifndef HW_BP_H
#define HW_BP_H

#include <stdbool.h>
#include <unistd.h>
#ifdef __x86_64__
typedef enum {
    ONE_BYTE = 0b00,
    TWO_BYTE = 0b01,
    FOUR_BYTE = 0b11,
    EIGHT_BYTE = 0b10,
} HWBP_LEN;

typedef enum {
    Execute = 0b00,
    Read = 0b11,
    ReadWrite = 0b11,
    Write = 0b01,
} HWBP_RW;
#else
#error "unsupported architecture"
#endif

typedef struct {
    pid_t pid;
    int index;
    bool is_set;
    size_t addr;
    char addr_key[17];

    HWBP_LEN len;
    HWBP_RW rw;
} hwbp_t;

void hwbp_init(hwbp_t *bp, pid_t pid, size_t addr, HWBP_LEN len, HWBP_RW rw);
bool hwbp_set(hwbp_t *bp);
bool hwbp_handle(hwbp_t *bp);
bool hwbp_unset(hwbp_t *bp);

#endif

#ifndef HW_BP_H
#define HW_BP_H

#include <stdbool.h>
#include <unistd.h>

typedef struct {
    pid_t pid;
    int index;
    bool is_set;
    size_t addr;
    char addr_key[17];
} hwbp_t;

void hwbp_init(hwbp_t *bp, pid_t pid, size_t addr);
bool hwbp_set(hwbp_t *bp);
bool hwbp_unset(hwbp_t *bp);

#endif

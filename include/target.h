#ifndef TARGET_H
#define TARGET_H

#include <sys/types.h>

typedef struct {
    pid_t pid;
} target_t;

int target_lauch(target_t *t, char *cmd);

#endif

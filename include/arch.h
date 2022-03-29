#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

#ifdef __x86_64__
static uint8_t INT3[] = {0xcc};
#else
#error "unsupported architecture"
#endif

#endif

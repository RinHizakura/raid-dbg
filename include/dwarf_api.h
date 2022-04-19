#ifndef DWARF_API_H
#define DWARF_API_H

#include "libdw.h"

typedef struct {
    Dwarf *inner;
    /* This is the stored offset to interate compile units */
    Dwarf_Off iter_next_off;
} dwarf_t;

typedef struct {
    Dwarf_Off off;
    size_t header_sizep;
} cu_t;

bool dwarf_init(dwarf_t *dwarf, char *file);

#endif

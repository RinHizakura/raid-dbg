#ifndef HASHTBL_H
#define HASHTBL_H

#include <search.h>
#include <stdbool.h>

typedef struct {
    struct hsearch_data htab;
    size_t size;
} hashtbl_t;

#endif

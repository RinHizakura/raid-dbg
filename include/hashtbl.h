#ifndef HASHTBL_H
#define HASHTBL_H

#include <search.h>
#include <stdbool.h>

typedef struct {
    struct hsearch_data htab;
    size_t size;
} hashtbl_t;

bool hashtbl_create(hashtbl_t *tbl, size_t size);
bool hashtbl_add(hashtbl_t *tbl, char *key, void *data);
bool hashtbl_fetch(hashtbl_t *tbl, char *key, void **data);
#endif

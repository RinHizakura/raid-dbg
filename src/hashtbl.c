#include "hashtbl.h"

bool hashtbl_create(hashtbl_t *tbl, size_t size)
{
    hcreate_r(size, &tbl->htab);
    tbl->size = size;

    return true;
}

bool hashtbl_add(hashtbl_t *tbl, char *key, void *data)
{
    int n = 0;
    ENTRY e, *ep;

    e.key = (char *) key;
    e.data = data;
    n = hsearch_r(e, ENTER, &ep, &tbl->htab);

    if (n == 0)
        return false;

    return true;
}

#include "hashtbl.h"

bool hashtbl_create(hashtbl_t *tbl, size_t size)
{
    *tbl = (hashtbl_t){.htab = (struct hsearch_data){0}, .size = 0};
    hcreate_r(size, &tbl->htab);
    tbl->size = size;

    return true;
}

bool hashtbl_add(hashtbl_t *tbl, char *key, void *data)
{
    int n;
    ENTRY e, *ep;

    e.key = key;
    e.data = data;
    n = hsearch_r(e, ENTER, &ep, &tbl->htab);

    if (n == 0)
        return false;

    return true;
}

bool hashtbl_fetch(hashtbl_t *tbl, char *key, void **data)
{
    int n;
    ENTRY e, *ep;

    e.key = key;
    n = hsearch_r(e, FIND, &ep, &tbl->htab);

    if (n == 0) {
        return false;
    }

    *data = ep->data;
    return true;
}

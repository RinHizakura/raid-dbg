#ifndef LIST_H
#define LIST_H

#include <stddef.h>

#define container_of(ptr, type, member)                      \
    ({                                                       \
        const typeof(((type *) 0)->member) *__mptr = (ptr);  \
        (type *) ((char *) __mptr - offsetof(type, member)); \
    })

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list, list->prev = list;
}

/* Insert a newer entry between two known consecutive entries. */
static inline void __list_add(struct list_head *newer,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = newer;
    newer->next = next;
    newer->prev = prev;
    prev->next = newer;
}

/* add a newer entry.
 * @newer: newer entry to be added
 * @head: list head to add it after
 * Insert a newer entry after the specified head.
 */
static inline void list_add(struct list_head *newer, struct list_head *head)
{
    __list_add(newer, head, head->next);
}

/* add a newer entry.
 * @newer: newer entry to be added
 * @head: list head to add it before
 * Insert a newer entry before the specified head.
 */
static inline void list_add_tail(struct list_head *newer,
                                 struct list_head *head)
{
    __list_add(newer, head->prev, head);
}

/* Delete a list entry by making the prev/next entries point to each other. */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/* deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

/* deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

/* tests whether a list is empty.
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 * list_splice_tail() - Add list nodes from a list to end of another list
 * @list: pointer to the head of the list with the node entries
 * @head: pointer to the head of the list
 *
 * All nodes from @list are added to to the end of the list of @head.
 * It is similar to list_add_tail but for multiple nodes. The @list head is not
 * modified and has to be initialized to be used as a valid list head/node
 * again.
 */
static inline void list_splice_tail(struct list_head *list,
                                    struct list_head *head)
{
    struct list_head *head_last = head->prev;
    struct list_head *list_first = list->next;
    struct list_head *list_last = list->prev;

    if (list_empty(list))
        return;

    head->prev = list_last;
    list_last->next = head;

    list_first->prev = head_last;
    head_last->next = list_first;
}

/**
 * list_splice_tail_init() - Move list nodes from a list to end of another list
 * @list: pointer to the head of the list with the node entries
 * @head: pointer to the head of the list
 *
 * All nodes from @list are added to to the end of the list of @head.
 * It is similar to list_add_tail but for multiple nodes.
 *
 * The @list head will not end up in an uninitialized state like when using
 * list_splice. Instead the @list is initialized again to the an empty
 * list/unlinked state.
 */
static inline void list_splice_tail_init(struct list_head *list,
                                         struct list_head *head)
{
    list_splice_tail(list, head);
    INIT_LIST_HEAD(list);
}

/* get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* get the first element from a lis.t
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/* iterate over list of given type.
 * @pos:      the type * to use as a loop cursor.
 * @head:     the head for your list.
 * @member:   the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                 \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                               \
         pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list entries and allow deletes
 * @entry: pointer used as iterator
 * @safe: @type pointer used to store info for next entry in list
 * @head: pointer to the head of the list
 * @member: name of the list_head member variable in struct type of @entry
 *
 * The current node (iterator) is allowed to be removed from the list. Any
 * other modifications to the the list will cause undefined behavior.
 */
#define list_for_each_entry_safe(entry, safe, head, member)            \
    for (entry = list_entry((head)->next, typeof(*entry), member),     \
        safe = list_entry(entry->member.next, typeof(*entry), member); \
         &entry->member != (head); entry = safe,                       \
        safe = list_entry(safe->member.next, typeof(*entry), member))

#endif

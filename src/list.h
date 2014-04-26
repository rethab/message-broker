#ifndef LIST_HEADER
#define LIST_HEADER

#define LIST_NOT_FOUND -2

/* root node of a linked list */
struct list {

    /* guards the list as a whole.
     * it is to be held in write
     * lock if the list is modified
     * (add, remove) and in read
     * mode if the contents are
     * (e.g. statistics updated).
     */
    pthread_rwlock_t *listrwlock;

    /* root node of the linked list */
    struct node *root;
};

/* node in a list. guarded by
 * listrwlock (in list struct) as
 * a whole and (potentially) by
 * locks in individual entries
 */
struct node {
    /* value of the node */
    void *entry;

    /* pointer to next node. null
     * if this node marks the end */
    struct node *next;   
};

/* initialize the list*/
int list_init(struct list *list);

/* frees the list. must be empty */
int list_destroy(struct list *list);

/* adds an entry to the end of the list */
int list_add(struct list *list, void *entry);

/* generic version to remove an element from a
 * list by supplying a comparison function */
int list_remove_generic(struct list *list,
        int (*cmp)(const void *, const void *),
        const void *entry);

/* removes an element from the list */
int list_remove(struct list *list, void *entry);

/* checks whether the list is empty */
int list_empty(struct list *list);

/* empties the list */
int list_clean(struct list *list);

/* returns the number of elements in the list */
int list_len(struct list *list);

/* checks whether an entry exists in the list */
int list_contains(struct list *list, void *entry);

#endif

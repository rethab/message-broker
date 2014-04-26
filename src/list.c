#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "list.h"

int list_init(struct list *list) {

    int ret;

    pthread_rwlock_t *rwlock = malloc(sizeof(pthread_rwlock_t));
    assert(rwlock != NULL);
    ret = pthread_rwlock_init(rwlock, NULL);
    assert(ret == 0);
    list->listrwlock = rwlock;

    list->root = NULL;

    return 0;
}

int list_destroy(struct list *list) {

    int ret;
    
    assert(list->root == NULL);

    ret = pthread_rwlock_destroy(list->listrwlock);
    assert(ret == 0);
    free(list->listrwlock);
    list->listrwlock = NULL;

    list->root = NULL;

    return 0;
}

int list_add(struct list *list, void *entry) {
    if (list->root == NULL) {
        list->root = malloc(sizeof (struct node));
        assert(list->root != NULL);
        list->root->entry = entry;
        list->root->next = NULL;
    } else {
        struct node *cur = list->root;
        for (;cur->next != NULL; cur = cur->next);
        cur->next = malloc(sizeof(struct list));
        assert(cur->next != NULL);
        cur->next->entry = entry;
        cur->next->next = NULL;
    }
    return 0;
}

int list_empty(struct list *list) {
    return list->root == NULL;
}

/* returns the number of elements in a list */
int list_len(struct list *list) {
    struct node *cur = list->root;
    int n = 0;
    for (; cur != NULL; cur = cur->next) {
        n++;
    }
    return n;
}


/* removes an element from the list based on the
 * compare function. the third arg is passed
 * to the compare function as the second parameter
 */
int list_remove_generic(struct list *list,
        int (*cmp)(const void *, const void *),
        const void *entry) {
    // empty list
    if (list->root == NULL) return LIST_NOT_FOUND;

    // replace root
    if (cmp(list->root->entry, entry)) {
        struct node *nxt = list->root->next;
        free(list->root);
        list->root = nxt;
        return 0;
    }
    struct node *prev = list->root;
    struct node *cur = list->root->next;
    while (cur != NULL) {
        if (cmp(cur->entry, entry)) {
            prev->next = cur->next;
            free(cur);
            return 0;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    return LIST_NOT_FOUND;
}

static int list_cmp_same_ref(const void *a, const void *b) {
    return a == b;
}

int list_remove(struct list *list, void *entry) {
    return list_remove_generic(list, list_cmp_same_ref, entry);
}

int list_clean(struct list *messages) {
    struct node *cur = messages->root;
    while (cur != NULL) {
        struct node *next = cur->next;
        free(cur);
        cur = next;
    }
    messages->root = NULL;
    return 0;
}

int list_contains(struct list *list, void *entry) {
    struct node *cur = list->root;
    for (; cur != NULL; cur = cur->next) {
        if (cur->entry == entry) return 1;
    }
    return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/list.h"


void test_list_len() {
    char a;
    struct list list;

    // delete in the middle
    list_init(&list);
    CU_ASSERT_EQUAL_FATAL(0, list_len(&list));

    list_add(&list, &a);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&list));

    list_add(&list, &a);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&list));

    list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, list_len(&list));
}

void test_list_empty() {
    char a;
    struct list list;

    // delete in the middle
    list_init(&list);
    list_add(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, list_empty(&list));

    list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(1, list_empty(&list));

    list_init(&list);
    CU_ASSERT_EQUAL_FATAL(1, list_empty(&list));
}

void test_add_remove_list() {
    char a,b,c,d;
    int ret;
    struct list list;

    // delete in the middle
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &b);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete last
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete first
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete all
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));
    ret = list_remove(&list, &b);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(1, list_len(&list));
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(0, list_len(&list));

    // delete two in the middle
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    list_add(&list, &d);
    ret = list_remove(&list, &b);
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &c));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &d));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // remove inexistent
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    ret = list_remove(&list, &d);
    CU_ASSERT_EQUAL_FATAL(LIST_NOT_FOUND, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_contains(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_contains(&list, &d));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));
} 

void test_list_clean() {
    int ret;

    char a, b;

    struct list list;
    list_init(&list);

    list_add(&list, &a);
    list_add(&list, &b);

    ret = list_clean(&list);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    CU_ASSERT_PTR_NULL_FATAL(list.root);

    list_destroy(&list);
}
void topic_add_list_suite() {
    CU_pSuite listSuite = CU_add_suite("list", NULL, NULL);
    CU_add_test(listSuite, "test_add_remove_list",
        test_add_remove_list);
    CU_add_test(listSuite, "test_list_empty",
        test_list_empty);
    CU_add_test(listSuite, "test_list_clean",
        test_list_clean);
}

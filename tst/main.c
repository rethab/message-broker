#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "util.c"
#include "stomp-test.c"
#include "topic-test.c"
#include "socket-test.c"
#include "broker-test.c"
#include "distributor-test.c"
#include "gc-test.c"
#include "list-test.c"

int main(int argc, char **argv) {
    install_segfault_handler();

    assert(CUE_SUCCESS == CU_initialize_registry());

    add_stomp_parse_suite();
    add_stomp_create_suite();
    topic_add_topic_suite();
    topic_add_list_suite();
    socket_test_suite();
    broker_test_suite();
    distributor_test_suite();
    gc_test_suite();

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}

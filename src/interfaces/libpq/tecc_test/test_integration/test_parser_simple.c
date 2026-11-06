/* Begin - ReqID:TECC4.0 */
/*
 * File: test_parser_simple.c
 * Abstract:
 *   Simple test program for TECC parser.
 *   Tests if parser can parse basic SQL statements.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/test_parser_simple.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include TECC parser header */
#include "../../frontend_parser/tecc_parser.h"

int main(void) {
    printf("=== TECC Parser Simple Test ===\n");

    const char *test_queries[] = {
        "SELECT 1",
        "SELECT 1;",
        "SELECT * FROM test",
        "CREATE TABLE test (id INT)",
        "CREATE KEK test_kek WITH (algorithm = 'SM4', key_length = 128)",
        NULL
    };

    for (int i = 0; test_queries[i] != NULL; i++) {
        printf("\n--- Test %d: %s ---\n", i+1, test_queries[i]);

        /* Call parser directly */
        List *parse_tree = tecc_parser_parse_list(test_queries[i]);

        if (parse_tree == NULL) {
            printf("FAIL: Parser returned NULL\n");
        } else {
            printf("PASS: Parser returned parse tree with %d elements\n",
                   list_length(parse_tree));

            /* Free the parse tree */
            /* Note: In real code, we would need to free the nodes properly */
        }
    }

    printf("\n=== Test Completed ===\n");
    return 0;
}
/* End - ReqID:TECC4.0 */
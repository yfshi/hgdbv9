#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define FRONTEND 1
#define FRONTEND_PARSER 1

#include "../../frontend_parser/fe_adapter.h"
#include "../../frontend_parser/tecc_nodes.h"

static int tests_passed = 0;
static int tests_failed = 0;
static int test_num = 0;

static void run_test(const char *sql, bool expect_success) {
    test_num++;
    List *parse_tree = tecc_parser_parse(sql);
    bool success = (parse_tree != NULL);

    if (success == expect_success) {
        printf("  OK [%d] %s\n", test_num, sql);
        tests_passed++;
    } else {
        printf("  FAIL [%d] %s\n", test_num, sql);
        printf("         Expected: %s, Got: %s\n",
               expect_success ? "success" : "failure",
               success ? "success" : "failure");
        tests_failed++;
    }
    if (parse_tree) {
        tecc_parser_free_tree(parse_tree);
    }
}

int main(void) {
    printf("=== INSERT ON CONFLICT Tests ===\n\n");

    /* 1. Basic INSERT (regression) */
    printf("-- Basic INSERT --\n");
    run_test("INSERT INTO t VALUES (1);", true);
    run_test("INSERT INTO t (id) VALUES (1);", true);
    run_test("INSERT INTO t VALUES (1, 'a'), (2, 'b');", true);

    /* 2. ON CONFLICT DO NOTHING */
    printf("\n-- ON CONFLICT DO NOTHING --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT DO NOTHING;", true);
    run_test("INSERT INTO t (id) VALUES (1) ON CONFLICT DO NOTHING;", true);

    /* 3. ON CONFLICT (columns) DO NOTHING */
    printf("\n-- ON CONFLICT (columns) DO NOTHING --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT (id) DO NOTHING;", true);
    run_test("INSERT INTO t VALUES (1) ON CONFLICT (id, name) DO NOTHING;", true);

    /* 4. ON CONFLICT (columns) WHERE DO NOTHING */
    printf("\n-- ON CONFLICT (columns) WHERE DO NOTHING --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT (id) WHERE id > 0 DO NOTHING;", true);

    /* 5. ON CONFLICT ON CONSTRAINT DO NOTHING */
    printf("\n-- ON CONFLICT ON CONSTRAINT DO NOTHING --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT ON CONSTRAINT t_pkey DO NOTHING;", true);

    /* 6. ON CONFLICT DO UPDATE SET */
    printf("\n-- ON CONFLICT DO UPDATE SET --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT DO UPDATE SET name = 'test';", true);
    run_test("INSERT INTO t VALUES (1) ON CONFLICT DO UPDATE SET name = 'test', age = 10;", true);

    /* 7. ON CONFLICT DO UPDATE SET WHERE */
    printf("\n-- ON CONFLICT DO UPDATE SET WHERE --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT DO UPDATE SET name = 'test' WHERE t.id > 0;", true);

    /* 8. ON CONFLICT (columns) DO UPDATE SET */
    printf("\n-- ON CONFLICT (columns) DO UPDATE SET --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT (id) DO UPDATE SET name = 'test';", true);
    run_test("INSERT INTO t VALUES (1) ON CONFLICT (id) DO UPDATE SET name = 'test' WHERE t.id > 0;", true);

    /* 9. ON CONFLICT ON CONSTRAINT DO UPDATE SET */
    printf("\n-- ON CONFLICT ON CONSTRAINT DO UPDATE SET --\n");
    run_test("INSERT INTO t VALUES (1) ON CONFLICT ON CONSTRAINT t_pkey DO UPDATE SET name = 'test';", true);

    /* 10. INSERT ... SELECT with ON CONFLICT */
    printf("\n-- INSERT ... SELECT with ON CONFLICT --\n");
    run_test("INSERT INTO t SELECT * FROM t2 ON CONFLICT DO NOTHING;", true);

    printf("\n=== Summary ===\n");
    printf("Total: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}

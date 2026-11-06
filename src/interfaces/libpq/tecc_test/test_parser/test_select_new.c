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
    printf("=== SELECT Enhancement Tests ===\n\n");

    /* Basic SELECT (baseline) */
    printf("-- Basic SELECT --\n");
    run_test("SELECT * FROM t1;", true);
    run_test("SELECT id, name FROM t1;", true);
    run_test("SELECT a AS x FROM t1;", true);
    run_test("SELECT DISTINCT id FROM t1;", true);
    run_test("SELECT * FROM t1 WHERE id > 10;", true);

    /* 1. Table aliases */
    printf("\n-- Table aliases --\n");
    run_test("SELECT * FROM t1 AS a;", true);
    run_test("SELECT * FROM t1 a;", true);
    run_test("SELECT a.id FROM t1 AS a;", true);
    run_test("SELECT a.id, b.name FROM t1 a, t2 b;", true);

    /* 2. JOIN */
    printf("\n-- JOIN --\n");
    run_test("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 LEFT JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 RIGHT JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 FULL JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 LEFT OUTER JOIN t2 ON t1.id = t2.id;", true);
    run_test("SELECT * FROM t1 CROSS JOIN t2;", true);

    /* 3. UNION / INTERSECT / EXCEPT */
    printf("\n-- UNION / INTERSECT / EXCEPT --\n");
    run_test("SELECT id FROM t1 UNION SELECT id FROM t2;", true);
    run_test("SELECT id FROM t1 UNION ALL SELECT id FROM t2;", true);
    run_test("SELECT id FROM t1 INTERSECT SELECT id FROM t2;", true);
    run_test("SELECT id FROM t1 EXCEPT SELECT id FROM t2;", true);

    /* 4. Subquery in FROM */
    printf("\n-- Subquery in FROM --\n");
    run_test("SELECT * FROM (SELECT id FROM t1) AS sub;", true);
    run_test("SELECT * FROM (SELECT id FROM t1) sub;", true);

    /* 5. Type cast (::) */
    printf("\n-- Type cast :: --\n");
    run_test("SELECT id::text FROM t1;", true);
    run_test("SELECT id::varchar(100) FROM t1;", true);

    /* 6. CASE expression */
    printf("\n-- CASE expression --\n");
    run_test("SELECT CASE WHEN id > 0 THEN 'pos' ELSE 'neg' END FROM t1;", true);
    run_test("SELECT CASE id WHEN 1 THEN 'one' WHEN 2 THEN 'two' END FROM t1;", true);
    run_test("SELECT CASE WHEN id > 0 THEN 'pos' END FROM t1;", true);

    /* 7. Combined */
    printf("\n-- Combined --\n");
    run_test("SELECT a.id, b.name FROM t1 AS a JOIN t2 AS b ON a.id = b.id WHERE a.id > 10;", true);
    run_test("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id UNION SELECT * FROM t1;", true);
    run_test("SELECT x::text AS val FROM (SELECT id FROM t1) AS sub;", true);

    /* 8. Verify existing still works */
    printf("\n-- Regression: DDL --\n");
    run_test("CREATE TABLE users (id int, name text);", true);

    printf("\n=== Summary ===\n");
    printf("Total: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}

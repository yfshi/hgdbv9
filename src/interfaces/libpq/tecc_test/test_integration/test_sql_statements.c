/* Begin - ReqID:TECC4.0 */
/*
 * File: test_sql_statements.c
 * Abstract:
 *   SQL statement test cases for TECC integration testing.
 *   Provides predefined test SQL statements and test case generation.
 *
 * Authored by <wangxp@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/test_sql_statements.c
 */

#include "include/test_integration.h"

/* Memory allocation functions - defined in test_integration_main.c */
extern void *test_palloc(size_t size);
extern void *test_palloc0(size_t size);
extern void test_pfree(void *ptr);
extern char *test_pstrdup(const char *str);

/* Test column information structure */
typedef struct {
    const char *table_name;
    const char *column_name;
    bool is_encrypted;
    const char *data_type;
} TestColumnInfo;

/* Test SQL statement structure */
typedef struct {
    const char *sql;
    const char *description;
    TestColumnInfo *encrypted_columns;
    int encrypted_column_count;
} TestSQLStatement;

/* Test table structure for users table */
static TestColumnInfo users_table_columns[] = {
    {"users", "id", false, "INT"},
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "email", false, "VARCHAR(100)"},
    {"users", "age_enc", true, "INT4_ENC"},
    {"users", "salary_enc", true, "NUMERIC_ENC(10,2)"}
};

static const int users_table_column_count = 5;

/* Static arrays for encrypted columns */
static TestColumnInfo insert_enc_cols_1[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"}
};

static TestColumnInfo insert_enc_cols_2[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"}
};

static TestColumnInfo insert_enc_cols_3[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"},
    {"users", "salary_enc", true, "NUMERIC_ENC(10,2)"}
};

/* Phase 1: Simple INSERT statements */
static TestSQLStatement phase1_insert_statements[] = {
    {
        "INSERT INTO users (id, name_enc, email) VALUES (1, 'Alice', 'alice@example.com');",
        "Simple INSERT with one encrypted column (name_enc)",
        insert_enc_cols_1, 1  /* name_enc is encrypted */
    },
    {
        "INSERT INTO users (id, name_enc, email, age_enc) VALUES (2, 'Bob', 'bob@example.com', 30);",
        "INSERT with multiple encrypted columns",
        insert_enc_cols_2, 2
    },
    {
        "INSERT INTO users VALUES (3, 'Charlie', 'charlie@example.com', 25, 50000.00);",
        "INSERT without column list (all columns)",
        insert_enc_cols_3, 3
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Static arrays for SELECT encrypted columns */
static TestColumnInfo select_enc_cols_1[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"}
};

static TestColumnInfo select_enc_cols_2[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"},
    {"users", "salary_enc", true, "NUMERIC_ENC(10,2)"}
};

/* Phase 2: SELECT statements */
static TestSQLStatement phase2_select_statements[] = {
    {
        "SELECT id, name_enc, email FROM users;",
        "SELECT with encrypted column in projection",
        select_enc_cols_1, 1
    },
    {
        "SELECT * FROM users;",
        "SELECT all columns (including encrypted)",
        select_enc_cols_2, 3
    },
    {
        "SELECT name_enc FROM users WHERE id = 1;",
        "SELECT encrypted column with WHERE condition",
        select_enc_cols_1, 1
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Static arrays for WHERE encrypted columns */
static TestColumnInfo where_enc_cols_1[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"}
};

static TestColumnInfo where_enc_cols_2[] = {
    {"users", "age_enc", true, "INT4_ENC"}
};

static TestColumnInfo where_enc_cols_3[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"}
};

/* Phase 3: WHERE clause conditions */
static TestSQLStatement phase3_where_statements[] = {
    {
        "SELECT * FROM users WHERE name_enc = 'Alice';",
        "WHERE condition with encrypted column equality",
        where_enc_cols_1, 1
    },
    {
        "SELECT * FROM users WHERE age_enc > 25;",
        "WHERE condition with encrypted column comparison",
        where_enc_cols_2, 1
    },
    {
        "SELECT * FROM users WHERE name_enc = 'Alice' AND age_enc < 30;",
        "WHERE with multiple encrypted column conditions",
        where_enc_cols_3, 2
    },
    {
        "SELECT * FROM users WHERE email = 'alice@example.com' OR name_enc = 'Bob';",
        "WHERE with mixed encrypted and plain columns",
        where_enc_cols_1, 1
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Static arrays for UPDATE encrypted columns */
static TestColumnInfo update_enc_cols_1[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"}
};

static TestColumnInfo update_enc_cols_2[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"},
    {"users", "salary_enc", true, "NUMERIC_ENC(10,2)"}
};

static TestColumnInfo update_enc_cols_3[] = {
    {"users", "age_enc", true, "INT4_ENC"}
};

/* Phase 4: UPDATE statements */
static TestSQLStatement phase4_update_statements[] = {
    {
        "UPDATE users SET name_enc = 'Alice Smith' WHERE id = 1;",
        "UPDATE encrypted column with WHERE condition",
        update_enc_cols_1, 1
    },
    {
        "UPDATE users SET age_enc = 31, salary_enc = 55000.00 WHERE name_enc = 'Bob';",
        "UPDATE multiple encrypted columns",
        update_enc_cols_2, 3
    },
    {
        "UPDATE users SET email = 'new@example.com' WHERE age_enc > 25;",
        "UPDATE plain column with WHERE on encrypted column",
        update_enc_cols_3, 1
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Static arrays for complex SQL encrypted columns */
static TestColumnInfo complex_enc_cols_1[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"}
};

static TestColumnInfo complex_enc_cols_2[] = {
    {"users", "name_enc", true, "VARCHAR_ENC(100)"},
    {"users", "age_enc", true, "INT4_ENC"},
    {"users", "salary_enc", true, "NUMERIC_ENC(10,2)"}
};

/* Phase 5: Complex SQL statements */
static TestSQLStatement phase5_complex_statements[] = {
    {
        "SELECT u1.name_enc, u2.email FROM users u1 JOIN users u2 ON u1.id = u2.id WHERE u1.age_enc > 25;",
        "JOIN query with encrypted columns",
        complex_enc_cols_1, 2
    },
    {
        "SELECT name_enc, (SELECT COUNT(*) FROM users WHERE age_enc > u.age_enc) AS older_count FROM users u;",
        "Subquery with encrypted column",
        complex_enc_cols_1, 2
    },
    {
        "SELECT AVG(age_enc) AS avg_age, MAX(salary_enc) AS max_salary FROM users GROUP BY name_enc;",
        "Aggregate functions on encrypted columns",
        complex_enc_cols_2, 3
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Static arrays for TECC-specific encrypted columns */
static TestColumnInfo tecc_enc_cols[] = {
    {"encrypted_users", "name", true, "VARCHAR_ENC(100)"},
    {"encrypted_users", "age", true, "INT4_ENC"}
};

/* TECC-specific statements */
static TestSQLStatement tecc_specific_statements[] = {
    {
        "CREATE KEK master_key;",
        "Create Key Encryption Key",
        NULL, 0
    },
    {
        "CREATE DEK data_key;",
        "Create Data Encryption Key",
        NULL, 0
    },
    {
        "DROP DEK data_key;",
        "Drop Data Encryption Key",
        NULL, 0
    },
    {
        "CREATE TABLE encrypted_users (id INT, name VARCHAR_ENC(100), age INT4_ENC);",
        "Create table with encrypted columns",
        tecc_enc_cols, 2
    },
    {NULL, NULL, NULL, 0}  /* Sentinel */
};

/* Function to get test SQL statements by phase */
TestSQLStatement *test_get_sql_statements_by_phase(int phase)
{
    switch (phase) {
        case 1:
            return phase1_insert_statements;
        case 2:
            return phase2_select_statements;
        case 3:
            return phase3_where_statements;
        case 4:
            return phase4_update_statements;
        case 5:
            return phase5_complex_statements;
        case 6:
            return tecc_specific_statements;
        default:
            return NULL;
    }
}

/* Function to create test suite from SQL statements */
TestSuite *test_create_suite_from_sql_statements(const char *suite_name, TestSQLStatement *statements)
{
    if (!suite_name || !statements) {
        test_log(LOG_LEVEL_ERROR, "SQL", "test_create_suite_from_sql_statements",
                 "Invalid parameters");
        return NULL;
    }

    TestSuite *suite = test_suite_create(suite_name);
    if (!suite) {
        test_log(LOG_LEVEL_ERROR, "SQL", "test_create_suite_from_sql_statements",
                 "Failed to create test suite");
        return NULL;
    }

    /* Add each SQL statement as a test case */
    int i;
    for (i = 0; statements[i].sql != NULL; i++) {
        char test_name[256];
        snprintf(test_name, sizeof(test_name), "SQL Test %d: %s", i + 1, statements[i].description);

        TestCase *test_case = test_case_create(test_name, statements[i].description, statements[i].sql);
        if (test_case) {
            test_suite_add_case(suite, test_case);
            test_log(LOG_LEVEL_DEBUG, "SQL", "test_create_suite_from_sql_statements",
                     "Added test case: %s", test_name);
        } else {
            test_log(LOG_LEVEL_WARNING, "SQL", "test_create_suite_from_sql_statements",
                     "Failed to create test case for: %s", statements[i].description);
        }
    }

    test_log(LOG_LEVEL_INFO, "SQL", "test_create_suite_from_sql_statements",
             "Created test suite '%s' with %d test cases", suite_name, suite->test_case_count);
    return suite;
}

/* Function to create phase-based test suites */
TestSuite **test_create_phase_test_suites(int *suite_count)
{
    const int num_phases = 6;
    TestSuite **suites = (TestSuite **)test_palloc0(sizeof(TestSuite *) * num_phases);
    if (!suites) {
        *suite_count = 0;
        return NULL;
    }

    const char *phase_names[] = {
        "Phase 1: Simple INSERT Statements",
        "Phase 2: SELECT Statements",
        "Phase 3: WHERE Clause Conditions",
        "Phase 4: UPDATE Statements",
        "Phase 5: Complex SQL Statements",
        "Phase 6: TECC-Specific Statements"
    };

    int created_count = 0;
    int phase;
    for (phase = 1; phase <= num_phases; phase++) {
        TestSQLStatement *statements = test_get_sql_statements_by_phase(phase);
        if (statements && statements[0].sql != NULL) {
            suites[created_count] = test_create_suite_from_sql_statements(phase_names[phase-1], statements);
            if (suites[created_count]) {
                created_count++;
            }
        }
    }

    *suite_count = created_count;
    return suites;
}

/* Function to free phase test suites */
void test_free_phase_test_suites(TestSuite **suites, int suite_count)
{
    if (!suites)
        return;

    int i;
    for (i = 0; i < suite_count; i++) {
        if (suites[i]) {
            test_suite_free(suites[i]);
        }
    }
    test_pfree(suites);
}

/* Function to execute all phase test suites */
void test_execute_all_phases(PGconn *conn)
{
    test_log(LOG_LEVEL_INFO, "SQL", "test_execute_all_phases",
             "Executing all test phases");

    int suite_count = 0;
    TestSuite **suites = test_create_phase_test_suites(&suite_count);

    if (!suites || suite_count == 0) {
        test_log(LOG_LEVEL_ERROR, "SQL", "test_execute_all_phases",
                 "Failed to create test suites");
        return;
    }

    test_log(LOG_LEVEL_INFO, "SQL", "test_execute_all_phases",
             "Created %d test suites", suite_count);

    /* Execute each test suite */
    int i;
    for (i = 0; i < suite_count; i++) {
        if (suites[i]) {
            test_suite_execute(suites[i], conn);
        }
    }

    /* Free test suites */
    test_free_phase_test_suites(suites, suite_count);

    test_log(LOG_LEVEL_INFO, "SQL", "test_execute_all_phases",
             "All test phases completed");
}

/* Function to validate encrypted columns in SQL statement */
bool test_validate_sql_encrypted_columns(const char *sql, TestColumnInfo *expected_columns, int expected_count)
{
    if (!sql) {
        test_log(LOG_LEVEL_ERROR, "SQL", "test_validate_sql_encrypted_columns",
                 "SQL statement is NULL");
        return false;
    }

    test_log(LOG_LEVEL_DEBUG, "SQL", "test_validate_sql_encrypted_columns",
             "Validating encrypted columns in SQL: %s", sql);

    /* TODO: Implement actual validation using parser */
    /* For now, just log the expected columns */
    if (expected_columns && expected_count > 0) {
        test_log(LOG_LEVEL_INFO, "SQL", "test_validate_sql_encrypted_columns",
                 "Expected %d encrypted column(s):", expected_count);
        int i;
        for (i = 0; i < expected_count; i++) {
            test_log(LOG_LEVEL_INFO, "SQL", "test_validate_sql_encrypted_columns",
                     "  - %s.%s (%s)",
                     expected_columns[i].table_name,
                     expected_columns[i].column_name,
                     expected_columns[i].data_type);
        }
    } else {
        test_log(LOG_LEVEL_INFO, "SQL", "test_validate_sql_encrypted_columns",
                 "No encrypted columns expected");
    }

    return true;
}

/* Function to print SQL statement information */
void test_print_sql_statement_info(const TestSQLStatement *stmt)
{
    if (!stmt || !stmt->sql) {
        test_log(LOG_LEVEL_ERROR, "SQL", "test_print_sql_statement_info",
                 "Invalid SQL statement");
        return;
    }

    test_print_separator();
    test_log(LOG_LEVEL_INFO, "SQL", "test_print_sql_statement_info",
             "SQL Statement: %s", stmt->description);
    test_log(LOG_LEVEL_INFO, "SQL", "test_print_sql_statement_info",
             "SQL: %s", stmt->sql);

    if (stmt->encrypted_columns && stmt->encrypted_column_count > 0) {
        test_log(LOG_LEVEL_INFO, "SQL", "test_print_sql_statement_info",
                 "Encrypted columns: %d", stmt->encrypted_column_count);
        int i;
        for (i = 0; i < stmt->encrypted_column_count; i++) {
            test_log(LOG_LEVEL_INFO, "SQL", "test_print_sql_statement_info",
                     "  - %s.%s",
                     stmt->encrypted_columns[i].table_name,
                     stmt->encrypted_columns[i].column_name);
        }
    } else {
        test_log(LOG_LEVEL_INFO, "SQL", "test_print_sql_statement_info",
                 "No encrypted columns");
    }
    test_print_separator();
}

/* Function to get test table creation SQL */
const char *test_get_table_creation_sql(void)
{
    static const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INT PRIMARY KEY,"
        "  name_enc VARCHAR_ENC(100),"
        "  email VARCHAR(100),"
        "  age_enc INT4_ENC,"
        "  salary_enc NUMERIC_ENC(10,2)"
        ");";

    return create_table_sql;
}

/* Function to get test data insertion SQL */
const char **test_get_test_data_insertion_sql(int *count)
{
    static const char *insert_sqls[] = {
        "INSERT INTO users (id, name_enc, email, age_enc, salary_enc) VALUES (1, 'Alice', 'alice@example.com', 25, 50000.00);",
        "INSERT INTO users (id, name_enc, email, age_enc, salary_enc) VALUES (2, 'Bob', 'bob@example.com', 30, 60000.00);",
        "INSERT INTO users (id, name_enc, email, age_enc, salary_enc) VALUES (3, 'Charlie', 'charlie@example.com', 35, 70000.00);",
        "INSERT INTO users (id, name_enc, email, age_enc, salary_enc) VALUES (4, 'David', 'david@example.com', 28, 55000.00);",
        "INSERT INTO users (id, name_enc, email, age_enc, salary_enc) VALUES (5, 'Eve', 'eve@example.com', 32, 65000.00);",
        NULL
    };

    if (count) {
        *count = 0;
        while (insert_sqls[*count] != NULL) {
            (*count)++;
        }
    }

    return insert_sqls;
}
/* End - ReqID:TECC4.0 */
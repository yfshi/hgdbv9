# Test transaction commit on session exit

setup {
    CREATE TABLE session_exit_test (
        id SERIAL PRIMARY KEY,
        data TEXT NOT NULL
    );
}

teardown {
    DROP TABLE session_exit_test;
}

session "s1"
setup {
    SET client_min_messages = 'ERROR';
    SET ivorysql.ora_commit_before_disconnect = on;
    BEGIN;
}

step s1_insert {
    INSERT INTO session_exit_test (data) VALUES ('test data 1');
    INSERT INTO session_exit_test (data) VALUES ('test data 2');
}

step s1_exit {
    SELECT pg_terminate_backend(pg_backend_pid());
}

session "s2"

step s2_advisory {
    SELECT pg_advisory_lock('session_exit_test'::regclass::int8);
}

step s2_verify {
    SELECT COUNT(*) FROM session_exit_test;
    SELECT * FROM session_exit_test ORDER BY id;
}

# Use advisory lock to ensure s1 completes exit before s2 verifies
permutation
    s2_advisory
    s1_insert
    s1_exit
    s2_verify
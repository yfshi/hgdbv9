CREATE TABLE insert_ignore_test (
    id INT PRIMARY KEY,
    name VARCHAR(50),
    email VARCHAR(100) UNIQUE
);

INSERT INTO insert_ignore_test (id, name, email) VALUES (1, 'Alice', 'alice@example.com');

INSERT IGNORE INTO insert_ignore_test (id, name, email) VALUES (1, 'Bob', 'bob@example.com');

select * from insert_ignore_test;
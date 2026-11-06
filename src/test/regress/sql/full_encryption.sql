-- Full encryption test
CREATE TABLE encrypted_table (
    id SERIAL PRIMARY KEY,
    secret TEXT ENCRYPTED KEY 'mykey'
);

INSERT INTO encrypted_table (secret) VALUES ('sensitive data');
SELECT secret FROM encrypted_table;
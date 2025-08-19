set enable_seqscan=off;

CREATE TABLE test_bit (
	i pg_catalog.bit(3)
);

INSERT INTO test_bit VALUES ('001'),('010'),('011'),('100'),('101'),('110');

CREATE INDEX idx_bit ON test_bit USING gin (i);

SELECT * FROM test_bit WHERE i<'100'::pg_catalog.bit(3) ORDER BY i;
SELECT * FROM test_bit WHERE i<='100'::pg_catalog.bit(3) ORDER BY i;
SELECT * FROM test_bit WHERE i='100'::pg_catalog.bit(3) ORDER BY i;
SELECT * FROM test_bit WHERE i>='100'::pg_catalog.bit(3) ORDER BY i;
SELECT * FROM test_bit WHERE i>'100'::pg_catalog.bit(3) ORDER BY i;

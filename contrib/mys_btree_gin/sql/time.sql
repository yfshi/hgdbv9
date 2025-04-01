-- time check

SET NLS_TIMESTAMP_FORMAT='YYYY-MM-DD HH24:MI:SS.FF9';

CREATE TABLE timetest(a time);

\copy timetest from 'data/time.data'

SET enable_seqscan=on;

SELECT count(*) FROM timetest WHERE a <  '18:11:11';

SELECT count(*) FROM timetest WHERE a <= '18:11:11';

SELECT count(*) FROM timetest WHERE a  = '18:11:11';

SELECT count(*) FROM timetest WHERE a >= '18:11:11';

SELECT count(*) FROM timetest WHERE a >  '18:11:11';

CREATE INDEX timeidx on timetest USING gin(a);

SET enable_seqscan=off;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM timetest WHERE a <  '18:11:11'::time;
SELECT count(*) FROM timetest WHERE a <  '18:11:11'::time;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM timetest WHERE a <= '18:11:11'::time;
SELECT count(*) FROM timetest WHERE a <= '18:11:11'::time;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM timetest WHERE a  = '18:11:11'::time;
SELECT count(*) FROM timetest WHERE a  = '18:11:11'::time;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM timetest WHERE a >= '18:11:11'::time;
SELECT count(*) FROM timetest WHERE a >= '18:11:11'::time;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM timetest WHERE a >  '18:11:11'::time;
SELECT count(*) FROM timetest WHERE a >  '18:11:11'::time;

CREATE TABLE test_time (
	i time
);
INSERT INTO test_time VALUES
	( '03:55:08' ),
	( '04:55:08' ),
	( '05:55:08' ),
	( '08:55:08' ),
	( '09:55:08' ),
	( '10:55:08' )
;
CREATE INDEX idx_time ON test_time USING gin (i);

drop table test_time;
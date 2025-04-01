-- datetime check

SET NLS_TIMESTAMP_FORMAT='YYYY-MM-DD HH24:MI:SS.FF9';

CREATE TABLE datetimetmp (a datetime);

\copy datetimetmp from 'data/datetime.data'

SET enable_seqscan=on;

SELECT count(*) FROM datetimetmp WHERE a <  '2004-10-26 08:55:08';

SELECT count(*) FROM datetimetmp WHERE a <= '2004-10-26 08:55:08';

SELECT count(*) FROM datetimetmp WHERE a  = '2004-10-26 08:55:08';

SELECT count(*) FROM datetimetmp WHERE a >= '2004-10-26 08:55:08';

SELECT count(*) FROM datetimetmp WHERE a >  '2004-10-26 08:55:08';

SELECT count(*) FROM datetimetmp WHERE a <> '2004-10-26 08:55:08';

SELECT a, a <-> '2004-10-26 08:55:08' FROM datetimetmp ORDER BY a <-> '2004-10-26 08:55:08' LIMIT 3;

CREATE INDEX mysdatetimeidx ON datetimetmp USING gist ( a );

SET enable_seqscan=off;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a <  '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a <  '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a <= '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a <= '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a  = '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a  = '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a >= '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a >= '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a >  '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a >  '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM datetimetmp WHERE a <> '2004-10-26 08:55:08'::datetime;
SELECT count(*) FROM datetimetmp WHERE a <> '2004-10-26 08:55:08'::datetime;

EXPLAIN (COSTS OFF)
SELECT a, a <-> '2004-10-26 08:55:08' FROM datetimetmp ORDER BY a <-> '2004-10-26 08:55:08' LIMIT 3;
SELECT a, a <-> '2004-10-26 08:55:08' FROM datetimetmp ORDER BY a <-> '2004-10-26 08:55:08' LIMIT 3;

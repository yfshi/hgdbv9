-- time check

CREATE TABLE timetmp (a pg_catalog.time);

\copy timetmp from 'data/time.data'

SET enable_seqscan=on;

SELECT count(*) FROM timetmp WHERE a <  '10:57:11';

SELECT count(*) FROM timetmp WHERE a <= '10:57:11';

SELECT count(*) FROM timetmp WHERE a  = '10:57:11';

SELECT count(*) FROM timetmp WHERE a >= '10:57:11';

SELECT count(*) FROM timetmp WHERE a >  '10:57:11';

SELECT a, a <-> '10:57:11' FROM timetmp ORDER BY a <-> '10:57:11' LIMIT 3;

CREATE INDEX timeidx ON timetmp USING gist ( a );

SET enable_seqscan=off;

SELECT count(*) FROM timetmp WHERE a <  '10:57:11'::pg_catalog.time;

SELECT count(*) FROM timetmp WHERE a <= '10:57:11'::pg_catalog.time;

SELECT count(*) FROM timetmp WHERE a  = '10:57:11'::pg_catalog.time;

SELECT count(*) FROM timetmp WHERE a >= '10:57:11'::pg_catalog.time;

SELECT count(*) FROM timetmp WHERE a >  '10:57:11'::pg_catalog.time;

EXPLAIN (COSTS OFF)
SELECT a, a <-> '10:57:11' FROM timetmp ORDER BY a <-> '10:57:11' LIMIT 3;
SELECT a, a <-> '10:57:11' FROM timetmp ORDER BY a <-> '10:57:11' LIMIT 3;

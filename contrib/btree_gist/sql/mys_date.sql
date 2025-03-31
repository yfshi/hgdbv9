-- date check

CREATE TABLE datetmp (a pg_catalog.date);

\copy datetmp from 'data/date.data'

SET enable_seqscan=on;

SELECT count(*) FROM datetmp WHERE a <  '2001-02-13';

SELECT count(*) FROM datetmp WHERE a <= '2001-02-13';

SELECT count(*) FROM datetmp WHERE a  = '2001-02-13';

SELECT count(*) FROM datetmp WHERE a >= '2001-02-13';

SELECT count(*) FROM datetmp WHERE a >  '2001-02-13';

SELECT a, a <-> '2001-02-13' FROM datetmp ORDER BY a <-> '2001-02-13' LIMIT 3;

CREATE INDEX dateidx ON datetmp USING gist ( a );

SET enable_seqscan=off;

SELECT count(*) FROM datetmp WHERE a <  '2001-02-13'::pg_catalog.date;

SELECT count(*) FROM datetmp WHERE a <= '2001-02-13'::pg_catalog.date;

SELECT count(*) FROM datetmp WHERE a  = '2001-02-13'::pg_catalog.date;

SELECT count(*) FROM datetmp WHERE a >= '2001-02-13'::pg_catalog.date;

SELECT count(*) FROM datetmp WHERE a >  '2001-02-13'::pg_catalog.date;

EXPLAIN (COSTS OFF)
SELECT a, a <-> '2001-02-13' FROM datetmp ORDER BY a <-> '2001-02-13' LIMIT 3;
SELECT a, a <-> '2001-02-13' FROM datetmp ORDER BY a <-> '2001-02-13' LIMIT 3;

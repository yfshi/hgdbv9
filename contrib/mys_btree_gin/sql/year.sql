-- year check

SELECT set_config('search_path', 'ivymy, '||current_setting('search_path'), false);

CREATE TABLE yeartest(y YEAR);

\copy yeartest from 'data/year.data'

SET enable_seqscan=on;

SELECT count(*) FROM yeartest WHERE y <  '2016';

SELECT count(*) FROM yeartest WHERE y <= '2016';

SELECT count(*) FROM yeartest WHERE y  = '2016';

SELECT count(*) FROM yeartest WHERE y >= '2016';

SELECT count(*) FROM yeartest WHERE y >  '2016';

CREATE INDEX yearidx on yeartest USING gin(y);

SET enable_seqscan=off;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM yeartest WHERE y <  '2016';
SELECT count(*) FROM yeartest WHERE y <  '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM yeartest WHERE y <= '2016';
SELECT count(*) FROM yeartest WHERE y <= '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM yeartest WHERE y  = '2016';
SELECT count(*) FROM yeartest WHERE y  = '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM yeartest WHERE y >= '2016';
SELECT count(*) FROM yeartest WHERE y >= '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM yeartest WHERE y >  '2016';
SELECT count(*) FROM yeartest WHERE y >  '2016';

SELECT set_config('search_path', trim(leading 'ivymy, ' from current_setting('search_path')), false);

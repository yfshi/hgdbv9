-- year check

SELECT set_config('search_path', 'ivymy, '||current_setting('search_path'), false);

CREATE TABLE year_gist_check(y YEAR);

\copy year_gist_check from 'data/year.data'

SET enable_seqscan=on;

SELECT count(*) FROM year_gist_check WHERE y <  '2016';

SELECT count(*) FROM year_gist_check WHERE y <= '2016';

SELECT count(*) FROM year_gist_check WHERE y  = '2016';

SELECT count(*) FROM year_gist_check WHERE y >= '2016';

SELECT count(*) FROM year_gist_check WHERE y >  '2016';

SELECT count(*) FROM year_gist_check WHERE y <> '2016';

CREATE INDEX yearidx on year_gist_check USING gist(y);

SET enable_seqscan=off;

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y <  '2016';
SELECT count(*) FROM year_gist_check WHERE y <  '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y <= '2016';
SELECT count(*) FROM year_gist_check WHERE y <= '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y  = '2016';
SELECT count(*) FROM year_gist_check WHERE y  = '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y >= '2016';
SELECT count(*) FROM year_gist_check WHERE y >= '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y >  '2016';
SELECT count(*) FROM year_gist_check WHERE y >  '2016';

EXPLAIN (COSTS OFF)
SELECT count(*) FROM year_gist_check WHERE y <> '2016';
SELECT count(*) FROM year_gist_check WHERE y <> '2016';

SELECT set_config('search_path', trim(leading 'ivymy, ' from current_setting('search_path')), false);
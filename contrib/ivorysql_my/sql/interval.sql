--
-- The grammar rewrites DATE_ADD(d, INTERVAL v unit) into dateadd(d, <interval>)
-- and DATE_SUB(d, INTERVAL v unit) into dateadd(d, -<interval>).  We first
-- create a dateadd(timestamp, interval) stub (= d + interval) so the cases run
-- end to end and the results can be observed.
--

\set ON_ERROR_STOP 0

--------------------------------------------------------------------------------
-- 1. Single units (scalar value) — expected result in the trailing comment
--------------------------------------------------------------------------------
SELECT DATE_ADD(TIMESTAMP '2008-12-31 23:59:59', INTERVAL 1 SECOND);        -- 2009-01-01 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 MINUTE);        -- 2008-01-02 00:01:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 HOUR);          -- 2008-01-02 01:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 DAY);           -- 2008-01-03 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 WEEK);          -- 2008-01-09 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 MONTH);         -- 2008-02-02 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 QUARTER);       -- 2008-04-02 00:00:00  (+3 months)
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 YEAR);          -- 2009-01-02 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 100000 MICROSECOND); -- 2008-01-02 00:00:00.1

--------------------------------------------------------------------------------
-- 2. Value as an expression / negative number
--------------------------------------------------------------------------------
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1+1 DAY);         -- 2008-01-04 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 2*3 HOUR);        -- 2008-01-02 06:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-02 00:00:00', INTERVAL -1 DAY);          -- 2008-01-01 00:00:00

--------------------------------------------------------------------------------
-- 3. Composite units (delimited-string value mapped to a SQL field range)
--------------------------------------------------------------------------------
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1:1' MINUTE_SECOND);       -- 2008-01-01 00:01:01
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '2:3' HOUR_MINUTE);         -- 2008-01-01 02:03:00
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '2:3:4' HOUR_SECOND);       -- 2008-01-01 02:03:04
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1 2' DAY_HOUR);            -- 2008-01-02 02:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1 2:3' DAY_MINUTE);        -- 2008-01-02 02:03:00
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1 1:1:1' DAY_SECOND);      -- 2008-01-02 01:01:01
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1-6' YEAR_MONTH);          -- 2009-07-01 00:00:00
SELECT DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1.5' SECOND_MICROSECOND);  -- 2008-01-01 00:00:01.5

--------------------------------------------------------------------------------
-- 4. DATE_SUB — mirror of DATE_ADD, with the interval negated
--------------------------------------------------------------------------------
SELECT DATE_SUB(TIMESTAMP '2009-01-01 00:00:00', INTERVAL 1 DAY);                     -- 2008-12-31 00:00:00
SELECT DATE_SUB(TIMESTAMP '2008-01-02 00:00:00', INTERVAL 1 QUARTER);                 -- 2007-10-02 00:00:00
SELECT DATE_SUB(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1:1' MINUTE_SECOND);       -- 2007-12-31 23:58:59
SELECT DATE_SUB(TIMESTAMP '2008-01-01 00:00:00', INTERVAL '1 1:1:1' DAY_SECOND);      -- 2007-12-30 22:58:59

--------------------------------------------------------------------------------
-- 5. Argument as a string literal / DATE (implicitly cast to timestamp)
--------------------------------------------------------------------------------
SELECT DATE_ADD('2008-01-02', INTERVAL 1 DAY);                                        -- 2008-01-03 00:00:00
SELECT DATE_ADD(DATE '2008-01-02', INTERVAL 1 MONTH);                                 -- 2008-02-02 00:00:00

--------------------------------------------------------------------------------
-- 6. Nested / used in ordinary expression positions such as WHERE
--------------------------------------------------------------------------------
SELECT DATE_ADD(DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL 1 DAY),
                INTERVAL 1 HOUR);                                                      -- 2008-01-02 01:00:00
SELECT 1 WHERE DATE_ADD(TIMESTAMP '2008-01-01 00:00:00', INTERVAL 1 DAY)
             > TIMESTAMP '2008-01-01 00:00:00';                                        -- 1

--------------------------------------------------------------------------------
-- 8. Invalid unit — expect a parse-time syntax error (BANANA is not a unit)
--------------------------------------------------------------------------------
SELECT DATE_ADD(now(), INTERVAL 1 BANANA);   -- ERROR: syntax error at or near "BANANA"

--------------------------------------------------------------------------------
-- 9. Regression guard: new unit keywords still work as column names/aliases,
--    and EXTRACT still accepts these units
--------------------------------------------------------------------------------
SELECT 1 AS week, 2 AS quarter, 3 AS day_hour, 4 AS microsecond;                      -- 1 2 3 4
SELECT EXTRACT(WEEK FROM DATE '2020-08-11'), EXTRACT(QUARTER FROM DATE '2020-08-11'); -- 33 3

-- Cleanup
DROP VIEW v_dateadd;
DROP FUNCTION dateadd(timestamp, interval);


/* Begin - bug#8226 */
SELECT ADDDATE('2024-02-29', INTERVAL -1 DAY);      -- 2024-02-28
SELECT ADDDATE('2024-02-29', INTERVAL -30 DAY);     -- 2024-01-30
SELECT ADDDATE('2023-02-28', INTERVAL -1 DAY);      -- 2023-02-27
SELECT ADDDATE('2024-02-29', INTERVAL -365 DAY);    -- 2023-03-01
SELECT ADDDATE('2023-03-01', INTERVAL -366 DAY);    -- 2022-02-28
SELECT ADDDATE('2024-03-05', INTERVAL -10 DAY);     -- 2024-02-24
/* End - bug#8226 */

/* Begin - bug#8236 */
SELECT DATE_ADD('2023-01-01', INTERVAL 100000 YEAR);    -- NULL, WARNING
SELECT DATE_ADD('2023-01-01', INTERVAL 1 INVALID_UNIT); -- ERROR
/* End - bug#8236 */

--
-- ReqID:RSR-SQL-Partition-interval
--

--
-- partition key and interval expression
--

-- error: unsupported partition key types
CREATE TABLE i_partitioned (
	a time
) PARTITION BY RANGE (a) INTERVAL (10);

-- error: unsupported partition key types
CREATE TABLE i_partitioned (
	a text
) PARTITION BY RANGE (a) INTERVAL (10);

-- error: unsupported partition key types
CREATE TABLE i_partitioned (
	a dsinterval
) PARTITION BY RANGE (a) INTERVAL (interval '1' hour);

-- error: multiple intervals
CREATE TABLE i_partitioned (
	a int,
	b int
) PARTITION BY RANGE (a) INTERVAL (1,2);

-- error: multiple partition keys
CREATE TABLE i_partitioned (
	a int,
	b int
) PARTITION BY RANGE (a,b) INTERVAL (10);

-- error: invalid collation
CREATE TABLE i_partitioned (
	a int,
	b int
) PARTITION BY RANGE (a) INTERVAL (10 collate aa_DJ);


--
-- interval data types
--

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a) INTERVAL (0);

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a float
) PARTITION BY RANGE (a) INTERVAL (0);

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a numeric
) PARTITION BY RANGE (a) INTERVAL (-1);

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a number
) PARTITION BY RANGE (a) INTERVAL (0);

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a date
) PARTITION BY RANGE (a) INTERVAL (interval '0' day);

-- error: must be greater than 0
CREATE TABLE i_partitioned (
	a timestamp
) PARTITION BY RANGE (a) INTERVAL (interval '-1' hour);

CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a) INTERVAL (1);
DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a float
) PARTITION BY RANGE (a) INTERVAL (1);
DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a numeric
) PARTITION BY RANGE (a) INTERVAL (10);
DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a number
) PARTITION BY RANGE (a) INTERVAL (1);
DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a date
) PARTITION BY RANGE (a) INTERVAL (interval '1' day);
DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a timestamp
) PARTITION BY RANGE (a) INTERVAL (interval '1' hour);
DROP TABLE i_partitioned;

--
-- create table
--

CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a) INTERVAL (10);

-- error: no one parititon
INSERT INTO i_partitioned VALUES (0);

-- error: invalid boundary
CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (MINVALUE) TO (MAXVALUE);

-- error: invalid boundary
CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (0) TO (MAXVALUE);

-- error: invalid boundary
CREATE TABLE i_partitioned_init PARTITION OF i_partitioned DEFAULT;

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (MINVALUE) TO (0);

INSERT INTO i_partitioned VALUES (-1);
INSERT INTO i_partitioned VALUES (0);
INSERT INTO i_partitioned VALUES (101);

\d+ i_partitioned

DROP TABLE i_partitioned_sys_p;
DROP TABLE i_partitioned_sys_p1;
INSERT INTO i_partitioned VALUES (0);
INSERT INTO i_partitioned VALUES (1);
INSERT INTO i_partitioned VALUES (21);

\d+ i_partitioned

DROP TABLE i_partitioned_sys_p;
DROP TABLE i_partitioned_sys_p1;
DROP TABLE i_partitioned_init;

\d+ i_partitioned

-- error: no one partition
INSERT INTO i_partitioned VALUES (-10);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (-100) TO (0);

INSERT INTO i_partitioned VALUES (-10);

-- error: no partition founded
INSERT INTO i_partitioned VALUES (-110);

CREATE TABLE i_partitioned_second PARTITION OF i_partitioned FOR VALUES FROM (-200) TO (-100);
INSERT INTO i_partitioned VALUES (-110);

-- success: 10 - 30
CREATE TABLE i_partitioned_third PARTITION OF i_partitioned FOR VALUES FROM (11) TO (30);

-- error: invalid parition boundary
CREATE TABLE i_partitioned_fourth PARTITION OF i_partitioned FOR VALUES FROM (10) TO (31);

-- error: overlap
CREATE TABLE i_partitioned_fourth PARTITION OF i_partitioned FOR VALUES FROM (10) TO (15);

-- successs
CREATE TABLE i_partitioned_fourth PARTITION OF i_partitioned FOR VALUES FROM (-222) TO (-212);

DROP TABLE i_partitioned_init;

-- successs: -55 - 10
CREATE TABLE i_partitioned_fifth PARTITION OF i_partitioned FOR VALUES FROM (-55) TO (5);

INSERT INTO i_partitioned VALUES (17);
INSERT INTO i_partitioned VALUES (57);

\d+ i_partitioned

DROP TABLE i_partitioned;

--
-- set interval
--

CREATE TABLE i_partitioned (
	a timestamp
) PARTITION BY RANGE (a) INTERVAL (interval '1' day);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (MINVALUE) TO ('2020-1-1');

\d+ i_partitioned

INSERT INTO i_partitioned VALUES ('2020-1-1');
UPDATE i_partitioned SET a = a + interval '3' day;
SELECT * FROM i_partitioned;

\d+ i_partitioned

ALTER TABLE i_partitioned SET INTERVAL (interval '1' day);
ALTER TABLE i_partitioned SET INTERVAL (interval '1' month);

\d+ i_partitioned

INSERT INTO i_partitioned VALUES ('2020-3-1');

\d+ i_partitioned

ALTER TABLE i_partitioned SET INTERVAL (interval '30' day);
INSERT INTO i_partitioned VALUES ('2021-3-1');

\d+ i_partitioned

ALTER TABLE i_partitioned SET INTERVAL (interval '29' day);
INSERT INTO i_partitioned VALUES ('2022-3-1');

\d+ i_partitioned

-- error: invalid interval expression
ALTER TABLE i_partitioned SET INTERVAL (10);

ALTER TABLE i_partitioned SET INTERVAL ();

\d+ i_partitioned

ALTER TABLE i_partitioned SET INTERVAL (interval '10' hour);

\d+ i_partitioned

DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a timestamp
) PARTITION BY RANGE (a) INTERVAL (interval '1' day);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (MINVALUE) TO ('2020-1-29');

\d+ i_partitioned

-- error
ALTER TABLE i_partitioned SET INTERVAL (interval '1' month);

INSERT INTO i_partitioned VALUES('2020-2-28');

\d+ i_partitioned

-- error
ALTER TABLE i_partitioned SET INTERVAL (interval '1' month);

INSERT INTO i_partitioned VALUES('2020-3-27');

\d+ i_partitioned

ALTER TABLE i_partitioned SET INTERVAL (interval '1' month);

\d+ i_partitioned

DROP TABLE i_partitioned;


--
-- attach/detach
--

CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a) INTERVAL (10);
CREATE TABLE i_partitioned_1 (a int);
CREATE TABLE i_partitioned_2 (a int);
CREATE TABLE i_partitioned_3 (a int);

-- error: unsupportted partition
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_1 DEFAULT;

-- error: invalid boundary
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_1 FOR VALUES FROM (MINVALUE) TO (MAXVALUE);

-- error: invalid boundary
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_1 FOR VALUES FROM (0) TO (MAXVALUE);

ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_1 FOR VALUES FROM (MINVALUE) TO (0);

-- suucess: 10 - 30
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_2 FOR VALUES FROM (11) TO (21);

INSERT INTO i_partitioned_3 VALUES (78);

-- error: constraint
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_3 FOR VALUES FROM (88) TO (100);

truncate i_partitioned_3;

-- error: overlap
ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_3 FOR VALUES FROM (10) TO (30);

ALTER TABLE i_partitioned ATTACH PARTITION i_partitioned_3 FOR VALUES FROM (43) TO (44);

\d+ i_partitioned

ALTER TABLE i_partitioned DETACH PARTITION i_partitioned_2;

DROP TABLE i_partitioned;

--
-- create composite partition table
--

CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a);

-- error: unsupported interval sub-partition
CREATE TABLE i_partitioned_init PARTITION OF i_partitioned
	FOR VALUES FROM (MINVALUE) TO (1000)
	PARTITION BY RANGE(a) INTERVAL (10);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned
	FOR VALUES FROM (MINVALUE) TO (1000)
	PARTITION BY RANGE(a);

DROP TABLE i_partitioned;

CREATE TABLE i_partitioned (
	a int
) PARTITION BY RANGE (a) INTERVAL (10);

-- error: unsupported interval sub-partition
CREATE TABLE i_partitioned_init PARTITION OF i_partitioned
	FOR VALUES FROM (MINVALUE) TO (1000)
	PARTITION BY RANGE(a) INTERVAL (10);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned
	FOR VALUES FROM (MINVALUE) TO (1000)
	PARTITION BY RANGE(a);

\d+ i_partitioned

INSERT INTO i_partitioned VALUES (999);
INSERT INTO i_partitioned VALUES (5555);

\d+ i_partitioned

DROP TABLE i_partitioned;

/* Begin - BUG#5647 */
CREATE TABLE i_partitioned (id int, dt date) PARTITION BY RANGE (id) INTERVAL (10);
CREATE TABLE i_partitioned_p1 PARTITION OF i_partitioned(dt) FOR VALUES FROM (MINVALUE) TO (10);
\d+ i_partitioned
INSERT INTO i_partitioned VALUES(31,'2000-01-01');
\d+ i_partitioned
INSERT INTO i_partitioned VALUES(11,'2000-01-01');
\d+ i_partitioned
UPDATE i_partitioned SET id=21 WHERE id=11;
\d+ i_partitioned
SELECT * FROM i_partitioned;
SELECT * FROM i_partitioned_p1;
SELECT * FROM i_partitioned_sys_p;
SELECT * FROM i_partitioned_sys_p1;
SELECT * FROM i_partitioned_sys_p2;

DROP TABLE i_partitioned;
/* End - BUG#5647 */

/* Begin - BUG#5652 */
CREATE TABLE i_partitioned (id numeric(4,2), dt date) PARTITION BY RANGE(id) INTERVAL (10.2345);
CREATE TABLE i_partitioned_p1 PARTITION OF i_partitioned(id) FOR VALUES FROM (MINVALUE) TO (10);
\d+ i_partitioned
INSERT INTO i_partitioned VALUES(11, '2000-01-01');
\d+ i_partitioned
SELECT * FROM i_partitioned_sys_p;

DROP TABLE i_partitioned;
/* End - BUG#5652 */

/* Begin - BUG#6801 */
CREATE TABLE i_partitioned (id int) PARTITION BY RANGE(id) INTERVAL (10);
ALTER TABLE i_partitioned SET INTERVAL(11);

DROP TABLE i_partitioned;
/* End - BUG#6801 */

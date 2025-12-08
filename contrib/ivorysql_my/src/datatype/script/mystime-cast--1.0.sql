
-------------------------------------------------------------------------------------------------
-- date to other time type 
-------------------------------------------------------------------------------------------------
-- mysdate to datetime timestamp time
CREATE CAST (ivymy.mysdate AS ivymy.mysdatetime)  WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdate AS ivymy.mystimestamp)  WITH INOUT AS ASSIGNMENT;
CREATE FUNCTION ivymy.mysdate_mystime(ivymy.mysdate) 
RETURNS ivymy.mystime 
AS 'MODULE_PATHNAME','mysdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystime) WITH FUNCTION ivymy.mysdate_mystime(ivymy.mysdate) AS ASSIGNMENT;

-- mysdate to datetimes timestamps times
CREATE FUNCTION ivymy.mysdate_mysdatetimes(ivymy.mysdate, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mysdate_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mysdate_mysdatetimes(ivymy.mysdate, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdate_mystimestamps(ivymy.mysdate, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdate_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdate_mystimestamps(ivymy.mysdate, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdate_mystimes(ivymy.mysdate, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdate_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdate_mystimes(ivymy.mysdate, integer)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mysdatetime to other time type 
-------------------------------------------------------------------------------------------------
-- mysdatetime to datetime timestamp time
CREATE CAST (ivymy.mysdatetime AS ivymy.mysdate)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;
-- mysdatetime to datetimes timestamps times
CREATE FUNCTION ivymy.mysdatetime_mysdatetimes(ivymy.mysdatetime, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mysdatetime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mysdatetime_mysdatetimes(ivymy.mysdatetime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetime_mystimestamps(ivymy.mysdatetime, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdatetime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdatetime_mystimestamps(ivymy.mysdatetime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetime_mystimes(ivymy.mysdatetime, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdatetime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdatetime_mystimes(ivymy.mysdatetime, integer)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mysdatetimes to other time type 
-------------------------------------------------------------------------------------------------
-- mysdatetimes to date datetime timestamp time
CREATE CAST (ivymy.mysdatetimes AS ivymy.mysdatetime)  WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mysdate)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;

-- mysdatetimes to timestamps times
CREATE FUNCTION ivymy.mysdatetimes_mystimestamps(ivymy.mysdatetimes, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdatetimes_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdatetimes_mystimestamps(ivymy.mysdatetimes, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetimes_mystimes(ivymy.mysdatetimes, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdatetimes_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdatetimes_mystimes(ivymy.mysdatetimes, integer)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mystimestamp to other time type 
-------------------------------------------------------------------------------------------------
-- mystimestamp to date datetime time
CREATE CAST (ivymy.mystimestamp AS ivymy.mysdate)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamp AS ivymy.mysdatetime) WITH INOUT AS IMPLICIT;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;
-- mystimestamp to datetimes timestamps times
CREATE FUNCTION ivymy.mystimestamp_mysdatetimes(ivymy.mystimestamp, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mystimestamp_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimestamp_mysdatetimes(ivymy.mystimestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimestamp_mystimestamps(ivymy.mystimestamp, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamp_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystimestamp_mystimestamps(ivymy.mystimestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimestamp_mystimes(ivymy.mystimestamp, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimestamp_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystimes)
WITH FUNCTION ivymy.mystimestamp_mystimes(ivymy.mystimestamp, integer)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mystimestamps to other time type 
-------------------------------------------------------------------------------------------------
-- mystimestamps to date datetime timestamp time
CREATE CAST (ivymy.mystimestamps AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdate)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdatetime) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;
-- mystimestamps to datetimes times
CREATE FUNCTION ivymy.mystimestamps_mysdatetimes(ivymy.mystimestamps, integer) 
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mystimestamps_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimestamps_mysdatetimes(ivymy.mystimestamps, integer)
AS IMPLICIT;

CREATE FUNCTION ivymy.mystimestamps_mystimes(ivymy.mystimestamps, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimestamps_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystimes)
WITH FUNCTION ivymy.mystimestamps_mystimes(ivymy.mystimestamps, integer)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mystime to other time type 
-------------------------------------------------------------------------------------------------
-- mystime to date timestamp 
CREATE FUNCTION ivymy.mystime_mysdate(ivymy.mystime) 
RETURNS ivymy.mysdate 
AS 'MODULE_PATHNAME','mystime_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mysdate) WITH FUNCTION ivymy.mystime_mysdate(ivymy.mystime) AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime) 
RETURNS ivymy.mystimestamp 
AS 'MODULE_PATHNAME','mystime_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mystimestamp) WITH FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime) AS ASSIGNMENT;

-- mystime to datetimes timestamps times year
CREATE FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mystime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mystimes)
WITH FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.time2year(ivymy.mystime)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'time_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;
CREATE CAST (ivymy.mystime as year)
WITH FUNCTION ivymy.time2year(ivymy.mystime)
AS ASSIGNMENT;

-------------------------------------------------------------------------------------------------
-- mystimes to other time type 
-------------------------------------------------------------------------------------------------
-- mystimes to date datetime timestamp
CREATE FUNCTION ivymy.mystimes_mysdate(ivymy.mystimes) 
RETURNS ivymy.mysdate 
AS 'MODULE_PATHNAME','mystimes_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mysdate)
WITH FUNCTION ivymy.mystimes_mysdate(ivymy.mystimes)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mysdatetime(ivymy.mystimes) 
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimes_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mysdatetime)
WITH FUNCTION ivymy.mystimes_mysdatetime(ivymy.mystimes)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mystimestamp(ivymy.mystimes) 
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimes_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mystimestamp)
WITH FUNCTION ivymy.mystimes_mystimestamp(ivymy.mystimes)
AS ASSIGNMENT;

-- mystimes to datetimes timestamps
CREATE FUNCTION ivymy.mystimes_mysdatetimes(ivymy.mystimes, integer) 
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mystimes_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimes_mysdatetimes(ivymy.mystimes, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mystimestamps(ivymy.mystimes, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimes_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystimes_mystimestamps(ivymy.mystimes, integer)
AS ASSIGNMENT;

--
-- brin index support for all data type 
--
CREATE FUNCTION ivymy.brin_minmax_opcinfo(internal)
RETURNS internal
AS 'brin_minmax_opcinfo'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_add_value(internal, internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_add_value'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_consistent(internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_consistent'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_union(internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_union'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* CREATE OPERATOR FAMILY */
CREATE OPERATOR FAMILY ivymy.mys_mysdate_minmax_ops USING brin;

/* CREATE OPERATOR CLASS for 'ivymy.mysdate' */
CREATE OPERATOR CLASS ivymy.mysdate_minmax_ops
	DEFAULT FOR TYPE ivymy.mysdate USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	2 	<= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	3 	=  (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	4  	>= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	5  	>  (ivymy.mysdate, ivymy.mysdate),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);

/* CREATE OPERATOR CLASS for 'ivymy.mysdatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetime_minmax_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	2 	<= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	3 	=  (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	4  	>= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	5  	>  (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);
		
		/* CREATE OPERATOR CLASS for 'ivymy.mystimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamp_minmax_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	2 	<= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	3 	=  (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	4  	>= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	5  	>  (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);

CREATE OPERATOR CLASS ivymy.mystime_minmax_ops
	DEFAULT FOR TYPE ivymy.mystime USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mystime, ivymy.mystime),
		OPERATOR	2 	<= (ivymy.mystime, ivymy.mystime),
		OPERATOR	3 	=  (ivymy.mystime, ivymy.mystime),
		OPERATOR	4  	>= (ivymy.mystime, ivymy.mystime),
		OPERATOR	5  	>  (ivymy.mystime, ivymy.mystime),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);		

-- Convert MySQL date, time, time(N), datetime, datetime(N), timestamp, timestamp(N) to year
CREATE OR REPLACE FUNCTION ivymy.date2year(ivymy.mysdate)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'date_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdate AS year)
WITH FUNCTION ivymy.date2year(ivymy.mysdate) AS ASSIGNMENT;

-- Fix Bug#Z5406 begin
CREATE OR REPLACE FUNCTION ivymy.times2year(ivymy.mystimes)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'times_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimes as year)
WITH FUNCTION ivymy.times2year(ivymy.mystimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.datetime2year(ivymy.mysdatetime)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'datetime_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdatetime as year)
WITH FUNCTION ivymy.datetime2year(ivymy.mysdatetime) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.datetimes2year(ivymy.mysdatetimes)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'datetimes_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdatetimes as year)
WITH FUNCTION ivymy.datetimes2year(ivymy.mysdatetimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamp2year(ivymy.mystimestamp)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamp_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimestamp as year)
WITH FUNCTION ivymy.timestamp2year(ivymy.mystimestamp) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamps2year(ivymy.mystimestamps)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamps_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimestamps as year)
WITH FUNCTION ivymy.timestamps2year(ivymy.mystimestamps) AS ASSIGNMENT;
-- Fix Bug#Z5406 end

/* contrib/hg_wait_states/hg_wait_states--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_wait_states" to load this file. \quit

-- The reader functions expose queries and utility statements that might
-- contain sensitive information. Keep all functions private by default and
-- only grant access explicitly to trusted monitoring roles.
CREATE FUNCTION hg_wait_states_samples(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT query_id int8,
	OUT session_id int4,
	OUT query_start_time timestamptz,
	OUT sample_time timestamptz,
	OUT wait_event_type text,
	OUT wait_event text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_wait_states_samples_internal(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT query_id int8,
	OUT session_id int4,
	OUT query_start_time timestamptz,
	OUT sample_time timestamptz,
	OUT wait_event_type text,
	OUT wait_event text,
	OUT sampling_interval int4
)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'hg_wait_states_samples_internal'
LANGUAGE C;

CREATE FUNCTION hg_wait_states_queries(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT query_id int8,
	OUT query text,
	OUT ref_start_ts timestamptz,
	OUT ref_end_ts timestamptz
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_wait_states_sessions(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT session_id int4,
	OUT dbname text,
	OUT username text,
	OUT ref_start_ts timestamptz,
	OUT ref_end_ts timestamptz,
	OUT application_name text,
	OUT client_hostname text,
	OUT session_start_ts timestamptz
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_wait_states_data(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT session_id int4,
	OUT dbname text,
	OUT username text,
	OUT query text,
	OUT query_start_time timestamptz,
	OUT sample_time timestamptz,
	OUT wait_event_type text,
	OUT wait_event text
)
RETURNS SETOF record
AS $$
WITH samp AS MATERIALIZED (
	SELECT * FROM hg_wait_states_samples(start_ts, end_ts)
),
sess AS MATERIALIZED (
	SELECT * FROM hg_wait_states_sessions(start_ts, end_ts)
),
qry AS MATERIALIZED (
	SELECT * FROM hg_wait_states_queries(start_ts, end_ts)
),
/*
 * Pick the right sessions row for every sample.  Because the OS pid is
 * reused across backends, the file may contain several sessions records
 * with the same session_id but different session_start_ts; for any given
 * sample only the latest one whose session_start_ts <= sample_time is the
 * actual owner of that sample.
 *
 * Note: we do *not* gate on the sessions file's [ref_start_ts, ref_end_ts]
 * window because the bgworker only writes a session record the first time
 * it observes a backend, so a session that spans a file rotation lives
 * only in the older file even though its samples land in the newer file.
 * (pid, session_start_ts) is already unique by construction in the file,
 * so DISTINCT ON below picks one row.
 */
samp_with_sess AS (
	SELECT DISTINCT ON (s.session_id, s.sample_time)
		   s.session_id, ss.dbname, ss.username, s.query_id,
		   s.query_start_time, s.sample_time,
		   s.wait_event_type, s.wait_event
	FROM samp s
	LEFT JOIN sess ss
	  ON ss.session_id = s.session_id
	 AND ss.session_start_ts <= s.sample_time
	ORDER BY s.session_id, s.sample_time, ss.session_start_ts DESC NULLS LAST
),
/*
 * hg_wait_states_queries() can repeat the same query_id once per queries
 * file that recorded it; deduplicate so that joining cannot multiply rows.
 */
qry_dedup AS (
	SELECT DISTINCT ON (query_id) query_id, query
	FROM qry
	ORDER BY query_id, ref_end_ts DESC
)
SELECT sws.session_id, sws.dbname, sws.username, q.query,
	   sws.query_start_time, sws.sample_time,
	   sws.wait_event_type, sws.wait_event
FROM samp_with_sess sws
LEFT JOIN qry_dedup q USING (query_id);
$$
LANGUAGE SQL;

-- function to generate wait events list and time spent on those wait events
CREATE FUNCTION hg_wait_states_wait_events(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT wait_event text,
	OUT wait_event_type text,
	OUT waittime int8,
	OUT pct_dbtime numeric
)
RETURNS SETOF record
AS $$
	WITH samp AS MATERIALIZED
	(
		SELECT * FROM hg_wait_states_samples_internal(start_ts, end_ts)
	),
	waitevents_cte AS
	(
		SELECT wait_event_type, wait_event, SUM(sampling_interval) AS waittime
		FROM samp
		GROUP BY wait_event_type, wait_event
	),
	total_dbtime AS
	(
		SELECT SUM(sampling_interval)::numeric AS dbtime FROM samp
	)
	SELECT wait_event, wait_event_type, waittime,
		   COALESCE((waittime::numeric * 100) / NULLIF(t.dbtime, 0),
					0::numeric) AS pct_dbtime
	FROM waitevents_cte, total_dbtime AS t;
$$
LANGUAGE SQL;

-- function to sql statements list and dbtime/cputime/waittime for each statement
CREATE FUNCTION hg_wait_states_sql_statements(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz,
	OUT query_id int8,
	OUT dbtime numeric,
	OUT waittime numeric,
	OUT cputime numeric,
	OUT top_waitevent text,
	OUT query text
)
RETURNS SETOF record
AS $$
/*
 * The original implementation joined four CTEs implicitly and built the
 * "top wait event" subquery without projecting query_id, which let
 * waittime values match across different queries and produced inflated
 * dbtime/waittime via the resulting cross-product.  We now materialize
 * the samples scan exactly once, aggregate each metric independently,
 * pick the top wait event per query_id with DISTINCT ON, and outer-join
 * everything so queries that are pure CPU or pure wait are still shown.
 */
WITH samp AS MATERIALIZED
(
	SELECT * FROM hg_wait_states_samples_internal(start_ts, end_ts)
),
dbtime_cte AS
(
	SELECT query_id, SUM(sampling_interval)::numeric AS dbtime
	FROM samp
	GROUP BY query_id
),
waittime_cte AS
(
	SELECT query_id, SUM(sampling_interval)::numeric AS waittime
	FROM samp
	WHERE wait_event IS NOT NULL
	GROUP BY query_id
),
cputime_cte AS
(
	SELECT query_id, SUM(sampling_interval)::numeric AS cputime
	FROM samp
	WHERE wait_event IS NULL
	GROUP BY query_id
),
top_event AS
(
	SELECT DISTINCT ON (query_id) query_id, wait_event AS top_waitevent
	FROM
	(
		SELECT query_id, wait_event, SUM(sampling_interval) AS we_waittime
		FROM samp
		WHERE wait_event IS NOT NULL
		GROUP BY query_id, wait_event
	) we
	ORDER BY query_id, we_waittime DESC
),
/*
 * hg_wait_states_queries() can return several rows per query_id when the
 * same query was observed across more than one queries file; keep just one
 * representative row per query_id to avoid duplicating output.
 */
queries_dedup AS
(
	SELECT DISTINCT ON (query_id) query_id, query
	FROM hg_wait_states_queries(start_ts, end_ts)
	ORDER BY query_id, ref_end_ts DESC
)
SELECT dt.query_id,
	   dt.dbtime,
	   COALESCE(wt.waittime, 0::numeric) AS waittime,
	   COALESCE(ct.cputime,  0::numeric) AS cputime,
	   te.top_waitevent,
	   q.query
FROM dbtime_cte dt
LEFT JOIN waittime_cte  wt USING (query_id)
LEFT JOIN cputime_cte   ct USING (query_id)
LEFT JOIN top_event     te USING (query_id)
LEFT JOIN queries_dedup q  USING (query_id);
$$
LANGUAGE SQL;

CREATE FUNCTION hg_wait_states_purge(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz
)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_wait_states_directory_size(
	IN start_ts timestamptz default '-infinity'::timestamptz,
	IN end_ts timestamptz default 'infinity'::timestamptz
)
RETURNS bigint
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION hg_wait_states_samples(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_samples_internal(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_queries(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_sessions(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_data(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_purge(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_wait_events(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_sql_statements(timestamptz, timestamptz) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_wait_states_directory_size(timestamptz, timestamptz) FROM PUBLIC;

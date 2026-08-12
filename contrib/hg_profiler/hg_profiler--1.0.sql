/* contrib/hg_profiler/hg_profiler--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_profiler" to load this file. \quit

/*
 * Storage layout
 * ==============
 *
 * hg_profiler keeps NO permanent tables of its own.  Captured profile
 * data is written to plain files under $PGDATA/hg_profiler/<trace_id>/:
 *
 *     trace.ini          per-trace metadata (Status, Owner, MaxSize, ...)
 *     trace-<pid>.csv    one append-only CSV file per backend that
 *                        contributes rows to the trace
 *
 * The on-disk format matches what SQL Profiler tooling expects.
 * hg_p_load_trace() materialises
 * those CSV rows into the temporary table _hg_tmp_tbl_p_profiler
 * (defined below) on demand.
 */

-- ==================================================================
-- C-callable management functions
-- ==================================================================

/*
 * hg_p_activate -- start a new trace.
 *
 * Returns a 20-character trace_id (YYYYMMDDhhmmss + 6-digit counter)
 * which is later passed to hg_p_deactivate / hg_p_load_trace.
 *
 * Empty oidvector for users/databases means "all users/databases".
 *
 * Errors out with ERRCODE_INSUFFICIENT_PRIVILEGE if a non-superuser
 * tries to activate a trace targeting users other than themselves.
 */
CREATE FUNCTION hg_p_activate(
	comments         text,
	users            oidvector,
	databases        oidvector,
	max_log_limit    integer,
	log_min_duration integer,
	time_limit       interval
) RETURNS text
AS 'MODULE_PATHNAME', 'hg_p_activate'
LANGUAGE C VOLATILE;

COMMENT ON FUNCTION hg_p_activate(text, oidvector, oidvector, integer, integer, interval) IS
$$Description:
- Start a new trace for the given user(s) and database(s).
Returns
- trace_id on success, which can be used to stop this trace.

Parameters:
  comments         - Any text to describe the purpose for the trace.
                   - Newline characters are not allowed.
  users            - Vector of user OID for whom queries needs to be logged.
                   - Empty oidvector suggests logging the queries for all the user.
  databases        - Vector of database OID for which sql needs to be logged.
                   - Empty oidvector suggests logging the queries for all the database.
  max_log_limit    - Approximate size limit for one trace in MB.
                   - Counts bytes appended to all trace-<pid>.csv files
                     under the same trace_id.
                   - Does not measure the trace directory's exact on-disk
                     size.
                   - 0 suggests no limit.
  log_min_duration - Logs only statements running at least this number of seconds.
  time_limit       - Run the trace for the given number of time interval.
                     Interval must be a positive value.$$;

/*
 * hg_p_deactivate -- stop a running trace.
 *
 * Returns true on success; emits an error if trace_id is not active or
 * the caller is neither the owner nor a superuser.
 */
CREATE FUNCTION hg_p_deactivate(trace_id text) RETURNS bool
AS 'MODULE_PATHNAME', 'hg_p_deactivate'
LANGUAGE C VOLATILE STRICT;

COMMENT ON FUNCTION hg_p_deactivate(text) IS
$$Description:
- Stops the trace for the given trace_id
Returns
- true on success, otherwise the error.

Parameters:
  trace_id - the return value of hg_p_activate function.$$;

/*
 * hg_p_active_traces -- list traces currently in the shared-memory active
 * slots.  Non-superusers see only the traces they themselves activated.
 */
CREATE FUNCTION hg_p_active_traces(
	OUT trace_id         text,
	OUT comments         text,
	OUT owner            oid,
	OUT start_time       timestamptz,
	OUT end_time         timestamptz,
	OUT max_log_limit    integer,
	OUT log_min_duration integer,
	OUT users            oidvector,
	OUT databases        oidvector,
	OUT final_status     integer
) RETURNS SETOF record
AS 'MODULE_PATHNAME', 'hg_p_active_traces'
LANGUAGE C VOLATILE;

COMMENT ON FUNCTION hg_p_active_traces() IS
$$Description:
For superusers:
- List down all the active trace(s).
For non-superusers:
- List down the active trace(s) initiated by them.

Returns:
- ID of the trace (trace_id)
- Description that set by user at the time of activation
- Owner of the trace
- Start time
- End time (scheduled stop time; null means no time_limit)
- Limit of the maximum trace size in MB
- Limit for logging queries that run at least this number of milliseconds
- List of User OID(s)
- List of database OID(s)
- Final status code (final_status):
  0 = server_restart
  1 = active
  2 = stopped
  3 = max_size
  4 = time_limit$$;

/*
 * hg_p_traces_list -- list every trace ever recorded (scans the on-disk
 * trace directories under $PGDATA/hg_profiler/).
 *
 *   status = 1  Active           (still running, in shmem)
 *   status = 2  Stopped          (hg_p_deactivate)
 *   status = 3  Max size exceeded
 *   status = 4  Time limit exceeded
 *   status = 0  Server restarted (was Active when the cluster went down)
 */
CREATE FUNCTION hg_p_traces_list(
	OUT trace_id         text,
	OUT comments         text,
	OUT owner            oid,
	OUT start_time       timestamptz,
	OUT end_time         timestamptz,
	OUT max_log_limit    integer,
	OUT log_min_duration integer,
	OUT users            oidvector,
	OUT databases        oidvector,
	OUT final_status     integer
) RETURNS SETOF record
AS 'MODULE_PATHNAME', 'hg_p_traces_list'
LANGUAGE C VOLATILE;

COMMENT ON FUNCTION hg_p_traces_list() IS
$$Description:
For superusers:
- List down all the trace(s)
For non-superusers:
- List down the trace(s) initiated by them.

Returns:
- ID of the trace (trace_id)
- Description that set by user at the time of activation
- Owner of the trace
- Start time
- End time (scheduled stop time; null means no time_limit)
- Limit of the maximum trace size in MB
- Limit for logging queries that run at least this number of milliseconds
- List of User OID(s)
- List of Database OID(s)
- Final status code (final_status):
  0 = server_restart
  1 = active
  2 = stopped
  3 = max_size
  4 = time_limit$$;

/*
 * hg_p_deactive_traces -- list every non-active trace recorded on disk.
 *
 * This is a SQL wrapper over hg_p_traces_list() that filters out
 * final_status = 1 (Active).
 */
CREATE FUNCTION hg_p_deactive_traces()
RETURNS TABLE
(
	trace_id         text,
	comments         text,
	owner            oid,
	start_time       timestamptz,
	end_time         timestamptz,
	max_log_limit    integer,
	log_min_duration integer,
	users            oidvector,
	databases        oidvector,
	final_status     integer
)
LANGUAGE SQL
VOLATILE
AS $$
	SELECT *
	FROM hg_p_traces_list()
	WHERE final_status <> 1
$$;

COMMENT ON FUNCTION hg_p_deactive_traces() IS
$$Description:
For superusers:
- List down all the non-active trace(s).
For non-superusers:
- List down the non-active trace(s) initiated by them.

Returns:
- Same columns as hg_p_traces_list(), but excludes final_status = 1 (active).$$;

/*
 * hg_p_load_trace -- read every trace-<pid>.csv file under
 * $PGDATA/hg_profiler/<trace_id>/ and INSERT the parsed rows into the
 * temp table _hg_tmp_tbl_p_profiler.  The temp table is created on
 * first call if it does not already exist.  Every invocation truncates
 * the temp table and reloads the trace from scratch.
 */
CREATE FUNCTION hg_p_load_trace(trace_id text) RETURNS bool
AS 'MODULE_PATHNAME', 'hg_p_load_trace'
LANGUAGE C VOLATILE STRICT;

COMMENT ON FUNCTION hg_p_load_trace(text) IS
$$Description:
- Load all the trace files of particular trace_id in the database in temporary table "_hg_tmp_tbl_p_profiler"
- Clear the temp table before each load, then import the trace again from scratch.

Returns:
- Returns true on successful loading the trace.

Parameters:
- Trace_id for the particular trace.$$;

/*
 * hg_p_cleanup -- permanently delete a trace's on-disk directory.
 *
 *   remove_active = true  -- if the trace is still active, stop it first
 *                            and then delete the directory.
 *   remove_active = false -- error out if the trace is still active.
 *
 * Only the trace owner or a superuser may remove a trace.
 */
CREATE FUNCTION hg_p_cleanup(trace_id text, remove_active bool) RETURNS bool
AS 'MODULE_PATHNAME', 'hg_p_cleanup'
LANGUAGE C VOLATILE STRICT;

COMMENT ON FUNCTION hg_p_cleanup(text, bool) IS
$$Description:
- Remove the trace permanently.

Returns:
- Returns true on successful removal of the trace.

Parameters:
- Trace_id for the particular trace, which needs to be removed.
- Remove active flag:
  True  - if the trace is still running, stop it and then delete it.
  False - results in an error if the given trace is still active.$$;

/*
 * hg_p_profiler_version -- numeric version of the loaded module.
 *
 * The C entry point is exported under both 'hg_p_version' (the alternate
 * symbol referenced by some tooling) and 'hg_p_profiler_version'.
 */
CREATE FUNCTION hg_p_profiler_version() RETURNS float8
AS 'MODULE_PATHNAME', 'hg_p_version'
LANGUAGE C IMMUTABLE STRICT;

COMMENT ON FUNCTION hg_p_profiler_version() IS
$$- This represents the current version of the SQL Profiler.
- This is used by profiler tooling.$$;

-- ==================================================================
-- Permissions
-- ==================================================================
REVOKE ALL    ON FUNCTION hg_p_activate(text, oidvector, oidvector, integer, integer, interval) FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_deactivate(text)         FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_active_traces()          FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_traces_list()            FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_deactive_traces()        FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_load_trace(text)         FROM PUBLIC;
REVOKE ALL    ON FUNCTION hg_p_cleanup(text, bool)      FROM PUBLIC;

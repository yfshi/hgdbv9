# hg_profiler

`hg_profiler` is a per-query SQL execution profiler for v9.
It uses the same on-disk format and public API expected by existing SQL
Profiler tooling:

* Captures the same set of metrics (`getrusage` deltas,
  `pgBufferUsage` deltas, EXPLAIN plan, query identification fields).
* Stores them in **append-only CSV files under `$PGDATA/hg_profiler/`**,
  one file per backend, plus a small `trace.ini` metadata file per trace.
* Hooks the executor (`ExecutorStart` / `ExecutorRun` / `ExecutorEnd`) and
  the shared-memory startup paths (`shmem_request` / `shmem_startup`).
* Honours the same set of user-facing functions:
  `hg_p_activate`, `hg_p_deactivate`, `hg_p_active_traces`,
  `hg_p_traces_list`, `hg_p_deactive_traces`, `hg_p_load_trace`,
  `hg_p_cleanup`, `hg_p_profiler_version`.

## Files

```
contrib/hg_profiler/
contrib/hg_profiler/hg_profiler.control        -- extension control file
contrib/hg_profiler/hg_profiler--1.0.sql       -- schema + function declarations + grants
contrib/hg_profiler/hg_profiler.c              -- module implementation (hooks + SQL funcs)
contrib/hg_profiler/Makefile			       -- build glue
contrib/hg_profilerREADME.md                   -- this file
```

## Installation

1. Build and install:

   ```sh
   cd contrib/hg_profiler
   make && make install
   ```

2. Add to `shared_preload_libraries` and restart PostgreSQL:

   ```ini
   shared_preload_libraries = 'hg_profiler'
   ```

3. Create the extension once per database that you intend to profile from:

   ```sql
   CREATE EXTENSION hg_profiler;
   ```

## On-disk layout

All trace data lives under `$PGDATA/hg_profiler/`:

```
$PGDATA/hg_profiler/
hg_profiler/20260505190000000001/
hg_profiler/20260505190000000001/trace.ini              -- per-trace metadata
hg_profiler/20260505190000000001/trace-12345.csv        -- one CSV per backend that hit the trace
hg_profiler/20260505190000000001/trace-12346.csv
hg_profiler/20260505191500000002/
hg_profiler/20260505191500000002/trace.ini
hg_profiler/20260505191500000002/trace-12345.csv
```

There are **no permanent SQL tables**.
hgp_load_trace() materialises
the rows on demand into the temp table _hg_tmp_tbl_p_profiler so that
clients using the SQL Profiler API can read the captured data.

### `trace.ini` format

A plain `Key=Value` text file, one pair per line.  `Comments` is
double-quoted CSV-style; everything else is bare.

```
Version=4.0
TraceId=20260505190000000001
Comments="long-running query investigation"
Owner=10
StartTime=2026-05-05 19:00:00.123456+08
EndTime=2026-05-05 20:00:00+08
NUsers=2
Users=10,11
NDatabases=1
Databases=12345
MaxLogLimit=100
LogMinDuration=50
Status=2
FinishTime=2026-05-05 19:30:11.987654+08
```

`Status` values used by the profiler are:

| Value | Meaning |
| --- | --- |
| `1` | Active (still running) |
| `2` | Stopped via `hg_p_deactivate` |
| `3` | Max size exceeded |
| `4` | Time limit exceeded |
| `0` | Server restart: was `Active` on disk but no shared-memory slot could be re-allocated after the cluster restarted |


### trace-<pid>.csv format

Each record has **29 comma-separated fields**, escaped RFC-4180-style
(text fields are wrapped in `"..."`, embedded quotes are doubled).
Embedded newlines inside quoted fields are preserved verbatim so that
multi-line `EXPLAIN` plans round-trip cleanly.  The reader in
`hg_p_load_trace()` tracks the quote state with `fgets()` so it can
re-assemble records that span several physical lines.

The field order follows the profiler CSV layout, except this
implementation does not emit `query_id`.

| # | Field | Type | Notes |
| --- | --- | --- | --- |
| 1 | start_time | timestamptz string | Captured at `ExecutorStart` |
| 2 | end_time | timestamptz string | Captured when the row is emitted |
| 3 | appl_name | text | `application_name` GUC |
| 4 | user_id | oid | `GetUserId()` |
| 5 | db_id | oid | `MyDatabaseId` |
| 6 | query_type | int | `CmdType` (1=SELECT, 2=UPDATE, 3=INSERT, 4=DELETE 閳? |
| 7 | query | text | SQL source text |
| 8 | explain | text | EXPLAIN output (TEXT format, newlines preserved) |
| 9 | duration | float8 | ms (`Instrumentation.total * 1000`) |
| 10 | rows_updated | int8 | `EState.es_processed` |
| 11 | fs_in | int8 | rusage `ru_inblock` delta |
| 12 | fs_out | int8 | rusage `ru_oublock` delta |
| 13 | page_faults | int8 | rusage `ru_majflt` delta |
| 14 | page_reclaims | int8 | rusage `ru_minflt` delta |
| 15 | swaps | int8 | rusage `ru_nswap` delta |
| 16 | sign_recv | int8 | rusage `ru_nsignals` delta |
| 17 | msg_recv | int8 | rusage `ru_msgrcv` delta |
| 18 | msg_snd | int8 | rusage `ru_msgsnd` delta |
| 19 | vol_contx_switch | int8 | rusage `ru_nvcsw` delta |
| 20 | invol_contx_switch | int8 | rusage `ru_nivcsw` delta |
| 21 | shared_blk_read | int8 | `pgBufferUsage.shared_blks_read` delta |
| 22 | shared_blk_written | int8 | `pgBufferUsage.shared_blks_written` delta |
| 23 | shared_blk_hit | int8 | `pgBufferUsage.shared_blks_hit` delta |
| 24 | local_blk_read | int8 | `pgBufferUsage.local_blks_read` delta |
| 25 | local_blk_written | int8 | `pgBufferUsage.local_blks_written` delta |
| 26 | local_blk_hit | int8 | `pgBufferUsage.local_blks_hit` delta |
| 27 | tmp_blk_read | int8 | `pgBufferUsage.temp_blks_read` delta |
| 28 | tmp_blk_written | int8 | `pgBufferUsage.temp_blks_written` delta |
| 29 | pid | int4 | `MyProcPid` (also encoded in the file name) |

## User-facing functions

```sql
hg_p_activate()
hg_p_deactivate()
hg_p_active_traces()
hg_p_traces_list()
hg_p_deactive_traces()
hg_p_load_trace(trace_id text)
hg_p_cleanup(trace_id text, remove_active bool)
```

## hg_p_load_trace: temp-table loader

The SQL Profiler API expects a temp table named
`_hg_tmp_tbl_p_profiler` to be available after `hg_p_load_trace`. This
implementation creates that table on demand, truncates it on every call,
and parses every `trace-<pid>.csv` file under
`$PGDATA/hg_profiler/<trace_id>/` into it, adding a `pid` column so
callers can disambiguate per-backend rows.

## GUCs

| GUC | Type | Default | Notes |
| --- | --- | --- | --- |
| `hg_profiler.enabled` | bool | `on` | Master switch; when `off`, `ExecutorEnd` does no profile work. |

All GUCs are `PGC_SUSET` (require superuser/SET to change at runtime).

## Cleanup

Use `hg_p_cleanup()` to remove a finished trace's on-disk data:

```sql
SELECT hg_p_cleanup('20260505190000000001', false);  -- error if still active
SELECT hg_p_cleanup('20260505190000000001', true);   -- stop & remove if active
```

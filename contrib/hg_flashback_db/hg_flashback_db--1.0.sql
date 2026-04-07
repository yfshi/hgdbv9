\echo Use "CREATE EXTENSION hg_flashback_db" to load this file. \quit

CREATE TABLE hg_flashback_db.restore_points_base (
    id          BIGSERIAL   PRIMARY KEY,
    name        TEXT        NOT NULL UNIQUE,
    type        TEXT        NOT NULL CHECK (type IN ('auto', 'manual')),
    lsn         PG_LSN     NOT NULL,
    rsp_time    TIMESTAMPTZ NOT NULL DEFAULT now(),
    backup_label TEXT,
    backup_type TEXT        CHECK (backup_type IS NULL
                                   OR backup_type IN ('full', 'diff', 'incr'))
);

CREATE INDEX restore_points_type_time_idx
    ON hg_flashback_db.restore_points_base (type, rsp_time DESC);

CREATE FUNCTION hg_flashback_db.create_restore_point(
    name TEXT,
    backup_type TEXT DEFAULT NULL
)
RETURNS PG_LSN
AS 'MODULE_PATHNAME', 'hg_flashback_db_create_restore_point'
LANGUAGE C VOLATILE;

CREATE FUNCTION hg_flashback_db.drop_restore_point(name TEXT, skip_backup BOOLEAN DEFAULT false)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'hg_flashback_db_drop_restore_point'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hg_flashback_db.cleanup_orphan_restore_points()
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'hg_flashback_db_cleanup_orphan_restore_points'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hg_flashback_db.pgbackrest_get_total_backup_size()
RETURNS BIGINT
AS 'MODULE_PATHNAME', 'pgbackrest_get_total_backup_size'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hg_flashback_db.pgbackrest_get_avg_backup_size(backup_type TEXT)
RETURNS BIGINT
AS 'MODULE_PATHNAME', 'pgbackrest_get_avg_backup_size'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hg_flashback_db.check_system_requirements()
RETURNS TABLE (check_name TEXT, passed BOOLEAN, message TEXT)
AS $$
    SELECT
        'wal_level'::TEXT,
        current_setting('wal_level') IN ('replica', 'logical'),
        CASE
            WHEN current_setting('wal_level') IN ('replica', 'logical')
            THEN 'wal_level = ' || current_setting('wal_level')
            ELSE 'wal_level must be replica or logical (current: '
                 || current_setting('wal_level') || ')'
        END
    UNION ALL
    SELECT
        'archive_mode'::TEXT,
        current_setting('archive_mode') = 'on',
        CASE
            WHEN current_setting('archive_mode') = 'on'
            THEN 'archive_mode = ' || current_setting('archive_mode')
            ELSE 'archive_mode must be on (current: '
                 || current_setting('archive_mode') || ')'
        END
    UNION ALL
    SELECT
        'archive_command'::TEXT,
        coalesce(current_setting('archive_command'), '') <> '',
        CASE
            WHEN coalesce(current_setting('archive_command'), '') <> ''
            THEN 'archive_command is configured'
            ELSE 'archive_command is not set'
        END;
$$ LANGUAGE SQL STABLE;

REVOKE ALL ON SCHEMA hg_flashback_db FROM PUBLIC;
GRANT USAGE ON SCHEMA hg_flashback_db TO PUBLIC;

REVOKE ALL ON hg_flashback_db.restore_points_base FROM PUBLIC;
GRANT SELECT ON hg_flashback_db.restore_points_base TO PUBLIC;

REVOKE ALL ON FUNCTION hg_flashback_db.create_restore_point(TEXT, TEXT) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_flashback_db.drop_restore_point(TEXT, BOOLEAN) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_flashback_db.cleanup_orphan_restore_points() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_flashback_db.pgbackrest_get_total_backup_size() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_flashback_db.pgbackrest_get_avg_backup_size(TEXT) FROM PUBLIC;

CREATE FUNCTION hg_flashback_db.get_flashback_stats()
RETURNS TABLE (
    oldest_flashback_scn BIGINT,
    oldest_flashback_time TIMESTAMPTZ,
    retention_target BIGINT,
    flashback_size BIGINT,
    estimated_flashback_size BIGINT,
    con_id INTEGER
)
AS $$
DECLARE
    v_oldest_flashback_time TIMESTAMPTZ;
    v_retention_target BIGINT;
    v_logging_interval BIGINT;
    v_flashback_size BIGINT;
    v_avg_full BIGINT;
    v_avg_diff BIGINT;
    v_diff_count BIGINT;
    v_estimated_flashback_size BIGINT;
BEGIN
    SELECT MIN(rsp_time) INTO v_oldest_flashback_time
    FROM hg_flashback_db.restore_points_base;

    BEGIN
        v_retention_target := current_setting('hg_flashback_db.db_flashback_retention_target')::BIGINT;
    EXCEPTION WHEN OTHERS THEN
        v_retention_target := 10080;
    END;

    BEGIN
        v_logging_interval := current_setting('hg_flashback_db.db_flashback_logging_interval')::BIGINT;
    EXCEPTION WHEN OTHERS THEN
        v_logging_interval := 60;
    END;

    BEGIN
        v_flashback_size := hg_flashback_db.pgbackrest_get_total_backup_size();
    EXCEPTION WHEN OTHERS THEN
        v_flashback_size := 0;
    END;

    BEGIN
        v_avg_full := COALESCE(hg_flashback_db.pgbackrest_get_avg_backup_size('full'), 0);
        v_avg_diff := COALESCE(hg_flashback_db.pgbackrest_get_avg_backup_size('diff'), 0);

        IF v_logging_interval > 0 THEN
            v_diff_count := v_retention_target / v_logging_interval;
        ELSE
            v_diff_count := 0;
        END IF;

        v_estimated_flashback_size := v_avg_full + (v_avg_diff * v_diff_count);
    EXCEPTION WHEN OTHERS THEN
        v_estimated_flashback_size := 0;
    END;

    oldest_flashback_scn := NULL;
    oldest_flashback_time := v_oldest_flashback_time;
    retention_target := v_retention_target;
    flashback_size := v_flashback_size;
    estimated_flashback_size := v_estimated_flashback_size;
    con_id := 0;

    RETURN NEXT;
END;
$$ LANGUAGE plpgsql;

CREATE VIEW hg_flashback_db.v$flashback_database_log AS
SELECT
    oldest_flashback_scn,
    oldest_flashback_time,
    retention_target,
    flashback_size,
    estimated_flashback_size,
    con_id
FROM hg_flashback_db.get_flashback_stats();

GRANT SELECT ON hg_flashback_db.v$flashback_database_log TO PUBLIC;

CREATE VIEW hg_flashback_db.show_restore_point AS
SELECT
    NULL::BIGINT AS "SCN",
    NULL::TIMESTAMP AS "RSP Time",
    'GUARANTED'::VARCHAR(10) AS "Type",
    rsp_time AS "Time",
    name AS "Name"
FROM hg_flashback_db.restore_points_base
ORDER BY rsp_time DESC;

GRANT SELECT ON hg_flashback_db.show_restore_point TO PUBLIC;

/* sys.dbms_lock_allocated */
CREATE TABLE sys.dbms_lock_allocated (
    name VARCHAR2(128) NOT NULL,
    lockid INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    expiration TIMESTAMP
);

CREATE INDEX dbms_lock_allocated_name_idx ON sys.dbms_lock_allocated(name);
CREATE INDEX dbms_lock_allocated_expr_idx ON sys.dbms_lock_allocated(expiration);

/* request lock by id */
CREATE OR REPLACE FUNCTION sys.dbmslock_request_by_id(
    id IN integer,
    lockmode IN integer DEFAULT 6,
    timeout IN number DEFAULT 32767,
    release_on_commit IN boolean DEFAULT FALSE
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_request_by_id'
LANGUAGE C;

/* request lock by lockhandle */
CREATE OR REPLACE FUNCTION sys.dbmslock_request_by_lockhandle(
    lockhandle IN varchar2,
    lockmode IN integer DEFAULT 6,
    timeout IN number DEFAULT 32767,
    release_on_commit IN boolean DEFAULT FALSE
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_request_by_lockhandle'
LANGUAGE C;

/* release lock by id */
CREATE OR REPLACE FUNCTION sys.dbmslock_release_by_id(
    id IN integer
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_release_by_id'
LANGUAGE C;

/* release lock by lockhandle */
CREATE OR REPLACE FUNCTION sys.dbmslock_release_by_lockhandle(
    lockhandle IN varchar2
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_release_by_lockhandle'
LANGUAGE C;

/* convert lockmode by id */
CREATE OR REPLACE FUNCTION sys.dbmslock_convert_by_id(
    id IN integer,
    lockmode IN integer,
    timeout IN number DEFAULT 32767
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_convert_by_id'
LANGUAGE C;

/* convert lockmode by lockhandle */
CREATE OR REPLACE FUNCTION sys.dbmslock_convert_by_lockhandle(
    lockhandle IN varchar2,
    lockmode IN integer,
    timeout IN number DEFAULT 32767
) RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_lock_convert_by_lockhandle'
LANGUAGE C;

/* DBMS_LOCK package define */
CREATE OR REPLACE PACKAGE SYS.DBMS_LOCK AUTHID CURRENT_USER AS
    NL_MODE CONSTANT INT := 1;
    SS_MODE CONSTANT INT := 2;
    SX_MODE CONSTANT INT := 3;
    S_MODE CONSTANT INT := 4;
    SSX_MODE CONSTANT INT := 5;
    X_MODE CONSTANT INT := 6;
    MAXWAIT CONSTANT NUMBER := 32767;
    PROCEDURE ALLOCATE_UNIQUE(lockname IN varchar2, lockhandle OUT varchar2, expiration_secs IN integer DEFAULT 864000);
    PROCEDURE ALLOCATE_UNIQUE_AUTONOMOUS(lockname IN varchar2, lockhandle OUT varchar2, expiration_secs IN integer DEFAULT 864000);
    PROCEDURE SLEEP(seconds IN number);
    FUNCTION REQUEST(id IN integer, lockmode IN integer DEFAULT X_MODE, timeout IN number DEFAULT MAXWAIT, release_on_commit IN boolean DEFAULT FALSE) RETURN INTEGER;
    FUNCTION REQUEST(lockhandle IN varchar2, lockmode IN integer DEFAULT X_MODE, timeout IN number DEFAULT MAXWAIT, release_on_commit IN boolean DEFAULT FALSE) RETURN INTEGER;
    FUNCTION RELEASE(id IN integer) RETURN INTEGER;
    FUNCTION RELEASE(lockhandle IN varchar2) RETURN INTEGER;
    FUNCTION CONVERT(id IN integer, lockmode IN integer, timeout IN number DEFAULT MAXWAIT) RETURN INTEGER;
    FUNCTION CONVERT(lockhandle IN varchar2, lockmode IN integer, timeout IN number DEFAULT MAXWAIT) RETURN INTEGER;
END;

/* DBMS_LOCK package body */
CREATE OR REPLACE PACKAGE BODY SYS.DBMS_LOCK IS
    PROCEDURE ALLOCATE_UNIQUE(lockname IN varchar2, lockhandle OUT varchar2, expiration_secs IN integer DEFAULT 864000) IS
    DECLARE
        lock_id INTEGER;
    BEGIN
        IF LENGTH(lockname) > 128 THEN
            RAISE EXCEPTION 'value too large for column "SYS.DBMS_LOCK_ALLOCATED.NAME" (actual: %, maximum: 128)', LENGTH(lockname);
        END IF;

        IF EXISTS (SELECT 1 FROM sys.dbms_lock_allocated WHERE name = lockname) THEN
            EXECUTE 'UPDATE sys.dbms_lock_allocated SET expiration = (clock_timestamp() + (interval ''1'' second * $1)) WHERE name = $2' USING expiration_secs, lockname;
        ELSE
            EXECUTE 'INSERT INTO sys.dbms_lock_allocated(name, expiration) values($1, (clock_timestamp() + (interval ''1'' second * $2)))' USING lockname, expiration_secs;
        END IF;

        DELETE FROM sys.dbms_lock_allocated WHERE expiration < clock_timestamp()  AND xmin::TEXT::BIGINT <> txid_current()
            AND lockid NOT IN (SELECT objid FROM pg_locks WHERE locktype = 'dbmslock');
        SELECT lockid INTO lock_id FROM sys.dbms_lock_allocated WHERE name = lockname;
        lockhandle := (LPAD(TO_HEX(lock_id), 8, '0') || LPAD(TO_HEX(lock_id), 8, '0'))::VARCHAR2;
        COMMIT;
    END;

    PROCEDURE ALLOCATE_UNIQUE_AUTONOMOUS(lockname IN varchar2, lockhandle OUT varchar2, expiration_secs IN integer DEFAULT 864000) IS
    DECLARE
        PRAGMA AUTONOMOUS_TRANSACTION;
        lock_id INTEGER;
    BEGIN
        IF LENGTH(lockname) > 128 THEN
            RAISE EXCEPTION 'value too large for column "SYS.DBMS_LOCK_ALLOCATED.NAME" (actual: %, maximum: 128)', LENGTH(lockname);
        END IF;

        IF EXISTS (SELECT 1 FROM sys.dbms_lock_allocated WHERE name = lockname) THEN
            EXECUTE 'UPDATE sys.dbms_lock_allocated SET expiration = (clock_timestamp() + (interval ''1'' second * $1)) WHERE name = $2' USING expiration_secs, lockname;
        ELSE
            EXECUTE 'INSERT INTO sys.dbms_lock_allocated(name, expiration) values($1, (clock_timestamp() + (interval ''1'' second * $2)))' USING lockname, expiration_secs;
        END IF;

        DELETE FROM sys.dbms_lock_allocated WHERE expiration < clock_timestamp()  AND xmin::TEXT::BIGINT <> txid_current()
            AND lockid NOT IN (SELECT objid FROM pg_locks WHERE locktype = 'dbmslock');
        SELECT lockid INTO lock_id FROM sys.dbms_lock_allocated WHERE name = lockname;
        lockhandle := (LPAD(TO_HEX(lock_id), 8, '0') || LPAD(TO_HEX(lock_id), 8, '0'))::VARCHAR2;
        COMMIT;
    END;

    PROCEDURE SLEEP(seconds IN number) IS
    BEGIN
        IF seconds IS NULL OR seconds < 0 OR seconds > 2147483647 THEN		-- BUG #z7617
            RAISE EXCEPTION 'invalid time limit specified';
        END IF;
        PERFORM PG_SLEEP(seconds);
    END;

    FUNCTION REQUEST(id IN integer, lockmode IN integer DEFAULT X_MODE, timeout IN number DEFAULT MAXWAIT, release_on_commit IN boolean DEFAULT FALSE)
    RETURN INTEGER IS
    BEGIN
        IF id IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_request_by_id(
            id => id,
            lockmode => lockmode,
            timeout => timeout,
            release_on_commit => release_on_commit);
    END;

    FUNCTION REQUEST(lockhandle IN varchar2, lockmode IN integer DEFAULT X_MODE, timeout IN number DEFAULT MAXWAIT, release_on_commit IN boolean DEFAULT FALSE)
    RETURN INTEGER IS
    BEGIN
        IF lockhandle IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_request_by_lockhandle(
            lockhandle => lockhandle,
            lockmode => lockmode,
            timeout => timeout,
            release_on_commit => release_on_commit);
    END;

    FUNCTION RELEASE(id IN integer) RETURN INTEGER IS
    BEGIN
        IF id IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_release_by_id(id => id);
    END;

    FUNCTION RELEASE(lockhandle IN varchar2) RETURN INTEGER IS
    BEGIN
        IF lockhandle IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_release_by_lockhandle(lockhandle => lockhandle);
    END;

    FUNCTION CONVERT(id IN integer, lockmode IN integer, timeout IN number DEFAULT MAXWAIT) RETURN INTEGER IS
    BEGIN
        IF id IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_convert_by_id(
            id => id,
            lockmode => lockmode,
            timeout => timeout);
    END;

    FUNCTION CONVERT(lockhandle IN varchar2, lockmode IN integer, timeout IN number DEFAULT MAXWAIT) RETURN INTEGER IS
    BEGIN
        IF lockhandle IS NULL THEN
            RETURN 3;
        END IF;
        RETURN sys.dbmslock_convert_by_lockhandle(
            lockhandle => lockhandle,
            lockmode => lockmode,
            timeout => timeout);
    END;
END;

GRANT SELECT ON SYS.DBMS_LOCK_ALLOCATED TO PUBLIC;
GRANT INSERT ON SYS.DBMS_LOCK_ALLOCATED TO PUBLIC;
GRANT DELETE ON SYS.DBMS_LOCK_ALLOCATED TO PUBLIC;
GRANT UPDATE ON SYS.DBMS_LOCK_ALLOCATED TO PUBLIC;
GRANT EXECUTE ON PACKAGE SYS.DBMS_LOCK TO PUBLIC;

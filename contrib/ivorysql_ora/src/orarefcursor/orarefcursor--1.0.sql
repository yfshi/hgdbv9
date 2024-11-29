/***************************************************************
 *
 * oracle cursor type (for portal name and address) support
 *
 ***************************************************************/
/* io Functions */
CREATE FUNCTION sys.orarefcursorin(cstring)
RETURNS sys.orarefcursor
AS 'MODULE_PATHNAME','orarefcursorin'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.orarefcursorout(sys.orarefcursor)
RETURNS CSTRING
AS 'MODULE_PATHNAME','orarefcursorout'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.orarefcursorrecv(internal)
RETURNS sys.orarefcursor
AS 'MODULE_PATHNAME','orarefcursorrecv'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.orarefcursorsend(sys.orarefcursor)
RETURNS bytea
AS 'MODULE_PATHNAME','orarefcursorsend'
LANGUAGE C
STRICT
STABLE;


CREATE FUNCTION sys.sysrefcursorin(cstring)
RETURNS sys.sys_refcursor
AS 'MODULE_PATHNAME','sysrefcursorin'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.sysrefcursorout(sys.sys_refcursor)
RETURNS CSTRING
AS 'MODULE_PATHNAME','sysrefcursorout'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.sysrefcursorrecv(internal)
RETURNS sys.sys_refcursor
AS 'MODULE_PATHNAME','sysrefcursorrecv'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.sysrefcursorsend(sys.sys_refcursor)
RETURNS bytea
AS 'MODULE_PATHNAME','sysrefcursorsend'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.orarefcursor_init()
RETURN void
AS 'MODULE_PATHNAME', 'orarefcursor_init'
LANGUAGE C
IMMUTABLE;

/* update pg_type for io functions */
UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname = 'orarefcursorin')
WHERE oid = (select oid from pg_type where typname='orarefcursor');

UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname = 'orarefcursorout')
WHERE oid = (select oid from pg_type where typname='orarefcursor');

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname = 'orarefcursorrecv')
WHERE oid = (select oid from pg_type where typname='orarefcursor');

UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname = 'orarefcursorsend')
WHERE oid = (select oid from pg_type where typname='orarefcursor');


UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname = 'sysrefcursorin')
WHERE oid = (select oid from pg_type where typname='sys_refcursor');

UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname = 'sysrefcursorout')
WHERE oid = (select oid from pg_type where typname='sys_refcursor');

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname = 'sysrefcursorrecv')
WHERE oid = (select oid from pg_type where typname='sys_refcursor');

UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname = 'sysrefcursorsend')
WHERE oid = (select oid from pg_type where typname='sys_refcursor');

/***************************************************************
 *
 * because we realize oracle cursor type using two kinds of type
 * orarefcursor and sys_refcursor which contol by GUC
 * ivorysql.check_strong_cursor_assign so we should support
 * two kinds of type exchange
 *
 ***************************************************************/
--exchange for sys_refcursor and orarefcursor
--convert orarefcursor to sys_refcursor
CREATE FUNCTION sys.oracursortosyscursor(sys.orarefcursor)
RETURNS sys.sys_refcursor
AS 'MODULE_PATHNAME','oracursor_syscursor'
LANGUAGE C
STRICT
IMMUTABLE;

--convert sys_refcursor to orarefcursor
CREATE FUNCTION sys.syscursortooracursor(sys.sys_refcursor)
RETURNS sys.orarefcursor
AS 'MODULE_PATHNAME','syscursor_oracursor'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE CAST (sys.orarefcursor AS sys.sys_refcursor )
WITH FUNCTION sys.oracursortosyscursor(sys.orarefcursor)
AS IMPLICIT;

CREATE CAST (sys.sys_refcursor AS sys.orarefcursor)
WITH FUNCTION sys.syscursortooracursor(sys.sys_refcursor)
AS IMPLICIT;


CREATE FUNCTION hg_alarm_check(cfpath cstring, cfname cstring)
RETURNS bool
AS 'MODULE_PATHNAME', 'hg_alarm_check'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hg_alarm_check(cfpath cstring)
RETURNS bool
AS 'MODULE_PATHNAME', 'hg_alarm_check'
LANGUAGE C VOLATILE STRICT;

CREATE OR REPLACE FUNCTION hg_alarm_stat (
    IN  cfpath cstring,
    OUT opname cstring,
    OUT opcur cstring,
    OUT oplimit cstring)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'hg_alarm_stat'
LANGUAGE C IMMUTABLE STRICT;
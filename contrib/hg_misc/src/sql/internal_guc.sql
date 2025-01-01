-- Begin - ReqID:SRS-Performance-GUC
CREATE FUNCTION internal_set_clog(int4)
RETURNS text
AS 'MODULE_PATHNAME', 'internal_set_clog'
LANGUAGE C STRICT;

CREATE FUNCTION internal_set_prune_interval(int4)
RETURNS text
AS 'MODULE_PATHNAME', 'internal_set_prune_interval'
LANGUAGE C STRICT;

CREATE FUNCTION internal_set_perf(bool, int4, int4, int4)
RETURNS text
AS 'MODULE_PATHNAME', 'internal_set_perf'
LANGUAGE C STRICT;
-- End - ReqID:SRS-Performance-GUC
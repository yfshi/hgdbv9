/* Begin - ReqID:SRS-SQL-TRIGGER */
CREATE FUNCTION sys.inserting()
RETURNS boolean
LANGUAGE c
STABLE
AS 'MODULE_PATHNAME', 'inserting';

CREATE FUNCTION sys.deleting()
RETURNS boolean
LANGUAGE c
STABLE
AS 'MODULE_PATHNAME', 'deleting';

CREATE FUNCTION sys.updating()
RETURNS boolean
LANGUAGE c
STABLE
AS 'MODULE_PATHNAME', 'updating';

CREATE FUNCTION sys.updating(text)
RETURNS boolean
LANGUAGE c
STABLE
AS 'MODULE_PATHNAME', 'updating_col';
/* End - ReqID:SRS-SQL-TRIGGER */

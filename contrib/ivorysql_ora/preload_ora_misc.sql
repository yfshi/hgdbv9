/* Begin - BUG#M0000195 */
/* Begin - ReqID:SRS-SQL-SEQ */
GRANT USAGE ON SCHEMA sys TO PUBLIC;
SET search_path TO sys;

CREATE table dual (DUMMY pg_catalog.bpchar(1));
insert into dual values('X');
GRANT SELECT ON dual TO PUBLIC;
/* End - ReqID:SRS-SQL-SEQ */

--
-- ROWID type
--
CREATE TYPE sys.rowid AS(rowoid OID, rowno bigint);
CREATE TYPE sys.urowid AS(rowoid OID, rowno bigint);

CREATE CAST (sys.rowid AS sys.urowid) WITH INOUT AS IMPLICIT;
CREATE CAST (sys.urowid AS sys.rowid) WITH INOUT AS IMPLICIT;
/* End - BUG#M0000195 */

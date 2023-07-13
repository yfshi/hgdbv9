/* Begin - ReqID:SRS-SQL-SEQ */
GRANT USAGE ON SCHEMA sys TO PUBLIC;
SET search_path TO sys;

CREATE table dual (DUMMY pg_catalog.bpchar(1));
insert into dual values('X');
GRANT SELECT ON dual TO PUBLIC;
/* End - ReqID:SRS-SQL-SEQ */

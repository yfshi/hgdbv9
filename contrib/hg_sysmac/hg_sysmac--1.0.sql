/* contrib/hg_sysmac/hg_sysmac--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_sysmac" to load this file. \quit

CREATE TYPE seclabel;

-- type convert

CREATE FUNCTION seclabel_in(cstring)
RETURNS seclabel
AS 'MODULE_PATHNAME', 'seclabel_in'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION seclabel_out(seclabel)
RETURNS cstring
AS 'MODULE_PATHNAME', 'seclabel_out'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- The type itself.

CREATE TYPE seclabel (
    INPUT          = seclabel_in,
    OUTPUT         = seclabel_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE        = extended,
    -- make it a non-preferred member of string type category
    CATEGORY       = 'S',
    PREFERRED      = false,
    COLLATABLE     = true
);

CREATE CAST (text AS seclabel) WITH inout AS IMPLICIT;

CREATE FUNCTION seclabel_eq( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_eq'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION seclabel_ne( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_ne'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION seclabel_gt( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_gt'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION seclabel_lt( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_lt'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION seclabel_ge( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_ge'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION seclabel_le( seclabel, seclabel )
RETURNS bool
AS 'MODULE_PATHNAME', 'seclabel_le'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

--
-- Operators.
--

CREATE OPERATOR = (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    COMMUTATOR = =,
    NEGATOR    = <>,
    PROCEDURE  = seclabel_eq,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel
);

CREATE OPERATOR <> (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    NEGATOR    = =,
    COMMUTATOR = <>,
    PROCEDURE  = seclabel_ne,
    RESTRICT   = neqsel,
    JOIN       = neqjoinsel
);

CREATE OPERATOR < (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    NEGATOR    = >=,
    COMMUTATOR = >,
    PROCEDURE  = seclabel_lt,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

CREATE OPERATOR > (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    NEGATOR    = <=,
    COMMUTATOR = <,
    PROCEDURE  = seclabel_gt,
    RESTRICT   = scalargtsel,
    JOIN       = scalargtjoinsel
);

CREATE OPERATOR >= (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    NEGATOR    = <,
    COMMUTATOR = <=,
    PROCEDURE  = seclabel_ge,
    RESTRICT   = scalargtsel,
    JOIN       = scalargtjoinsel
);

CREATE OPERATOR <= (
    LEFTARG    = seclabel,
    RIGHTARG   = seclabel,
    NEGATOR    = >,
    COMMUTATOR = >=,
    PROCEDURE  = seclabel_le,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

-- old style interface

CREATE FUNCTION compare_seclabel(seclabel,seclabel)
RETURNS int
AS 'MODULE_PATHNAME', 'seclabel_cmp'
LANGUAGE C IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION compare_seclabel OWNER TO syssso; /* BUG#Z7896 */

-- row-level function

CREATE FUNCTION mac_row_user_seclabel()
RETURNS text
AS 'MODULE_PATHNAME', 'mac_row_user_seclabel'
LANGUAGE C VOLATILE  STRICT PARALLEL SAFE;

CREATE FUNCTION mac_row_select_policy(seclabel)
RETURNS bool
AS 'MODULE_PATHNAME', 'mac_row_select_policy'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION mac_row_delete_policy(seclabel)
RETURNS bool
AS 'MODULE_PATHNAME', 'mac_row_delete_policy'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION mac_row_update_policy(seclabel)
RETURNS bool
AS 'MODULE_PATHNAME', 'mac_row_update_policy'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION mac_row_insert_policy(Oid)
RETURNS bool
AS 'MODULE_PATHNAME', 'mac_row_insert_policy'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

--  unregister label provider for test

CREATE FUNCTION unregister_seclabel_provider(text)
RETURNS void
AS 'MODULE_PATHNAME', 'unregister_seclabel_provider'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

--  code for get/set.

CREATE FUNCTION get_user_seclabel(oid)
RETURNS text
AS 'MODULE_PATHNAME', 'get_user_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION get_user_seclabel OWNER TO syssso; /* BUG#Z7896 */

CREATE FUNCTION set_user_seclabel(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_user_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION set_user_seclabel OWNER TO syssso; /* BUG#Z7896 */

CREATE FUNCTION get_table_seclabel(oid)
RETURNS text
AS 'MODULE_PATHNAME', 'get_table_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION get_table_seclabel OWNER TO syssso; /* BUG#Z7896 */

CREATE FUNCTION set_table_seclabel(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_table_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION set_table_seclabel OWNER TO syssso; /* BUG#Z7896 */

CREATE FUNCTION get_column_seclabel(oid, int4)
RETURNS text
AS 'MODULE_PATHNAME', 'get_column_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION get_column_seclabel OWNER TO syssso; /* BUG#Z7896 */

CREATE FUNCTION set_column_seclabel(oid, int4, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_column_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;
ALTER FUNCTION set_column_seclabel OWNER TO syssso; /* BUG#Z7896 */

--  policy set functions

CREATE FUNCTION apply_row_policy(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'apply_row_policy'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
ALTER FUNCTION apply_row_policy(oid, text) OWNER TO syssso; /* BUG#Z7896 */

/*
 * parameter:
 * 		(table_name, action, is_dump)
 */
CREATE FUNCTION apply_row_policy(oid, text, bool)
RETURNS text
AS 'MODULE_PATHNAME', 'apply_row_policy'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
ALTER FUNCTION apply_row_policy(oid, text, bool) OWNER TO syssso; /* BUG#Z7896 */

/*
 * Create Mandatory Access Control (MAC) row-level policies:
 *	 - SELECT: user_seclabel >= row_seclabel
 *	 - UPDATE: user_seclabel = row_seclabel
 *	 - DELETE: user_seclabel = row_seclabel
 *	 - INSERT: table_seclabel >= user_seclabel
 *
 * When `is_dump` is true, it indicates that the current operation is pg_dump.
 * Since pg_dump automatically exports policies, this function does not need
 * to create the policy again.
 */
create or replace function apply_table_policy(schema_name text, table_name text, is_partition bool, is_dump bool) returns void as $$
declare
    sql text;
begin
    IF NOT is_partition THEN
        /* Begin - Bug #5996 */
        sql = 'alter table '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) ' add column if not exists __hg_seclabel__ hg_security.seclabel';
        execute sql;
        sql = 'alter table '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) 'alter column __hg_seclabel__ set default (hg_security.mac_row_user_seclabel())';
        execute sql;
        /* End - Bug #5996 */
    END IF;
    IF NOT is_dump THEN
        sql = 'create policy "'
              operator(pg_catalog.||) '__Policysel'
              operator(pg_catalog.||) schema_name
              operator(pg_catalog.||) table_name
              operator(pg_catalog.||) '__" on '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||)replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) ' for select using (hg_security.mac_row_select_policy(__hg_seclabel__))';
        execute sql;
        sql = 'create policy "'
              operator(pg_catalog.||) '__Policyins'
              operator(pg_catalog.||) schema_name
              operator(pg_catalog.||) table_name
              operator(pg_catalog.||) '__" on '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) ' for insert with check (hg_security.mac_row_insert_policy(('''
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              ''')::regclass))';
        execute sql;
        sql = 'create policy "'
              operator(pg_catalog.||) '__Policydel'
              operator(pg_catalog.||) schema_name
              operator(pg_catalog.||) table_name
              operator(pg_catalog.||) '__" on '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) ' for delete using (hg_security.mac_row_delete_policy(__hg_seclabel__))';
        execute sql;
        sql = 'create policy "'
              operator(pg_catalog.||) '__Policyupd'
              operator(pg_catalog.||) schema_name
              operator(pg_catalog.||) table_name
              operator(pg_catalog.||) '__" on '
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) replace(schema_name, '"', '""')
              operator(pg_catalog.||) '"."'
              operator(pg_catalog.||) replace(table_name, '"', '""')
              operator(pg_catalog.||) '"'
              operator(pg_catalog.||) ' for update using (hg_security.mac_row_update_policy(__hg_seclabel__))';
        execute sql;
    END IF;
end;
$$ language plpgsql
security invoker;


create or replace function enable_table_policy(schema_name text, table_name text) returns void as $$
declare
    sql text;
begin
    sql = 'alter table '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) ' FORCE ROW LEVEL SECURITY ';
    execute sql;
    sql = 'alter table '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) ' ENABLE ROW LEVEL SECURITY';
    execute sql;
end;
$$ language plpgsql
security invoker;

create or replace function disable_table_policy(schema_name text, table_name text) returns void as $$
declare
    sql text;
begin
    sql = 'alter table '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) ' no FORCE ROW LEVEL SECURITY ';
    execute sql;
    sql = 'alter table '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) ' disable ROW LEVEL SECURITY';
    execute sql;
end;
$$ language plpgsql
security invoker;

create or replace function drop_table_policy(schema_name text, table_name text) returns void as $$
declare
    sql text;
begin
    sql = 'drop policy "'
          operator(pg_catalog.||) '__Policysel'
          operator(pg_catalog.||) schema_name
          operator(pg_catalog.||) table_name
          operator(pg_catalog.||) '__" on '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"';
    execute sql;
    sql = 'drop policy "'
          operator(pg_catalog.||) '__Policyins'
          operator(pg_catalog.||) schema_name
          operator(pg_catalog.||) table_name
          operator(pg_catalog.||) '__" on '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"';
    execute sql;
    sql = 'drop policy "'
          operator(pg_catalog.||) '__Policydel'
          operator(pg_catalog.||) schema_name
          operator(pg_catalog.||) table_name
          operator(pg_catalog.||) '__" on '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"';
    execute sql;
    sql = 'drop policy "'
          operator(pg_catalog.||) '__Policyupd'
          operator(pg_catalog.||) schema_name
          operator(pg_catalog.||) table_name
          operator(pg_catalog.||) '__" on '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"';
    execute sql;
    sql = 'alter table '
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) replace(schema_name, '"', '""')
          operator(pg_catalog.||) '"."'
          operator(pg_catalog.||) replace(table_name, '"', '""')
          operator(pg_catalog.||) '"'
          operator(pg_catalog.||) ' drop column __hg_seclabel__ cascade';
    execute sql;
end;
$$ language plpgsql
security invoker;

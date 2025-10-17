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

CREATE FUNCTION set_user_seclabel(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_user_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION get_table_seclabel(oid)
RETURNS text
AS 'MODULE_PATHNAME', 'get_table_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION set_table_seclabel(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_table_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION get_column_seclabel(oid, int4)
RETURNS text
AS 'MODULE_PATHNAME', 'get_column_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION set_column_seclabel(oid, int4, text)
RETURNS text
AS 'MODULE_PATHNAME', 'set_column_seclabel'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

--  policy set functions

CREATE FUNCTION apply_row_policy(oid, text)
RETURNS text
AS 'MODULE_PATHNAME', 'apply_row_policy'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*
 * Create Mandatory Access Control (MAC) row-level policies:
 *	 - SELECT: user_seclabel >= row_seclabel
 *	 - UPDATE: user_seclabel = row_seclabel
 *	 - DELETE: user_seclabel = row_seclabel
 *	 - INSERT: table_seclabel >= user_seclabel
 */
create or replace function apply_table_policy(table_name text, is_partition bool) returns void as $$
declare
    sql text;
begin
    IF NOT is_partition THEN
        sql='alter table '||table_name||' add column if not exists __hg_seclabel__ seclabel default (mac_row_user_seclabel())';
        execute sql;
    END IF;
    sql='create policy policysel'||table_name||' on '||table_name||' for select using (mac_row_select_policy(__hg_seclabel__))';
    execute sql;
    sql='create policy policyins'||table_name||' on '||table_name||' for insert with check (mac_row_insert_policy(('''||table_name||''')::regclass))';
    execute sql;
    sql='create policy policydel'||table_name||' on '||table_name||' for delete using (mac_row_delete_policy(__hg_seclabel__))';
    execute sql;
    sql='create policy policyupd'||table_name||' on '||table_name||' for update using (mac_row_update_policy(__hg_seclabel__))';
    execute sql;
end;
$$ language plpgsql
security invoker;

create or replace function enable_table_policy(table_name text) returns void as $$
declare
    sql text;
begin
    sql='alter table '||table_name||' FORCE ROW LEVEL SECURITY ';
    execute sql;
    sql='alter table '||table_name||' ENABLE ROW LEVEL SECURITY';
    execute sql;
end;
$$ language plpgsql
security invoker;

create or replace function disable_table_policy(table_name text) returns void as $$
declare
    sql text;
begin
    sql='alter table '||table_name||' no FORCE ROW LEVEL SECURITY ';
    execute sql;
    sql='alter table '||table_name||' disable ROW LEVEL SECURITY';
    execute sql;
end;
$$ language plpgsql
security invoker;

create or replace function drop_table_policy(table_name text) returns void as $$
declare
    sql text;
begin
    sql='drop policy policysel'||table_name||' on '||table_name;
    execute sql;
    sql='drop policy policyins'||table_name||' on '||table_name;
    execute sql;
    sql='drop policy policydel'||table_name||' on '||table_name;
    execute sql;
    sql='drop policy policyupd'||table_name||' on '||table_name;
    execute sql;
    sql='alter table '||table_name||' drop column __hg_seclabel__ cascade';
    execute sql;
end;
$$ language plpgsql
security invoker;

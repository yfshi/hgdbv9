/***************************************************************
 *
 * MySQL set support function
 *
 ***************************************************************/
create or replace function ivymy.mys_set_user_var(variable_name varchar(64), variable_value pg_catalog.anycompatible)
returns void
as 'MODULE_PATHNAME', 'mysSetUserVar'
LANGUAGE C PARALLEL SAFE;

create or replace function ivymy.mys_set_global_variable(variable_name varchar(64), variable_value pg_catalog.anycompatible, defaultval bool)
returns void
as 'MODULE_PATHNAME', 'setSystemVariable'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

create or replace function ivymy.mys_set_session_variable(variable_name varchar(64), variable_value pg_catalog.anycompatible, defaultval bool)
returns void
as 'MODULE_PATHNAME', 'setSessionVariable'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

create or replace function ivymy.mys_set_local_variable(variable_name varchar(64), variable_value pg_catalog.anycompatible, defaultval bool)
returns void
as 'MODULE_PATHNAME', 'setLocalVariable'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

create or replace function ivymy.mys_set_names(variable_name varchar(64), defaultval bool)
returns void
as 'MODULE_PATHNAME', 'setnames'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

create or replace function ivymy.mys_set_charset(variable_name varchar(64), defaultval bool)
returns void
as 'MODULE_PATHNAME', 'setcharset'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

create or replace function ivymy.mys_set_user_var_in_expr(variable_name varchar(64), variable_value pg_catalog.anycompatible)
returns pg_catalog.anycompatible
as 'MODULE_PATHNAME', 'mysSetUserVar'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION user_variable_by_thread(
	OUT THREAD_ID INT,
	OUT VARIABLE_NAME VARCHAR(64),
	OUT VARIABLE_VALUE VARCHAR(1024))
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'return_all_user_variable'
LANGUAGE C PARALLEL SAFE;

CREATE VIEW performance_schema.user_variable_by_thread AS
		select THREAD_ID, VARIABLE_NAME, VARIABLE_VALUE
		from user_variable_by_thread();

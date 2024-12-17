/*
 * oracle plsql exception map ivorysql plisql
 */

-- create mapping table
CREATE TABLE sys._ivorysql_ora_exception_map
(
	PgSqlCode varchar2(5) not NULL COLLATE C,
	OraErrorCode integer NOT NULL,
	Message TEXT COLLATE C
);

--create index
CREATE INDEX _ivorysql_ora_exception_map_pgsqlcode on sys._ivorysql_ora_exception_map using hash(PgSqlCode);
CREATE INDEX _ivorysql_ora_exception_map_oracode on sys._ivorysql_ora_exception_map using hash(OraErrorCode);

--load mapping data into table
--insert oracle predefined exception
--access_into_null
INSERT INTO sys._ivorysql_ora_exception_map values('42704', -6530, 'IVY-06530: Reference to uninitialized composite');

--case_not_found
INSERT INTO sys._ivorysql_ora_exception_map values('20000', -6592, 'IVY-06592: CASE not found while executing CASE statement');

--collection_is_null
INSERT INTO sys._ivorysql_ora_exception_map values('22000', -6531, 'IVY-06531: Reference to uninitialized collection');

--cursor_already_open
INSERT INTO sys._ivorysql_ora_exception_map values('42P03', -6511, 'IVY-06511: PL/iSQL: cursor already open');

--dup_val_on_index
INSERT INTO sys._ivorysql_ora_exception_map values('23505',-1, 'IVY-00001: unique constraint (string.string) violated');

--invalid cursor
INSERT INTO sys._ivorysql_ora_exception_map values('34000', -1001, 'IVY-01001: invalid cursor');

--invalid_number --not found
INSERT INTO sys._ivorysql_ora_exception_map values('22P02', -1722, 'IVY-01722: invalid number');

--log_denied
INSERT INTO sys._ivorysql_ora_exception_map values('08001', -1017, 'IVY-01017: login denied');

--no_data_found
INSERT INTO sys._ivorysql_ora_exception_map values('P0002', 100, 'IVY-01403: no data found');

--1403
INSERT INTO sys._ivorysql_ora_exception_map values('P0002', -1403, 'IVY-01403: no data found');

--no_data_needed --not found

--not_logged_on --not found
INSERT INTO sys._ivorysql_ora_exception_map values('08003', -1012, 'IVY-01012: not logged on');

--program_error -- not found

--rowtype_mistmtach
INSERT INTO sys._ivorysql_ora_exception_map values('42804', -6504, 'IVY-06504: Return types of Result Set variables or query do not match');

--self_is_null -- not found

--storage_error -- not found

--SUBSCRIPT_BEYOND_COUNT --not found

--SUBSCRIPT_OUTSIDE_LIMIT --not found

--SYS_INVALID_ROWID -- not found

--TIMEOUT_ON_RESOURCE -- not found

--TOO_MANY_ROWS
INSERT INTO sys._ivorysql_ora_exception_map values('P0003', -1422, 'IVY-01422: exact fetch returns more than requested number of rows');

--VALUE_ERROR -6502
INSERT INTO sys._ivorysql_ora_exception_map values('22000', -6502, 'IVY-06502: PL/SQL: numeric or value error: character string buffer too small');


--ZERO_DIVIDE
INSERT INTO sys._ivorysql_ora_exception_map values('22012', -1476, 'IVY-01476: divisor is equal to zero');

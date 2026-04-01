--
-- Regression tests for Oracle dbms_rowid package
--

-- ROWID_CREATE
SELECT DBMS_ROWID.rowid_create(1, 333, 1, 2, 1000);
SELECT DBMS_ROWID.rowid_create(NULL, 333, 1, 2, 1000);--check invalid rowid_type
SELECT DBMS_ROWID.rowid_create(-100, 333, 1, 2, 1000);--check invalid rowid_type
SELECT DBMS_ROWID.rowid_create(1.5, 333, 1, 2, 1000);--check invalid rowid_type

--ROWID_INFO
declare
	rowid_type number:=1;
	object_number number:=1;
	relative_fno number:=1;
	block_number number:=1;
	row_number number:=1;
begin
	DBMS_ROWID.rowid_info((333,11), rowid_type, object_number, relative_fno, block_number, row_number);
	raise info '% % % % %',rowid_type, object_number, relative_fno, block_number, row_number;
end;
/

--ROWID_BLOCK_NUMBER
SELECT dbms_rowid.rowid_block_number((32811,1)) FROM DUAL;
create table test_rowid_tab(id int, name text)with rowid;
insert into test_rowid_tab values (1, 'aa');

declare
	v_valid_rowid rowid;
	v_block_num number;
begin
	select rowid into v_valid_rowid from test_rowid_tab where id = 1;
	v_block_num := dbms_rowid.rowid_block_number(v_valid_rowid);
	dbms_output.put_line(v_block_num);
end;
/
drop table test_rowid_tab;
select dbms_rowid.rowid_block_number((33283,1));--invalid rowid,block return -1
select dbms_rowid.rowid_block_number(NULL);-- return 0
select dbms_rowid.rowid_block_number('');-- return 0

--ROWID_TO_absolute_fno
SELECT DBMS_ROWID.ROWID_TO_absolute_fno((32811,1), 'PUBLIC','TEST') FROM DUAL;

--ROWID_TO_EXTENDED
SELECT DBMS_ROWID.ROWID_TO_EXTENDED((32811,1), NULL,NULL,0) FROM DUAL;

--ROWID_TO_RESTRICTED
SELECT DBMS_ROWID.ROWID_TO_RESTRICTED((32811,1), 0) FROM DUAL;

--ROWID_VERIFY
SELECT DBMS_ROWID.ROWID_VERIFY((32811,1), NULL,NULL,0) FROM DUAL;

--ROWID_TYPE
SELECT DBMS_ROWID.rowid_type((32811,1)) FROM DUAL;

--ROWID_OBJECT
SELECT DBMS_ROWID.rowid_object((32811,1)) FROM DUAL;

--ROWID_ROWID_RELATIVE_FNO
SELECT DBMS_ROWID.rowid_relative_fno((32811,1)) FROM DUAL;

--test case conversion
SET ivorysql.identifier_case_switch = interchange;
create table public.test(id int) with rowid;
insert into public.test values (1);
declare
	rowi_id rowid;
	fno_number number;
begin
	select rowid into rowi_id from public.test where id = 1;
	fno_number:= dbms_rowid.rowid_to_absolute_fno(rowi_id,'PUBLIC','TEST') from dual;
	raise info 'get ralative fno number succes!';
end;
/
DROP TABLE public.test;

create table public."BB"(id int)with rowid;
INSERT INTO public."BB" values (1);
declare
	rowi_id rowid;
	fno_number number;
begin
	select rowid into rowi_id from public."BB" where id = 1;
	fno_number:= dbms_rowid.rowid_to_absolute_fno(rowi_id,'PUBLIC','BB') from dual;
	raise info 'get ralative fno number succes!';
end;
/
drop table public."BB";

create table public."bb"(id int)with rowid;
INSERT INTO "bb" values (1);
declare
	rowi_id rowid;
	fno_number number;
begin
	select rowid into rowi_id from public."bb" where id = 1;
	fno_number:= dbms_rowid.rowid_to_absolute_fno(rowi_id,'PUBLIC','bb') from dual;
	raise info 'get ralative fno number succes!';
end;
/
drop table public."bb";
reset ivorysql.identifier_case_switch;

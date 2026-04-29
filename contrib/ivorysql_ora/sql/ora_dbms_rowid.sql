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

--Bgein bug#Z7665, all these param expected:invalid rowid
set serveroutput on;
declare
	type num_tab is table of number;
	v_obj_nums num_tab := num_tab(4294967296, -100,123.4,null,'');
	v_rowid rowid;
begin
	for i in 1..v_obj_nums.count loop
	begin
		v_rowid := dbms_rowid.rowid_create(
			rowid_type => 1,
			object_number => v_obj_nums(i),
			relative_fno => 200,
			block_number => 100,
			row_number => 5
		);
	dbms_output.put_line('rowid' || i || ': ' || v_rowid || ' (object_number=' || v_obj_nums(i) || ')');
	exception
	when others then
		dbms_output.put_line('invalid rowid');
	end;
	end loop;
end;
/
--End bug#Z7665

--Bgein bug#Z7666, first is ok,remaining export error
declare
	type num_tab is table of number;
	row_num num_tab := num_tab(0,-100,null,'');
	v_rowid rowid;
begin
	for i in 1..row_num.count loop
	begin
		v_rowid := dbms_rowid.rowid_create(
			rowid_type => 1,
			object_number => 33256,
			relative_fno => 100,
			block_number => 200,
			row_number => row_num(i));
		dbms_output.put_line('success: ' || v_rowid);
		exception
		when others then
			dbms_output.put_line(sqlerrm);
	end;
	end loop;
end;
/
--End bug#Z7666

--Bgein bug#Z7667, export invalid ROWID
declare
	v_rowid0 rowid;
	v_rowid1 rowid;
begin
	v_rowid0 := dbms_rowid.rowid_create(1,'',100,200,'');
	v_rowid1 := dbms_rowid.rowid_create(1,NULL,100,200,NULL);
	dbms_output.put_line(v_rowid0);
	dbms_output.put_line(v_rowid1);
end;
/

declare
	v_rowid0 rowid;
	v_rowid1 rowid;
begin
	v_rowid0 := dbms_rowid.rowid_create(NULL,33256,NULL,NULL,10);
	v_rowid1 := dbms_rowid.rowid_create('',33256,'','',10);
end;
/
--End bug#Z7667

--Bgein bug#Z7668, expect: invalid ROWID
create table  test_rowid_tab_1 (id  int, name text) with rowid;
insert into   test_rowid_tab_1 values(1, 'aaa');
create table  test_rowid_tab_2 (id number) with rowid;
insert into  test_rowid_tab_2 values (1);

declare
	v_old_rowid    rowid; --table test_rowid_tab_1's rowid
	v_schema_name  varchar2(30) := 'PUBLIC';
	v_object_name  varchar2(30) := 'TEST_ROWID_TAB_2'; --query object is test_rowid_tab_2
	v_convert_type  integer  := 1;
	v_new_rowid     rowid;
begin
	--get test_rowid_tab_1's rowid
	select rowid into  v_old_rowid from test_rowid_tab_1 where id = 1;
	--test_rowid_tab_1's rowid1 but v_object_name is test_rowid_tab_2
	v_new_rowid := dbms_rowid.rowid_to_extended(
	old_rowid       => v_old_rowid,
	schema_name     => v_schema_name,
	object_name     => v_object_name,
	conversion_type => v_convert_type
	);
	dbms_output.put_line('old rowid:' || v_old_rowid);
	dbms_output.put_line('convert new rowid:' || v_new_rowid);
end;
/
drop table test_rowid_tab_1;
drop table test_rowid_tab_2;
--End bug#Z7668

--Bgein bug#Z7669, expect: NULL and ''
declare
	v_old_rowid     rowid := NULL;
	v_convert_type  integer  := 1;
	v_new_rowid    rowid;
	begin
	v_new_rowid := dbms_rowid.rowid_to_restricted(v_old_rowid,v_convert_type);
	dbms_output.put_line(v_new_rowid);
end;
/

declare
	v_old_rowid     rowid := '';
	v_convert_type  integer  := 1;
	v_new_rowid    rowid;
	begin
	v_new_rowid := dbms_rowid.rowid_to_restricted(v_old_rowid,v_convert_type);
	dbms_output.put_line(v_new_rowid);
end;
/
--End bug#Z7669

--Bgein bug#Z7670, expect :1
declare
	v_null_rowid    rowid := NULL;
	v_rowid_type       number;
begin
	v_rowid_type := dbms_rowid.rowid_type(v_null_rowid);
	dbms_output.put_line(v_rowid_type);
end;
/

declare
	v_null_rowid    rowid := '';
	v_rowid_type       number;
begin
	v_rowid_type := dbms_rowid.rowid_type(v_null_rowid);
	dbms_output.put_line(v_rowid_type);
end;
/
--End bug#Z7670

--Bgein bug#Z7671, expect : 0 
declare
	v_null_rowid    rowid := '';
	v_row_num       number;
begin
	v_row_num := dbms_rowid.rowid_row_number(v_null_rowid);
	dbms_output.put_line(v_row_num);
end;
/

declare
	v_null_rowid    rowid := NULL;
	v_row_num       number;
begin
	v_row_num := dbms_rowid.rowid_row_number(v_null_rowid);
	dbms_output.put_line(v_row_num);
end;
/
--End bug#Z7671

--Bgein bug#Z7672，expect：0
create table  test_rowid_tab (id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');
declare
	v_old_rowid     rowid ;
	v_schema_name   varchar2(30) := 'PUBLIC';
	v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
	v_convert_type  integer  := 0;
	v_result    number;
begin
	select rowid into  v_old_rowid from test_rowid_tab where id = 1;
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
	dbms_output.put_line(v_result);
end;
/
DROP TABLE test_rowid_tab;
--End bug#Z7672

--Bgein bug#Z7673, expect:0
create table  ptable_range(id int,name text,sale_date date) partition by range (sale_date) with rowid;
create table  range_2020_01 partition of ptable_range for values from('2020-01-01') to ('2020-04-01');
create table  range_2020_02 partition of ptable_range for values from('2020-04-01') to ('2020-07-01');
insert into ptable_range values(1,'aaa','2020-03-02');
insert into ptable_range values(2,'bbb','2020-06-01');
declare
	type id_array is table of  number;
	v_ids id_array := id_array(1, 2);
	v_old_rowid  rowid;
	v_schema_name   varchar2(30) := 'PUBLIC';
	v_object_name   varchar2(30) := 'PTABLE_RANGE';
	v_convert_type  integer  := 1;
	v_result    number;
begin
	for i in 1..v_ids.count loop
	begin
		select rowid into  v_old_rowid from ptable_range where id = v_ids(i);
		v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
		dbms_output.put_line(v_result);
	end;
	end loop;
end;
/
drop table ptable_range;
--End bug#Z7673

--Bgein bug#Z7674,expect : 0
create table  test_rowid_tab (id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');
declare
    v_old_rowid     rowid := NULL;
    v_schema_name   varchar2(30) := 'PUBLIC';
    v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
    v_convert_type  integer  := 1;
    v_result    number;
begin
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
	dbms_output.put_line(v_result);
end;
/
declare
    v_old_rowid     rowid := '';
    v_schema_name   varchar2(30) := 'PUBLIC';
    v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
    v_convert_type  integer  := 1;
    v_result    number;
begin
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
	dbms_output.put_line(v_result);
end;
/
drop table test_rowid_tab;
--End bug#Z7674

--Bgein bug#Z7675, expect error: table or view does not exist
create table  test_rowid_tab(id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');

declare
	v_old_rowid     rowid ;
	v_schema_name   varchar2(30) := 'INVALID_SCHEMA_999'; -- non-exist schema
	v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
	v_convert_type  integer  := 0;
	v_result    number;
begin
	select rowid into  v_old_rowid from test_rowid_tab where id = 1;
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
	dbms_output.put_line(v_result);
end;
/

declare
	v_old_rowid     rowid ;
	v_schema_name   varchar2(30) := 'INVALID_SCHEMA';
	v_object_name   varchar2(30) := 'test_rowid_tab_111'; -- non-exist object
	v_convert_type  integer  := 0;
	v_result    number;
begin
	select rowid into  v_old_rowid from test_rowid_tab where id = 1;
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name,v_object_name,v_convert_type);
	dbms_output.put_line(v_result);
end;
/
DROP TABLE test_rowid_tab;
--End bug#Z7675

--Bgein bug#Z7676, expect : 0
create table  test_rowid_tab (id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');
declare
	v_old_rowid        rowid;
	v_convert_type  integer  :=1;
	v_result    number;
begin
	select rowid into  v_old_rowid from test_rowid_tab where id = 1;
	v_result := dbms_rowid.rowid_verify(v_old_rowid,NULL,NULL,v_convert_type);
	dbms_output.put_line(v_result);
end;
/
declare
	v_old_rowid        rowid;
	v_convert_type  integer  :=1;
	v_result    number;
begin
	select rowid into  v_old_rowid from test_rowid_tab where id = 1;
	v_result := dbms_rowid.rowid_verify(v_old_rowid,'','',v_convert_type);
	dbms_output.put_line(v_result);
end;
/
DROP TABLE test_rowid_tab;
--End bug#Z7676

--Bgein bug#Z7677, expect: 0
create table  "123" (id number) with rowid;
insert into  "123" values (1);
create table  test_rowid_tab (id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');
declare
	v_old_rowid     rowid ;
	v_schema_name   varchar2(30) := 'PUBLIC';
	v_object_name   INT := 123;    -- not varchar2 type, will do implicit conversion
	v_convert_type  integer  := 1;
	v_result        number;
begin
	select rowid into  v_old_rowid from "123" where id = 1; -- get valid rowid
	--param is int for v_object_name
	v_result := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name, v_object_name, v_convert_type);
	dbms_output.put_line(v_result);
end;
/
 declare
    v_old_rowid     rowid ;
    v_schema_name   varchar2(30) := 'PUBLIC';
    v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
    v_convert_type  varchar2(1) := '1';
    v_result    number;
begin
    select rowid into  v_old_rowid from test_rowid_tab where id = 1;
    v_result  := dbms_rowid.rowid_verify(v_old_rowid, v_schema_name, v_object_name, v_convert_type);
    dbms_output.put_line(v_result);
end;
/
DROP TABLE "123";
DROP TABLE test_rowid_tab;
--End bug#Z7677

--Bgein bug#Z7678, expect : invalid ROWID
create table test_rowid_tab (id int, name text) with rowid;
insert into test_rowid_tab values(1, 'aaa');
declare
	v_rowid rowid := NULL;
	v_schema_name varchar2(30) := 'PUBLIC';
	v_object_name varchar2(30) := 'TEST_ROWID_TAB';
	v_abs_fno number;
begin
	v_abs_fno := dbms_rowid.rowid_to_absolute_fno(
		row_id => v_rowid,
		schema_name => v_schema_name,
		object_name => v_object_name
		);
	dbms_output.put_line('rowid:' || v_rowid);
	dbms_output.put_line('absolute_fno:' || v_abs_fno);
end;
/
DROP TABLE test_rowid_tab;
--End bug#Z7678

--Bgein bug#Z7679,expect : invalid ROWID
create table  test_rowid_tab_1 (id  int, name text) with rowid;
insert into   test_rowid_tab_1 values(1, 'aaa');
create table  test_rowid_tab_2 (id number) with rowid;
insert into  test_rowid_tab_2 values (1);
declare
	v_rowid        rowid; -- table test_rowid_tab_1's rowid
	v_schema_name  varchar2(30) := 'PUBLIC';
	v_object_name  varchar2(30) := 'TEST_ROWID_TAB_2'; -- query object is test_rowid_tab_2
	v_abs_fno      number;
begin
	-- get test_rowid_tab_1's rowid
	select rowid into  v_rowid from test_rowid_tab_1 where id = 1;
	-- test_rowid_tab_1's rowid1 but v_object_name is test_rowid_tab_2
	v_abs_fno := dbms_rowid.rowid_to_absolute_fno(
		row_id      => v_rowid,
		schema_name => v_schema_name,
		object_name => v_object_name
		);
	dbms_output.put_line(v_abs_fno);
end;
/
DROP TABLE test_rowid_tab_1;
DROP TABLE test_rowid_tab_2;
--End bug#Z7679

--Bgein bug#Z7683
create table  test_rowid_tab (id  int, name text) with rowid;
insert into  test_rowid_tab values(1, 'aaa');
declare
    v_old_rowid     rowid := NULL;
    v_schema_name   varchar2(30) := 'PUBLIC';
    v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
    v_convert_type  integer  := 1;
    v_new_rowid     rowid;
begin
    v_new_rowid := dbms_rowid.rowid_to_extended(
        old_rowid       => v_old_rowid,
        schema_name     => v_schema_name,
        object_name     => v_object_name,
        conversion_type => v_convert_type
        );
    dbms_output.put_line('convert new rowid:' || v_new_rowid);
end;
/
declare
    v_old_rowid     rowid := '';
    v_schema_name   varchar2(30) := 'PUBLIC';
    v_object_name   varchar2(30) := 'TEST_ROWID_TAB';
    v_convert_type  integer  := 1;
    v_new_rowid     rowid;
begin
    v_new_rowid := dbms_rowid.rowid_to_extended(
        old_rowid       => v_old_rowid,
        schema_name     => v_schema_name,
        object_name     => v_object_name,
        conversion_type => v_convert_type
        );
    dbms_output.put_line('convert new rowid:' || v_new_rowid);
end;
/
DROP TABLE test_rowid_tab;
--End bug#Z7683

--Bgein bug#Z7692
create table  ptable(id int,td varchar(100)) partition by list(td) with rowid;
create table  pt_1  partition of ptable for values in ('2023-08-05');
create table  pt_2  partition of ptable for values in ('2023-08-06');
insert into ptable values(1,'2023-08-05');
insert into ptable values(2,'2023-08-06');
declare
	type id_array is table of  number;
	v_ids id_array := id_array(1, 2);
	v_rowid  rowid;
	v_abs_fno   number;
	v_partition_name varchar2(100);
	v_exp_abs_fno       number ;
begin
	for i in 1..v_ids.count loop
	begin
		v_partition_name := 'pt_' || v_ids(i);
		select rowid into  v_rowid from ptable where id = v_ids(i);
		select relfilenode into v_exp_abs_fno from pg_class where relname = v_partition_name;
		v_abs_fno := dbms_rowid.rowid_to_absolute_fno(
			row_id      => v_rowid,
			schema_name => 'PUBLIC',
			object_name => 'PTABLE'
			);
		if  v_abs_fno = v_exp_abs_fno  then
			dbms_output.put_line('success');
		else
			dbms_output.put_line('failed');
		end if;
		exception
			when others then
				dbms_output.put_line(sqlerrm);
	end;
	end loop;
end;
/

declare
	type id_array is table of  number;
	v_ids id_array := id_array(1, 2);
	v_rowid  rowid;
	v_abs_fno   number;
	v_partition_name varchar2(100);
	table_name		 varchar2(100);
	v_exp_abs_fno       number ;
begin
	for i in 1..v_ids.count loop
	begin
		v_partition_name := 'pt_' || v_ids(i);
		table_name := upper(v_partition_name);
		select rowid into  v_rowid from ptable where id = v_ids(i);
		select relfilenode into v_exp_abs_fno from pg_class where relname = v_partition_name;
		v_abs_fno := dbms_rowid.rowid_to_absolute_fno(
			row_id      => v_rowid,
			schema_name => 'PUBLIC',
			object_name => table_name
			);
		if  v_abs_fno = v_exp_abs_fno  then
			dbms_output.put_line('success');
		else
			dbms_output.put_line('failed');
		end if;
		exception
			when others then
				dbms_output.put_line(sqlerrm);
	end;
	end loop;
end;
/
DROP TABLE ptable;
--End bug#Z7692
reset ivorysql.identifier_case_switch;
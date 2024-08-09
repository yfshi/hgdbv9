--create table
SET plisql.plisql_default_into_strict = false;
create table employees(employee_id integer, first_name varchar(256),last_name varchar(256),salary integer,job_id integer);
insert into employees values(1,'zhang','san',200000,1);
insert into employees values(2,'wang','wu',4000000,2);
insert into employees values(3,'zhou','yang',500000,3);
insert into employees values(4,'li','dong',6000000,4);
create table mds(ids integer,name varchar(23));
insert into mds values(1,'xiexie');
insert into mds values(1,'zhangwei');
create table schedule(week integer,
  mon varchar(10),
  tue varchar(10),
  wed varchar(10),
  thu varchar(10),
  fri varchar(10),
  sat varchar(10),
  sun varchar(10));
create table test_insert(id integer,name varchar2(256));


DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(256),
    last   varchar(266)
  );

  friend name_rec;
BEGIN
  friend.first := 'John';
  friend.last := 'Smith';
  raise notice '%', friend ;
END;
/

declare
 type deptrectyp is record (
  dept_id integer not null := 10,
  dept_name varchar(30) not null := 'admins',
  mgr_id     integer,
  loc_id     integer
 );
  record_var deptrectyp;
begin
  raise info '%,%,%,%', record_var.dept_id, record_var.dept_name, record_var.mgr_id, record_var.loc_id;
  record_var.mgr_id := record_var.dept_id;
  raise info '%,%,%,%', record_var.dept_id, record_var.dept_name, record_var.mgr_id, record_var.loc_id;
end;
/

--assign null value to record
DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(256),
    last   varchar(266)
  );

  friend name_rec;
  friend2 name_rec;
BEGIN
  friend2 := friend;
  raise notice '%,%', friend2.first,friend2.last ;
END;
/

DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(256),
    last   varchar(266)
  );

  friend name_rec;
  friend2 name_rec;
BEGIN
  friend.first := 'John';
  friend.last := 'Smith';
  friend2 := friend;
  raise notice '%,%', friend2.first,friend2.last ;
END;
/

DECLARE
  TYPE name_rec IS RECORD (
    first  int,
    last   text
  );

  friend name_rec;
  friend2 name_rec;
BEGIN
  friend.first := 11;
  friend.last := 'Smith';
  friend2 := friend;
  raise notice '%,%', friend2.first,friend2.last ;
END;
/

--record as argument
declare
  type mds is record(id integer,name varchar(23));
  b mds;
  ret integer;
  function square(original mds default b) return integer;
  function square(original mds default b) return integer
  as
       original_squared integer;
  begin
       original_squared := original.id * original.id;
       return original_squared;
   end;
begin
    b.id := 23;
    ret := square();
    raise info '%', ret;
   b.id := 10;
   ret := square();
   raise info '%', ret;
end;
/
--INFO:  529
--INFO:  100

--test inline function'argument is udt type
declare
  type record1 is record(id integer);
  rec_var1 record1;
  function udt_func(var1 record1) return record1;
  function udt_func(var1 record1) return record1 as
     rec_var1 record1;
   begin
      rec_var1.id := var1.id + 1;
      return rec_var1;
   end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
   raise info '%',rec_var1.id;
end;
/
--2

declare
  type record1 is record(id integer);
  rec_var1 record1;
  function udt_func(var1 record1) return record1;
  function udt_func(var1 record1) return record1 as
     rec_var1 record1;
   begin
      raise info 'var1 === %',var1.id;
      rec_var1.id := var1.id + 1;
      return rec_var1;
   end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
   raise info '%',rec_var1.id;
end;
/
--INFO:  var1 === 1
--INFO:  2

--ERROR:  record type record1 does not matched with record type record2
declare
  type record1 is record(id integer);
   type record2 is record(id integer, name int);
  rec_var1 record1;
  function udt_func(var1 record1) return record1;
  function udt_func(var1 record1) return record1 as
     rec_var1 record2;
   begin
      rec_var1:= var1;
      return rec_var1;
   end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
end;
/


CREATE OR REPLACE PACKAGE My_Types IS
	TYPE My_Rec IS RECORD (a int, b int);
	FUNCTION Init_My_Rec RETURN My_Rec;
END My_Types;
/
CREATE OR REPLACE PACKAGE BODY My_Types IS
	FUNCTION Init_My_Rec RETURN My_Rec IS
		Rec My_Rec;
	BEGIN
		Rec.a := 10;
		Rec.b := 11;
		RETURN Rec;
	END Init_My_Rec;
END My_Types;
/

DECLARE
	r My_Types.My_Rec;
BEGIN
    r.a := 1;
	r.b := 2;
	raise info 'r.a ==== %',r.a;
	raise info 'r.b ==== %',r.b;
	r := My_Types.Init_My_Rec();
	raise info 'r.a ==== %',r.a;
	raise info 'r.b ==== %',r.b;
END;
/

--nested record
DECLARE
  TYPE age_rec1 IS RECORD (
    years  INTEGER DEFAULT 35
  );

  TYPE name_rec IS RECORD (c1 age_rec1);

  name name_rec;
BEGIN
  name.c1.years = 11;
  raise info 'name.a.a.a.years = %',name.c1.years ;
END;
/

DECLARE
  TYPE age_rec1 IS RECORD (
    years  INTEGER DEFAULT 35
  );

  TYPE name_rec IS RECORD (c1 age_rec1);
  TYPE name_rec2 IS RECORD (c2 name_rec);

  name name_rec2;
BEGIN
  name.c2.c1.years = 11;
  raise info 'name.c2.c1.years  ==== %',name.c2.c1.years ;
END;
/

--notiation
DECLARE
  TYPE t_size IS RECORD (x int, y int);
  c_small t_size;
BEGIN
  c_small := t_size(100, 200);
  raise notice '%,%', c_small.x,c_small.y;
END;
/
--NOTICE:  100,200

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(1, c => 3);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  1,<NULL>,3,<NULL>

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(1, 2, c => 3,d => 4);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  1,2,3,4

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(1, c => 3, OTHERS => 2);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  1,2,3,2

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(1, OTHERS => 2);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  1,2,2,2

DECLARE
  TYPE t_size IS RECORD (x int, y int);
  c_small t_size;
BEGIN
  c_small := t_size(y=>100, x=>200);
  raise notice '%,%', c_small.x,c_small.y;
END;
/
--NOTICE:  200,100

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(OTHERS => 2);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  2,2,2,2

DECLARE 
  TYPE r IS RECORD(a int, b int, c int, d int);  
  rec r;
BEGIN  
  rec := r(c => 3, OTHERS => 2);  
  raise notice '%,%,%,%', rec.a, rec.b, rec.c, rec.d;
END;
/
--NOTICE:  2,2,3,2

DECLARE 
  TYPE r IS RECORD(a int, b int, c int);  
  rec r;
BEGIN  
  rec := r(c => 3.0, c => 3.0, OTHERS => 2);  
  raise notice '%,%,%', rec.a,rec.b,rec.c;
END;
/
--multiple instances of named argument in list

DECLARE 
  TYPE r IS RECORD(a int, b int, c int);  
  rec r;
BEGIN  
  rec := r(c => 3.0, 1, OTHERS => 2);  
END;
/
--a positional parameter association may not follow a named association

DECLARE 
  TYPE r IS RECORD(a int, b int, c int);  
  rec r;
BEGIN  
  rec := r(1,1,1,1,1);  
END;
/
--wrong number or types of arguments in call to 'R'

DECLARE 
  TYPE r IS RECORD(a int, b int, c int);  
  rec r;
BEGIN  
  rec := r(a => 3.0, b => 3.0, c => 3.0,d => 3.0);  
END;
/
--wrong number or types of arguments in call to 'R'

--Define a record type
declare
  type deptrectyp is record (
  dept_id integer not null := 10,
  dept_name varchar(30) not null := 'admins',
  mgr_id     integer,
  loc_id     integer
  );
  record_var deptrectyp;
begin
  raise info '%, %, %, %', record_var.dept_id, record_var.dept_name, record_var.mgr_id, record_var.loc_id;
  record_var.mgr_id := record_var.dept_id;
  raise info '%, %, %, %', record_var.dept_id, record_var.dept_name, record_var.mgr_id, record_var.loc_id;
end;
/

--field definition NOT NULL or default
DECLARE
  TYPE DeptRecTyp IS RECORD (
    dept_id    INTEGER NOT NULL := 10,
    dept_name  VARCHAR(30) NOT NULL default 'Administration',
    mgr_id     INTEGER := 200,
    loc_id     INTEGER := 1700
  );
  dept_rec DeptRecTyp;
  dept_rec1 DeptRecTyp;
BEGIN
  dept_rec1.dept_id := 23;
  raise info 'dept_id:%',dept_rec.dept_id;
  raise info 'dept_name:%',dept_rec.dept_name;
  raise info 'mgr_id:%',dept_rec.mgr_id;
  raise info 'loc_id:%',dept_rec.loc_id;
  raise info 'rec1.id:%',dept_rec1.dept_id;
END;
/

declare
 type dept_rec is record (id integer,name varchar(23));
 type dept_rec is record(ids integer,name varchar(256));
 mds dept_rec;
begin
 NULL;
end;
/

DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(256),
    last   varchar(266)
  );
  TYPE contact IS RECORD (
    name  name_rec,                    -- nested record
    phone varchar(256)
  );
  friend contact;
BEGIN
  friend.name.first := 'John';
  friend.name.last := 'Smith';
  friend.phone := '1-650-555-1234';
  raise info 'address=%',friend.name.first || ' ' || friend.name.last || ' ' || friend.phone;
END;
/

declare
  type test_r is record (id integer default 23,name varchar(23));
  type test_r2 is record (r1 test_r, salary integer);
  test_var1 test_r;
  test_var2 test_r2;
  cur3_rowvar test_var2%type;
begin
  cur3_rowvar.r1.id := 24;
  test_var1.name := 'welcome';
  test_var2.r1.name := test_var1.name;
  raise notice 'cur3_rowvar.r1.id = %, cur3_rowvar.r1.name = %', cur3_rowvar.r1.id, cur3_rowvar.r1.name;
  raise notice 'test_var1.id = %, test_var1.name = %', test_var1.id, test_var1.name;
  raise notice 'test_var2.r1.id = %, test_var2.r1.name = %', test_var2.r1.id, test_var2.r1.name;
end;
/

--NOT NULL
declare
  type rt is record(c1 int not null,c2 varchar(10));
  var1 rt;
begin
  null;
end;
/

declare
  type rt is record(c1 int,c2 varchar(10));
  var1 rt not null;
begin
  null;
end;
/

declare
  type rt is record(c1 int,c2 varchar(10));
  var1 rt not null := rt(1,2);
begin
  null;
end;
/

--test label.record.xxx
<<test_label>> declare
  type c_4 is record (id integer,name varchar(23));
  c_5 c_4;
  c_3 c_4;
begin
  test_label.c_3.id := 23;
  test_label.c_5.id := test_label.c_3.id;
  raise notice 'c_5.id =%', test_label.c_5.id;
end;
/

--record variable assign
DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(23),
    last   varchar(23)
  );
  name1 name_rec;
  name2 name_rec;
BEGIN
  name1.first := 'Jane'; name1.last := 'Smith';
  raise notice 'name1:% %',name1.first,name1.last;
  name2 := name1;
  raise notice 'name2:% %',name2.first,name2.last;
END;
/

--test record column assign to a record column
DECLARE
  TYPE name_rec IS RECORD (
    first  varchar(256),
    last   varchar(256)
  );
  TYPE phone_rec IS RECORD (
    name  name_rec,                    -- nested record
    phone varchar(256)
  );
  TYPE email_rec IS RECORD (
    name  name_rec,                    -- nested record
    email varchar(256)
  );
  phone_contact phone_rec;
  email_contact email_rec;
BEGIN
  phone_contact.name.first := 'John';
  phone_contact.name.last := 'Smith';
  phone_contact.phone := '1-650-555-1234';
  email_contact.name := phone_contact.name;
  email_contact.email := (
    email_contact.name.first || '.' ||
    email_contact.name.last  || '@' ||
    'example.com'
  );
  raise notice 'email_contact.email=%',email_contact.email;
END;
/

--test select into to a record variable
DECLARE
  TYPE RecordTyp IS RECORD (
    last varchar(256),
    id   integer
  );
  rec1 RecordTyp;
BEGIN
  SELECT last_name, employee_id INTO rec1
  FROM employees
  WHERE job_id = 1;
  raise notice '%','Employee #' || rec1.id || ' = ' || rec1.last;
END;
/

--test fetch into
CREATE OR REPLACE PROCEDURE proc1(id int) 
AS
  TYPE EmpRecTyp IS RECORD (
    emp_id  integer,
    salary  integer
  );
  desc_salary CURSOR FOR
    SELECT employee_id, salary
    FROM employees ORDER BY salary DESC;
  highest_paid_emp       EmpRecTyp;
BEGIN
  open desc_salary;
  LOOP
  FETCH desc_salary into highest_paid_emp;
  exit when NOT FOUND;
  raise notice '%',
    'Highest Paid: #' ||
    highest_paid_emp.emp_id || ', $' ||
    highest_paid_emp.salary ;
  end loop;
END;
/
CALL proc1(1);
DROP PROCEDURE proc1;

--test using returning into to assign a record variable
DECLARE
  TYPE EmpRec IS RECORD (
    last_name  varchar(256),
    salary     integer
  );
  emp_info    EmpRec;
  old_salary  integer;
BEGIN
  SELECT salary INTO old_salary
   FROM employees
   WHERE employee_id = 1;
  UPDATE employees
    SET salary = salary * 1.1
    WHERE employee_id = 1
    RETURNING last_name, salary INTO emp_info;
    raise notice '%',
    'Salary of ' || emp_info.last_name || ' raised from ' ||
    old_salary || ' to ' || emp_info.salary;
END;
/

--test rowtype into record
declare
  type record_type is record (
  id integer,
  name varchar(256),
  name2 varchar(256),
  job_id integer,
  salary integer);
  record_var record_type;
  row_var employees%rowtype;
  record_var1 record_var%type;
begin
  record_var1.name := 'a record var type of variable';
  row_var.employee_id := 35;
  record_var := row_var;
  raise info '%', row_var;
  raise info '%', record_var1.name;
  raise info '%', record_var.id;
end;
/

--test insert or update into rows by record
declare
  default_week schedule%rowtype;
  i integer;
begin
  default_week.mon := '0800-1700';
  default_week.tue := '0800-1700';
  default_week.wed := '0800-1700';
  default_week.thu := '0800-1700';
  default_week.fri := '0800-1700';
  default_week.sat := 'Day Off';
  default_week.sun := 'Day Off';
  for i in 1..6 loop
      default_week.week := i;
      insert into schedule values default_week;
  end loop;
end;
/

declare
  default_week schedule%rowtype;
begin
  default_week.mon := 'Day Off';
  default_week.tue := '0900-1800';
  default_week.wed := '0900-1800';
  default_week.thu := '0900-1800';
  default_week.fri := '0900-1800';
  default_week.sat := '0900-1800';
  default_week.sun := 'Day Off';
  for i in 1..3 LOOP
     default_week.week = i;
     update schedule
     set ROW = default_week where week = i;
  end loop;
end;
/

select * from schedule order by week;

--test function has Oracle record
create or replace function test_udt_var(arg1 integer) return integer as
  type record_type is record (id integer, name varchar(23) default 'a name');
  record_var1 record_type;
begin
  record_var1.id := arg1;
  raise info '%,%', record_var1.id, record_var1.name;
  return 23;
end;
/
select * from test_udt_var(23);

create or replace function test_udt_var(arg1 integer) return integer as
  type record_type is record (id integer,name varchar(23) default 'xiexie');
  record_var1 record_type;
begin
  record_var1.id := arg1;
  raise info '%,%', record_var1.id, record_var1.name;
  return 24;
end;
/
select * from test_udt_var(24);
drop function test_udt_var(integer);

create or replace function test_udt_var2(arg1 integer) return integer as
  type record_type is record (id integer,name varchar(23));
  type record_type1 is record (id integer, name varchar(24));
  record_var1 record_type;
  record_var2 record_type1;
begin
  record_var1.id := arg1;
  record_var2.name := 'xiexie';
  raise info '%,%',record_var1.name,record_var1.id;
  raise info '%,%',record_var2.name,record_var2.id;
  return 23;
end;
/
select * from test_udt_var2(23);

create or replace function test_udt_var2(arg1 integer) return integer as
  type record_type is record (id integer,name varchar(23));
  type record_type1 is record (id integer, name varchar(24));
  record_var1 record_type;
  record_var2 record_type1;
begin
  record_var1.id := arg1;
  record_var2.name := 'xiexie';
  raise info '%,%',record_var1.name,record_var1.id;
  raise info '%,%',record_var2.name,record_var2.id;
  return 24;
end;
/
select * from test_udt_var2(23);
drop function test_udt_var2(integer);

--test xxx.xxx%type
<<test_label>>
declare
  type record_type is record (id integer,name varchar(23));
  record_var1 record_type;
  record_var2  test_label.record_var1%type;
begin
  record_var2.id := 23;
  raise info '%', record_var2.id;
end;
/

--test rowtype.rowtype.column raise error
declare
  type record_type1 is record(id integer,name varchar(23));
  type record_type2 is record(type_rec record_type1, name varchar(23));
  var1 record_type2;
begin
  var1.type_rec.id := 23;
  var1.type_rec.names := 'xiexie';
end;
/

--record.column appeared in where clause 
create table tt1(id integer,name varchar(23), goolds varchar(256));

create or replace function test(id integer) return integer is
  type mds is record(style integer,style_sfx varchar(23), size_desc varchar(256));
  pix_rec mds;
  var3 integer;
  var4 varchar(256);
begin
  select id,name into var3,var4 from tt1 where goolds = trim(nvl(pix_rec.style,'') || nvl(pix_rec.style,'')) and id = 1;
  return 23;
end;
/
drop function test(integer);
drop table tt1;



--function return type is record
CREATE TABLE tabpackage(id number, id1 number);
CREATE OR REPLACE PACKAGE My_Types IS
  TYPE My_Rec IS RECORD (a NUMBER, b NUMBER);
  FUNCTION Init_My_Rec RETURN My_Rec;
END My_Types;
/
CREATE OR REPLACE PACKAGE BODY My_Types IS
  FUNCTION Init_My_Rec RETURN My_Rec IS
  	Rec My_Rec;
  BEGIN
  	Rec.a := 0;
  	Rec.b := 1;
  	insert into tabpackage(id, id1) values (Rec.a, Rec.b);
  	RETURN Rec;
  END Init_My_Rec;
END My_Types;
/

DECLARE
  r CONSTANT My_Types.My_Rec := My_Types.Init_My_Rec();
BEGIN
  raise notice 'r.a = %', r.a;
  raise notice 'r.b = %', r.b;
END;
/

select * from tabpackage;


create table pkg_t(a NUMBER, b NUMBER);
create or replace package pkgt is
  r pkg_t%rowtype := My_Types.Init_My_Rec();
  procedure print(id integer);
end;
/
create or replace package body pkgt is
  procedure print(id integer) is
  begin
   raise notice 'r.a = %', r.a;
   raise notice 'r.b = %', r.b;
  end;
end;
/
begin
  pkgt.print(23);
end;
/

create table mds2(id integer, name varchar(23));

declare
  type r_t is record(id integer default 2, name varchar(23) default 'xiexie');
  var1 r_t;
  var2 mds2%rowtype;
  type r_t1 is record(t r_t default var1, name varchar(23));
  var3 mds2%rowtype default var1;
  var4 r_t1;
begin
  raise notice 'var1.id = %', var1.id;
  raise notice 'var2.id = %', var2.id;
  raise notice 'var3.name = %', var3.name;
  raise notice 'var4.t.id = %', var4.t.id;
end;
/

--package
create or replace package test_pkg is
  type r_t is record(id integer default 2, name varchar(23) default 'xiexie');
  var1 r_t;
  var2 mds2%rowtype;
  type r_t1 is record(t r_t default var1, name varchar(23));
  var3 mds2%rowtype default var1;
  var4 r_t1;
end;
/

begin
  raise notice 'test_pkg.var1.id = %', test_pkg.var1.id;
  raise notice 'test_pkg.var2.id = %', test_pkg.var2.id;
  raise notice 'test_pkg.var3.name = %', test_pkg.var3.name;
  raise notice 'test_pkg.var4.t.id = %', test_pkg.var4.t.id;
end;
/


--more complex testcase that declare record type in package
create or replace package test_pkg1 is
  type r_t is record(id integer default 2, name varchar(23) default 'xiexie');
  var1 r_t;
  var2 mds2%rowtype;
  type r_t1 is record(t r_t default var1, name varchar(23));
  var3 mds2%rowtype default var1;
  var4 r_t1;
  function test(id integer) return integer;
end;
/

create or replace package body test_pkg1 is
  function test(id integer) return integer is
  	var5 mds2%rowtype default var1;
  	var6 r_t1 default var4;
  begin
  	raise notice 'var5.id = %', var5.id;
  	raise notice 'var6.t.id = %', var6.t.id;
  	return 23;
  end;
begin
  var1.id := 3;
  var4.t.id := 6;
  raise notice 'init block';
end;
/

declare
  ret integer;
begin
  raise notice 'test_pkg1.var1.id = %', test_pkg1.var1.id;
  raise notice 'test_pkg1.var2.id = %', test_pkg1.var2.id;
  raise notice 'test_pkg1.var3.name = %', test_pkg1.var3.name;
  raise notice 'test_pkg1.var4.t.id = %', test_pkg1.var4.t.id;
  ret := test_pkg1.test(23);
end;
/


DECLARE
  TYPE age_rec1 IS RECORD (
    years  INTEGER DEFAULT 35,
    months INTEGER DEFAULT 6
  );
  TYPE name_rec IS RECORD (
    first  varchar(23) DEFAULT 'John',
    last   varchar(23) DEFAULT 'Doe',
    age    age_rec1  
    );
  name name_rec;
BEGIN
  name := NULL;
END;
/

create table test_mds(id integer,name varchar2(256));
create or replace package test_pkg is
  var_row test_mds%rowtype;
  function test_main(num integer, var3 test_mds%rowtype) return integer;
end;
/
create or replace package body test_pkg is
  function test_main(num integer, var3 test_mds%rowtype) return integer is
    var1 test_mds%rowtype;
    c1 CURSOR FOR select * from test_mds where id = var1.id;
  begin
    var1 := var3;
    for v_c1 in c1 loop
       raise info '%,%',v_c1.id,v_c1.name;
    end loop;
    return 23;
  end;
end;
/
insert into test_mds values(1,'sfsjf');
insert into test_mds values(2,'sjfdsjf');
declare
  var1 integer;
  var2 test_mds%rowtype;
begin
  select * into var2 from test_mds where id = 1;
  var1 := test_pkg.test_main(23, var2);
end;
/



create or replace function test_f(var1 test_mds%rowtype) return integer is
begin
  return var1.id;
end;
/
create or replace function test_main(ids integer, var3 test_mds%rowtype) return integer is
  var1 test_mds%rowtype;
  c1 cursor for select * from test_mds where id = test_f(var1) or id = 2 order by id;
begin
  var1 := var3;
  for v_c1 in c1 loop
     raise info '%,%',v_c1.id,v_c1.name;
  end loop;
  return 23;
end;
/
declare
  var1 integer;
  var2 test_mds%rowtype;
begin
  select * into var2 from test_mds where id = 1;
  var1 := test_main(23, var2);
end;
/


create or replace function test_main_pk(ids integer, var3 test_mds%rowtype) return integer is
  c1 CURSOR for select * from test_mds where id = test_f(test_pkg.var_row) or id = 2 order by id;
begin
  test_pkg.var_row := var3;
  for v_c1 in c1 loop
     raise info '%,%',v_c1.id,v_c1.name;
  end loop;
  return 23;
end;
/
declare
  var1 integer;
  var2 test_mds%rowtype;
begin
  select * into var2 from test_mds where id = 1;
  var1 := test_main_pk(23, var2);
end;
/

--dynamic sql
declare
  type mds is record(id integer,name varchar2(256));
  var1 mds;
begin
  var1.id := 2;
  var1.name := 'ok';
  if var1.id = 2 then
    begin
     execute 'insert into test_insert values($1,$2)' using var1.id,var1.name;
    end;
  end if;
end;
/
select * from test_insert;

--inline function
declare
  type mds is record(id integer,name varchar(23));
  b mds;
  ret integer;
  function square(original mds default b) return integer;
  function square(original mds default b) return integer
  as
    original_squared integer;
  begin
    original_squared := original.id * original.id;
    return original_squared;
  end;
begin
  b.id := 23;
  ret := square();
  raise info '%', ret;
  b.id := 10;
  ret := square();
  raise info '%', ret;
end;
/

declare
  type record1 is record(id integer);
  rec_var1 record1;
  function udt_func(var1 record1) return record1;
  function udt_func(var1 record1) return record1 as
    rec_var1 record1;
  begin
    rec_var1.id := var1.id + 1;
    return rec_var1;
  end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
   raise info '%',rec_var1.id;
end;
/

declare
  type record1 is record(id integer);
  type record2 is record(id integer,name varchar(23));
  rec_var1 record1;
  rec_var2 record2;
  function udt_func(var1 record1) return record1;
  function udt_func(var1 record2) return record2;
  function udt_func(var1 record1) return record1 as
     rec_var1 record1;
  begin
    rec_var1.id := var1.id + 1;
    raise info 'record1.id =%', rec_var1.id;
    return rec_var1;
  end;
  function udt_func(var1 record2) return record2 as
    rec_var2 record2;
  begin
    rec_var2.id := var1.id + 1;
    rec_var2.name := 'argument is record2';
    return rec_var2;
  end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
   raise info 'rec_var1.id = %',rec_var1.id;
   rec_var2.id := 2;
   rec_var2 := udt_func(rec_var2);
   raise info 'rec_var2.id = %, rec_var2.name = %', rec_var2.id, rec_var2.name;
end;
/

--pass by different record types, failed
declare
  type record1 is record(id integer);
  type record2 is record(id integer,name varchar(23));
  rec_var1 record1;
  rec_var2 record2;
  function udt_func(var1 record1) return record1 as
    rec_var1 record1;
  begin
    rec_var1.id := var1.id + 1;
    return rec_var1;
  end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
   raise info 'rec_var1.id = %', rec_var1.id;
   rec_var2.id := 2;
   rec_var2 := udt_func(rec_var2);
   raise info 'rec_var2.id = %', rec_var2.id;
end;
/

declare
  type record1 is record(id integer, name varchar(23));
  type record2 is record(id integer,name varchar(23));
  rec_var1 record1;
  rec_var2 record2;
  function udt_func(var1 record1) return record1 as
    rec_var1 record1;
  begin
    rec_var1.id := var1.id + 1;
    return rec_var1;
  end;
begin
   rec_var1.id := 1;
   rec_var1 := udt_func(rec_var1);
  raise info 'rec_var1.id = %', rec_var1.id;
   rec_var2.id := 2;
   rec_var2 := udt_func(rec_var2);
   raise info 'rec_var2.id = %', rec_var2.id;
end;
/

--test return type miss match function declare return type
--test failed
declare
  type rec_typ1 is record(id integer,name varchar(23));
  type rec_typ2 is record(name varchar(23), id integer);
  rec_var1 rec_typ1;
  function test_inline() return rec_typ1
  is
    var1 rec_typ2;
  begin
    var1.id := 23;
    var1.name := 'does not match';
    return var1;
  end;
begin
  rec_var1 := test_inline();
  raise info 'rec_var1=%', rec_var1;
end;
/

--test failed
declare
  type rec_typ1 is record(id integer,name varchar(23));
  type rec_typ2 is record(id integer,name varchar(23), salary integer);
  rec_var1 rec_typ1;
  function test_inline() return rec_typ1
  is
    var1 rec_typ2;
  begin
    var1.id := 23;
    var1.name := 'does not match';
    return var1;
  end;
begin
  rec_var1 := test_inline();
  raise info 'rec_var1=%', rec_var1;
end;
/

--test failed
declare
  type rec_typ1 is record(id integer);
  type rec_typ2 is record(id integer,name varchar(23));
  rec_var1 rec_typ1;
  function test_inline() return rec_typ1
  is
    var1 rec_typ2;
  begin
    var1.id := 23;
    var1.name := 'does not match';
    return var1;
  end;
begin
  rec_var1 := test_inline();
  raise info 'rec_var1=%', rec_var1;
end;
/

--test failed
declare
  type rec_typ1 is record(id integer, name varchar(23));
  type rec_typ2 is record(id integer,name varchar(23));
  rec_var1 rec_typ1;
  function test_inline() return rec_typ1
  is
    var1 rec_typ2;
  begin
    var1.id := 23;
    var1.name := 'does not match';
    return var1;
  end;
begin
  rec_var1 := test_inline();
  raise info 'rec_var1=%', rec_var1;
end;
/

create or replace function ora_inline_proc() return integer
is
  type rec_typ1 is record(id integer, name varchar(23));
  type rec_typ2 is record(rec1 rec_typ1,id integer,name varchar(23));
  var2 rec_typ2;
  function test_inline(id integer) return rec_typ2
  is
    var2 rec_typ2;
  begin
    var2.rec1.name := 'var2.rec1.name';
    var2.rec1.id := id;
    var2.id := 25;
    var2.name := 'record include record';
    return var2;
  end;
begin
    var2 := test_inline(23);
    raise info 'var2.rec1.id =%, var2.rec1.name =%, var2.id = %, var2.name = %', var2.rec1.id, var2.rec1.name, var2.id, var2.name;
    return var2.id;
end;
/
select * from ora_inline_proc();
drop function ora_inline_proc();


--inline function return object_type
create type test_inline_type as (id integer,name varchar(23));

--test failed
declare
  var1 test_inline_type;
  function test_inline(arg1 integer) return test_inline_type
  is
      type mds is record(id integer,name varchar(23), salary integer);
  var1 mds;
  begin
       var1.id := 23 + arg1;
   var1.name := 'a object type';
   return var1;
  end;
begin
  var1 := test_inline(23);
  raise info 'var1=%', var1;
end;
/


--function has out argument
declare
  type mds is record(id integer,name varchar(23));
  type mds1 is record(id mds, name varchar(23));
  var1 mds1;
  function test_out(name out varchar) return integer is
  begin
    name := 'a out variable';
    return 23;
  end;
begin
  var1.id.id := test_out(var1.id.name);
  raise info '%', var1;
end;
/

--test failed
declare
  type mds is record(id integer,name varchar(23));
  type mds1 is record(id integer,name varchar(23));
  type mds2 is record(id mds, id2 mds1);
  var1 mds2;
  ret integer;
  function test_mds(id mds) return integer is
  begin
    raise info 'mds = %', id;
    return 23;
  end;
begin
  ret := test_mds(var1.id);
  raise info 'failed';
  ret := test_mds(var1.id2);
end;
/
 
declare
  type mds is record(id integer,name varchar(23));
  type mds1 is record(id integer,name varchar(23));
  type mds2 is record(id mds, id2 mds1);
  var1 mds2;
  ret integer;
  function test_mds(id mds) return integer is
  begin
     raise info 'mds = %', id;
     return 23;
  end;
  function test_mds(id out mds1) return integer is
  begin
    id.id := 23;
    id.name := 'out name';
    return 23;
  end;
begin
  ret := test_mds(var1.id);
  raise info 'sucess';
  ret := test_mds(var1.id2);
  raise info 'var1=%',var1;
end;
/


declare
  type mds is record(id integer,name varchar(23));
  type mds1 is record(id integer,name varchar(23));
  type mds2 is record(id mds, id2 mds1);
  var1 mds2;
  ret integer;
  function test_mds(id mds) return integer is
  begin
     raise info 'mds = %', id;
     return 23;
  end;
  function test_mds(id in out mds1) return integer is
  begin
    id.id := 23 + id.id;
    id.name := 'out name';
    return 23;
  end;
begin
  ret := test_mds(var1.id);
  raise info 'sucess';
  var1.id2.id := 1;
  ret := test_mds(var1.id2);
  raise info 'var1=%',var1;
end;
/

create or replace procedure proc_r()
is
   p1 integer;
   type record1 is record(id integer,name varchar);
   r2 record1;

   function ref_func(ref_var out record1) return integer;
   function ref_func(ref_var out record1) return integer
   is
   begin
		raise notice 'Invoked version';
      return 2;
   end;
begin
    p1 := ref_func(r2);
end;
/
call  proc_r();
drop procedure proc_r();


--test ok
declare
  type rec_typ1 is record(id integer, name varchar(23));
  type rec_typ2 is record(rec1 rec_typ1,id integer,name varchar(23));
  rec_var1 rec_typ1;
  name varchar(256);
  var2 rec_typ2;
  function test_inline(id integer,name out varchar, var2 out rec_typ2) return rec_typ1
  is
    var1 rec_typ1;
  begin
    var1.id := 23;
    var1.name := 'does not match';
    name := 'out a varchar';
    var2.rec1 := var1;
    var2.rec1.id := 24;
    var2.id := 25;
    var2.name := 'record include record';
    return var1;
  end;
begin
  rec_var1 := test_inline(23, name, var2);
  raise info 'rec_var1.id = %, rec_var1.name = %', rec_var1.id, rec_var1.name;
  raise info 'name = %', name;
  raise info 'var2 = %', var2;
end;
/

--inline function returns udt type
declare
  type rec_typ is record(id integer,name varchar(256));
  type rec_typ2 is record(rec1 rec_typ, name varchar(256));
  rec_var rec_typ2;
  rec_var1 rec_typ2;
  name varchar(256);
  id integer;
  function test_out(id integer, var1 in out rec_typ2, name out varchar) return rec_typ2
  is
    ret_var1 integer;
    rec_var rec_typ2;
    name1 varchar(256);
    function test_out1(id integer, var1 in out rec_typ2, name out varchar) return integer
    is
      var2 integer;
    begin
       raise info 'var1=%', var1;
       raise info 'name=%', name;
       var2 := id + 2;
       name := 'a test_out1 name';
       var1.rec1.id := 1;
       var1.rec1.name := 'a test_out1 var1.rec1.name';
       var1.name := 'a test_out1 var1.name';
       return var2;
    end;
  begin
    raise info 'var1=%', var1;
    rec_var.rec1.id := 0;
	rec_var.rec1.name := 'a test_out rec_var.rec1.name';
	rec_var.name := 'a test_out rec_var.name';
	name1 := 'must be assign to null';
    ret_var1 := test_out.test_out1(id, rec_var, name1);
	raise info 'test_out rec_var=%', rec_var;
	raise info 'test_out name1=%', name1;
	raise info 'test_out ret_var1=%', ret_var1;
	name := 'test out name';
    rec_var.rec1.id := 1;
	rec_var.rec1.name := 'test_out rec_typ2.name';
	rec_var.name := 'test out name';
	var1.rec1.id := 99;
	var1.rec1.name := 'a test_out out var1.rec1.name';
	var1.name := 'a test_out out var1.name';
	return rec_var;
  end;
begin
  id := 1;
  rec_var.rec1.id := 100;
  rec_var.rec1.name := 'declare rec_var.rec1.name';
  rec_var.name := 'declare rec_var.name';
  rec_var1 := test_out(id, rec_var, name);
  raise info 'rec_var=%', rec_var;
  raise info 'name=%', name;
  raise info 'rec_var1=%', rec_var1;
end;
/


create or replace function f_test1(arg1 integer) return integer is
  type mds is record(id integer,name varchar(23));
  pix_rec mds;
begin
  if (pix_rec.id = 1) then
  raise info 'xiexie';
  end if;
  return 23;
end;
/
select f_test1(23);

create or replace function f_test2(arg integer) return integer is
  type mds is record(id integer,name varchar(23));
  pix_rec mds;
begin
  pix_rec.id := 1;
  pix_rec.name := 'xiexie';
  if (pix_rec.id = 1 and pix_rec.name = 'xiexie') then
  raise info '%', pix_rec.name;
  end if;
  return 23;
end;
/
select f_test2(23);

drop function f_test1(integer);
drop function f_test2(integer);



create table tt1(id integer, name varchar(23));
insert into tt1 values(1,'xiexie_ok');

declare
  type r is record(id integer, name varchar(23));
  b r;
  a tt1%rowtype;
  vc_sql varchar2(256);
  var1 integer;
begin
  vc_sql := 'select 2,id, name from tt1';
  EXECUTE vc_sql into var1,b.id, a.name;
  raise info '%,%,%', var1,b.id, a.name;
end;
/

declare
  type r is record(id integer, name varchar(23));
  b r;
  name varchar(23);
begin
  b.id := 23;
  name := case  when b.id = 1 then 'xiexie' else 'welcome' end || ' ok';
end;
/

declare
  a tt1%rowtype;
  v1 int;
begin
  CASE when a.id=1 THEN
    raise notice 'ok';
  else
    raise notice 'else';
  end CASE;
end;
/
drop table tt1;


declare
  type mds is record(id integer,name varchar(23));
  var1 mds;
begin
  var1.id := 1.1;
  raise notice '%', var1.id;
end;
/
declare
  type mds is record(id integer,name varchar(23));
  var1 mds;
  var2 number(38);
begin
  var2 := 123.4;
  var1.id := var2;
  raise notice '%', var1.id;
end;
/


--nested record
declare
  type mds1 is record(id integer,name varchar(23));
  type mds2 is record(id mds1,name varchar(23));
  type mds3 is record(id mds2,name varchar(24));
  type mds4 is record(id mds3,name varchar(25));
  var1 mds4;
begin
  var1.id.id.id.id := 1;
  var1.id.id.id.name := 'welcome to xieixe';
  var1.name := 'xiexie';
  raise notice 'var1.id.id.id.id = %', var1.id.id.id.id;
  raise notice 'var1.id.id.id.name = %', var1.id.id.id.name;
  raise notice 'var1.name = %', var1.name;
end;
/

--test more segments in package
create schema test;

--create a pkg package on the schema
create or replace package test.pkg is
  type mds1 is record(id integer,name varchar(23));
  type mds2 is record(id mds1, name varchar(24));
  type mds3 is record(id mds2, name varchar(25));
  function test(id integer) return integer;
  var1 mds3;
end;
/
create or replace package body test.pkg is
  function test(id integer) return integer is
  begin
   pkg.var1.id.id.name := 'pk';
   raise info '%', pkg.var1;
   return pkg.var1.id.id.id;
  end;
end;
/
select * from test.pkg.test(23);

declare
 var1 test.pkg.var1.id.id%type;
begin
  var1.id := 23;
  var1.name := 'ok';
  test.pkg.var1.id.id.id := 23;
  test.pkg.var1.id.id.name := 'ok';
  var1.id := test.pkg.test(test.pkg.var1.id.id.id);
  raise info '%', test.pkg.var1.id.id.name;
end;
/
drop package test.pkg;
drop schema test;


create or replace package test_pkg is
type mds is record(id integer,name varchar(23));
type mds2 is record(id mds, name varchar(23));
var1 mds2;
end;
/
declare
  var1 test_pkg.mds2;
  var2 test_pkg.mds;
begin
  var1.name := 'a local var1';
  test_pkg.var1 := var1;
  var2.id := 1;
  var2.name := 'a locale var2';
  test_pkg.var1.id := var2;
  test_pkg.var1.id.name := 'a pkg var1.id.name';
  raise info '%', test_pkg.var1;
end;
/


--test more than three idents
declare
 type mds1 is record(id integer,name varchar(23));
 type mds2 is record(id mds1,name varchar(23));
 type mds3 is record(id mds2,name varchar(24));
 type mds4 is record(id mds3,name varchar(25));
 type mds5 is record(id mds4, name varchar(26));
 type mds6 is record(id mds5, name varchar(256));
 function test_out(name out varchar) return integer is
 begin
   name := 'a out level';
   return 1;
 end;
 var1 mds1;
 var2 mds2;
 var3 mds3;
 var4 mds4;
 var5 mds5;
 var6 mds6;
begin
  --one level
  var1.id := 1;
  var1.name := 'one level';
  --two level
  var2.id.id := var1.id + 1;
  var2.id.name := var1.name;
  var2.name := 'two level';
  --three level
  var3.id := var2;
  var3.name := 'three level';
  var3.id.id.id := var2.id.id + 1;
  --four level
  var4.id := var3;
  var4.name := 'fouth level';
  var4.id.id.id.id := var3.id.id.id + 1;
  -- fifth level
  var5.id := var4;
  var5.name := 'fifth level';
  var5.id.id.id.id.id := var4.id.id.id.id + 1;
  --six level
  var6.id := var5;
  var6.name := 'six level';
  var6.id.id.id.id.id.id := var5.id.id.id.id.id + 1;
  raise info '%', var1;
  raise info '%', var2;
  raise info '%', var3;
  raise info '%', var4;
  raise info '%', var5;
  raise info '%', var6;
  raise info '%', var6.id.id.id.id.id;
  raise info '%', var6.id.id.id.id.id.name;
  var6.id.id.id.id.id.id := test_out(var6.id.id.id.id.id.name);
  raise info '%', var6;
end;
/


create table outtab(c1 int,c2 varchar2(20),c3 number);

--test ok
create or replace procedure pt() as
declare
	type r_type is record(c1 outtab.c1%type, c2 outtab.c2%type, c3 outtab.c3%type);
	revchar char(50);
	invalue2 r_type;
	FUNCTION test_return_out(recout out outtab%ROWTYPE) return varchar2 IS
	BEGIN
	recout.c1 := 100;
	recout.c2 := 10000;
	recout.c3 := 10000;
	return 'ok';
	END ;

begin
	revchar := test_return_out(invalue2);
	raise notice 'invalue2 = %, %, %, retvar = %', invalue2.c1, invalue2.c2, invalue2.c3, revchar;
end;
/
call pt();
drop  procedure pt();


--test ok
create or replace function test_return_out(recout out outtab%ROWTYPE) return varchar2 is
begin
  recout.c1 := 100;
  recout.c2 := 10000;
  recout.c3 := 10000;
  return 'ok';
end;
/

--test ok
declare
  type r_type is record(c1 outtab.c1%type, c22 outtab.c2%type, c3 outtab.c3%type);
  invalue1 r_type;
  revchar char(50);
begin
   revchar := test_return_out(invalue1);
   raise notice 'invalue1 = %, %, %, retvar = %', invalue1.c1, invalue1.c22, invalue1.c3, revchar;
end;
/

drop function test_return_out(outtab%ROWTYPE);
drop table outtab;



CREATE OR REPLACE PACKAGE r_types AUTHID DEFINER IS
  TYPE r_type_1 IS RECORD (f VARCHAR2(5) := 'abcde');
  TYPE r_type_2 IS RECORD (f VARCHAR2(5));
END;
/

--print x.f is abcde y.f isnull z is snull
CREATE OR REPLACE PROCEDURE p (
  x OUT r_types.r_type_1,
  y OUT r_types.r_type_2,
  z OUT VARCHAR2)
AUTHID CURRENT_USER IS
BEGIN
  raise notice 'x.f is %', NVL(x.f,'NULL');
  raise notice 'y.f is %', NVL(y.f,'NULL');
  raise notice 'z is %', NVL(z,'NULL');
  y.f := 'new';
  z := 'value';
END;
/
DECLARE
  r1 r_types.r_type_1;
  r2 r_types.r_type_2;
  s  VARCHAR2(5) := 'fghij';
BEGIN
  p (r1, r2, s);
  raise notice 'r1.f = %, r2.f = %, s = %', r1.f, r2.f, s;
END;
/
DROP PROCEDURE p (r_types.r_type_1, r_types.r_type_2, VARCHAR2);

CREATE OR REPLACE PACKAGE r_types AUTHID DEFINER IS
  TYPE r_type_1 IS RECORD (f VARCHAR2(5) := 'abcde');
  TYPE r_type_2 IS RECORD (f VARCHAR2(5));
END;
/

--print abcde
declare
  var1 r_types.r_type_1;
  var2 r_types.r_type_2;
  var3 r_types.r_type_1;
begin
  raise notice '%', var1.f;
  raise notice '%', var2.f;
  var3.f := NULL;
  var1 := var3;
   raise notice '%', var1.f;
end;
/

--print invoke test_default invoke test_f 23
declare
  var1 integer;
  function test_default(id integer) return integer is
  begin
    var1 := 23;
	raise notice 'invoke test_default';
	return 1;
  end;
  function test_f(id integer default test_default(23)) return integer is
  begin
    raise notice 'invoke test_f';
    raise notice '%', var1;
	return 2;
  end;
begin
  var1 := test_f();
end;
/

--print abcde abcde
declare
  var1 r_types.r_type_1;
  var2 r_types.r_type_2;
  var3 r_types.r_type_1;
  id integer;
  function test_f(id out r_types.r_type_1) return integer is
  begin
    raise notice '%',id.f;
	return 1;
  end;
begin
  raise notice '%',var1.f;
  var3.f := NULL;
  var1 := var3;
  id := test_f(var1);
end;
/

--print abcde
declare
  var1 r_types.r_type_1;
  var2 r_types.r_type_2;
  var3 r_types.r_type_1;
  id integer;
  function test_f(id in out r_types.r_type_1) return integer is
  begin
    raise notice '%',id.f;
	return 1;
  end;
begin
  raise notice '%',var1.f;
  var3.f := NULL;
  var1 := var3;
  id := test_f(var1);
end;
/

--print xiexie 23 test_f test_f1xxx
declare
  var1 integer := 23;
  var2 varchar2(256) := 'xiexie';
  function test_f(id integer) return integer is
    var3 varchar2(256) := 'test_f';
  begin
    raise notice '%',var3;
	return 1;
  end;
  function test_f1(name1 varchar2 default 'xiexie') return integer is
     var4 integer;
	 var5 varchar2(256) := 'test_f1';
  begin
     raise notice '%',var5;
	 raise notice '%',name1;
	 return var4;
  end;
begin
  raise notice '%',var2;
  raise notice '%',var1;
  var1 := test_f(23);
  var1 := test_f1('xxx');
end;
/

--oracle raise error,but we success
declare
  var1 integer := 23;
  var2 varchar2(256) := 'xiexie';
  function test_f(id integer) return integer is
    var3 varchar2(256) := 'test_f';
  begin
    raise notice '%',var3;
	return 1;
  end;
  var4 varchar2(256) := 'after';
  function test_f1(name1 varchar2 default 'xiexie') return integer is
     var4 integer;
	 var5 varchar2(256) := 'test_f1';
  begin
     raise notice '%',var5;
	 raise notice '%',name1;
	 return var4;
  end;
  var5 varchar2(256) := 'after function';
  var6 integer;
begin
  raise notice '%',var2;
  raise notice '%',var1;
  raise notice '%',var5;
  raise notice '%',var6;
  var1 := test_f(23);
  var1 := test_f1('xxx');
end;
/



--declare record type in a package 
CREATE OR REPLACE PACKAGE r_types AUTHID DEFINER IS
  TYPE r_type_1 IS RECORD (f VARCHAR2(5) := 'abcde');
  TYPE r_type_2 IS RECORD (f VARCHAR2(5));
END;
/

create or replace package pkg as
  type mds is record(id integer,name varchar(23));
  gvar mds;
  function test(id integer) return integer;
  function test(id integer,name varchar) return integer;
end pkg;
/

create or replace package public.pkg as
  type mds is record(id integer,name varchar(23));
  gvar mds;
  function test(id integer) return integer;
  function test(id varchar) return varchar;
  function test(id integer,name varchar) return integer;
end;
/

create or replace package body pkg as
  function test(id integer) return integer is
  var1 integer;
  begin
     var1 := id;
     return var1 + 1;
  end;
  function test(id varchar) return varchar is
     var1 varchar(23);
  begin
    var1 := 'welcome to test';
    return var1;
  end;
  function test(id integer,name varchar) return integer is
     var1 integer;
     name1 varchar(25);
  begin
    var1 := id + 1;
    name1 := 'xiexie';
    return 23;
  end;
end pkg;
/

--test ok
create or replace package body pkg as
  function test(id integer) return integer is
  var1 integer;
  begin
     var1 := id;
     return var1 + 1;
  end;
  function test(id integer,name varchar) return integer is
  var1 integer;
  name1 varchar(25);
  begin
    var1 := id + 1;
    name1 := 'xiexie';
    return 23;
  end;
  function test(id varchar) return varchar is
    var1 integer;
  begin
    var1 := 23;
    return id;
  end;
end;
/

--test ok
create or replace package body pkg as
  function test(id integer) return integer is
  var1 integer;
  begin
     var1 := id;
     raise info 'gvar=%', gvar;
     gvar.id := gvar.id + 1;
     gvar.name := 'welcome';
     return var1 + 1;
   end;
   function test(id integer,name varchar) return integer is
     var1 integer;
     name1 varchar(25);
   begin
     var1 := id + 1;
     name1 := 'xiexie';
     return 23;
   end;
   function test(id varchar) return varchar is
     var1 integer;
   begin
     var1 := 23;
	 return id;
   end;
   begin
     gvar.id := 23;
	 gvar.name := 'global init block';
	raise info 'package init block %', gvar;
end;
/

select *from pkg.test(23);

declare
  var integer;
begin
  raise info '%', pkg.gvar;
  pkg.gvar.id := 1;
  pkg.gvar.name := 'package var change';
  raise info 'pkg.gvar=%', pkg.gvar;
end;
/
--INFO:  (24,welcome)
--INFO:  pkg.gvar=(1,"package var change")

declare
  var pkg.mds;
begin
  var.name := 'welcome to beijing';
  raise info '%', var;
end;
/

declare
  var1 pkg.mds;
  var2 pkg.mds;
  var3 pkg.mds;
  name varchar(256);
  function test_package(ids out pkg.mds, ids1 pkg.mds,name1 out varchar) return pkg.mds is
    var1 pkg.mds;
  begin
      ids.id := ids1.id + 1;
      ids.name := ids1.name;
      name1 := 'wel come to pkgtype';
      var1.id := 1;
      var1.name := 'a return pkg type';
      return var1;
  end;
begin
  var1.id := pkg.gvar.id;
  var1.name := pkg.gvar.name;
  raise info 'var1=%', var1;
  var3.id := 0;
  var3.name := 'a in package type';
  var1 := test_package(var2, var3, name);
  raise info 'var1=%',var1;
  raise info 'var2=%', var2;
  raise info 'var3=%', var3;
  raise info 'name=%', name;
end;
/

--package'var as out variable in function
declare
  var1 pkg.mds;
  var3 pkg.mds;
  name varchar(256);
  function test_package(ids out pkg.mds, ids1 pkg.mds,name1 out varchar) return pkg.mds is
    var1 pkg.mds;
  begin
      ids.id := ids1.id + 1;
      ids.name := ids1.name;
      name1 := 'wel come to pkgtype';
      var1.id := 1;
      var1.name := 'a return pkg type';
      return var1;
  end;
begin
  var1.id := pkg.gvar.id;
  var1.name := pkg.gvar.name;
  raise info 'var1=%', var1;
  var3.id := 0;
  var3.name := 'a in package type';
  var1 := test_package(pkg.gvar, var3, name);
  raise info 'var1=%',var1;
  raise info 'pkg.gvar=%', pkg.gvar;
  raise info 'var3=%', var3;
  raise info 'name=%', name;
end;
/


declare
  var1 pkg.mds;
begin
  var1.id := pkg.gvar.id;
  var1.name := pkg.gvar.name;
  raise info 'var1=%', var1;
end;
/

--test call package function
declare
   var1 integer;
 begin
   var1 := pkg.test(23);
   pkg.gvar.id := 123;
   pkg.gvar.name := 'must print me';
   var1 := pkg.test(24);
   var1 := pkg.test(23,'welcome to beijing');
 end;
 /

declare
  var1 pkg.gvar%type;
  var2 pkg.gvar.name%type;
begin
  var1.id := 23;
  var2 := 'a pkg type of type';
  var1.name := var2;
  raise info 'var1=%', var1;
end;
/

--test function rely on package
create or replace function test_package(id integer) return integer is
  var1 pkg.mds;
  var2 integer;
begin
  var1 := pkg.gvar;
  var2 := pkg.test(23);
  pkg.gvar.id := var1.id + 1;
  raise info 'var1=%', var1;
  return var2;
end;
/

--test ok
select * from test_package(23);

drop package pkg;
--test failed
select * from test_package(23);


create or replace package public.pkg as
  type mds is record(id integer,name varchar(23));
  gvar mds;
  function test(id integer) return integer;
  function test(id varchar) return varchar;
  function test(id integer,name varchar) return integer;
end;
/
--test ok
create or replace package body pkg as
  function test(id integer) return integer is
  var1 integer;
  begin
    var1 := id;
    return var1 + 1;
  end;
  function test(id varchar) return varchar is
    var1 varchar(23);
   begin
      var1 := 'welcome to test';
      return var1;
   end;
  function test(id integer,name varchar) return integer is
  var1 integer;
  name1 varchar(25);
  begin
     var1 := id + 1;
     name1 := 'xiexie';
     return 23;
   end;
end pkg;
/

--test ok
select test_package(23);

drop function test_package(integer);


--test package ref cursor and explicit cursor %rowtype
delete from mds;
insert into mds values(1,'welcome to beijing');

create or replace package pkg as
  c1 cursor for select * from mds;
  type test_r is record (id integer default 23,name varchar(23));
  type test_r2 is record (r1 test_r, salary integer);
  test_var1 test_r;
  test_var2 test_r2;
  cur3_rowvar test_var2%type;
  function test(id integer) return integer;
end;
/

create or replace package body pkg as
  function test(id integer) return integer is
  var1 integer;
 begin
   var1 := 25;
   raise info 'test_var1=%', test_var1;
   raise info 'test_var2=%', test_var2;
   raise info 'cur3_rowvar=%', cur3_rowvar;
   return var1;
 end;
end;
/

declare
  var1 integer;
  var2 mds%rowtype;
begin
  pkg.test_var1.name := 'a test_var1';
  pkg.test_var2.salary := 23;

  open pkg.c1;
  fetch pkg.c1 into var2;
  raise info 'var2=%', var2;
  close pkg.c1;

  pkg.cur3_rowvar.salary := 25;

  var1 := pkg.test(23);
end;
/

drop package pkg;



--test function return package'datum
create or replace package pkg is
  type mds is record(id integer, name varchar(23));
  var1 varchar(256);
  var2 mds;
end;
/

create or replace package body pkg is
begin
  var2.id := 1;
  var2.name := 'welcome to tuple';
  var1 := 'return a pkg datum';
  raise info 'init block';
end;
/

--create function
create or replace function test_pa(id integer) return varchar is
  var2 pkg.mds;
  function test_pa1(id integer) return pkg.mds is
  begin
    return pkg.var2;
  end;
begin
  var2 := test_pa1(23);
  raise info 'var2=%',var2;
  return pkg.var1;
end;
/

--access and print init block and pkg.var2 and return a pkg datum
declare
  ret varchar(256);
begin
  ret := test_pa(23);
  raise info 'ret=%', ret;
end;
/

drop function test_pa(integer);
drop package pkg;



-- test re create package body
--recreate
create or replace package pkg is
  var1 constant integer := 23;
  var2 constant integer := 24;
  var3 integer;
end;
/

create or replace package body pkg is
begin
  var3 := 3;
  raise info '%','init block';
end;
/

--print 23,init block,3
Begin
  raise info '%',pkg.var1;
  raise info '%',pkg.var3;
End;
/

--re create body add type and constant
create or replace package body pkg is
  type mdss is record(id integer,name varchar(23));
  var4 constant integer := 4;
begin
   var3 := 3;
   raise info '%','init block';
end;
/

--print init block and 3
begin
  raise info '%',pkg.var3;
end;
/

drop package pkg;



--test package references package
create or replace package pkg is
  type mds is record(id integer,name varchar(256));
  var1 mds;
  function test(id integer, var2 out mds) return mds;
end;
/

create or replace package body pkg is
  function test(id integer, var2 out mds) return mds is
  var3 mds;
  begin
    var2.id := 1;
    var2.name := 'a out name';
    var3.id := var2.id + 1;
    var3.name := var2.name || ' change to return';
    return var3;
  end;
end;
/

create or replace package pkg1 is
  type mdss is record(id integer, name varchar(256));
  var1 mdss;
  var2 pkg.mds;
  function test(id integer, var3 out mdss) return mdss;
end;
/

create or replace package body pkg1 is
  function test(id integer, var3 out mdss) return mdss is
  var4 mdss;
  begin
    var3.id := 0;
    var3.name := 'a pkg1 name';
    var4.id := var3.id + 1;
    var4.name := var3.name || ' pkg1 to return';
    return var4;
  end;
end;
/

--invok
declare
  var1 pkg1.mdss;
  var2 pkg.mds;
  var3 pkg1.mdss;
  var4 pkg.mds;
begin
  var4 := pkg.test(23,var2);
  var3 := pkg1.test(23,var1);
  raise info 'var2=%,var4=%',var2,var4;
  raise info 'var1=%,var3=%',var1,var3;
end;
/

--create a function rely on pkg1
create or replace function test_rely(id integer) return integer is
  var1 pkg1.mdss;
begin
  var1.id := pkg1.var1.id;
  var1.name := 'a rely function';
  pkg1.var1.name := 'a change value';
  raise info 'pkg1.var1=%', pkg1.var1;
  return 23;
end;
/

--call function print (,"a change value")
select * from test_rely(23);

--test print (23,"a change value")
declare
  var1 integer;
begin
  pkg1.var1.id := 23;
  var1 := test_rely(23);
end;
/

--drop package pkg;
drop package pkg;

--call function and report error
select * from test_rely(23);

drop package pkg1;
drop function test_rely(integer);


--test body reference package variable and pkg.xx
create or replace package pkg is
  type mds is record(id integer,name varchar(256));
  var1 mds;
  function test(id integer) return integer;
  function test(id varchar) return integer;
end;
/

create or replace package body pkg is
  var4 pkg.mds;
  function test(id integer) return integer is
  var1 pkg.mds;
  begin
    pkg.var1.id := pkg.var1.id + 1;
    pkg.var1.name := pkg.var1.name || 'xiexie';
    var1.id := pkg.var1.id + 1;
    var1.name := 'welcome to beijing';
    raise info 'var1=%',var1;
    return 0;
  end;
  function test(id varchar) return integer is
  var1 integer;
  begin
     var1 := pkg.test(23);
      return pkg.var1.id;
  end;
begin
  pkg.var1.id := 0;
  pkg.var1.name := 'init block';
  raise info 'init block';
end;
/

--print init block var1=(2,"welcome to beijing") pkg.var1=(1,"init blockxiexie")
declare
  ret integer;
begin
  ret := pkg.test('welcome');
  raise info 'pkg.var1=%',pkg.var1;
end;
/

--accesss block variable and report error
begin
 pkg.var4.id := 1;
end;
/

drop package pkg;



--test schema function argument and return type rely on package
--create package and body
create or replace package pkg is
  type type1 is record(id integer,name varchar(256));
  var1 type1;
  type type2 is record(id integer,name varchar(256));
end;
/

create or replace package body pkg is
begin
  raise info 'init block';
end;
/

create or replace package pkg1 is
  type type1 is record(id integer,name varchar(23));
  type type2 is record(id integer,name varchar(23));
end;
/

--create schema function which argument rely on package
create or replace function func_relay_pkg(id integer, pkg_type pkg.type1) return integer is
begin
  raise info '%',pkg_type.name;
  return 23;
end;
/

--reload failed
create function func_relay_pkg(id integer, pkg_type pkg1.type2) return integer is
begin
  raise info '%', pkg_type.name;
  return 23;
end;
/

--test ok
declare
 id integer;
 var1 pkg.type1;
begin
  var1.id := 23;
  var1.name := 'print me';
  id := func_relay_pkg(23, var1);
end;
/

-- test failed
declare
  id2 integer;
  type type1 is record(id integer,name varchar(256));
  var1 type1;
begin
   var1.id := 23;
   var1.name := 'print me';
   id2 := func_relay_pkg(23, var1);
end;
/
declare
  id2 integer;
  type type1 is record(id integer,name varchar(256));
  var1 type1;
begin
   var1.id := 23;
   var1.name := 'print me';
   id2 := func_relay_pkg(pkg_type =>var1, id =>23);
end;
/


--create schema function which return type is in package
create or replace function test_fun(id integer, name varchar) return pkg.type1 is
  var1 pkg.type1;
begin
  var1.id := 23;
  var1.name := 'return me';
  return var1;
end;
/

--test ok
declare
  var1 pkg.type1;
begin
  var1 := test_fun(23,'xiexie');
  raise info '%',var1.name;
end;
/

--create function use out
create or replace function test_out(id integer, tarray out pkg.type1) return pkg.type1 is
  var1 pkg.type1;
begin
  var1.id := 1;
  var1.name := 'xiexie';
  tarray.id := 2;
  tarray.name := 'out value';
  return var1;
end;
/

--test ok
declare
  var1 pkg.type1;
  var2 pkg.type1;
begin
  var1 := test_out(23, var2);
  raise info '%', var1.name;
  raise info '%',var2.name;
end;
/


drop function test_out(integer, orarecord);
drop function test_fun(integer, varchar);
drop function func_relay_pkg(integer, orarecord);
drop package pkg;


--test %type
create or replace package pkg is
  type rec is record(id integer,name varchar(23));
  var1 rec;
  var2 integer;
end;
/

create or replace function test_return_record(ids integer) return pkg.var1%type
is
  id pkg.var1%type;
begin
  id.id := 2;
  id.name := 'welcome to beijing';
  return id;
end;
/

select test_return_record(23) from dual;

create or replace function test_return_record_out(id out pkg.var1%type, name out varchar, id1 out pkg.var1%type) return integer
is
begin
  id.id := 1;
  id.name := 'first out name';
  name := 'second out varchar';
  id1.id := 2;
  id1.name := 'third out name';
  return 23;
end;
/

declare
  var1 pkg.var1%type;
  var2 pkg.var1%type;
  name varchar(25);
  id integer;
begin
  id := test_return_record_out(var1,name,var2);
  raise info '%,%,%', var1,name,var2;
end;
/

drop function test_return_record_out(orarecord, varchar, orarecord);

create or replace function test_return_record_out(id out pkg.var1%type, name out varchar, id1 out pkg.var1%type) return void
is
begin
  id.id := 1;
  id.name := 'first out name';
  name := 'second out varchar';
  id1.id := 2;
  id1.name := 'third out name';
end;
/

declare
  var1 pkg.var1%type;
  var2 pkg.var1%type;
  name varchar(25);
  id integer;
begin
  perform test_return_record_out(var1,name,var2);
  raise info '%,%,%', var1,name,var2;
end;
/

create or replace function test_record_out(id out pkg.var1%type) return void
is
begin
  id.id := 1;
  id.name := 'first out name';
end;
/

declare
  var1 pkg.var1%type;
begin
  perform test_record_out(var1);
  raise info '%', var1;
end;
/

drop function test_return_record_out(orarecord, varchar, orarecord);
drop function test_record_out(orarecord);
drop function test_return_record(integer);
drop package pkg;
drop package pkg1;



--test package references itself by package name
--test ok
create or replace package pkg is
type mds1 is record(id integer,name varchar(23));
function test(id integer) return integer;
var1 mds1;
var2 pkg.mds1;
var3 pkg.mds1;
end;
/

--test ok
create or replace package body pkg is
 var4 pkg.mds1;
 function test(id integer) return integer is
 begin
   pkg.var2.id := 23;
   pkg.var2.name := 'a schema pkg test';
   raise info '%', pkg.var2.name;
   return 23;
 end;
begin
  var4.id := 1;
  var4.name := 'a package';
  var4.id := pkg.test(23);
end;
/


--print twice a schema pkg test
declare
  var1 integer;
begin
  var1 := public.pkg.test(23);
end;
/

--print a schema pkg test
declare
  var1 integer;
begin
  var1 := public.pkg.test(23);
end;
/

drop package public.pkg;

--test failed
create or replace package pkg is
  type mds1 is record(id integer,name varchar(23));
  function test(id integer) return integer;
  var1 mds1;
  var2 pkg.mds1;
  var3 pkg.mds3;
end;
/


--test schema and package as the same name
create schema pkg;
set search_path to pkg;

create or replace package pkg is
 type mds is record(id integer,name varchar(23));
 pkg1 mds;
end;
/

create or replace package body pkg is
begin
  pkg.pkg1.id := 1;
  pkg.pkg1.name := 'init string';
  raise info 'init pkg.pkg';
end;
/

create or replace package pkg1 is
 var1 integer;
 function test(id integer) return integer;
end;
/

create or replace package body pkg1 is
 function test(id integer) return integer is
 begin
    return pkg.pkg1.id;
 end;
begin
  raise info 'init pkg.pkg1';
end;
/

declare
  var1 integer;
begin
  var1 := pkg1.test(23);
  raise info 'var1=%,pkg.pkg1.id=%', var1, pkg.pkg1.id;
end;
/

drop package pkg;
drop package pkg1;
drop schema pkg;



set search_path to public;
create table test(id integer,name varchar(23));

create or replace package pkg is
  id integer;
  function test(id out integer) return test;
end;
/

create or replace package body pkg is
  function test(id out integer) return test is
    var1 test%rowtype;
  begin
    id := pkg.id;
    var1.id := pkg.id + 1;
    var1.name := 'xiexie';
    return var1;
end;
begin
  raise  info '%', 'init block';
end;
/

declare
  type mds is record(id integer,name varchar(23));
  var1 mds;
  var2 int;
  i int;
begin
  for i in 1 .. 10 LOOP
    pkg.id := i;
	var1 := pkg.test(var2);
	raise info 'var1=%,var2=%', var1,var2;
  end loop;
end;
/

drop package pkg;
drop table test;


create or replace package dwdai_pkg is
  type mds is record(id integer,name varchar(23));
  var1 integer;
  function test(id integer,name varchar) return integer;
end;
/

create or replace package dwdai_pkg1 is
  type mds1 is record(id integer,name varchar(23));
  var1 integer;
  function f1(id integer) return integer;
end;
/

create or replace package body dwdai_pkg is
  function test(id integer,name varchar) return integer is
    var1 integer;
    var2 dwdai_pkg1.mds1;
  begin
     dwdai_pkg1.var1 := 23;
     var1 := 23;
  end;
end;
/


create or replace package body dwdai_pkg1 is
  function f1(id integer) return integer is
    var1 integer;
    var2 dwdai_pkg.mds;
  begin
     dwdai_pkg.var1 := 23;
     var1 := 23;
  end;
end;
/

declare
  var1 integer;
begin
  var1 := dwdai_pkg1.var1;
end;
/

drop package dwdai_pkg1;
drop package dwdai_pkg;

--test refernece its self
create or replace package dwdai_pkg is
  type mds is record(id integer,name varchar(23));
  var1 integer;
  function test(id integer,name varchar) return integer;
  var2 public.dwdai_pkg.mds;
end;
/

create or replace package body dwdai_pkg is
  var3 public.dwdai_pkg.mds;
  function test(id integer,name varchar) return integer is
	var2 public.dwdai_pkg.mds;
  begin
      dwdai_pkg.var1 := 23;
      public.dwdai_pkg.var1 := 23;
      return 23;
  end;
end;
/

drop package dwdai_pkg;


create or replace package pkg_end as
  type mds is record(id integer,name varchar(23));
  gvar mds;
  function test(id integer) return integer;
  function test(id varchar) return varchar;
  function test(id integer,name varchar) return integer;
end;
/
--test ok
create or replace package body pkg_end as
  function test(id integer) return integer is
  var1 integer;
  begin
     var1 := id;
     return var1 + 1;
  end test;
  function test(id varchar) return varchar is
     var1 varchar(23);
  begin
       var1 := 'welcome to test';
       return var1;
  end test;
  function test(id integer,name varchar) return integer is
  PROCEDURE GET_YSBDS AS 
  V_ITEMPSTR VARCHAR2(6000); 
  BEGIN 
    raise notice 'hello on nest test';
  END GET_YSBDS; 
  begin
    GET_YSBDS();
    return 23;
  end test;
end pkg_end;
/
BEGIN
  raise notice '%', pkg_end.test(1);
  raise notice'%', pkg_end.test(1,'hello');
  raise notice'%', pkg_end.test('hello');
END;
/
drop package pkg_end;



create or replace package pkg_end as
	type mds is record(id integer,name varchar(23));
	gvar mds;
	function test(id integer) return integer;
	function test(id varchar) return varchar;
	function test(id integer,name varchar) return integer;
end;
/
--test ok
create or replace package body pkg_end as
	function test(id integer) return integer is
		var1 integer;
	begin
		var1 := id;
		return var1 + 1;
	end test;
	function test(id varchar) return varchar is
		var1 varchar(23);
	begin
		var1 := 'welcome to test';
		return var1;
	end test;
	function test(id integer,name varchar) return integer is
		PROCEDURE GET_YSBDS AS
			V_ITEMPSTR VARCHAR2(6000);
		BEGIN
			dbms_output.put_line('hello on nest test');
		END GET_YSBDS;
	begin
		GET_YSBDS();
		return 23;
	end test;
end pkg_end;
/

declare
	id integer;
begin
	id := pkg_end.test(23);
	raise info '%', id;
end;
/
drop package pkg_end;


create or replace package dwdai_pkg is
	procedure test(id integer);
	function test1(id integer) return integer;
	type mds is record(id integer,name varchar(23));
	var1 mds;
end;
/

create or replace package body dwdai_pkg is
	type test2 is record(id integer,name varchar(23));
	var2 mds;
	procedure test(id integer) is
		procedure test_inline(id integer);
		procedure test_inline(id integer) is
		begin
			raise info '%', id;
		end;
		var3 integer;
	begin
		raise info '%', id;
	end;
	function func_test2(id integer) return integer is
		function test_inline2(id integer) return integer is
		begin
			return 24;
		end;
	begin
		return 23;
	end;
	function test1(id integer) return integer is
		procedure test22(id integer) is
		begin
			raise info '%', id;
		end;
	begin
		return 25;
	end;
begin
	var1.id := 23;
end;
/

begin
  dwdai_pkg.test(23);
end;
/



create schema test;

create or replace package test.pkg as
	type mds is record(id integer,name varchar(23));
	var1 mds;
	gvar int;
end pkg;
/

--test ok
create or replace package body test.pkg as
begin
	gvar := 1;
	var1.id := 23;
	var1.name := 'xiexie';
end;
/

declare
	var1 int;
	var2 test.pkg.mds;
	var3 varchar(25);
begin
	var1 := test.pkg.gvar;
	var2 := test.pkg.var1;
	var3 := test.pkg.var1.name;
	raise info '%,%,%', var1,var2,var3;
end;
/

drop package test.pkg;
drop schema test;


CREATE OR REPLACE PACKAGE pkg AS
    TYPE rec_type IS RECORD (id integer, name varchar2(20));
    pvar rec_type;
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg AS
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer IS
        var1 pvar%TYPE;
    BEGIN
        var1.id := ivar.id + 1;
        var1.name := 'Bob#' || var1.id;
        pvar := var1;
        RETURN var1.id;
    END;
END;
/

DECLARE
    var1 pkg.pvar%TYPE;
BEGIN
    var1.id := 1;
    var1.name := 'Bob#' || var1.id;

    raise notice '% . %', var1.id , var1.name;
    raise notice '% . %', pkg.rec_test(var1), pkg.pvar.name;
END;
/


CREATE OR REPLACE PACKAGE pkg AS
    TYPE rec_type IS RECORD (id integer, name varchar2(20));
    pvar rec_type;
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg AS
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer IS
        var1 pvar%TYPE;
    BEGIN
        raise notice '% . %', ivar.id, ivar.name;
        var1.id := ivar.id + 1;
        var1.name := 'Bob#' || var1.id;
        pvar.id := var1.id;
        pvar.name := var1.name;
        RETURN var1.id;
    END;
END;
/

DECLARE
    var1 pkg.pvar%TYPE;
BEGIN
    var1.id := 1;
    var1.name := 'Bob#' || var1.id;

    raise notice '% . %', pkg.rec_test(var1), pkg.pvar.name;
END;
/


CREATE OR REPLACE PACKAGE pkg AS
    TYPE rec_type IS RECORD (id integer, name varchar2(20));
    pvar rec_type;
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg AS
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer IS
        var1 pvar%TYPE;
    BEGIN
        var1.id := ivar.id + 1;
        var1.name := 'Bob#' || var1.id;
        pvar := var1;
        RETURN var1.id;
    END;
END;
/

DECLARE
    var1 pkg.pvar%TYPE;
	var2 varchar2(256);
BEGIN
    var1.id := 1;
    var1.name := 'Bob#' || var1.id;

    raise notice '% . %', var1.id, var1.name;
	var2 := pkg.rec_test(var1)  || pkg.pvar.name;
    raise notice '%', var2;
END;
/

CREATE OR REPLACE PACKAGE pkg AS
    TYPE rec_type IS RECORD (id integer, name varchar2(20));
    pvar rec_type;
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer;
	function rect_test1(id integer) return integer;
END;
/

CREATE OR REPLACE PACKAGE BODY pkg AS
    FUNCTION rec_test(ivar pkg.pvar%TYPE) RETURN integer IS
        var1 pvar%TYPE;
    BEGIN
        var1.id := ivar.id + 1;
        var1.name := 'Bob#' || var1.id;
        pvar := var1;
        RETURN var1.id;
    END;
	function rect_test1(id integer) return integer is
	   var2 rec_type;
	begin
	   var2.id := 1;
	   var2.name := 'Bob#' || var2.id;
	   raise notice '% . %', rec_test(var2), pvar.name;
	   return 23;
	end;
END;
/

select pkg.rect_test1(23) from dual;


--complex package init
CREATE OR REPLACE package test_complex_pkg IS
  c1 CURSOR FOR SELECT * FROM dual;
  type mds IS record(id integer,name varchar2(256));
  var1 mds;
  FUNCTION test_f(id out integer) RETURN varchar2;
  FUNCTION test_f2(id integer) RETURN integer;
end;
/

CREATE OR REPLACE package body test_complex_pkg IS
  var2 mds;
  i integer;
  FUNCTION test_f(id out integer) RETURN varchar2 IS
  BEGIN
	id := var1.id;
	RETURN var1.name;
  end;
  FUNCTION test_f2(id integer) RETURN integer IS
  BEGIN
     RETURN var2.id;
  end;
BEGIN
  var1.id := 23;
  var1.name := 'welcome to beijing' || var1.id;
  var2.name := test_f(var2.id);
  FOR i IN 1 .. 10 LOOP
    var2.name := test_f(var2.id);
	var1.id := var1.id + var2.id;
	raise info '%',var1;
	raise info '%', var2;
  END loop;
end;
/

SELECT test_complex_pkg.test_f2(23) FROM dual;
SELECT test_complex_pkg.test_f2(23) FROM dual;

DROP package test_complex_pkg;


create or replace package test_pkg1 is
  type mds is record(id integer,name varchar2(256));
  type mds1 is record(id integer,name varchar2(256));
  type mds2 is record(id integer,name varchar2(256));
  type mds3 is record(id integer,name varchar2(256));
  type mds4 is record(id integer,name varchar2(256));
  type mds5 is record(id integer,name varchar2(256));
  type mds6 is record(id integer,name varchar2(256));
  type mds7 is record(id integer,name varchar2(256));
  type mds8 is record(id integer,name varchar2(256));
  type mds9 is record(id integer,name varchar2(256));
  type mds10 is record(id integer,name varchar2(256));
  type mds11 is record(id integer,name varchar2(256));
  type mds12 is record(id integer,name varchar2(256));
  type mds13 is record(id integer,name varchar2(256));
  type mds14 is record(id integer,name varchar2(256));
  type mds15 is record(id integer,name varchar2(256));
  type mds16 is record(id integer,name varchar2(256));
  type mds17 is record(id integer,name varchar2(256));
  type mds18 is record(id integer,name varchar2(256));
  type mds19 is record(id integer,name varchar2(256));
end;
/

create or replace package test_pkg is
  type mds is record(id integer,name varchar2(256));
  type mds1 is record(id integer,name varchar2(256));
  type mds2 is record(id integer,name varchar2(256));
  type mds3 is record(id integer,name varchar2(256));
  type mds4 is record(id integer,name varchar2(256));
  type mds5 is record(id integer,name varchar2(256));
  type mds6 is record(id integer,name varchar2(256));
  type mds7 is record(id integer,name varchar2(256));
  type mds8 is record(id integer,name varchar2(256));
  type mds9 is record(id integer,name varchar2(256));
  type mds10 is record(id integer,name varchar2(256));
  type mds11 is record(id integer,name varchar2(256));
  type mds12 is record(id integer,name varchar2(256));
  type mds13 is record(id integer,name varchar2(256));
  type mds14 is record(id integer,name varchar2(256));
  function test_f(id2 integer) return integer;
end;
/

create or replace package body test_pkg is
  var1 test_pkg1.mds1;
  function test_f(id2 integer) return integer is
    type mds15 is record(id integer,name varchar2(256));
    type mds16 is record(id integer,name varchar2(256));
    type mds17 is record(id integer,name varchar2(256));
    type mds18 is record(id integer,name varchar2(256));
  begin
     return 1;
  end;
end;
/

SELECT test_pkg.test_f(23) FROM dual;

DROP package test_pkg;
DROP package test_pkg1;


--INSERT INTO table_name VALUES rec_var
DELETE FROM schedule;
DECLARE
  TYPE myrecord IS RECORD(  week  NUMBER,
  Mon   VARCHAR2(10),
  Tue   VARCHAR2(10),
  Wed   VARCHAR2(10),
  Thu   VARCHAR2(10),
  Fri   VARCHAR2(10),
  Sat   VARCHAR2(10),
  Sun   VARCHAR2(10)
  );
  default_week  myrecord;
BEGIN
  default_week.Mon := '0800-1700';
  default_week.Tue := '0800-1700';
  default_week.Wed := '0800-1700';
  default_week.Thu := '0800-1700';
  default_week.Fri := '0800-1700';
  default_week.Sat := 'Day Off';
  default_week.Sun := 'Day Off';
  FOR i IN 1 .. 6 LOOP
    default_week.week    := i;
    INSERT INTO schedule VALUES default_week;
  END LOOP;
END;
/
SELECT * FROM schedule order by week;

delete from mds;
declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
begin
  var1.id := 23;
  var1.name_1 := 'xiexie';
  insert into mds values var1;
end;
/
select * from mds order by ids;

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
  var2 sds;
begin
  var1.id := 24;
  var1.name_1 := 'wecome';
  insert into mds values var1 returning ids,name into var2;
  raise notice '%',var2;
end;
/

select * from mds order by ids;
declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
  var2 record;
begin
  var1.id := 25;
  var1.name_1 := 'a';
  insert into mds values var1 returning ids,name into var2;
  raise notice '%',var2;
  insert into mds values var2;
end;
/

select * from mds order by ids;

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
  var2 varchar(24);
begin
  var1.id := 26;
  var1.name_1 := 'b';
  insert into mds values var1 returning name into var2;
  raise notice '%',var2;
end;
/

--test record null to insert
delete from mds;
declare
  var1 record;
begin
  select * into var1 from mds;
  insert into mds values var1;
end;
/
select * from mds order by ids;
select count(*) from mds;


--If the VALUES clause of an INSERT statement contains a record variable, no other value is allowed in the clause.
DECLARE
  TYPE myrecord IS RECORD(Mon   VARCHAR2(10),
  Tue   VARCHAR2(10),
  Wed   VARCHAR2(10),
  Thu   VARCHAR2(10),
  Fri   VARCHAR2(10),
  Sat   VARCHAR2(10),
  Sun   VARCHAR2(10)
  );
  default_week  myrecord;
BEGIN
  default_week.Mon := '0800-1700';
  default_week.Tue := '0800-1700';
  default_week.Wed := '0800-1700';
  default_week.Thu := '0800-1700';
  default_week.Fri := '0800-1700';
  default_week.Sat := 'Day Off';
  default_week.Sun := 'Day Off';

  INSERT INTO schedule(Mon,
     Tue,
     Wed,
     Thu,
     Fri,
     Sat,
     Sun)
   VALUES default_week;
END;
/

create table other_max(userid number(8) generated always as identity  start with 1 increment by 2,uname varchar2(20));

DECLARE
  TYPE rec_type IS RECORD (description varchar2(20));
  r rec_type := rec_type('a');
BEGIN
  INSERT INTO other_max(uname) VALUES r;
END;
/

create table idcol_alter5 (userid number generated BY DEFAULT as identity  (start with 20 maxvalue 22),uname varchar2(20));
create table idcol_alter6 (userid number generated BY DEFAULT ON NULL as identity  (start with 20 maxvalue 22),uname varchar2(20));

DECLARE
  TYPE rec_type IS RECORD (description varchar2(20));
  r rec_type := rec_type('a');
BEGIN
  INSERT INTO idcol_alter5(uname) VALUES r;
END;
/

DECLARE
  TYPE rec_type IS RECORD (description varchar2(20));
  r rec_type := rec_type('a');
BEGIN
  INSERT INTO idcol_alter6(uname) VALUES r;
END;
/

--test failed
declare
  type sds is record(id integer);
  var1 sds;
begin
  var1.id := 23;
  insert into mds values var1;
end;
/


--Updating Rows with Records
SELECT * FROM schedule order by week;
declare
  TYPE myrecord IS RECORD(  week  NUMBER,
  Mon   VARCHAR2(10),
  Tue   VARCHAR2(10),
  Wed   VARCHAR2(10),
  Thu   VARCHAR2(10),
  Fri   VARCHAR2(10),
  Sat   VARCHAR2(10),
  Sun   VARCHAR2(10)
  );
  default_week  myrecord;
begin
  default_week.mon := 'Day Off';
  default_week.tue := '0900-1800';
  default_week.wed := '0900-1800';
  default_week.thu := '0900-1800';
  default_week.fri := '0900-1800';
  default_week.sat := '0900-1800';
  default_week.sun := 'Day Off';
  FOR i IN 1 .. 3 LOOP
    default_week.week    := i;
    UPDATE schedule
    SET ROW = default_week
    WHERE week = i;
  END LOOP;
end;
/
SELECT * FROM schedule order by week;


DELETE FROM mds;
INSERT INTO mds VALUES(1, 'a');

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
begin
  var1.id := 24;
  var1.name_1 := 'wecome';
  update mds set row = var1;
end;
/

select * from mds;

--record columns datatype are compatible with table columns datatype
create cast(varchar as integer) with inout as implicit;

declare
  type sds is record(id varchar(23),name_1 varchar(23));
  var1 sds;
begin
  var1.id := 22;
  var1.name_1 := 'hi';
  update mds set row = var1;
end;
/
select * from mds;
drop cast(varchar as integer);

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
  var2 varchar(23);
begin
  var1.id := 25;
  var1.name_1 := 'a';
  update mds set row = var1 returning name into var2;
  raise notice '%', var2;
end;
/

declare
  var1 record;
  var2 varchar(23);
begin
  select * into var1 from mds;
  update mds set row = var1 returning name into var2;
  raise notice '%', var2;
end;
/

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 sds;
  var2 sds;
begin
  var1.id := 26;
  var1.name_1 := 'b';
  update mds set row = var1 returning ids,name into var2;
  raise notice '%', var2;
end;
/

declare
  type sds is record(id integer,name_1 varchar(23));
  var1 record;
  var2 sds;
begin
  select * into var1 from mds;
  update mds set row = var1 returning ids,name into var2;
  raise notice '%', var2;
  raise notice '%', var1;
end;
/

--ERROR:  not enough values
DECLARE
  TYPE rec_type IS RECORD (description varchar2(20));
  r rec_type := rec_type('a');
BEGIN
  UPDATE  idcol_alter5 SET ROW =  r;
END;
/

--ERROR:  not enough values
DECLARE
  TYPE rec_type IS RECORD (description varchar2(20));
  r rec_type := rec_type('a');
BEGIN
  UPDATE idcol_alter6 SET ROW = r;
END;
/

--execption about Updating Rows with Records
--test failed columns not match
declare
  type sds is record(id integer,name_1 varchar(23),name_2 varchar(24));
  var1 sds;
begin
  var1.id := 23;
  var1.name_1 := 'welcome';
  var1.name_2 := 'to beijing';
  update mds set ROW = var1;
end;
/

--test failed for column less
declare
  type sds is record(id integer);
  var1 sds;
begin
  var1.id := 23;
  update mds set ROW = var1;
end;
/

--the record filed is %rowtype
delete from mds;
insert into mds values(1,'welcome to beijing');

declare
  type test_r is record (ids integer default 23,name varchar(23));
  var2 test_r;
  cursor c1 IS select * from mds;
  c2 c1%rowtype;
begin
  open c1;
  fetch c1 into c2;
  raise notice 'c2: %,%', c2.ids, c2.name;
  var2 := c2;
  raise notice 'var2: %,%', var2.ids, var2.name;
  close c1;
end;
/

--error
declare
  type test_r is record (ids integer default 23,name varchar(23));
  var2 test_r;
  cursor c1 IS select * from mds;
  c2 c1%rowtype;
begin
  select * into var2 from mds;
  raise notice 'var2: %,%', var2.ids, var2.name;
  c2 := var2;
  raise notice 'c2: %,%', c2.ids, c2.name;
  close c1;
end;
/

declare
  type test_r is record (ids integer default 23,name varchar(23));
  var2 test_r;
  cursor c1 IS select * from mds;
begin
  open c1;
  fetch c1 into var2;
  raise notice 'var2: %,%', var2.ids, var2.name;
  close c1;
end;
/

declare
  type test_r is record(a mds%rowtype);
  cursor c1 IS select * from mds;
  type test_r2 is record(a c1%rowtype);
  r test_r;
  c c1%rowtype;
  r2 test_r2;
begin
  r.a.ids := 101;
  r.a.name := 'a';
  raise notice 'r: %,%', r.ids, r.name;
end;
/

declare
  type test_r is record(a mds%rowtype);
  cursor c1 IS select * from mds;
  type test_r2 is record(a c1%rowtype);
  r test_r;
  c c1%rowtype;
  r2 test_r2;
begin
  r.a.ids := 102;
  r.a.name := 'b';
  raise notice 'r: %,%', r.a.ids, r.a.name;
end;
/

declare
  cursor c1 IS select * from mds;
  type test_r2 is record(a c1%rowtype);
  c c1%rowtype;
  r2 test_r2;
begin
  r2.a.ids := 103;
  r2.a.name := 'c';
  raise notice 'r: %,%', r2.a.ids, r2.a.name;
end;
/

--nested record filed is %rowtype
declare
  type test_r is record (ids integer default 23,name varchar(23));
  type test_r2 is record (c1 test_r);
  var2 test_r2;
  cursor c1 IS select * from mds;
  c2 c1%rowtype;
begin
  open c1;
  fetch c1 into c2;
  raise notice 'c2: %,%', c2.ids, c2.name;
  var2.c1 := c2;
  raise notice 'var2: %,%', var2.c1.ids, var2.c1.name;
  close c1;
end;
/

--error
declare
  type test_r is record (ids integer default 23,name varchar(23));
  type test_r2 is record (c1 test_r);
  var2 test_r2;
  cursor c1 IS select * from mds;
  c2 c1%rowtype;
begin
  select * into var2.c1 from mds;
  raise notice 'var2: %,%', var2.c1.ids, var2.c1.name;
  c2 := var2.c1;
  raise notice 'c2: %,%', c2.ids, c2.name;
  close c1;
end;
/

declare
  type test_r is record (ids integer default 23,name varchar(23));
  type test_r2 is record (c1 test_r);
  var2 test_r2;
  cursor c1 IS select * from mds;
begin
  open c1;
  fetch c1 into var2.c1;
  raise notice 'var2: %,%', var2.c1.ids, var2.c1.name;
  close c1;
end;
/

declare
  type test_r is record(a mds%rowtype);
  type test_r2 is record(c1 test_r);
  cursor c1 IS select * from mds;
  c c1%rowtype;
  r2 test_r2;
begin
  r2.c1.a.ids := 104;
  r2.c1.a.name := 'd';
  raise notice 'r: %,%', r2.c1.a.ids,r2.c1.a.name;
end;
/

declare
  cursor c1 IS select * from mds;
  type test_r1 is record(a c1%rowtype);
  type test_r2 is record(col test_r1);
  c c1%rowtype;
  r2 test_r2;
begin
  r2.col.a.ids := 105;
  r2.col.a.name := 'e';
  raise notice 'r: %,%', r2.col.a.ids, r2.col.a.name;
end;
/


--test *_arguments views
set ivorysql.identifier_case_switch = INTERCHANGE;
create or replace package pkg is
  type rt is record(id int,name varchar(10));
end;
/
create or replace function fun1(c2 number, c1 pkg.rt) return pkg.rt is
begin
  return c1;
end;
/
select argument_name,data_type,pls_type from all_arguments where object_name='FUN1';
select argument_name,data_type,pls_type from dba_arguments where object_name='FUN1';
select argument_name,data_type,pls_type from user_arguments where object_name='FUN1';

create or replace procedure pro1(col pkg.rt) is
begin
  null;
end;
/

select argument_name,data_type,pls_type from all_arguments where object_name='PRO1';
select argument_name,data_type,pls_type from dba_arguments where object_name='PRO1';
select argument_name,data_type,pls_type from user_arguments where object_name='PRO1';

reset ivorysql.identifier_case_switch;

--clean table
drop table mds;
drop table mds2;
drop table employees;
drop table schedule;
drop package pkgt;
drop table pkg_t;
drop package My_Types;
drop table tabpackage;
drop table test_mds;
drop function test_f;
drop function test_main;
drop function test_main_pk;
drop table test_insert;
drop type test_inline_type;
drop table other_max;
drop table idcol_alter5;
drop table idcol_alter6;
drop function fun1(number, pkg.rt);
drop procedure pro1(pkg.rt);
drop package pkg;
--
-- Tests for plisql's autonomous transaction.
--
-- ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION
--

--
-- supported autonomous routine
--
create table ax_employees(id number primary key, salary number);
create table ax_log(log_id number(6),new_sal number(8,2),old_sal number(8,2));

-- create procedure
create or replace procedure nm_lower(empid number, amount number) as
begin
  update ax_employees set salary = salary - amount where id = empid;
end;
/

-- create autonomous procedure
create or replace procedure ax_lower(empid number, amount number) as
  pragma autonomous_transaction;
begin
  update ax_employees set salary = salary - amount where id = empid;
end;
/

-- create autonomous procedure with subproc
create or replace procedure ax_lower_subproc(empid number, amount number) as
  pragma autonomous_transaction;
  procedure subproc is
    pragma autonomous_transaction;
  begin
    update ax_employees set salary = salary - amount where id = empid;
    raise notice 'this is output of nested subproc';
    commit;
  end subproc;
begin
  subproc;
  update ax_employees set salary = salary - amount where id = empid;
  rollback;
end;
/

-- create function
create or replace function nmf_lower(empid number, amount number) return number as
  new_sal number;
begin
  update ax_employees set salary = salary - amount where id = empid;
  select salary into new_sal from ax_employees where id = empid;
  return new_sal;
end;
/

-- create autonomous function
create or replace function atf_lower(empid number, amount number) return number as
  pragma autonomous_transaction;
  new_sal number;
begin
  update ax_employees set salary = salary - amount where id = empid;
  select salary into new_sal from ax_employees where id = empid;
  return new_sal;
end;
/

-- create package
create or replace package emp_action as
  procedure nm_raise(empid number, amount number);
  procedure ax_raise(empid number, amount number);
  function nmf_raise(empid number, amount number) return number;
  function atf_raise(empid number, amount number) return number;
end;
/

create or replace package body emp_action as
  procedure nm_raise(empid number, amount number) as
  begin
    update ax_employees set salary = salary + amount where id = empid;
  end;

  procedure ax_raise(empid number, amount number) as
    pragma autonomous_transaction;
  begin
    update ax_employees set salary = salary + amount where id = empid;
  end;

  function nmf_raise(empid number, amount number) return number as
    new_sal number;
  begin
    update ax_employees set salary = salary + amount where id = empid;
    select salary into new_sal from ax_employees where id = empid;
    return new_sal;
  end;

  function atf_raise(empid number, amount number) return number as
    pragma autonomous_transaction;
    new_sal number;
  begin
    update ax_employees set salary = salary + amount where id = empid;
    select salary into new_sal from ax_employees where id = empid;
    return new_sal;
  end;
end;
/

-- create autonomous function for trigger
create or replace function ax_log_sal_proc() return trigger as
  pragma autonomous_transaction;
begin
  insert into ax_log(log_id,new_sal,old_sal) values(old.id, new.salary, old.salary);
  return new;
end;
/

-- create trigger
create or replace trigger ax_log_sal_trig
  before update of salary on ax_employees for each row
  execute function ax_log_sal_proc();

-- create autonomous procedure for corss-checking
create or replace procedure ax_cross as
  pragma autonomous_transaction;
begin
  emp_action.ax_raise(1, 10);
  ax_lower(1, 10);
  raise notice '%', emp_action.atf_raise(2, 10);
  emp_action.nm_raise(2, 10);
  commit;
  nm_lower(2, 20);
  commit;
  emp_action.nm_raise(3, 10);
  commit;
  raise notice '%', emp_action.atf_raise(3, 10);
  nm_lower(3, 20);
  commit;
  ax_lower(4, 20);
  raise notice '%', emp_action.atf_raise(4, 20);
  raise notice '%', emp_action.nmf_raise(4, 10);
  rollback;
end;
/

-- create procedure for corss-checking
create or replace procedure nm_cross as
begin
  emp_action.ax_raise(1, 10);
  ax_lower(1, 10);
  raise notice '%', emp_action.atf_raise(2, 10);
  nm_lower(2, 10);
  raise notice '%', atf_lower(3, 10);
  emp_action.nm_raise(3, 10);
  ax_lower(4, 20);
  raise notice '%', emp_action.atf_raise(4, 10);
  raise notice '%', emp_action.nmf_raise(4, 10);
  rollback;
end;
/

-- error: define autonomous twice
create or replace procedure ax_err_object as
  pragma autonomous_transaction;
  pragma autonomous_transaction;
begin
end;
/
drop procedure ax_err_object;

create or replace procedure ax_err_object as
  procedure subproc as
    pragma autonomous_transaction;
    i int;
    pragma autonomous_transaction;
  begin
  end;
  pragma autonomous_transaction;
begin
end;
/
drop procedure ax_err_object;

create or replace function ax_err_object() return void as
  pragma autonomous_transaction;
  pragma autonomous_transaction;
begin
end;
/
drop function ax_err_object;

create or replace function ax_err_object() return void as
  pragma autonomous_transaction;
  function subfunc() return void as
    pragma autonomous_transaction;
    i int;
    pragma autonomous_transaction;
  begin
  end;
begin
end;
/
drop function ax_err_object;

create or replace package ax_err_object as
  procedure proc;
  function func() return void;
end;
/
create or replace package body ax_err_object as
  procedure proc as
    pragma autonomous_transaction;
    i int;
    pragma autonomous_transaction;
  begin
  end;
  function func() return void as
    pragma autonomous_transaction;
    i int;
    pragma autonomous_transaction;
  begin
  end;
end;
/
drop package ax_err_object;

-- error: define autonomous in execution segment
create or replace procedure ax_err_object as
begin
  pragma autonomous_transaction;
  insert into ax_employees values(100, 100);
end;
/
drop procedure ax_err_object;

create or replace function ax_err_object() return void as
begin
  pragma autonomous_transaction;
  insert into ax_employees values(100, 100);
end;
/
drop function ax_err_object;

create or replace package ax_err_object as
  procedure proc;
  function func() return void;
end;
/
create or replace package body ax_err_object as
  procedure proc as
    pragma autonomous_transaction;
  begin
    pragma autonomous_transaction;
  end;
  function func() return void as
    pragma autonomous_transaction;
  begin
    pragma autonomous_transaction;
  end;
end;
/
drop package ax_err_object;

declare
  pragma autonomous_transaction;
begin
  pragma autonomous_transaction;
end;
/

-- error: define autonomous in non-top anonymous block
begin
  declare
    pragma autonomous_transaction;
  begin
    insert into ax_employees values(100, 100);
  end;
end;
/
declare
  pragma autonomous_transaction;
begin
  declare
    pragma autonomous_transaction;
  begin
    insert into ax_employees values(100, 100);
  end;
end;
/

-- error: define autonomous transaction in package
create or replace package ax_err_object as
  pragma autonomous_transaction;
  procedure proc;
  function func() return void;
end;
/
create or replace package ax_err_object as
  procedure proc;
end;
/
create or replace package body ax_err_object as
  pragma autonomous_transaction;
  procedure proc as
    pragma autonomous_transaction;
  begin
  end;
end;
/
create or replace package body ax_err_object as
  procedure proc as
    pragma autonomous_transaction;
  begin
  end;
begin
  pragma autonomous_transaction;
end ax_err_object;
/
drop package ax_err_object;

-- test procedure
truncate ax_employees;
insert into ax_employees values(1, 100);
select * from ax_employees;
call ax_lower(1,10);
select * from ax_employees;

-- test function
truncate ax_employees;
insert into ax_employees values(1, 100);
select * from ax_employees;
select atf_lower(1,10);
select * from ax_employees;

-- test package
truncate ax_employees;
insert into ax_employees values(1, 100);
call emp_action.ax_raise(1, 10);
select * from ax_employees;
select emp_action.atf_raise(1, 10) from dual;
select * from ax_employees;

-- test anonymous block
truncate ax_employees;
insert into ax_employees values(1, 100);

declare
  pragma autonomous_transaction;
  empid number(6) := 1;
  amount number(6,2) := 10;
begin
  update ax_employees set salary = salary - amount where id = empid;
  commit;
end;
/
select * from ax_employees;

-- test trigger
truncate ax_employees;
insert into ax_employees values(115, 100);
insert into ax_employees values(116, 100);
update ax_employees set salary = salary * 1.05 where id = 115;
update ax_employees set salary = salary * 1.05 where id = 116;
select * from ax_log;

-- test nested subproc
truncate ax_employees;
insert into ax_employees values(1, 100);
insert into ax_employees values(2, 100);
call ax_lower_subproc(1, 10);
select * from ax_employees;

begin;
declare
  procedure subproc as
  begin
    commit;
  end;
  pragma autonomous_transaction;
begin
  subproc;
end;
/
end;

declare
  xid1 bigint;
  xid2 bigint;
  procedure subproc as
    pragma autonomous_transaction;
  begin
    xid2 := txid_current();
    commit;
  end;
begin
  xid1 := txid_current();
  for i in 1..100 loop
    subproc;
  end loop;

  if xid2 - xid1 < 100 then
    raise notice 'number of xacts error';
  end if;
end;
/

-- test cross-calling between in normal and autonomous procs
truncate ax_employees;
insert into ax_employees values(1,100);
insert into ax_employees values(2,100);
insert into ax_employees values(3,100);
insert into ax_employees values(4,100);
call ax_cross();
select * from ax_employees;
call nm_cross();
select * from ax_employees;

-- test unexpected error
truncate ax_employees;
declare
  pragma autonomous_transaction;
begin
  insert into ax_employees values(1,100);
  insert into ax_employees values(1,100);
end;
/

-- cleanup
drop procedure nm_cross;
drop procedure ax_cross;
drop procedure nm_lower;
drop procedure ax_lower;
drop procedure ax_lower_subproc;
drop function nmf_lower;
drop function atf_lower;
drop package emp_action;
drop trigger ax_log_sal_trig on ax_employees;
drop function ax_log_sal_proc();
drop table ax_employees;
drop table ax_log;


--
-- nested and recursive calling
--
create table ax_log(message varchar2(200));

-- recursive calling
create or replace procedure ax_recursive(start_level number, end_level number) is
  pragma autonomous_transaction;
begin
  insert into ax_log values('Level: ' || start_level);
  commit;

  if (start_level < end_level) then
    ax_recursive(start_level + 1, end_level);
  end if;

  raise notice 'Level: %', start_level;
end;
/

create or replace function ax_recursive_func(start_level number, end_level number) return number as
  pragma autonomous_transaction;
  l number;
begin
  insert into ax_log values('Level: ' || start_level);
  commit;

  if (start_level < end_level) then
    l := ax_recursive_func(start_level + 1, end_level);
    raise notice 'Level: %', l;
  end if;

  return start_level;
end;
/

create or replace package ax_recursive_pack as
  procedure subproc(start_level number, end_level number);
  function subfunc(start_level number, end_level number) return number;
end;
/
create or replace package body ax_recursive_pack as
  procedure subproc(start_level number, end_level number) as
    pragma autonomous_transaction;
  begin
    insert into ax_log values('Level: ' || start_level);
    commit;

    if (start_level < end_level) then
      ax_recursive(start_level + 1, end_level);
    end if;

    raise notice 'Level: %', start_level;
  end;

  function subfunc(start_level number, end_level number) return number as
    pragma autonomous_transaction;
    l number;
  begin
    insert into ax_log values('Level: ' || start_level);
    commit;

    if (start_level < end_level) then
      l := ax_recursive_func(start_level + 1, end_level);
      raise notice 'Level: %', l;
    end if;

    return start_level;
  end;
end;
/

show ivorysql.autontrans_nesting_level;
show ivorysql.max_autonomous_transactions;

truncate ax_log;
call ax_recursive(1, 16);
select count(*) from ax_log;

truncate ax_log;
call ax_recursive(1, 17);
select count(*) from ax_log;

truncate ax_log;
select ax_recursive_func(1, 16);
select count(*) from ax_log;

truncate ax_log;
select ax_recursive_func(1, 17);
select count(*) from ax_log;

truncate ax_log;
call ax_recursive_pack.subproc(1, 16);
select count(*) from ax_log;

truncate ax_log;
call ax_recursive_pack.subproc(1, 17);
select count(*) from ax_log;

truncate ax_log;
select ax_recursive_pack.subfunc(1, 16);
select count(*) from ax_log;

truncate ax_log;
select ax_recursive_pack.subfunc(1, 17);
select count(*) from ax_log;

set ivorysql.autontrans_nesting_level = 10;
truncate ax_log;
call ax_recursive(1, 11);
select count(*) from ax_log;
truncate ax_log;
select ax_recursive_func(1, 11);
select count(*) from ax_log;
truncate ax_log;
call ax_recursive_pack.subproc(1, 11);
select count(*) from ax_log;
truncate ax_log;
select ax_recursive_pack.subfunc(1, 11);
select count(*) from ax_log;
reset ivorysql.autontrans_nesting_level;

set ivorysql.max_autonomous_transactions = 6;
set ivorysql.autontrans_nesting_level = 33;
set ivorysql.autontrans_nesting_level = 32;
truncate ax_log;
call ax_recursive(1, 32);
select count(*) from ax_log;
truncate ax_log;
select ax_recursive_func(1, 32);
select count(*) from ax_log;
truncate ax_log;
call ax_recursive_pack.subproc(1, 32);
select count(*) from ax_log;
truncate ax_log;
select ax_recursive_pack.subfunc(1, 32);
select count(*) from ax_log;
reset ivorysql.autontrans_nesting_level;

-- nested calling
create or replace procedure ax_nest as
  pragma autonomous_transaction;
  clevel integer := 1;
  procedure subproc as
    pragma autonomous_transaction;
    procedure subproc as
      pragma autonomous_transaction;
      procedure subproc as
        pragma autonomous_transaction;
        procedure subproc as
          pragma autonomous_transaction;
          procedure subproc as
            --pragma autonomous_transaction;
            procedure subproc as
            pragma autonomous_transaction;
            begin
              insert into ax_log values('Level: ' || clevel);
              commit;
              clevel := clevel + 1;
            end;
          begin
            insert into ax_log values('NLevel: ' || clevel);
            commit;
            clevel := clevel + 1;
            subproc;
          end;
        begin
          insert into ax_log values('Level: ' || clevel);
          commit;
          clevel := clevel + 1;
          subproc;
        end;
      begin
        insert into ax_log values('Level: ' || clevel);
        commit;
        clevel := clevel + 1;
        subproc;
      end;
    begin
      insert into ax_log values('Level: ' || clevel);
      commit;
      clevel := clevel + 1;
      subproc;
    end;
  begin
    insert into ax_log values('Level: ' || clevel);
    commit;
    clevel := clevel + 1;
    subproc;
  end;
begin
  insert into ax_log values('Level: ' || clevel);
  clevel := clevel + 1;
  commit;
  subproc;
end;
/

truncate ax_log;
set ivorysql.autontrans_nesting_level = 6;
call ax_nest();
set ivorysql.autontrans_nesting_level = 5;
call ax_nest();
select count(*) from ax_log;
reset ivorysql.autontrans_nesting_level;

-- cleanup
drop procedure ax_recursive;
drop function ax_recursive_func;
drop package ax_recursive_pack;
drop procedure ax_nest;
drop table ax_log;


--
-- parallel
--
create table ax_employees(id number, salary number);
insert into ax_employees select generate_series(1,10000);

create or replace function nm_query_plan(query text) return setof text as
begin
  return query execute 'explain ' || query;
end;
/

create or replace function ax_query_plan(query text) return setof text as
  pragma autonomous_transaction;
begin
  return query execute 'explain ' || query;
end;
/

create or replace function ax_query_plan_setparallel(query text) return setof text as
  pragma autonomous_transaction;
begin
  set force_parallel_mode to on;
  set max_parallel_workers_per_gather to 4;
  return query execute 'explain ' || query;
end;
/

set force_parallel_mode=on;
set max_parallel_workers_per_gather=4;
select nm_query_plan('select * from ax_employees');
select ax_query_plan('select * from ax_employees');
select ax_query_plan_setparallel('select * from ax_employees');
reset force_parallel_mode;
reset max_parallel_workers_per_gather;

-- cleanup
drop function ax_query_plan_setparallel;
drop function ax_query_plan;
drop function nm_query_plan;


--
-- GUC options
--
create or replace procedure ax_guc_set(guc text, var name) as
declare
  pragma autonomous_transaction;
  query text;
begin
  raise notice 'ax_guc_set: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  raise notice 'ax_guc_set: %', current_setting(guc);
end;
/

create or replace procedure ax_guc_set_commit(guc text, var name) as
declare
  pragma autonomous_transaction;
  query text;
begin
  raise notice 'ax_guc_set_commit: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  commit;
  raise notice 'ax_guc_set_commit: %', current_setting(guc);
end;
/

create or replace procedure ax_guc_set_rollback(guc text, var name) as
declare
  pragma autonomous_transaction;
  query text;
begin
  raise notice 'ax_guc_set_rollback: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  rollback;
  raise notice 'ax_guc_set_rollback: %', current_setting(guc);
end;
/

create or replace procedure nm_guc_set(guc text, var name) as
declare
  query text;
begin
  raise notice 'nm_guc_set: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  raise notice 'nm_guc_set: %', current_setting(guc);
end;
/

create or replace procedure nm_guc_set_commit(guc text, var name) authid current_user as
declare
  query text;
begin
  raise notice 'nm_guc_set_commit: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  commit;
  raise notice 'nm_guc_set_commit: %', current_setting(guc);
end;
/

create or replace procedure nm_guc_set_rollback(guc text, var name) authid current_user as
declare
  query text;
begin
  raise notice 'nm_guc_set_rollback: %', current_setting(guc);
  query := 'set ' || guc || ' to ' || var;
  execute query;
  rollback;
  raise notice 'nm_guc_set_rollback: %', current_setting(guc);
end;
/

declare
  pragma autonomous_transaction;
  outvar text;
begin
  nm_guc_set('log_min_messages', 'debug1');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  nm_guc_set_rollback('log_min_messages', 'debug2');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  nm_guc_set_commit('log_min_messages', 'debug1');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  ax_guc_set('log_min_messages', 'debug3');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  ax_guc_set_rollback('log_min_messages', 'debug4');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  ax_guc_set_commit('log_min_messages', 'debug5');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  begin
    rollback;
  end;

  rollback;
end;
/
show log_min_messages;
reset log_min_messages;

do $$
declare
  outvar text;
begin
  call nm_guc_set('log_min_messages', 'debug1');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  call nm_guc_set_rollback('log_min_messages', 'debug2');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  call nm_guc_set_commit('log_min_messages', 'debug1');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  call ax_guc_set('log_min_messages', 'debug3');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  call ax_guc_set_rollback('log_min_messages', 'debug4');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  call ax_guc_set_commit('log_min_messages', 'debug5');
  show log_min_messages into outvar;
  raise notice '%', outvar;

  begin
    rollback;
  end;

  rollback;
end;
$$ language plpgsql;
show log_min_messages;
reset log_min_messages;

-- cleanup
drop procedure ax_guc_set;
drop procedure ax_guc_set_commit;
drop procedure ax_guc_set_rollback;
drop procedure nm_guc_set;
drop procedure nm_guc_set_commit;
drop procedure nm_guc_set_rollback;


--
-- exception
--
create table ax_test(id int);

create or replace procedure nm_except as
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    raise notice 'exception and do nothing';
end;
/

create or replace procedure nm_except_commit authid current_user as
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    commit;
    raise notice 'exception and commit';
end;
/

create or replace procedure nm_except_rollback authid current_user as
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    rollback;
    raise notice 'exception and rollback';
end;
/

create or replace procedure nm_commit_except_rollback authid current_user as
begin
  insert into ax_test values(1);
  commit;
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    rollback;
    raise notice 'exception and rollback';
end;
/

create or replace procedure ax_except as
  pragma autonomous_transaction;
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    raise notice 'exception and do nothing';
end;
/

create or replace procedure ax_except_commit as
  pragma autonomous_transaction;
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    commit;
    raise notice 'exception and commit';
end;
/

create or replace procedure ax_except_rollback as
  pragma autonomous_transaction;
begin
  insert into ax_test values(1);
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    rollback;
    raise notice 'exception and rollback';
end;
/

create or replace procedure ax_commit_except_rollback authid current_user as
  pragma autonomous_transaction;
begin
  insert into ax_test values(1);
  commit;
  insert into ax_test values('a');
  insert into ax_test values(2);
exception
  when others then
    rollback;
    raise notice 'exception and rollback';
end;
/

truncate ax_test;
call nm_except();
select * from ax_test;
call nm_except_commit();
select * from ax_test;
call nm_except_rollback();
select * from ax_test;

truncate ax_test;
call ax_except();
select * from ax_test;
call ax_except_commit();
select * from ax_test;
call ax_except_rollback();
select * from ax_test;

truncate ax_test;
begin;
call nm_except();
call nm_except_commit();
call nm_except_rollback();
rollback;
select * from ax_test;

truncate ax_test;
begin;
call ax_except();
call ax_except_commit();
call ax_except_rollback();
rollback;
select * from ax_test;

-- cleanup
drop procedure nm_except;
drop procedure nm_except_commit;
drop procedure nm_except_rollback;
drop procedure ax_except;
drop procedure ax_except_commit;
drop procedure ax_except_rollback;


--
-- pass by value
--
create or replace procedure nm_proc(in a int, inout b int, out c int) as
begin
  c := a + 1;
  b := b + 1;
end;
/

create or replace procedure nm_proc_commit(in a int, inout b int, out c int) authid current_user as
begin
  c := a + 1;
  b := b + 1;
  commit;
end;
/

create or replace procedure nm_proc_rollback(in a int, inout b int, out c int) authid current_user as
begin
  c := a + 1;
  b := b + 1;
  rollback;
end;
/

create or replace procedure ax_proc(in a int, inout b int, out c int) as
  pragma autonomous_transaction;
begin
  c := a + 1;
  b := b + 1;
end;
/

create or replace procedure ax_proc_commit(in a int, inout b int, out c int) as
  pragma autonomous_transaction;
begin
  c := a + 1;
  b := b + 1;
  commit;
end;
/

create or replace procedure ax_proc_rollback(in a int, inout b int, out c int) as
  pragma autonomous_transaction;
begin
  c := a + 1;
  b := b + 1;
  rollback;
end;
/

call nm_proc(1,2,3);
call nm_proc_commit(1,2,3);
call nm_proc_rollback(1,2,3);
call ax_proc(1,2,3);
call ax_proc_commit(1,2,3);
call ax_proc_rollback(1,2,3);

declare
  pragma autonomous_transaction;
  a int;
  b int;
  c int;
begin
  a := 1;
  b := 2;
  c := 3;
  nm_proc(a, b, c);
  nm_proc_commit(a, b, c);
  nm_proc_commit(a, b, c);
  raise notice '(%,%,%)', a, b, c;
  ax_proc(a, b, c);
  ax_proc_commit(a, b, c);
  ax_proc_commit(a, b, c);
  raise notice '(%,%,%)', a, b, c;
end;
/

-- cleanup
drop procedure nm_proc;
drop procedure nm_proc_commit;
drop procedure nm_proc_rollback;
drop procedure ax_proc;
drop procedure ax_proc_commit;
drop procedure ax_proc_rollback;


--
-- portal
--
create table ax_test(id int);

-- fors
truncate ax_test;
insert into ax_test select generate_series(1,10);
declare
  pragma autonomous_transaction;
  r record;
begin
  for r in select * from ax_test loop
    raise notice '%', r;
    commit;
  end loop;
end;
/

-- forc
truncate ax_test;
insert into ax_test select generate_series(1,10);
declare
  pragma autonomous_transaction;
  cv cursor is select * from ax_test;
  r int;
begin
  for r in cv loop
    raise notice '%', r;
    rollback;
  end loop;
end;
/

-- fori
truncate ax_test;
declare
  pragma autonomous_transaction;
  r int;
begin
  for r in 1..10 loop
    insert into ax_test values(r);
    if r < 5 then
      raise notice 'commit: %', r;
      commit;
    else
      raise notice 'rollback: %', r;
      rollback;
    end if;
  end loop;
end;
/

-- sys_refcursor
truncate ax_test;
insert into ax_test select generate_series(1,10);

create or replace procedure ax_cproc(out cv sys_refcursor) as
  pragma autonomous_transaction;
begin
  open $1 for select * from ax_test;
end;
/

create or replace function ax_cfunc() return sys_refcursor as
declare
  pragma autonomous_transaction;
  rc sys_refcursor;
begin
  open rc for select * from ax_test;
  return rc;
end;
/

declare
  rc sys_refcursor;
  r record;
begin
  ax_cproc(rc);
  loop
    fetch rc into r;
    EXIT when rc%notfound;
    raise notice '%', r;
  end loop;
  close rc;
end;
/

begin;
declare
  pragma autonomous_transaction;
  sc sys_refcursor;
  r record;
begin
  sc := ax_cfunc();
  loop
    fetch sc into r;
    exit when sc%notfound;
    raise notice '%', r;
  end loop;
end;
/
end;

-- read/close ouuter cursor
truncate ax_test;
insert into ax_test values(100),(200);
declare
  cursor c_test is select id from ax_test;
  v_id ax_test.id%type;
  procedure subproc as
  begin
    loop
      fetch c_test into v_id;
      exit when c_test%notfound;
      raise notice 'subproc: %', v_id;
    end loop;
    close c_test;
  end;
begin
  insert into ax_test values(300);
  open c_test;
  fetch c_test into v_id;
  raise notice 'main: %', v_id;
  subproc;
end;
/

truncate ax_test;
insert into ax_test values(100),(200);
declare
  pragma autonomous_transaction;
  cursor c_test is select id from ax_test;
  v_id ax_test.id%type;
  procedure subproc as
  begin
    loop
      fetch c_test into v_id;
      exit when c_test%notfound;
      raise notice 'subproc: %', v_id;
    end loop;
    close c_test;
  end;
begin
  insert into ax_test values(300);
  open c_test;
  fetch c_test into v_id;
  raise notice 'main: %', v_id;
  subproc;
end;
/

truncate ax_test;
insert into ax_test values(100),(200);
declare
  pragma autonomous_transaction;
  cursor c_test is select id from ax_test;
  v_id ax_test.id%type;
  procedure subproc as
  begin
  open c_test;
  fetch c_test into v_id;
  raise notice 'subproc: %', v_id;
  end;
begin
  insert into ax_test values(300);
  subproc;
  loop
    fetch c_test into v_id;
    exit when c_test%notfound;
    raise notice 'main: %', v_id;
  end loop;
  close c_test;
end;
/

-- cleanup
drop procedure ax_cproc;
drop function ax_cfunc;
drop table ax_test;


--
-- temprory table
--
create temp table ax_tmp_preserve(id int, name varchar2(20)) on commit preserve rows;
create temp table ax_tmp_delete(id int, name varchar2(20)) on commit delete rows;
create global temp table ax_gtmp_preserve(id int, name varchar2(20)) on commit preserve rows;
create global temp table ax_gtmp_delete(id int, name varchar2(20)) on commit delete rows;

-- local temp, on commit preserve
truncate ax_tmp_preserve;
insert into ax_tmp_preserve values(1, '1'),(2, '2');
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_tmp_preserve values(100, '100');
    raise notice 'subproc';
    for r in select * from ax_tmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_tmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_tmp_preserve values(10, '10');
  subproc;
  insert into ax_tmp_preserve values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_preserve loop
    raise notice '%', r;
  end loop;
  commit;
   raise notice 'main1';
  for r in select * from ax_tmp_preserve loop
    raise notice '%', r;
  end loop;
end;
/

truncate ax_tmp_preserve;
insert into ax_tmp_preserve values(1, '1'),(2, '2');
declare
  procedure subproc as
    pragma autonomous_transaction;
    procedure ssubproc as
      pragma autonomous_transaction;
    begin
      insert into ax_tmp_preserve values(1000, '1000');
      raise notice 'ssubproc';
      for r in select * from ax_tmp_preserve loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'ssubproc1';
      for r in select * from ax_tmp_preserve loop
        raise notice '%', r;
      end loop;
    end;
  begin
    insert into ax_tmp_preserve values(100, '100');
    ssubproc;
    insert into ax_tmp_preserve values(200, '200');
    raise notice 'subproc';
    for r in select * from ax_tmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_tmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_tmp_preserve values(10, '10');
  subproc;
  insert into ax_tmp_preserve values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_preserve loop
    raise notice '%', r;
  end loop;
  commit;
   raise notice 'main1';
  for r in select * from ax_tmp_preserve loop
    raise notice '%', r;
  end loop;
end;
/

-- local temp, on commit delete
insert into ax_tmp_delete values(1, '1'),(2, '2');

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_tmp_delete values(100, '100');
    raise notice 'subproc';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_tmp_delete values(10, '10');
  subproc;
  insert into ax_tmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
    procedure ssubproc as
      pragma autonomous_transaction;
    begin
      insert into ax_tmp_delete values(1000, '1000');
      raise notice 'ssubproc';
      for r in select * from ax_tmp_delete loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'ssubproc1';
      for r in select * from ax_tmp_delete loop
        raise notice '%', r;
      end loop;
    end;
  begin
    insert into ax_tmp_delete values(100, '100');
    ssubproc;
    insert into ax_tmp_delete values(200, '200');
    raise notice 'subproc';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_tmp_delete values(10, '10');
  subproc;
  insert into ax_tmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    raise notice 'subproc';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
     raise notice 'subproc1';
    for r in select * from ax_tmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
  end;
begin
  insert into ax_tmp_delete values(10, '10');
  subproc;
  insert into ax_tmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    raise notice 'subproc';
    commit;
  end;
begin
  insert into ax_tmp_delete values(10, '10');
  subproc;
  insert into ax_tmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_tmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

-- local temp, on commit drop
create temp table ax_tmp_drop(id int, name varchar2(20)) on commit drop;
insert into ax_tmp_drop values(1, '1'),(2, '2');
-- deadlock in plisql-autonx-deadlock.spec

-- global temp, on commit preserve
truncate ax_gtmp_preserve;
insert into ax_gtmp_preserve values(1, '1'),(2, '2');
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_gtmp_preserve values(100, '100');
    raise notice 'subproc';
    for r in select * from ax_gtmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_gtmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
  end;
begin
  insert into ax_gtmp_preserve values(10, '10');
  subproc;
  insert into ax_gtmp_preserve values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_preserve loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_gtmp_preserve loop
    raise notice '%', r;
  end loop;
end;
/

truncate ax_gtmp_preserve;
insert into ax_gtmp_preserve values(1, '1'),(2, '2');
declare
  procedure subproc as
    pragma autonomous_transaction;
    procedure ssubproc as
      pragma autonomous_transaction;
    begin
      insert into ax_gtmp_preserve values(1000, '1000');
      raise notice 'ssubproc';
      for r in select * from ax_gtmp_preserve loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'ssubproc1';
      for r in select * from ax_gtmp_preserve loop
        raise notice '%', r;
      end loop;
    end;
  begin
    insert into ax_gtmp_preserve values(100, '100');
    ssubproc;
    insert into ax_gtmp_preserve values(200, '200');
    raise notice 'subproc';
    for r in select * from ax_gtmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_gtmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_gtmp_preserve values(10, '10');
  subproc;
  insert into ax_gtmp_preserve values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_preserve loop
    raise notice '%', r;
  end loop;
  commit;
   raise notice 'main1';
  for r in select * from ax_gtmp_preserve loop
    raise notice '%', r;
  end loop;
end;
/

-- global temp, on commit delete
insert into ax_gtmp_delete values(1, '1'),(2, '2');

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_gtmp_delete values(100, '100');
    raise notice 'subproc';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
  end;
begin
  insert into ax_gtmp_delete values(10, '10');
  subproc;
  insert into ax_gtmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
    procedure ssubproc as
      pragma autonomous_transaction;
    begin
      insert into ax_gtmp_delete values(1000, '1000');
      raise notice 'ssubproc';
      for r in select * from ax_gtmp_delete loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'ssubproc1';
      for r in select * from ax_gtmp_delete loop
        raise notice '%', r;
      end loop;
    end;
  begin
    insert into ax_gtmp_delete values(100, '100');
    ssubproc;
    insert into ax_gtmp_delete values(200, '200');
    raise notice 'subproc';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
  end;
begin
  insert into ax_gtmp_delete values(10, '10');
  subproc;
  insert into ax_gtmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    raise notice 'subproc';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'subproc1';
    for r in select * from ax_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
  end;
begin
  insert into ax_gtmp_delete values(10, '10');
  subproc;
  insert into ax_gtmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    raise notice 'subproc';
    commit;
  end;
begin
  insert into ax_gtmp_delete values(10, '10');
  subproc;
  insert into ax_gtmp_delete values(20, '20');
  raise notice 'main';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
  commit;
  raise notice 'main1';
  for r in select * from ax_gtmp_delete loop
    raise notice '%', r;
  end loop;
end;
/

-- cleanup
truncate ax_gtmp_preserve;
drop table ax_tmp_preserve;
drop table ax_tmp_delete;
drop table ax_gtmp_preserve;
drop table ax_gtmp_delete;


--
-- commit in subtransaction
--
declare
  errmsg text;
begin
  commit;
exception
  when others then
    get stacked diagnostics errmsg = message_text;
    raise notice '%', errmsg;
    commit;
end;
/

declare
  pragma autonomous_transaction;
  errmsg text;
begin
  commit;
exception
  when others then
    get stacked diagnostics errmsg = message_text;
    raise notice '%', errmsg;
    commit;
end;
/


--
-- resource isolation
--

-- [skip] Xact: TopTransactionContext, TopTransactionResourceOwner

-- Xact: transaction_isolation
set default_transaction_isolation to 'repeatable read';
show transaction_isolation;
begin;
set transaction_isolation to 'repeatable read';
show transaction_isolation;
declare
  pragma autonomous_transaction;
begin
  raise notice 'AT:  %', current_setting('transaction_isolation');
end;
/
show transaction_isolation;
end;
show transaction_isolation;
reset default_transaction_isolation;

-- Xact: transaction_read_only
set default_transaction_read_only to off;
show transaction_read_only;
begin;
set transaction_read_only to on;
show transaction_read_only;
declare
  pragma autonomous_transaction;
begin
  raise notice 'AT:  %', current_setting('transaction_read_only');
end;
/
show transaction_read_only;
end;
show transaction_read_only;
reset default_transaction_read_only;

-- Xact: transaction_deferrable
set default_transaction_deferrable to off;
show transaction_deferrable;
begin;
set transaction_deferrable to on;
show transaction_deferrable;
declare
  pragma autonomous_transaction;
begin
  raise notice 'AT:  %', current_setting('transaction_deferrable');
end;
/
show transaction_deferrable;
end;
show transaction_deferrable;
reset default_transaction_deferrable;

-- [skip] Xact: CheckXidAlive, bsysscan

-- Xact: XactTopFullTransactionId
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
  x1 xid8;
  x2 xid8;
begin
  x1 := pg_current_xact_id();
  subproc;
  x2 := pg_current_xact_id();
  if x1 != x2 then
    raise notice 'XactTopFullTransactionId mismatch';
  end if;
end;
/

-- [skip] Xact: nParallelCurrentXids, ParallelCurrentXids

-- Xact: MyXactFlags
create temp table ax_t(id int) on commit delete rows;
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
begin
  insert into ax_t values(10);
  subproc;
end;
/
select * from ax_t;
drop table ax_t;

-- [skip] Xact: nUnreportedXids, unreportedXids
-- [skip] Xact: currentSubTransactionId, currentCommandId

-- Xact: currentCommandIdUsed
create table ax_t(id int);
insert into ax_t values(1);

create or replace function ax_t_tfunc() return trigger as
  pragma autonomous_transaction;
begin
  return new;
end;
/

create or replace trigger ax_t_trig
  before update of id on ax_t for each row
  execute function ax_t_tfunc();

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
begin
  update ax_t set id=2 where id=1;
  subproc;
  for r in (select * from ax_t) loop
    raise notice '%', r;
  end loop;
end;
/

drop table ax_t;
drop function ax_t_tfunc;

-- Xact: xactStartTimestamp
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
  end;
  t1 text;
  t2 text;
begin
  t1 := systimestamp();
  subproc;
  t2 := systimestamp();
  if t1 != t2 then
    raise notice 'xactStartTimestamp mismatch';
  end if;
end;
/

-- [skip] Xact: xactStopTimestamp
-- [skip] Xact: forceSyncCommit
-- [skip] Xact: xact_is_sampled
-- [skip] Xact: Xact_callbacks, SubXact_callbacks, Xact_callbacks_once

-- [skip] Process: MyProc

-- LWLock: held_lwlocks, num_held_lwlocks
create table ax_t(id int);
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
  end;
begin
  insert into ax_t values(1);
  subproc;
end;
/
drop table ax_t;

-- [skip] CV: cv_sleep_target

-- Lock: FastPathLocalUseCount, LockMethodLocalHash
create table ax_t();
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
begin
  lock table ax_t in access share mode;
  subproc;
end;
/
drop table ax_t;

-- [skip] Lmgr: speculativeInsertionToken

-- PredLock: LocalPredicateLockHash, MySerializableXact, MyXactDidWrite, SavedSerializableXact
create table ax_t();
declare
  pragma autonomous_transaction;
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
begin
  set transaction_isolation to 'serializable';
  subproc;
end;
/
drop table ax_t;

-- [skip] HtabSeqScan: seq_scan_tables, seq_scan_level, num_seq_scans

-- Trigger: afterTriggers
create table ax_t(id int);
create table ax_t_log(id int);

create or replace function ax_t_func() return trigger as
begin
  insert into ax_t_log values(new.id);
  return new;
end;
/

create or replace trigger ax_t_trig
  after insert on ax_t for each row
  execute function ax_t_func();
/

declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_t values(100);
  end;
begin
  insert into ax_t values(10);
  subproc;
end;
/

drop table ax_t;
drop table ax_t_log;
drop function ax_t_func;

-- [skip] Snapmgr: CurrentSnapshotData, SecondarySnapshotData, CatalogSnapshotData, CurrentSnapshot, SecondarySnapshot, CatalogSnapshot, HistoricSnapshot
-- [skip] Snapmgr: TransactionXmin, RecentXmin
-- [skip] Snapmgr: tuplecid_data
-- [skip] Snapmgr: ActiveSnapshot, OldestActiveSnapshot
-- [skip] Snapmgr: RegisteredSnapshots, FirstSnapshotSet, FirstXactSnapshot
-- [skip] Snapmgr: exportedSnapshots

-- LargeObject: lo_heap_r, lo_index_r, cookies, cookies_size, lo_cleanup_needed, fscxt
create table ax_lo_stash(fd int, loid oid);
insert into ax_lo_stash(loid) select lo_creat(100);
begin;
update ax_lo_stash set fd = lo_open(loid, CAST(x'20000' | x'40000' AS integer));
select lowrite(fd, '
I wandered lonely as a cloud
That floats on high o''er vales and hills,
When all at once I saw a crowd,
A host, of golden daffodils;
Beside the lake, beneath the trees,
Fluttering and dancing in the breeze.

Continuous as the stars that shine
And twinkle on the milky way,
They stretched in never-ending line
Along the margin of a bay:
Ten thousand saw I at a glance,
Tossing their heads in sprightly dance.

The waves beside them danced; but they
Out-did the sparkling waves in glee:
A poet could not but be gay,
In such a jocund company:
I gazed--and gazed--but little thought
What wealth the show to me had brought:

For oft, when on my couch I lie
In vacant or in pensive mood,
They flash upon that inward eye
Which is the bliss of solitude;
And then my heart with pleasure fills,
And dances with the daffodils.

         -- William Wordsworth
') from ax_lo_stash;
select lo_close(fd) from ax_lo_stash;
end;
select md5(lo_get(loid)) from ax_lo_stash;

declare
  r1 text;
  r2 text;
  procedure subproc as
    pragma autonomous_transaction;
  begin
    select md5(lo_get(loid)) into r2 from ax_lo_stash;
  end;
begin
  select md5(lo_get(loid)) into r1 from ax_lo_stash;
  subproc;
  if r1 != r2 then
    raise notice 'large object mismatch';
  end if;
end;
/

select lo_unlink(loid) from ax_lo_stash;
drop table ax_lo_stash;

-- [skip] Portal: ActivePortal->portalSnapshot

-- Notify: pendingActions, pendingNotifies
unlisten *;
listen xout;
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    listen at;
    for r in (select * from pg_listening_channels()) loop
      raise notice 'at: %', r;
    end loop;
    commit;
  end;
begin
  listen mt;
  subproc;
  for r in (select * from pg_listening_channels()) loop
    raise notice 'mt: %', r;
  end loop;
end;
/
select * from pg_listening_channels();
unlisten *;

-- [skip] Smgr: unowned_relns
-- [skip] Files: have_xact_temporary_files, numAllocatedDescs, allocatedDescs, tempTableSpaces, numTempTableSpaces

-- Combocid: comboHash, comboCids, usedComboCids, sizeComboCids
create table ax_combo(id int);
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    insert into ax_combo values(100);
    delete from ax_combo;
    commit;
    for r in (select cmin,cmax,* from ax_combo) loop
      raise notice 'MT: %', r;
    end loop;
  end;
begin
  insert into ax_combo values(1);
  delete from ax_combo;
  subproc;
  for r in (select cmin,cmax,* from ax_combo) loop
    raise notice 'MT: %', r;
  end loop;
end;
/
select cmin,cmax,* from ax_combo;
drop table ax_combo;

-- PgStat: pgStatXactStack
create table ax_pgstat(id int);
begin;
insert into ax_pgstat values(1);
savepoint s1;
insert into ax_pgstat values(2);
savepoint s2;
declare
  pragma autonomous_transaction;
begin
  insert into ax_pgstat values(3);
  commit;
end;
/
select * from ax_pgstat;
end;
select * from ax_pgstat;
drop table ax_pgstat;

-- Enum: uncommitted_enums
create type ax_enum as enum('a');
create table ax_enum_t(id int, b ax_enum);
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    commit;
  end;
begin
  alter type ax_enum add value 'b';
  subproc;
  insert into ax_enum_t values(1, 'b');
end;
/
drop table ax_enum_t;
drop type ax_enum;

select enumsortorder as num,enumlabel as label from pg_enum where enumtypid='ax_enum'::regtype;

-- PendingStorage: pendingDeletes, pendingSyncHash
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    rollback;
  end;
begin
  create table ax_t(id int);
  insert into ax_t values(1);
  subproc;
  insert into ax_t values(2);
end;
/
select count(*) from ax_t;
drop table ax_t;

-- Relmapper: active_shared_updates, active_local_updates, pending_shared_updates, pending_local_updates
select rolname from pg_authid where rolname='syssso';
declare
	procedure subproc as
		pragma autonomous_transaction;
	begin
		commit;
	end;
begin
	reindex table pg_authid;
	subproc;
	rollback;
end;
/
select rolname from pg_authid where rolname='syssso';

-- Relcache: eoxact_list_overflowed, eoxact_list, eoxact_list_len, EOXactTupleDescArray, NextEOXactTupleDescNum, EOXactTupleDescArrayLen
declare
  procedure subproc as
    pragma autonomous_transaction;
  begin
    rollback;
  end;
  rc refcursor;
begin
  create table ax_t(id int);
  insert into ax_t values(1);
  open rc for select * from ax_t;
  subproc;
  close rc;
  rollback;
end;
/

-- [skip] Inval: transInvalInfo

-- [skip] MultiXact: OldestMemberMXactId[MyBackendId], OldestVisibleMXactId[MyBackendId], MXactContext, MXactCache, MXactCacheMembers

-- Plisql
-- Plpgsql
create or replace procedure ax_pgproc as $$
begin
  raise notice 'ax_pgproc';
end;
$$ language plpgsql;

create or replace procedure ax_iproc as
  pragma autonomous_transaction;
begin
  raise notice 'ax_iproc';
end;
/

do $$
begin
  call ax_pgproc();
  call ax_iproc();
end;
$$;

declare
  pragma autonomous_transaction;
begin
  ax_pgproc;
  ax_iproc;
end;
/

drop procedure ax_pgproc;
drop procedure ax_iproc;


--
-- misc
--

-- fdw
/*
create database testdb;
\c testedb

create server ax_fserver foreign data wrapper postgres_fdw options(host '127.0.0.1', port '5432', dbname 'postgres');
create user mapping for pg server ax_fserver options (user 'pg', password '123');
create foreign table ax_ftable(id number, salary number) server ax_fserver options (schema_name 'public', table_name 'ax_employees');


begin;
insert into ax_ftable values(1,100);
declare
	pragma autonomous_transaction;
begin
	perform * from ax_ftable;
	commit;
end;
/
select * from ax_ftable;
rollback;
select * from ax_ftable;
*/

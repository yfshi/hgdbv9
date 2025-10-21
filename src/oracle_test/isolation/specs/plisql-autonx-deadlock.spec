# ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION
#
# The deadlock detector has a special case for autonomous transaction deadlocks.
# A session deadlock occurs when inner transaction attempt a lock upgrade while
# the outter transaction hold a lock upgrade on the same object;  and the locks
# conflict with those already held,   so the inner transaction can complete its
# upgrade until the outter releases locks.  We assume his scenario as deadlock.

setup
{
  create table a1 ();
  create table a2 ();
}

teardown
{
  drop table a1;
  drop table a2;
}

session s1
setup         { begin; }
step s1a1s    { lock table a1 in access share mode; }
step ax_s1a1e
{
  declare
    pragma autonomous_transaction;
  begin
    lock table a1 in access exclusive mode;
  END;
}
step s1lsto   { SET lock_timeout = '1s'; SET statement_timeout = '10s'; }
step ax_s1a2e
{
  declare
    pragma autonomous_transaction;
  begin
    lock table a2 in access exclusive mode;
  END;
}
teardown      { commit; }

session s2
setup         { begin; }
step s2a1e    { lock table a1 in access exclusive mode; }
step s2a2s    { lock table a2 in access share mode; }
teardown      { commit; }

session s3
step begin    { begin; }
step end      { end; }
step preserve
{
  create temp table ax_tmp_preserve(id int, name varchar2(20)) on commit preserve rows;
  insert into ax_tmp_preserve values(1, '1'),(2, '2');
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
        update ax_tmp_preserve set name='111' where id=1; -- deadlock
    end;
  begin
    update ax_tmp_preserve set name='111' where id=1;
    subproc;
    raise notice 'main';
    for r in select * from ax_tmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
  drop table ax_tmp_preserve;
}
step preserve1
{
  create temp table ax_tmp_preserve(id int, name varchar2(20)) on commit preserve rows;
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into x_tmp_preserve values(100, '100');  -- deadlock
      raise notice 'subproc';
      for r in select * from x_tmp_preserve loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from x_tmp_preserve loop
        raise notice '%', r;
      end loop;
    end;
  begin
    create temp table x_tmp_preserve(id int, name varchar2(20)) on commit preserve rows;
    insert into x_tmp_preserve values(10, '10');
    subproc;
    insert into x_tmp_preserve values(20, '20');
    raise notice 'main';
    for r in select * from x_tmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from x_tmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
  drop table ax_tmp_preserve;
}
step delete
{
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into x_tmp_delete values(100, '100');  -- deadlock
      raise notice 'subproc';
      for r in select * from x_tmp_delete loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from x_tmp_delete loop
        raise notice '%', r;
      end loop;
    end;
  begin
    create temp table x_tmp_delete(id int, name varchar2(20)) on commit delete rows;
    insert into x_tmp_delete values(10, '10');
    subproc;
    insert into x_tmp_delete values(20, '20');
    raise notice 'main';
    for r in select * from x_tmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from x_tmp_delete loop
      raise notice '%', r;
    end loop;
  end;
}
step drop
{
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into x_tmp_drop values(100, '100');  -- deadlock
      raise notice 'subproc';
      for r in select * from x_tmp_drop loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from x_tmp_drop loop
        raise notice '%', r;
      end loop;
    end;
  begin
    create temp table x_tmp_drop(id int, name varchar2(20)) on commit drop;
    insert into x_tmp_drop values(10, '10');
    subproc;
    insert into x_tmp_drop values(20, '20');
    raise notice 'main';
    for r in select * from x_tmp_drop loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from x_tmp_drop loop
      raise notice '%', r;
    end loop;
  end;
}
step drop1
{
  create temp table ax_tmp_drop(id int, name varchar2(20)) on commit drop;
  insert into ax_tmp_drop values(1, '1'),(2, '2');
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into ax_tmp_drop values(100, '100');
      raise notice 'subproc';
      for r in select * from ax_tmp_drop loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from ax_tmp_drop loop
        raise notice '%', r;
      end loop;
      commit;
    end;
  begin
    insert into ax_tmp_drop values(10, '10');
    subproc;
    insert into ax_tmp_drop values(20, '20');
    raise notice 'main';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
  end;
}
step drop2
{
  create temp table ax_tmp_drop(id int, name varchar2(20)) on commit drop;
  insert into ax_tmp_drop values(1, '1'),(2, '2');
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      raise notice 'subproc';
      for r in select * from ax_tmp_drop loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from ax_tmp_drop loop
        raise notice '%', r;
      end loop;
      commit;
    end;
  begin
    insert into ax_tmp_drop values(10, '10');
    subproc;
    insert into ax_tmp_drop values(20, '20');
    raise notice 'main';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
  end;
}
step drop3
{
  create temp table ax_tmp_drop(id int, name varchar2(20)) on commit drop;
  insert into ax_tmp_drop values(1, '1'),(2, '2');
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      raise notice 'subproc';
      commit;
    end;
  begin
    insert into ax_tmp_drop values(10, '10');
    subproc;
    insert into ax_tmp_drop values(20, '20');
    raise notice 'main';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from ax_tmp_drop loop
      raise notice '%', r;
    end loop;
  end;
}
step gpreserve
{
  create global temp table ax_gtmp_preserve(id int, name varchar2(20)) on commit preserve rows;
  insert into ax_gtmp_preserve values(1, '1'),(2, '2');
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
        update ax_gtmp_preserve set name='111' where id=1;  -- deadlock
    end;
  begin
    update ax_gtmp_preserve set name='111' where id=1;
    subproc;
    raise notice 'main';
    for r in select * from ax_gtmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
  drop table ax_gtmp_preserve;
}
step gpreserve1
{
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into x_gtmp_preserve values(100, '100');  -- deadlock
      raise notice 'subproc';
      for r in select * from x_gtmp_preserve loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from x_gtmp_preserve loop
        raise notice '%', r;
      end loop;
    end;
  begin
    create global temp table x_gtmp_preserve(id int, name varchar2(20)) on commit preserve rows;
    insert into x_gtmp_preserve values(10, '10');
    subproc;
    insert into x_gtmp_preserve values(20, '20');
    raise notice 'main';
    for r in select * from x_gtmp_preserve loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from x_gtmp_preserve loop
      raise notice '%', r;
    end loop;
  end;
}
step gdelete
{
  declare
    procedure subproc as
      pragma autonomous_transaction;
    begin
      insert into x_gtmp_delete values(100, '100');  -- deadlock
      raise notice 'subproc';
      for r in select * from x_gtmp_delete loop
        raise notice '%', r;
      end loop;
      commit;
      raise notice 'subproc1';
      for r in select * from x_gtmp_delete loop
        raise notice '%', r;
      end loop;
    end;
  begin
    create global temp table x_gtmp_delete(id int, name varchar2(20)) on commit delete rows;
    insert into x_gtmp_delete values(10, '10');
    subproc;
    insert into x_gtmp_delete values(20, '20');
    raise notice 'main';
    for r in select * from x_gtmp_delete loop
      raise notice '%', r;
    end loop;
    commit;
    raise notice 'main1';
    for r in select * from x_gtmp_delete loop
      raise notice '%', r;
    end loop;
  end;
}

permutation s1a1s ax_s1a1e
permutation s1a1s s2a2s s1lsto s2a1e ax_s1a2e
permutation preserve preserve1 delete drop begin drop1 end begin drop2 end begin drop3 end gpreserve gpreserve1 gdelete
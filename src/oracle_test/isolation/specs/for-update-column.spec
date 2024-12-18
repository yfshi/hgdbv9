# Test FOR UPDATE OF [schema.][table.] column  [,[schema.][table.] ..] clause 
# FOR UPDATE OF [schema.][table.] column  [,[schema.][table.] ..]
# FOR UPDATE OF [schema.][table.] column  [,[schema.][table.] ..] NOWAIT
# FOR UPDATE OF [schema.][table.] column  [,[schema.][table.] ..] WAIT integer

setup
{
  CREATE TABLE t(a int, b varchar2(20));
  CREATE TABLE t3(id int);
  CREATE TABLE tmp1(id int);
  CREATE TABLE tmp2(id int);
  INSERT INTO t VALUES(1, '1');
  INSERT INTO t VALUES(2, '2');
  INSERT INTO t VALUES(3, '3');
  INSERT INTO t VALUES(4, '4');

  INSERT INTO t3 VALUES(1);
  INSERT INTO t3 VALUES(10);

  INSERT INTO tmp1 VALUES(1);
  INSERT INTO tmp1 VALUES(2);
  INSERT INTO tmp2 VALUES(1);

  CREATE VIEW v1 AS SELECT * FROM t;
  CREATE VIEW v3 AS SELECT * FROM t3;
}

teardown
{
  DROP TABLE t;
  DROP TABLE t3;
  DROP TABLE tmp1;
  DROP TABLE tmp2;
  DROP VIEW v1;
  DROP VIEW v3;
}

session s1
step s1a { BEGIN; }
step s1b { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF a; }
step s1c { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF t.a; }
step s1d { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF public.t.a; }
step s1e { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF a,b; }
step s1f { SELECT t.a FROM t, t3 WHERE t.a = t3.id FOR UPDATE OF public.t.a, public.t3.id; }
step s1g { SELECT t.a FROM t, t3 FOR UPDATE OF a; }
step s1h { SELECT t.a FROM t, t3 FOR UPDATE OF id; }
step s1i { SELECT a FROM (SELECT id as a FROM t3, t WHERE id = 1)  FOR UPDATE OF a; }
step s1j { SELECT a FROM (SELECT id1 as a FROM (select id as id1 from t, t3))  FOR UPDATE OF a; }
step s1k { SELECT t.a FROM t3 join t on t3.id = t.a FOR UPDATE OF a; }
step s1l { SELECT a FROM v1, v3 FOR UPDATE OF id; }
step s1z { COMMIT; }

session s2
step s2a { BEGIN; }
step s2b { UPDATE t SET a = 11 WHERE a = 1; }
step s2c { SELECT t.a FROM t WHERE a = 1 FOR UPDATE NOWAIT; }
step s2d { UPDATE t3 SET id = 11 WHERE id = 1; }
step s2e { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF a NOWAIT; }
step s2f { SELECT t.a FROM t WHERE a = 1 FOR UPDATE OF a SKIP LOCKED; }
step s2z { ROLLBACK; }

session s3
step s3a { BEGIN; }
step s3b { SELECT * from tmp1, tmp2 FOR UPDATE OF id; }
step s3c { SELECT * from tmp1, tmp2 FOR UPDATE OF tmp1.id; }
step s3d { SELECT id AS i from tmp1 FOR UPDATE; }
step s3e { SELECT id from tmp1 FOR UPDATE OF id, tmp1; }
step s3f { SELECT id from tmp1 FOR UPDATE OF i; }
step s3g { SELECT a FROM (SELECT id+1 AS a FROM tmp1) FOR UPDATE;  }
step s3h { SELECT a FROM (SELECT id+1 AS a FROM tmp1) FOR UPDATE a;  }
step s3y { ROLLBACK; }
step s3z { COMMIT; }

session s4
step s4a { BEGIN; }
step s4b { SELECT * from tmp1 FOR UPDATE OF id; }
step s4c { SELECT * from tmp2 FOR UPDATE OF id; }
step s4z { COMMIT; }

session s5
step s5a { BEGIN; }
step s5b { SELECT * from tmp1 FOR UPDATE WAIT 5; }
step s5c { SELECT tmp1.id from tmp1, tmp2 WHERE tmp1.id = tmp2.id FOR UPDATE OF tmp1.id WAIT 5; }
step s5z { ROLLBACK; }

permutation s1a s1b s2a s2b s1z s2z
permutation s1a s1b s2a s2c s1z s2z
permutation s1a s1c s2a s2b s1z s2z
permutation s1a s1d s2a s2b s1z s2z
permutation s1a s1e s2a s2b s1z s2z
permutation s1a s1f s2a s2b s1z s2z
permutation s1a s1f s2a s2d s1z s2z
permutation s1a s1g s2a s2d s1z s2z
permutation s1a s1g s2a s2e s1z s2z
permutation s1a s1h s2a s2d s1z s2z
permutation s1a s1h s2a s2b s1z s2z
permutation s1a s1g s2a s2f s1z s2z
permutation s1a s1i s2a s2d s1z s2z
permutation s1a s1j s2a s2d s1z s2z
permutation s1a s1k s2a s2b s1z s2z
permutation s1a s1l s2a s2d s1z s2z
permutation s3a s3b s3y
permutation s3a s3c s4a s4b s3z s4z
permutation s3a s3c s4a s4c s3z s4z
permutation s3a s3d s3y
permutation s3a s3e s3y
permutation s3a s3f s3y
permutation s3a s3g s4a s4b s3z s4z
permutation s3a s3h s3y
permutation s4a s4b s5a s5b s4z s5z
permutation s4a s4b s5a s4z s5c s5z 
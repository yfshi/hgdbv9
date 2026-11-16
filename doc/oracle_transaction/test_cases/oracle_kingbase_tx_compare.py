import json, time, traceback, sys
import oracledb
import psycopg2
import os

ORACLE_DSN = dict(
    user=os.getenv('ORACLE_USER', 'test'),
    password=os.getenv('ORACLE_PASSWORD', 'test'),
    dsn=os.getenv('ORACLE_DSN', 'localhost:1522/FREEPDB1'),
)
KINGBASE_DSN = dict(
    host=os.getenv('KINGBASE_HOST', '127.0.0.1'),
    port=int(os.getenv('KINGBASE_PORT', '54321')),
    user=os.getenv('KINGBASE_USER', 'kingbase'),
    password=os.getenv('KINGBASE_PASSWORD', 'Kingbase123'),
    dbname=os.getenv('KINGBASE_DB', 'kingbase'),
)

results = []

class DB:
    def __init__(self, kind, autocommit=False, kb_slr='on'):
        self.kind = kind
        self.autocommit = autocommit
        if kind == 'oracle':
            self.conn = oracledb.connect(**ORACLE_DSN)
            self.conn.autocommit = autocommit
        else:
            self.conn = psycopg2.connect(**KINGBASE_DSN)
            self.conn.autocommit = autocommit
        self.cur = self.conn.cursor()
        if kind == 'kingbase' and kb_slr is not None:
            self.execute(f"SET ora_statement_level_rollback = {kb_slr}")
            if not autocommit:
                self.conn.commit()

    def execute(self, sql):
        self.cur.execute(sql)

    def query(self, sql):
        self.cur.execute(sql)
        return self.cur.fetchall()

    def commit(self):
        self.conn.commit()

    def rollback(self):
        self.conn.rollback()

    def close(self):
        try: self.cur.close()
        except Exception: pass
        try: self.conn.close()
        except Exception: pass

    def drop_table(self, name):
        try:
            if self.kind == 'oracle':
                self.execute(f"DROP TABLE {name} PURGE")
            else:
                self.execute(f"DROP TABLE IF EXISTS {name}")
            self.commit()
        except Exception:
            self.rollback()

    def reset_table(self, name):
        self.drop_table(name)
        self.execute(f"CREATE TABLE {name}(id INT)")
        self.commit()

    def rows(self, name):
        try:
            self.execute(f"SELECT id FROM {name} ORDER BY 1")
            return [r[0] for r in self.cur.fetchall()]
        except Exception as e:
            return 'ERR:' + em(e)

    def table_exists(self, name):
        try:
            self.execute(f"SELECT COUNT(*) FROM {name}")
            self.cur.fetchall()
            return True
        except Exception:
            return False

def em(e):
    return str(e).splitlines()[0]

def add(tid, dbkind, observations):
    results.append({'id': tid, 'db': dbkind, 'observations': observations})
    print(tid, dbkind, json.dumps(observations, ensure_ascii=False))

def run(kind, tid, func, kb_slr='on', autocommit=False):
    db = None
    try:
        db = DB(kind, autocommit=autocommit, kb_slr=kb_slr)
        func(db)
    except Exception as e:
        add(tid, kind, {'fatal': em(e)})
    finally:
        if db: db.close()

def read_committed(kind, table):
    d = DB(kind, autocommit=True, kb_slr=None)
    try:
        return d.rows(table)
    finally:
        d.close()

# ---------------- tests ----------------

def t01(db):
    d = DB(db.kind, autocommit=True, kb_slr=None)
    try:
        d.reset_table('T01')
        d.execute('INSERT INTO T01 VALUES (1)')
        d.commit()
        add('T01_autocommit_on_visibility', d.kind, {'visible_other_session': read_committed(d.kind, 'T01')})
    finally:
        d.close()

def t02(db):
    db.reset_table('T02')
    db.execute('INSERT INTO T02 VALUES (1)')
    before = read_committed(db.kind, 'T02')
    db.commit()
    after = read_committed(db.kind, 'T02')
    add('T02_autocommit_off_visibility', db.kind, {'before_commit': before, 'after_commit': after})

def t03(db):
    obs = {}
    try: db.execute('COMMIT'); db.commit(); obs['commit'] = 'ok'
    except Exception as e: obs['commit'] = em(e)
    try: db.execute('ROLLBACK'); db.rollback(); obs['rollback'] = 'ok'
    except Exception as e: obs['rollback'] = em(e)
    add('T03_no_tx_commit_rollback', db.kind, obs)

def t04(db):
    db.reset_table('T04')
    obs = {}
    try:
        db.execute('INSERT INTO T04 VALUES (1)')
        try: db.execute("INSERT INTO T04 VALUES ('bad')")
        except Exception as e: obs['bad_error'] = em(e)
        try: db.execute('INSERT INTO T04 VALUES (2)')
        except Exception as e: obs['after_error_insert'] = em(e)
        try: db.commit(); obs['commit'] = 'ok'
        except Exception as e: obs['commit'] = em(e)
    except Exception as e: obs['outer'] = em(e)
    add('T04_explicit_failed_dml', db.kind, obs)
    add('T04_rows', db.kind, {'rows': read_committed(db.kind, 'T04')})

def t05(db):
    db.reset_table('T05')
    obs = {}
    try:
        db.execute('INSERT INTO T05 VALUES (1)')
        try:
            if db.kind == 'oracle': db.execute('SELECT 1/0 FROM dual')
            else: db.execute('SELECT 1/0')
        except Exception as e: obs['select_error'] = em(e)
        try:
            db.execute('INSERT INTO T05 VALUES (2)')
            db.commit()
            obs['commit'] = 'ok'
        except Exception as e: obs['after_error'] = em(e)
    except Exception as e: obs['outer'] = em(e)
    add('T05_select_error', db.kind, obs)
    add('T05_rows', db.kind, {'rows': read_committed(db.kind, 'T05')})

def t06(db):
    db.reset_table('T06')
    obs = {}
    try:
        db.execute('INSERT INTO T06 VALUES (1)')
        try:
            db.execute("""BEGIN
                INSERT INTO T06 VALUES (2);
                RAISE_APPLICATION_ERROR(-20001,'raise');
            END;""")
        except Exception as e: obs['block_error'] = em(e)
        try:
            db.execute('INSERT INTO T06 VALUES (3)')
            db.commit(); obs['commit'] = 'ok'
        except Exception as e: obs['after_error'] = em(e)
    except Exception as e: obs['outer'] = em(e)
    add('T06_unhandled_raise_prior_tx', db.kind, obs)
    add('T06_rows', db.kind, {'rows': read_committed(db.kind, 'T06')})

def t07(db):
    db.reset_table('T07')
    obs = {}
    try:
        db.execute('INSERT INTO T07 VALUES (1)')
        try:
            db.execute("""BEGIN
                INSERT INTO T07 VALUES (2);
                INSERT INTO T07 VALUES ('bad');
            END;""")
        except Exception as e: obs['block_error'] = em(e)
        try:
            db.execute('INSERT INTO T07 VALUES (3)')
            db.commit(); obs['commit'] = 'ok'
        except Exception as e: obs['after_error'] = em(e)
    except Exception as e: obs['outer'] = em(e)
    add('T07_unhandled_sql_prior_tx', db.kind, obs)
    add('T07_rows', db.kind, {'rows': read_committed(db.kind, 'T07')})

def t08(db):
    db.reset_table('T08')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T08 VALUES (1);
            INSERT INTO T08 VALUES ('bad');
            INSERT INTO T08 VALUES (2);
        EXCEPTION WHEN OTHERS THEN
            INSERT INTO T08 VALUES (3);
        END;""")
        db.commit(); obs['commit'] = 'ok'
    except Exception as e: obs['error'] = em(e)
    add('T08_handled_exception', db.kind, obs)
    add('T08_rows', db.kind, {'rows': read_committed(db.kind, 'T08')})

def t08b(db):
    t08(db)

def t09(db):
    db.reset_table('T09')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T09 VALUES (1);
            BEGIN
                INSERT INTO T09 VALUES ('bad');
            EXCEPTION WHEN OTHERS THEN
                INSERT INTO T09 VALUES (2);
            END;
            INSERT INTO T09 VALUES (3);
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T09_nested_exception', db.kind, obs)
    add('T09_rows', db.kind, {'rows': read_committed(db.kind, 'T09')})

def t10(db):
    db.reset_table('T10')
    obs = {}
    try:
        db.execute("""BEGIN
            BEGIN
                INSERT INTO T10 VALUES ('bad');
            EXCEPTION WHEN OTHERS THEN
                INSERT INTO T10 VALUES (2);
                RAISE_APPLICATION_ERROR(-20002,'handler error');
            END;
        EXCEPTION WHEN OTHERS THEN
            INSERT INTO T10 VALUES (3);
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T10_handler_raises_outer', db.kind, obs)
    add('T10_rows', db.kind, {'rows': read_committed(db.kind, 'T10')})

def t11(db):
    db.reset_table('T11')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T11 VALUES (1);
            BEGIN
                INSERT INTO T11 VALUES ('bad');
            EXCEPTION WHEN OTHERS THEN
                COMMIT;
            END;
            INSERT INTO T11 VALUES (2);
            COMMIT;
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T11_commit_in_exception', db.kind, obs)
    add('T11_rows', db.kind, {'rows': read_committed(db.kind, 'T11')})

def t12(db):
    db.reset_table('T12')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T12 VALUES (1);
            BEGIN
                INSERT INTO T12 VALUES ('bad');
            EXCEPTION WHEN OTHERS THEN
                ROLLBACK;
            END;
            INSERT INTO T12 VALUES (2);
            COMMIT;
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T12_rollback_in_exception', db.kind, obs)
    add('T12_rows', db.kind, {'rows': read_committed(db.kind, 'T12')})

def t13(db):
    db.reset_table('T13')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P13 AS
        BEGIN
            INSERT INTO T13 VALUES (1);
            BEGIN
                INSERT INTO T13 VALUES ('bad');
            EXCEPTION WHEN OTHERS THEN
                INSERT INTO T13 VALUES (2);
                COMMIT;
            END;
            INSERT INTO T13 VALUES (3);
        END;""")
        db.commit()
        db.execute('BEGIN P13; END;')
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T13_proc_exception_commit', db.kind, obs)
    add('T13_rows', db.kind, {'rows': read_committed(db.kind, 'T13')})

def t14(db):
    db.reset_table('T14')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P14 AS
        BEGIN
            INSERT INTO T14 VALUES (4);
            COMMIT;
        EXCEPTION WHEN OTHERS THEN
            INSERT INTO T14 VALUES (5);
        END;""")
        db.commit()
        db.execute('BEGIN P14; END;')
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T14_proc_commit_body_exception', db.kind, obs)
    add('T14_rows', db.kind, {'rows': read_committed(db.kind, 'T14')})

def t15(db):
    db.reset_table('T15')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P15 AS
        BEGIN
            INSERT INTO T15 VALUES (2);
            RAISE_APPLICATION_ERROR(-20003,'proc raise');
        END;""")
        db.commit()
        db.execute('INSERT INTO T15 VALUES (1)')
        try: db.execute('BEGIN P15; END;')
        except Exception as e: obs['proc_error'] = em(e)
        try:
            db.execute('INSERT INTO T15 VALUES (3)')
            db.commit(); obs['commit'] = 'ok'
        except Exception as e: obs['after_error'] = em(e)
    except Exception as e: obs['outer'] = em(e)
    add('T15_proc_unhandled_prior_tx', db.kind, obs)
    add('T15_rows', db.kind, {'rows': read_committed(db.kind, 'T15')})

def t16(db):
    db.reset_table('T16')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T16 VALUES (1);
            SAVEPOINT s1;
            INSERT INTO T16 VALUES (2);
            ROLLBACK TO SAVEPOINT s1;
            INSERT INTO T16 VALUES (3);
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T16_plsql_savepoint', db.kind, obs)
    add('T16_rows', db.kind, {'rows': read_committed(db.kind, 'T16')})

def t17(db):
    db.reset_table('T17')
    obs = {}
    try:
        db.execute('SAVEPOINT s2')
        db.execute('INSERT INTO T17 VALUES (4)')
        db.execute('ROLLBACK TO SAVEPOINT s2')
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T17_sql_savepoint_no_begin', db.kind, obs)
    add('T17_rows', db.kind, {'rows': read_committed(db.kind, 'T17')})

def t18(db):
    db.reset_table('T18')
    obs = {}
    try:
        db.execute('INSERT INTO T18 VALUES (1)')
        db.execute('SAVEPOINT s1')
        db.execute('INSERT INTO T18 VALUES (2)')
        try:
            db.execute('BEGIN ROLLBACK TO SAVEPOINT s1; END;')
        except Exception as e: obs['plsql_rollback_error'] = em(e)
        db.execute('INSERT INTO T18 VALUES (3)')
        db.commit()
    except Exception as e: obs['outer'] = em(e)
    add('T18_sql_sp_plsql_rollback', db.kind, obs)
    add('T18_rows', db.kind, {'rows': read_committed(db.kind, 'T18')})

def t19(db):
    db.reset_table('T19')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P19 AS
        BEGIN
            SAVEPOINT psp;
            INSERT INTO T19 VALUES (2);
        END;""")
        db.commit()
        db.execute('INSERT INTO T19 VALUES (1)')
        db.execute('BEGIN P19; END;')
        try:
            db.execute('ROLLBACK TO SAVEPOINT psp')
        except Exception as e: obs['sql_rollback_error'] = em(e)
        db.execute('INSERT INTO T19 VALUES (3)')
        db.commit()
    except Exception as e: obs['outer'] = em(e)
    add('T19_plsql_sp_sql_rollback', db.kind, obs)
    add('T19_rows', db.kind, {'rows': read_committed(db.kind, 'T19')})

def t20(db):
    db.reset_table('T20')
    db.drop_table('DDL_T20')
    obs = {}
    try:
        db.execute('INSERT INTO T20 VALUES (5)')
        db.execute('CREATE TABLE DDL_T20(id INT)')
        db.rollback()
    except Exception as e: obs['error'] = em(e)
    add('T20_ddl_implicit_commit', db.kind, obs)
    add('T20_rows', db.kind, {'rows': read_committed(db.kind, 'T20')})
    d = DB(db.kind, autocommit=True, kb_slr=None)
    try:
        add('T20_ddl_exists', db.kind, {'exists': d.table_exists('DDL_T20')})
    finally: d.close()

def t21(db):
    db.reset_table('T21')
    obs = {}
    try:
        db.execute('INSERT INTO T21 VALUES (1)')
        db.execute('INSERT INTO T21 VALUES (2)')
        db.commit()
        db.execute('TRUNCATE TABLE T21')
        db.rollback()
    except Exception as e: obs['error'] = em(e)
    add('T21_truncate_implicit_commit', db.kind, obs)
    add('T21_rows', db.kind, {'rows': read_committed(db.kind, 'T21')})

def t22(db):
    # READ ONLY snapshot
    setup = DB(db.kind, autocommit=True, kb_slr=None)
    try:
        setup.drop_table('T22')
        setup.execute('CREATE TABLE T22(id INT)')
        setup.execute('INSERT INTO T22 VALUES (1)')
        setup.commit()
    finally: setup.close()
    time.sleep(2)
    a = DB(db.kind, autocommit=False, kb_slr=None)
    b = DB(db.kind, autocommit=False, kb_slr=None)
    obs = {}
    try:
        a.execute('SET TRANSACTION READ ONLY')
        a.execute('SELECT COUNT(*) FROM T22'); first = a.cur.fetchone()[0]
        b.execute('INSERT INTO T22 VALUES (2)'); b.commit()
        a.execute('SELECT COUNT(*) FROM T22'); second = a.cur.fetchone()[0]
        a.execute('SELECT COUNT(*) FROM T22 WHERE id=2'); id2 = a.cur.fetchone()[0]
        obs.update({'first': first, 'second': second, 'id2': id2})
        a.rollback()
    except Exception as e: obs['error'] = em(e)
    finally: a.close(); b.close()
    add('T22_read_only_snapshot', db.kind, obs)

def t23(db):
    setup = DB(db.kind, autocommit=True, kb_slr=None)
    try:
        setup.drop_table('T23')
        setup.execute('CREATE TABLE T23(id INT)')
        setup.execute('INSERT INTO T23 VALUES (1)')
        setup.commit()
    finally: setup.close()
    time.sleep(2)
    a = DB(db.kind, autocommit=False, kb_slr=None)
    b = DB(db.kind, autocommit=False, kb_slr=None)
    obs = {}
    try:
        a.execute('SET TRANSACTION ISOLATION LEVEL SERIALIZABLE')
        a.execute('SELECT COUNT(*) FROM T23'); first = a.cur.fetchone()[0]
        b.execute('INSERT INTO T23 VALUES (2)'); b.commit()
        a.execute('SELECT COUNT(*) FROM T23'); second = a.cur.fetchone()[0]
        obs.update({'first': first, 'second': second})
        a.rollback()
    except Exception as e: obs['error'] = em(e)
    finally: a.close(); b.close()
    add('T23_serializable_snapshot', db.kind, obs)

def t24(db):
    db.reset_table('T24'); db.reset_table('T24_LOG')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P24 AS
        PRAGMA AUTONOMOUS_TRANSACTION;
        BEGIN
            INSERT INTO T24_LOG VALUES (1);
            COMMIT;
        END;""")
        db.commit()
        db.execute('INSERT INTO T24 VALUES (1)')
        db.execute('BEGIN P24; END;')
        db.rollback()
    except Exception as e: obs['error'] = em(e)
    add('T24_autonomous_commit', db.kind, obs)
    add('T24_rows', db.kind, {'main': read_committed(db.kind, 'T24'), 'log': read_committed(db.kind, 'T24_LOG')})

def t25(db):
    db.reset_table('T25'); db.reset_table('T25_LOG')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE P25 AS
        PRAGMA AUTONOMOUS_TRANSACTION;
        BEGIN
            INSERT INTO T25_LOG VALUES (2);
            RAISE_APPLICATION_ERROR(-20004,'ax error');
        END;""")
        db.commit()
        db.execute('INSERT INTO T25 VALUES (1)')
        try: db.execute('BEGIN P25; END;')
        except Exception as e: obs['ax_error'] = em(e)
        db.execute('INSERT INTO T25 VALUES (3)')
        db.commit()
    except Exception as e: obs['outer'] = em(e)
    add('T25_autonomous_unhandled', db.kind, obs)
    add('T25_rows', db.kind, {'main': read_committed(db.kind, 'T25'), 'log': read_committed(db.kind, 'T25_LOG')})


def t26(db):
    db.reset_table('T26')
    obs = {}
    try:
        db.execute("""CREATE OR REPLACE PROCEDURE Q26 AS
        BEGIN
            INSERT INTO T26 VALUES (3);
            INSERT INTO T26 VALUES ('bad');
        END;""")
        db.commit()
        db.execute("""CREATE OR REPLACE PROCEDURE P26 AS
        BEGIN
            INSERT INTO T26 VALUES (2);
            Q26;
        END;""")
        db.commit()
        db.execute("""BEGIN
            INSERT INTO T26 VALUES (1);
            P26;
        EXCEPTION WHEN OTHERS THEN
            INSERT INTO T26 VALUES (4);
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T26_nested_proc_handled', db.kind, obs)
    add('T26_rows', db.kind, {'rows': read_committed(db.kind, 'T26')})

TESTS = [
    ('T01', t01), ('T02', t02), ('T03', t03), ('T04', t04), ('T05', t05),
    ('T06', t06), ('T07', t07), ('T08', t08), ('T09', t09), ('T10', t10),
    ('T11', t11), ('T12', t12), ('T13', t13), ('T14', t14), ('T15', t15),
    ('T16', t16), ('T17', t17), ('T18', t18), ('T19', t19), ('T20', t20),
    ('T21', t21), ('T22', t22), ('T23', t23), ('T24', t24), ('T25', t25),
    ('T26', t26),
]

for kind in ['oracle', 'kingbase']:
    for tid, fn in TESTS:
        run(kind, tid, fn)

# Kingbase default off variant for PL/SQL handled exception
def t08_off(db):
    db.reset_table('T08O')
    obs = {}
    try:
        db.execute("""BEGIN
            INSERT INTO T08O VALUES (1);
            INSERT INTO T08O VALUES ('bad');
            INSERT INTO T08O VALUES (2);
        EXCEPTION WHEN OTHERS THEN
            INSERT INTO T08O VALUES (3);
        END;""")
        db.commit()
    except Exception as e: obs['error'] = em(e)
    add('T08_kingbase_default_off', db.kind, obs)
    add('T08_kingbase_default_off_rows', db.kind, {'rows': read_committed(db.kind, 'T08O')})

run('kingbase', 'T08O', t08_off, kb_slr='off')

CLEANUP_TABLES = [f'T{i:02d}' for i in range(1, 27)] + ['T24_LOG', 'T25_LOG', 'T08O', 'DDL_T20']
for kind in ['oracle', 'kingbase']:
    d = DB(kind, autocommit=True, kb_slr=None)
    try:
        for t in CLEANUP_TABLES:
            d.drop_table(t)
    finally:
        d.close()

with open(os.getenv('TX_RESULT_FILE', '/tmp/tx_compare_results.json'), 'w', encoding='utf-8') as f:
    json.dump(results, f, ensure_ascii=False, indent=2)
print('WROTE', os.getenv('TX_RESULT_FILE', '/tmp/tx_compare_results.json'))
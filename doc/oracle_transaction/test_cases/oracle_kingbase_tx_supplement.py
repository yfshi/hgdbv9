import oracledb, psycopg2, json, os
ORACLE_DSN=dict(
 user=os.getenv('ORACLE_USER','test'),
 password=os.getenv('ORACLE_PASSWORD','test'),
 dsn=os.getenv('ORACLE_DSN','localhost:1522/FREEPDB1'),
)
KINGBASE_DSN=dict(
 host=os.getenv('KINGBASE_HOST','127.0.0.1'),
 port=int(os.getenv('KINGBASE_PORT','54321')),
 user=os.getenv('KINGBASE_USER','kingbase'),
 password=os.getenv('KINGBASE_PASSWORD','Kingbase123'),
 dbname=os.getenv('KINGBASE_DB','kingbase'),
)
out=[]
def em(e): return str(e).splitlines()[0]
class DB:
 def __init__(self,kind,autocommit=False):
  self.kind=kind
  if kind=='oracle': self.conn=oracledb.connect(**ORACLE_DSN); self.conn.autocommit=autocommit
  else:
   self.conn=psycopg2.connect(**KINGBASE_DSN); self.conn.autocommit=autocommit
   self.cur=self.conn.cursor(); self.cur.execute('SET ora_statement_level_rollback = on'); self.conn.commit()
  self.cur=self.conn.cursor()
 def ex(self,s): self.cur.execute(s)
 def q(self,s): self.cur.execute(s); return [r[0] for r in self.cur.fetchall()]
 def commit(self): self.conn.commit()
 def rollback(self): self.conn.rollback()
 def close(self): self.conn.close()
 def drop(self,n): 
  try:
   self.ex(f"DROP TABLE {n} PURGE" if self.kind=='oracle' else f"DROP TABLE IF EXISTS {n}"); self.commit()
  except Exception: self.rollback()
 def reset(self,n):
  self.drop(n); self.ex(f'CREATE TABLE {n}(id INT)'); self.commit()
 def rows(self,n):
  try: return self.q(f'SELECT id FROM {n} ORDER BY 1')
  except Exception as e: return 'ERR:'+em(e)
def add(t,k,o): out.append({'id':t,'db':k,'obs':o}); print(t,k,json.dumps(o,ensure_ascii=False))

# A target retention
for kind in ['oracle','kingbase']:
 d=DB(kind)
 try:
  d.reset('SA'); d.ex('INSERT INTO SA VALUES (1)'); d.ex('SAVEPOINT s'); d.ex('ROLLBACK TO SAVEPOINT s'); d.ex('INSERT INTO SA VALUES (2)'); d.ex('ROLLBACK TO SAVEPOINT s'); d.ex('INSERT INTO SA VALUES (3)'); d.commit()
  err=None
 except Exception as e: err=em(e)
 add('SA_target_retention',kind,{'error':err})
 d2=DB(kind,autocommit=True)
 try: add('SA_target_retention_rows',kind,{'rows':d2.rows('SA')})
 finally: d2.close(); d.close()

# B same-name
for kind in ['oracle','kingbase']:
 d=DB(kind); err=None
 try:
  d.reset('SB'); d.ex('INSERT INTO SB VALUES (1)'); d.ex('SAVEPOINT s'); d.ex('INSERT INTO SB VALUES (2)'); d.ex('SAVEPOINT s'); d.ex('ROLLBACK TO SAVEPOINT s'); d.ex('INSERT INTO SB VALUES (3)'); d.commit()
 except Exception as e: err=em(e)
 add('SB_same_name',kind,{'error':err})
 d2=DB(kind,autocommit=True)
 try: add('SB_same_name_rows',kind,{'rows':d2.rows('SB')})
 finally: d2.close(); d.close()

# C commit and chain
for kind in ['oracle','kingbase']:
 d=DB(kind); err=None
 try:
  d.reset('SC'); d.ex('INSERT INTO SC VALUES (1)'); d.ex('COMMIT AND CHAIN'); d.ex('INSERT INTO SC VALUES (2)'); d.commit()
 except Exception as e: err=em(e)
 add('SC_commit_and_chain',kind,{'error':err})
 d2=DB(kind,autocommit=True)
 try: add('SC_rows',kind,{'rows':d2.rows('SC')})
 finally: d2.close(); d.close()

# D function commit
for kind in ['oracle','kingbase']:
 d=DB(kind); obs={}
 try:
  d.reset('SD')
  d.ex("""CREATE OR REPLACE FUNCTION F_COMMIT RETURN NUMBER AS
  BEGIN
    INSERT INTO SD VALUES (1);
    COMMIT;
    RETURN 1;
  END;""")
  d.commit()
  obs['create']='ok'
  try:
   d.ex('SELECT F_COMMIT FROM dual' if kind=='oracle' else 'SELECT F_COMMIT()')
   obs['call']='ok'
  except Exception as e: obs['call']=em(e)
 except Exception as e: obs['create']=em(e)
 add('SD_function_commit',kind,obs)
 d2=DB(kind,autocommit=True)
 try: add('SD_rows',kind,{'rows':d2.rows('SD')})
 finally: d2.close(); d.close()

# E trigger commit
for kind in ['oracle','kingbase']:
 d=DB(kind); obs={}
 try:
  d.reset('SE'); d.reset('SE_LOG')
  d.ex("""CREATE OR REPLACE TRIGGER TRG_SE AFTER INSERT ON SE FOR EACH ROW
  BEGIN
    INSERT INTO SE_LOG VALUES (:NEW.id);
    COMMIT;
  END;""")
  d.commit(); obs['create']='ok'
  try:
   d.ex('INSERT INTO SE VALUES (1)'); d.commit(); obs['insert']='ok'
  except Exception as e: obs['insert']=em(e)
 except Exception as e: obs['create']=em(e)
 add('SE_trigger_commit',kind,obs)
 d2=DB(kind,autocommit=True)
 try: add('SE_rows',kind,{'rows':d2.rows('SE_LOG')})
 finally: d2.close(); d.close()

# F forall failure (needs BULK In-BIND collection)
for kind in ['oracle','kingbase']:
 d=DB(kind); obs={}
 try:
  d.drop('SF2')
  d.ex('CREATE TABLE SF2(id INT PRIMARY KEY)')
  d.commit()
  d.ex("""DECLARE
    TYPE t IS TABLE OF INT;
    v t := t(1, 1, 3);
  BEGIN
    FORALL i IN 1..v.COUNT
      INSERT INTO SF2 VALUES (v(i));
  END;""")
  d.commit()
 except Exception as e: obs['error']=em(e)
 add('SF_forall_failure',kind,obs)
 d2=DB(kind,autocommit=True)
 try: add('SF_rows',kind,{'rows':d2.rows('SF2')})
 finally: d2.close(); d.close()

CLEANUP_TABLES = ['SA', 'SB', 'SC', 'SD', 'SE', 'SE_LOG', 'SF', 'SF2']
for kind in ['oracle', 'kingbase']:
    d = DB(kind, autocommit=True)
    try:
        for t in CLEANUP_TABLES:
            d.drop(t)
    finally:
        d.close()

json.dump(out, open(os.getenv('TX_SUPP_RESULT_FILE', '/tmp/tx_supp_results.json'), 'w'), ensure_ascii=False, indent=2)
print('done')

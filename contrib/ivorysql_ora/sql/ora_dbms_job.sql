--
-- DBMS_JOB PACKAGE.
--
CREATE USER system WITH superuser;

\c - system

/* Begin - ReqID:SRS-PLSQL-DBMS_JOB */
DECLARE
  job_number INTEGER;
BEGIN
  Call DBMS_JOB.submit(job_number,
'INSERT INTO random_data (random_number, insert_currenttime)
  VALUES (111 , SYSTIMESTAMP);',
sysdate,
'sysdate+1.0/24/60/60',
FALSE 
  );
  COMMIT;
END;
/

DECLARE
  job_number INTEGER;
BEGIN
  call DBMS_JOB.submit(
    job       => job_number,
    what      => 'INSERT INTO random_data (random_number, insert_currenttime) VALUES (133, SYSTIMESTAMP);',
    next_date => sysdate,
    interval  => 'freq=yearly;interval=3;BYMONTH=11;BYMONTHDAY=11;BYHOUR=16;BYMINUTE=11;BYSECOND=2',
    no_parse  => FALSE 
  );
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.isubmit(3, 'INSERT INTO random_data (random_number, insert_currenttime)
  VALUES (8 , SYSTIMESTAMP);', TO_DATE('2024-01-18 12:30:45', 'YYYY-MM-DD HH24:MI:SS')
, 'sysdate+1.0/24/60', FALSE 
  );
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.broken(job => 1, broken => TRUE);
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.broken(job => 2, broken => TRUE);
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.broken(job => 1, broken => FALSE, next_date => to_date('2024-10-21 16:15:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.broken(job => 2, broken => FALSE, next_date => to_date('2024-10-21 16:15:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.broken(job => 1, broken => false);
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.broken(job => 2, broken => false);
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.change(1,null,null,'sysdate+10.0/24/60');
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.change(2,null,null,'freq=yearly;interval=3;BYMONTH=12;BYMONTHDAY=12;BYHOUR=16;BYMINUTE=11;BYSECOND=2');
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.change(1,null,null,'sysdate+10.0/24/60');
  COMMIT;
END;
/

BEGIN
 Call DBMS_JOB.change(2,null,null,'freq=yearly;interval=3;BYMONTH=12;BYMONTHDAY=12;BYHOUR=16;BYMINUTE=11;BYSECOND=2');
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.INSTANCE(job=>1,instance=>2,force=>false);
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.INSTANCE(job=>2,instance=>2,force=>false);
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.INTERVAL(1,'sysdate+1.0/24/60');
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.INTERVAL(2,'freq=yearly;interval=3;BYMONTH=12;BYMONTHDAY=13;BYHOUR=16;BYMINUTE=11;BYSECOND=2');
  COMMIT;
END;
/


BEGIN
  Call DBMS_JOB.run(1); 
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.run(2); 
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.WHAT(1,'INSERT INTO random_data (random_number, insert_currenttime)
  VALUES (100, SYSTIMESTAMP); ');
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.WHAT(2,'INSERT INTO random_data (random_number, insert_currenttime)
  VALUES (100, SYSTIMESTAMP); ');
  COMMIT;
END;
/


BEGIN
  call DBMS_JOB.NEXT_DATE(1,sysdate);
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.NEXT_DATE(2,sysdate);
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.NEXT_DATE(1,to_date('2024-10-21 17:00:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.NEXT_DATE(2,to_date('2024-10-21 17:00:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/


BEGIN
  call DBMS_JOB.NEXT_DATE(1,to_date('3000-10-21 17:00:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.NEXT_DATE(2,to_date('3000-10-21 17:00:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

BEGIN
  call DBMS_JOB.NEXT_DATE(3,to_date('3000-10-21 17:00:00','yyyy-mm-dd hh24:mi:ss'));
  COMMIT;
END;
/

DECLARE
   v_job_number INTEGER := 1;
   v_mycall VARCHAR2(4000);
BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:  % ' , v_mycall;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;
END;
/

DECLARE
   v_job_number INTEGER := 2;
   v_mycall VARCHAR2(4000);
BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:  % ' , v_mycall;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;
END;
/

DECLARE
   v_job_number INTEGER := 3;
   v_mycall VARCHAR2(4000);
BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:  % ' , v_mycall;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;
END;
/

DECLARE
   v_job_number INTEGER := 1;
   v_mycall VARCHAR2(4000);
   v_myinst VARCHAR2(4000);

BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall, v_myinst);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:% ' , v_mycall ;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;

   IF v_myinst IS NOT NULL THEN
       RAISE NOTICE 'Successfully exported job instance information: %' ,v_myinst;
   ELSE
      RAISE NOTICE 'Failed to export job instance information';
   END IF;
END;
/

DECLARE
   v_job_number INTEGER := 2;
   v_mycall VARCHAR2(4000);
   v_myinst VARCHAR2(4000);

BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall, v_myinst);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:% ' , v_mycall ;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;

   IF v_myinst IS NOT NULL THEN
       RAISE NOTICE 'Successfully exported job instance information: %' ,v_myinst;
   ELSE
      RAISE NOTICE 'Failed to export job instance information';
   END IF;
END;
/

DECLARE
   v_job_number INTEGER := 3;
   v_mycall VARCHAR2(4000);
   v_myinst VARCHAR2(4000);

BEGIN
   
   Call DBMS_JOB.USER_EXPORT(v_job_number, v_mycall, v_myinst);

   
   IF v_mycall IS NOT NULL THEN
      RAISE NOTICE 'Successfully exported job information:% ' , v_mycall ;
   ELSE
      RAISE NOTICE 'Failed to export job information';
   END IF;

   IF v_myinst IS NOT NULL THEN
       RAISE NOTICE 'Successfully exported job instance information: %' ,v_myinst;
   ELSE
      RAISE NOTICE 'Failed to export job instance information';
   END IF;
END;
/

BEGIN
  Call DBMS_JOB.REMOVE(job => 1);
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.REMOVE(job => 2);
  COMMIT;
END;
/

BEGIN
  Call DBMS_JOB.REMOVE(job => 3);
  COMMIT;
END;
/

SELECT * FROM DBA_JOBS order by job;
SELECT * FROM USER_JOBS order by job;
SELECT * FROM DBA_JOBS_RUNNING order by job;
/* End - ReqID:SRS-PLSQL-DBMS_JOB */

/* BEGIN - BUG#M0000412 */
\c - system
create user test password 'Hello@123';
create schema authorization test;
grant execute on package sys.dbms_job to test;

\c - test
create table test_dbms_job(ct_time date);

DECLARE
  jobid integer;
BEGIN
  dbms_job.submit(jobid,'insert into test_dbms_job values (sysdate);',sysdate,'sysdate+1.0/24/60');
END;
/

DECLARE
  jobid INTEGER;
BEGIN
  select job into jobid from user_jobs;
  Call DBMS_JOB.REMOVE(job => jobid);
  COMMIT;
END;
/
drop table test_dbms_job;

\c - system
revoke execute on package sys.dbms_job from test;
drop schema test;
drop user test;
/* END - BUG#M0000412 */
declare
    name text;
begin
        select into name 10::regrole;
		execute format('set session authorization %s;',name);
end;
/

DROP USER system;

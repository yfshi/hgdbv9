/* Begin - ReqID:SRS-PLSQL-DBMS_JOB */
CREATE FUNCTION sys.evaluate_calendar_string(text, timestamp, timestamp, boolean)
RETURNS timestamp
AS 'MODULE_PATHNAME'
LANGUAGE C;

/* CREATE FUNCTION */
CREATE OR REPLACE FUNCTION sys.evaluate_next_schedule_expression (interval_expression text)
RETURN pg_catalog.timestamp SECURITY INVOKER
IS
    resultdatetime pg_catalog.timestamp;
BEGIN

    EXECUTE 'SELECT ' || interval_expression || ' FROM DUAL' INTO resultdatetime;

    RETURN resultdatetime;
END;

/* CREATE FUNCTION */
CREATE OR REPLACE FUNCTION sys.evaluate_jobnextrun(dbmsjobid IN NUMBER) 
RETURN timestamp SECURITY INVOKER
IS
    newjobnextrun timestamp;
    jobenabled BOOLEAN;
BEGIN
    SELECT j.jobenabled 
    INTO jobenabled
    FROM pgagent.pga_job j
    WHERE j.jobid = dbmsjobid;

    IF jobenabled THEN
        set nls_date_format="YYYY-MM-DD HH24:MI:SS";

        IF (SELECT s.dbmsjob_calendar_enable 
            FROM pgagent.pga_schedule s
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid) THEN
            
            SELECT MIN(sys.evaluate_calendar_string(s.dbmsjob_interval, j.jobnextrun::pg_catalog.timestamp, NULL, false)) 
            INTO newjobnextrun
            FROM pgagent.pga_schedule s, pgagent.pga_job j
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid
              AND j.jobid = dbmsjobid;
        ELSE
            SELECT MIN(sys.evaluate_next_schedule_expression(s.dbmsjob_interval))
            INTO newjobnextrun
            FROM pgagent.pga_schedule s
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid;
        END IF;

        UPDATE pgagent.pga_job j
        SET j.jobnextrun = newjobnextrun::pg_catalog.timestamp 
        WHERE j.jobid = dbmsjobid;
    ELSE
        UPDATE pgagent.pga_job j
        SET j.jobnextrun = NULL 
        WHERE j.jobid = dbmsjobid;
    END IF;

    RETURN newjobnextrun;
END;

/* Begin - BUG#Z4792 */
/* Same as sys.evaluate_jobnextrun above, except not update pgagent.pga_job */
CREATE OR REPLACE FUNCTION sys.evaluate_jobnextrun_for_trigger(dbmsjobid IN NUMBER) 
RETURN timestamp SECURITY INVOKER
IS
    newjobnextrun timestamp;
    jobenabled BOOLEAN;
BEGIN
    SELECT j.jobenabled INTO jobenabled FROM pgagent.pga_job j WHERE j.jobid = dbmsjobid;

    IF jobenabled THEN
        set nls_date_format="YYYY-MM-DD HH24:MI:SS";

        IF (SELECT s.dbmsjob_calendar_enable 
            FROM pgagent.pga_schedule s
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid) THEN
            SELECT MIN(sys.evaluate_calendar_string(s.dbmsjob_interval, j.jobnextrun::pg_catalog.timestamp, NULL, false))
            INTO newjobnextrun
            FROM pgagent.pga_schedule s, pgagent.pga_job j
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid
              AND j.jobid = dbmsjobid;
        ELSE
            SELECT MIN(sys.evaluate_next_schedule_expression(s.dbmsjob_interval))
            INTO newjobnextrun
            FROM pgagent.pga_schedule s
            WHERE s.jscenabled = TRUE AND s.jscjobid = dbmsjobid;
        END IF;
    ELSE
        newjobnextrun := NULL;
    END IF;

    RETURN newjobnextrun;
END;
/* End - BUG#Z4792 */

/* BEGIN - BUG#M0000412 */
CREATE OR REPLACE PROCEDURE sys.pgagent_setval() AUTHID DEFINER IS
  next_job_id INTEGER;
BEGIN
  LOOP
    SELECT nextval('pgagent.pga_job_jobid_seq') INTO next_job_id;

    IF NOT EXISTS (SELECT 1 FROM pgagent.pga_job WHERE jobid = next_job_id) THEN
        PERFORM setval('pgagent.pga_job_jobid_seq', next_job_id, FALSE);
        EXIT;
    END IF;
  END LOOP;
END;


/* sys.insert_pgagent_tables procedure is used in DBMS_JOB.SUBMIT procedure */
CREATE OR REPLACE PROCEDURE sys.insert_pgagent_tables(
  interval_expression_jobenabled BOOLEAN DEFAULT TRUE,
  result_datetime IN pg_catalog.timestamptz,
  no_parse IN BOOLEAN DEFAULT FALSE,
  instance IN INTEGER DEFAULT 0,
  force IN BOOLEAN DEFAULT FALSE,
  current_user_varchar VARCHAR2(128),
  interval_expression_failures INTEGER DEFAULT 0,
  nls_env_varchar VARCHAR2(300),
  what IN VARCHAR2,
  current_database_name VARCHAR2(128),
  interval IN VARCHAR2 DEFAULT 'null',
  next_date IN pg_catalog.timestamptz,
  dbmsjob_calendar_enable IN BOOLEAN,
  job_id OUT INTEGER
) AUTHID DEFINER IS
BEGIN
  INSERT INTO pgagent.pga_job (jobjclid, jobname, jobdesc, jobhostagent, jobenabled, jobnextrun, dbmsjob_flag, dbmsjob_no_parse, dbmsjob_instance, dbmsjob_force, log_user, priv_user, schema_user, failures, nls_env)
  VALUES (1, 'dbms_job', ''::text, ''::text, interval_expression_jobenabled, result_datetime::pg_catalog.timestamptz, true, no_parse, instance, force, current_user_varchar, current_user_varchar, current_user_varchar, 0, nls_env_varchar)
  RETURNING jobid INTO job_id;
  
  INSERT INTO pgagent.pga_jobstep (jstjobid, jstname, jstdesc, jstenabled, jstkind, jstcode, jstconnstr, jstdbname, jstonerror)
  VALUES (job_id, 'dbms_job_step', ''::text, true, 's', what, '', current_database_name, 'f');
  
  IF (next_date IS NULL) THEN
    INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, dbmsjob_interval, dbmsjob_calendar_enable)
      VALUES (job_id, 'job1', ''::text, interval, dbmsjob_calendar_enable);
  ELSE
    INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, jscstart, dbmsjob_interval, dbmsjob_calendar_enable)
      VALUES (job_id, 'job1', ''::text, next_date, interval, dbmsjob_calendar_enable);
  END IF;
END;

/* sys.insert_pgagent_tables2 procedure is used in DBMS_JOB.ISUBMIT procedure */
CREATE OR REPLACE PROCEDURE sys.insert_pgagent_tables2(
  interval_expression_jobenabled BOOLEAN DEFAULT TRUE,
  result_datetime IN pg_catalog.timestamptz,
  no_parse IN BOOLEAN DEFAULT FALSE,
  current_user_varchar VARCHAR2(128),
  interval_expression_failures INTEGER DEFAULT 0,
  nls_env_varchar VARCHAR2(300),
  what IN VARCHAR2,
  current_database_name VARCHAR2(128),
  interval IN VARCHAR2 DEFAULT 'null',
  next_date IN pg_catalog.timestamptz,
  dbmsjob_calendar_enable IN BOOLEAN,
  job_id IN INTEGER
) AUTHID DEFINER IS
BEGIN
  INSERT INTO pgagent.pga_job (jobid, jobjclid, jobname, jobdesc, jobhostagent, jobenabled, jobnextrun, dbmsjob_flag, dbmsjob_no_parse, log_user, priv_user, schema_user, failures, nls_env)
  VALUES (job_id, 1, 'dbms_job', ''::text, ''::text, interval_expression_jobenabled, result_datetime::pg_catalog.timestamptz, true, no_parse, current_user_varchar, current_user_varchar, current_user_varchar, 0, nls_env_varchar);
  
  INSERT INTO pgagent.pga_jobstep (jstjobid, jstname, jstdesc, jstenabled, jstkind, jstcode, jstconnstr, jstdbname, jstonerror)
  VALUES (job_id, 'dbms_job_step', ''::text, true, 's', what, '', current_database_name, 'f');
  
  IF (next_date IS NULL) THEN
    INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, dbmsjob_interval, dbmsjob_calendar_enable)
      VALUES (job_id, 'job1', ''::text, interval, dbmsjob_calendar_enable);
  ELSE
    INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, jscstart, dbmsjob_interval, dbmsjob_calendar_enable)
      VALUES (job_id, 'job1', ''::text, next_date, interval, dbmsjob_calendar_enable);
  END IF;
END;

CREATE OR REPLACE PROCEDURE sys.broken(
    job IN INTEGER,
    broken IN BOOLEAN,
    next_date IN DATE DEFAULT SYSDATE
) AUTHID DEFINER IS
    v_job_count NUMBER;
    v_job_broken BOOLEAN;
    new_job_broken BOOLEAN;
    now_future BOOLEAN := FALSE;
    v_jobdbmsjob_calendar_enable BOOLEAN;
    result_datetime pg_catalog.timestamptz;
    v_dbmsjob_interval text;
    fixed_sysdate TIMESTAMP;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';
    fixed_sysdate := sysdate;

    IF next_date >= fixed_sysdate THEN
    now_future := TRUE;
    END IF;

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;
      
    SELECT jobenabled INTO v_job_broken
    FROM pgagent.pga_job
    WHERE jobid = job;

    SELECT dbmsjob_calendar_enable, dbmsjob_interval 
    INTO v_jobdbmsjob_calendar_enable, v_dbmsjob_interval
    FROM pgagent.pga_schedule
    WHERE jscjobid = job;

    IF broken IS NULL THEN
        RAISE EXCEPTION 'Wrong number or type of argument in call BROKEN'; /* BUG#Z4577 */
    ELSE
        new_job_broken := broken;
    END IF;

    IF v_job_count > 0 THEN
        IF v_job_broken OR NOT new_job_broken THEN
            IF now_future THEN
                result_datetime := next_date::timestamp;
            ELSE
                IF v_jobdbmsjob_calendar_enable THEN
                    result_datetime := evaluate_calendar_string(v_dbmsjob_interval, next_date::timestamp, NULL, true);
                ELSE
                    result_datetime := sys.evaluate_next_schedule_expression(v_dbmsjob_interval);
                END IF;
            END IF;

            UPDATE pgagent.pga_job
            SET jobenabled = NOT new_job_broken
            WHERE jobid = job;
            
            UPDATE pgagent.pga_job
            SET jobnextrun = result_datetime
            WHERE jobid = job;
        END IF;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END broken;

CREATE OR REPLACE PROCEDURE sys.change (
    job IN INTEGER,
    what IN VARCHAR2,
    next_date IN DATE,
    interval IN VARCHAR2,
    instance IN INTEGER DEFAULT NULL,
    force IN BOOLEAN DEFAULT FALSE
) AUTHID DEFINER IS
    v_job_count NUMBER;

    result_datetime pg_catalog.timestamptz;
    has_keyword BOOLEAN;

    new_nextdate TIMESTAMP;
    fixed_sysdate TIMESTAMP;
    v_dbmsjob_interval TEXT;
    v_dbmsjob_calendar_enable BOOLEAN;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    fixed_sysdate := sysdate;

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count = 0 THEN
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;

    IF interval IS NOT NULL THEN
        has_keyword := interval::text ILIKE '%FREQ%' OR 
                       interval::text ILIKE '%INTERVAL%' OR
                       interval::text ILIKE '%BYMONTH%' OR
                       interval::text ILIKE '%BYMONTHDAY%' OR
                       interval::text ILIKE '%BYHOUR%' OR
                       interval::text ILIKE '%BYMINUTE%' OR
                       interval::text ILIKE '%BYSECOND%';

        UPDATE pgagent.pga_schedule
        SET dbmsjob_calendar_enable = has_keyword
        WHERE jscjobid = job;

        IF has_keyword THEN
            new_nextdate := evaluate_calendar_string(interval::text, fixed_sysdate, NULL, TRUE);
        ELSE
            BEGIN

                new_nextdate := sys.evaluate_next_schedule_expression(interval::text);
                    EXCEPTION WHEN others THEN
                        
                    RAISE EXCEPTION 'IVY-27465: invalid value % for attribute REPEAT_INTERVAL',interval;
                    RETURN;
            END;
            
            IF new_nextdate < fixed_sysdate THEN
                RAISE EXCEPTION 'IVY-27470: failed to re-enable "SYS"."DBMS_JOB$_% " after making requested change  IVY-12005: may not schedule automatic refresh for times in the past',job;
            END IF;
        END IF;
    END IF;

    IF interval IS NULL THEN
        SELECT dbmsjob_interval, dbmsjob_calendar_enable 
        INTO v_dbmsjob_interval, v_dbmsjob_calendar_enable
        FROM pgagent.pga_schedule
        WHERE jscjobid = job;

        IF v_dbmsjob_calendar_enable THEN
            new_nextdate := evaluate_calendar_string(v_dbmsjob_interval::text, current_timestamp, NULL, TRUE);
        ELSE
            new_nextdate := sys.evaluate_next_schedule_expression(v_dbmsjob_interval::text);
        END IF;
    END IF;

    IF what IS NOT NULL AND next_date IS NULL AND interval IS NULL THEN
        UPDATE pgagent.pga_jobstep
        SET jstcode = what
        WHERE jstjobid = job;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NOT NULL AND next_date IS NOT NULL AND interval IS NULL THEN
        UPDATE pgagent.pga_jobstep
        SET jstcode = what
        WHERE jstjobid = job;

        IF next_date >= fixed_sysdate THEN
            new_nextdate := next_date;
        END IF;
        
        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NOT NULL AND next_date IS NULL AND interval IS NOT NULL THEN
        UPDATE pgagent.pga_jobstep
        SET jstcode = what
        WHERE jstjobid = job;

        UPDATE pgagent.pga_schedule
        SET 
            dbmsjob_interval = interval,
            dbmsjob_calendar_enable = has_keyword
        WHERE jscjobid = job;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NOT NULL AND next_date IS NOT NULL AND interval IS NOT NULL THEN
        UPDATE pgagent.pga_jobstep
        SET jstcode = what
        WHERE jstjobid = job;

        UPDATE pgagent.pga_schedule
        SET 
            dbmsjob_interval = interval,
            dbmsjob_calendar_enable = has_keyword
        WHERE jscjobid = job;

        IF next_date > fixed_sysdate THEN
            new_nextdate := next_date;
        END IF;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NULL AND next_date IS NULL AND interval IS NOT NULL THEN
        UPDATE pgagent.pga_schedule
        SET 
            dbmsjob_interval = interval,
            dbmsjob_calendar_enable = has_keyword
        WHERE jscjobid = job;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NULL AND next_date IS NOT NULL AND interval IS NULL THEN
        IF next_date >= fixed_sysdate THEN
            new_nextdate := next_date;
        END IF;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSIF what IS NULL AND next_date IS NOT NULL AND interval IS NOT NULL THEN
        IF next_date <= fixed_sysdate THEN
        ELSE
            new_nextdate := next_date;
        END IF;

        UPDATE pgagent.pga_schedule
        SET 
            dbmsjob_interval = interval,
            dbmsjob_calendar_enable = has_keyword
        WHERE jscjobid = job;

        UPDATE pgagent.pga_job
        SET jobnextrun = new_nextdate::text::pg_catalog.timestamptz
        WHERE jobid = job;

    ELSE
        RETURN;
    END IF;
END change;

CREATE OR REPLACE PROCEDURE sys.instance (
    job IN INTEGER,
    instance IN INTEGER,
    force IN BOOLEAN DEFAULT FALSE
) AUTHID DEFINER IS
    v_job_count NUMBER;
BEGIN
    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN

        IF instance IS NOT NULL THEN
            UPDATE pgagent.pga_job
            SET dbmsjob_instance = instance
            WHERE jobid = job;
        END IF;

        IF force IS NOT NULL THEN
            UPDATE pgagent.pga_job
            SET dbmsjob_force = force
            WHERE jobid = job;
        END IF;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END instance;

CREATE OR REPLACE PROCEDURE sys.interval(
    job IN INTEGER,
    interval IN VARCHAR2
) AUTHID DEFINER IS
    v_job_count NUMBER;
    v_next_date pg_catalog.timestamptz;
    has_keyword BOOLEAN;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN
        IF interval IS NOT NULL THEN

            has_keyword := interval::text ILIKE '%FREQ%' OR 
                        interval::text ILIKE '%INTERVAL%' OR
                        interval::text ILIKE '%ByMONTH%' OR
                        interval::text ILIKE '%BYMONTHDAY%' OR
                        interval::text ILIKE '%BYHOUR%' OR
                        interval::text ILIKE '%BYMINUTE%' OR
                        interval::text ILIKE '%BYSECOND%';


            IF has_keyword THEN
				/* Begin - BUG#Z4579 */
				/* Oracle calendar syntax jobs use the last execution time as the start date */
				SELECT joblastrun INTO v_next_date FROM pgagent.pga_job WHERE jobid = job;
				IF v_next_date IS NULL THEN
					v_next_date = now();
				END IF;
				/* End - BUG#Z4579 */

                v_next_date := evaluate_calendar_string(interval::text, v_next_date::pg_catalog.timestamp, NULL, false);

                UPDATE pgagent.pga_schedule
                SET 
                    dbmsjob_interval = interval,
                    dbmsjob_calendar_enable = true
                WHERE
                    jscjobid = job;

                UPDATE pgagent.pga_job
                SET jobnextrun = v_next_date::pg_catalog.timestamptz
                WHERE jobid = job;

            ELSE

                BEGIN
                    v_next_date := sys.evaluate_next_schedule_expression(interval::text);
                EXCEPTION
                    WHEN others THEN                      
                    RAISE EXCEPTION 'IVY-27465: invalid value %s for attribute REPEAT_INTERVAL',interval;
                END;

                    UPDATE pgagent.pga_schedule
                    SET dbmsjob_interval = interval,
                        dbmsjob_calendar_enable = false
                    WHERE jscjobid = job;

                    UPDATE pgagent.pga_job
                    SET jobnextrun = v_next_date::pg_catalog.timestamptz
                    WHERE jobid = job;
            END IF;
        ELSE
            Call DBMS_JOB.REMOVE(job);
        END IF;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END interval;

CREATE OR REPLACE PROCEDURE sys.next_date (
    job IN INTEGER,
    next_date IN DATE
) AUTHID DEFINER IS
    v_job_count NUMBER;
    v_next_date pg_catalog.timestamptz;
    v_dbmsjob_interval text;
    v_dbmsjob_calendar_enable boolean;
    fixed_sysdate TIMESTAMP;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';
    fixed_sysdate := sysdate;

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;
    
    IF v_job_count > 0 THEN
        IF next_date IS NOT NULL THEN
            IF next_date >= fixed_sysdate THEN
                v_next_date := next_date;
            ELSE
                SELECT dbmsjob_interval, dbmsjob_calendar_enable 
                INTO v_dbmsjob_interval, v_dbmsjob_calendar_enable
                FROM pgagent.pga_schedule
                WHERE jscjobid = job;

                SELECT jobnextrun 
                INTO v_next_date
                FROM pgagent.pga_job
                WHERE jobid = job;

                IF v_dbmsjob_calendar_enable THEN
                    v_next_date := evaluate_calendar_string(v_dbmsjob_interval::text, v_next_date::pg_catalog.timestamp, NULL, false);
                ELSE
                    v_next_date := sys.evaluate_next_schedule_expression(v_dbmsjob_interval::text);
                END IF;
            END IF;   
        ELSE
            v_next_date := fixed_sysdate;
        END IF;

        UPDATE pgagent.pga_job
        SET jobnextrun = v_next_date::text::pg_catalog.timestamptz
        WHERE jobid = job;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;    
END next_date;

CREATE OR REPLACE PROCEDURE sys.remove(job IN INTEGER) AUTHID DEFINER IS
    v_job_count INTEGER;
BEGIN
    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN

        DELETE FROM pgagent.pga_job WHERE jobid = job;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;

END remove;

CREATE OR REPLACE PROCEDURE sys.run (
    job IN INTEGER,
    force IN BOOLEAN DEFAULT FALSE
) AUTHID DEFINER IS
    v_job_count NUMBER;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN

        UPDATE pgagent.pga_job
        SET jobnextrun = SYSTIMESTAMP
        WHERE jobid = job;
		UPDATE pgagent.pga_job SET jobenabled = true WHERE jobid = job; /* BUG#M0000528 */
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END run;

CREATE OR REPLACE PROCEDURE sys.what (
    job IN INTEGER,
    what IN VARCHAR2
) AUTHID DEFINER IS
    v_job_count NUMBER;
    v_next_date  pg_catalog.timestamptz;
    v_dbmsjob_interval TEXT;
    v_dbmsjob_calendar_enable BOOLEAN;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    IF what IS NULL THEN
        RAISE EXCEPTION 'IVY-27451: JOB_ACTION cannot be NULL';
    END IF;

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN
        IF what IS NOT NULL THEN

            SELECT dbmsjob_interval, dbmsjob_calendar_enable 
            INTO v_dbmsjob_interval, v_dbmsjob_calendar_enable
            FROM pgagent.pga_schedule
            WHERE jscjobid = job;

            SELECT jobnextrun 
            INTO v_next_date
            FROM pgagent.pga_job
            WHERE jobid = job;

            IF v_dbmsjob_calendar_enable THEN
                v_next_date := evaluate_calendar_string(v_dbmsjob_interval::text, v_next_date::pg_catalog.timestamp, NULL, false);
            ELSE
                v_next_date := sys.evaluate_next_schedule_expression(v_dbmsjob_interval::text);
            END IF;
            
            UPDATE pgagent.pga_jobstep
            SET jstcode = what
            WHERE jstjobid = job;

            UPDATE pgagent.pga_job
            SET jobnextrun = v_next_date::text::pg_catalog.timestamptz
            WHERE jobid = job;
        END IF;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END what;

CREATE OR REPLACE PROCEDURE sys.user_export (
    job IN INTEGER,
    mycall IN OUT VARCHAR2
) AUTHID DEFINER IS
    v_what VARCHAR2(4000);
    v_next_date VARCHAR2(100);
    v_interval VARCHAR2(100);
    v_no_parse VARCHAR2(10);
    v_job_count NUMBER;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN

        SELECT jstcode INTO v_what
        FROM pgagent.pga_jobstep
        WHERE jstjobid = job;

        SELECT jobnextrun::pg_catalog.timestamp INTO v_next_date
        FROM pgagent.pga_job
        WHERE jobid = job;

        v_next_date := TO_CHAR(v_next_date::pg_catalog.timestamp, 'YYYY-MM-DD HH24:MI:SS');
        SELECT dbmsjob_interval INTO v_interval
        FROM pgagent.pga_schedule
        WHERE jscjobid = job;

        mycall := 'dbms_job.isubmit(job=>' || job || ' ,what=>''' || v_what || ''', next_date=>' || 'to_date(''' || v_next_date || ''' , ''YYYY-MM-DD HH24:MI:SS'')' || ', interval=>''' || v_interval || ''', no_parse=>TRUE);';
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END user_export;

CREATE OR REPLACE PROCEDURE sys.user_export (
    job IN INTEGER,
    mycall IN OUT VARCHAR2,
    myinst IN OUT VARCHAR2
) AUTHID DEFINER IS
    v_what VARCHAR2(4000);
    v_next_date VARCHAR2(100);
    v_interval VARCHAR2(100);
    v_no_parse VARCHAR2(10);
    v_instance NUMBER;
    v_job_count NUMBER;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';

    SELECT COUNT(*)
    INTO v_job_count
    FROM pgagent.pga_job
    WHERE jobid = job;

    IF v_job_count > 0 THEN

        SELECT jstcode INTO v_what
        FROM pgagent.pga_jobstep
        WHERE jstjobid = job;

        SELECT jobnextrun::pg_catalog.timestamp INTO v_next_date
        FROM pgagent.pga_job
        WHERE jobid = job;

        v_next_date := TO_CHAR(v_next_date::pg_catalog.timestamp, 'YYYY-MM-DD HH24:MI:SS');

        SELECT dbmsjob_interval INTO v_interval
        FROM pgagent.pga_schedule
        WHERE jscjobid = job;

        SELECT dbmsjob_no_parse INTO v_no_parse
        FROM pgagent.pga_job
        WHERE jobid = job;

        mycall := 'dbms_job.isubmit(job=>' || job || ',what=>''' || v_what || ''', next_date=>' || 'to_date(''' || v_next_date || ''' , ''YYYY-MM-DD HH24:MI:SS'')' || ', interval=>''' || v_interval || ''', no_parse=>TRUE);';
        SELECT dbmsjob_instance INTO v_instance
        FROM pgagent.pga_job
        WHERE jobid = job;

        IF v_instance = 0 THEN
            myinst := NULL;
        ELSE
            myinst := 'dbms_job.instance(job=>' || job || ',instance=>' || v_instance || ', force=>TRUE);';
        END IF;
    ELSE
        RAISE EXCEPTION 'IVY-23421:job number % is not a job in the job queue', job;
    END IF;
END user_export;
/* END - BUG#M0000412 */

/* create dbms_job package specification */
CREATE OR REPLACE NONEDITIONABLE PACKAGE sys.dbms_job AUTHID CURRENT_USER IS


  PROCEDURE isubmit    ( job       IN  INTEGER,
                         what      IN  VARCHAR2,
                         next_date IN  DATE,
                         interval  IN  VARCHAR2 DEFAULT 'null',
                         no_parse  IN  BOOLEAN DEFAULT FALSE);


  PROCEDURE submit    ( job       OUT INTEGER,
                        what      IN  VARCHAR2,
                        next_date IN  DATE DEFAULT sysdate,
                        interval  IN  VARCHAR2 DEFAULT 'null',
                        no_parse  IN  BOOLEAN DEFAULT FALSE,
                        instance  IN  INTEGER DEFAULT 0,
                        force     IN  BOOLEAN DEFAULT FALSE );


  PROCEDURE remove    ( job       IN  INTEGER );


  PROCEDURE change    ( job       IN  INTEGER,
                        what      IN  VARCHAR2,
                        next_date IN  DATE,
                        interval  IN  VARCHAR2,
                        instance  IN  INTEGER DEFAULT NULL,
                        force     IN  BOOLEAN DEFAULT FALSE);


  PROCEDURE what      ( job       IN  INTEGER,
                        what      IN  VARCHAR2 );


  PROCEDURE next_date ( job       IN  INTEGER,
                        next_date IN  DATE     );


  PROCEDURE instance ( job        IN INTEGER,
                       instance   IN INTEGER,
		                   force      IN BOOLEAN DEFAULT FALSE);


  PROCEDURE interval  ( job       IN  INTEGER,
                        interval  IN  VARCHAR2 );


  PROCEDURE broken    ( job       IN  INTEGER,
                        broken    IN  BOOLEAN,
                        next_date IN  DATE DEFAULT SYSDATE );


  PROCEDURE run       ( job       IN  INTEGER,
			                  force     IN  BOOLEAN DEFAULT FALSE);


  PROCEDURE user_export ( job    IN     INTEGER,
                          mycall IN OUT VARCHAR2);


  PROCEDURE user_export ( job     IN     INTEGER,
                         mycall   IN OUT VARCHAR2,
                         myinst   IN OUT VARCHAR2);

END;


/* create dbms_job package body */
CREATE OR REPLACE PACKAGE BODY sys.dbms_job IS
     
PROCEDURE SUBMIT (
    job OUT INTEGER,
    what IN VARCHAR2,
    next_date IN DATE DEFAULT sysdate,
    interval IN VARCHAR2 DEFAULT 'null',
    no_parse IN BOOLEAN DEFAULT FALSE,
    instance IN INTEGER DEFAULT 0,
    force IN BOOLEAN DEFAULT FALSE
) IS
    job_id      INTEGER;
    current_database_name VARCHAR2(128);
    current_user_varchar VARCHAR2(128);
    client_encoding_varchar VARCHAR2(100);
    lc_collate_varchar VARCHAR2(100);
    datestyle_varchar VARCHAR2(100);
    nls_env_varchar VARCHAR2(300);
    ivorysql_enable_emptystring_to_null_varchar text;
    current_job_id INTEGER;
    next_job_id INTEGER;
    temp_next_job_id INTEGER;

    interval_is_null BOOLEAN := FALSE;
    result_datetime pg_catalog.timestamptz;
    has_keyword BOOLEAN := FALSE;
    now_future BOOLEAN := FALSE;
    interval_expression_jobenabled BOOLEAN := TRUE;
    interval_expression_failures INTEGER := 0;
    fixed_sysdate TIMESTAMP;
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';
    
    fixed_sysdate := sysdate;

    IF next_date >= fixed_sysdate THEN
        now_future := TRUE;
        result_datetime := next_date;
    END IF;

    IF what IS NULL THEN
        RAISE EXCEPTION 'IVY-27451: JOB_ACTION cannot be NULL';
    END IF;

    IF interval IS NULL THEN
        IF next_date < fixed_sysdate THEN
            result_datetime := fixed_sysdate;
            interval_is_null := TRUE;
        END IF;    
    END IF;    

    SELECT current_user INTO current_user_varchar;
    SELECT current_database() INTO current_database_name;
    SELECT current_setting('client_encoding') INTO client_encoding_varchar;
    SELECT current_setting('lc_collate') INTO lc_collate_varchar;
    SELECT current_setting('datestyle') INTO datestyle_varchar;
    SELECT current_setting('ivorysql.enable_emptystring_to_null') INTO ivorysql_enable_emptystring_to_null_varchar;
    
    nls_env_varchar := 'client_encoding: ' || current_setting('client_encoding') || ', lc_collate: ' || current_setting('lc_collate') || ', datestyle: ' || current_setting('datestyle');

    set ivorysql.enable_emptystring_to_null=off;

	/* BEGIN - BUG#M0000412 */
	/*
	 * call sys.pgagent_setval procedure to operator the sequence in schema pgagent.
	 * because sys.pgagent_setval is runed with the Owner rights (DEFINER), if a user
	 * call the package dbms_job to create a job, the user can't operator the objects
	 * in schema pgagent.
	 */
    sys.pgagent_setval();
    /* END - BUG#M0000412 */

    has_keyword := interval::text ILIKE '%FREQ%' OR 
                   interval::text ILIKE '%INTERVAL%' OR
                   interval::text ILIKE '%ByMONTH%' OR
                   interval::text ILIKE '%BYMONTHDAY%' OR
                   interval::text ILIKE '%BYHOUR%' OR
                   interval::text ILIKE '%BYMINUTE%' OR
                   interval::text ILIKE '%BYSECOND%';

    IF has_keyword THEN

        IF NOT interval_is_null THEN
            IF now_future THEN
                PERFORM evaluate_calendar_string(interval::text, next_date::timestamp, NULL, true);
            ELSE
                result_datetime := evaluate_calendar_string(interval::text, next_date::timestamp, NULL, true);
            END IF;
        END IF;

	    /* BEGIN - BUG#M0000412 */
	    /*
	     * call sys.insert_pgagent_tables procedure to operator the tables in schema pgagent.
	     * because sys.insert_pgagent_tables is runed with the Owner rights (DEFINER), if a user
	     * call the package dbms_job to create a job, the user can't operator the objects
	     * in schema pgagent.
	     */
        sys.insert_pgagent_tables(true,
		                         result_datetime::pg_catalog.timestamptz,
		                         no_parse,
		                         instance,
		                         force,
		                         current_user_varchar,
		                         0,
		                         nls_env_varchar,
								 what,
								 current_database_name,
								 interval,
								 NULL,
								 true,
								 job_id);
        /* END - BUG#M0000412 */
    ELSE
        BEGIN
            IF NOT interval_is_null THEN
                result_datetime := sys.evaluate_next_schedule_expression(interval::text);
            END IF;
        EXCEPTION WHEN others THEN     
            RAISE EXCEPTION 'IVY-27465: invalid value % for attribute REPEAT_INTERVAL',interval;
            RETURN;
        END;

        IF NOT interval_is_null THEN
            IF result_datetime <= fixed_sysdate THEN
                interval_expression_jobenabled := false;
                interval_expression_failures := 1;
            END IF;

            IF now_future THEN
                result_datetime := next_date::timestamp;
            END IF;            
        END IF;

	    /* BEGIN - BUG#M0000412 */
	    /*
	     * call sys.insert_pgagent_tables procedure to operator the tables in schema pgagent.
	     * because sys.insert_pgagent_tables is runed with the Owner rights (DEFINER), if a user
	     * call the package dbms_job to create a job, the user can't operator the objects
	     * in schema pgagent.
	     */
        sys.insert_pgagent_tables(interval_expression_jobenabled,
		                         result_datetime::pg_catalog.timestamptz,
		                         no_parse,
		                         instance,
		                         force,
		                         current_user_varchar,
		                         interval_expression_failures,
		                         nls_env_varchar,
								 what,
								 current_database_name,
								 interval,
								 next_date::text::pg_catalog.timestamptz,
								 false,
								 job_id);
        /* END - BUG#M0000412 */
    END IF;


    EXECUTE 'SET ivorysql.enable_emptystring_to_null = ' || quote_literal(ivorysql_enable_emptystring_to_null_varchar);

    JOB := job_id;
        
END SUBMIT;




PROCEDURE broken(
    job IN INTEGER,
    broken IN BOOLEAN,
    next_date IN DATE DEFAULT SYSDATE
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.broken(job,
             broken,
			 next_date);
  /* END - BUG#M0000412 */
END broken;


PROCEDURE change (
    job IN INTEGER,
    what IN VARCHAR2,
    next_date IN DATE,
    interval IN VARCHAR2,
    instance IN INTEGER DEFAULT NULL,
    force IN BOOLEAN DEFAULT FALSE
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.change(job,
             what,
			 next_date,
			 interval,
			 instance,
			 force);
  /* END - BUG#M0000412 */
END change;


PROCEDURE instance (
    job IN INTEGER,
    instance IN INTEGER,
    force IN BOOLEAN DEFAULT FALSE
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.instance(job,
			   instance,
			   force);
  /* END - BUG#M0000412 */
END instance;



PROCEDURE interval(
    job IN INTEGER,
    interval IN VARCHAR2
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.interval(job,
			   interval);
  /* END - BUG#M0000412 */
END interval;



PROCEDURE next_date (
    job IN INTEGER,
    next_date IN DATE
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.next_date(job,
			    next_date);
  /* END - BUG#M0000412 */
END next_date;



PROCEDURE remove(job IN INTEGER) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.remove(job);
  /* END - BUG#M0000412 */
END remove;



PROCEDURE run (
    job IN INTEGER,
    force IN BOOLEAN DEFAULT FALSE
) IS
    v_job_count NUMBER;
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.run(job,
		  force);
  /* END - BUG#M0000412 */
END run;



PROCEDURE what (
    job IN INTEGER,
    what IN VARCHAR2
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.what(job,
           what);
  /* END - BUG#M0000412 */
END what;




PROCEDURE isubmit(
    job IN INTEGER,
    what IN VARCHAR2,
    next_date IN DATE,
    interval IN VARCHAR2 DEFAULT 'null',
    no_parse IN BOOLEAN DEFAULT FALSE
) IS
    
    current_database_name VARCHAR2(128);
    current_user_varchar VARCHAR2(128);
    client_encoding_varchar VARCHAR2(100);
    lc_collate_varchar VARCHAR2(100);
    datestyle_varchar VARCHAR2(100);
    nls_env_varchar VARCHAR2(300);
    
    ivorysql_enable_emptystring_to_null_varchar text;
    next_date_to_text text;

    result_datetime pg_catalog.timestamptz;
    has_keyword BOOLEAN;
    now_future BOOLEAN := FALSE;
    interval_expression_jobenabled BOOLEAN := TRUE;
    interval_expression_failures INTEGER := 0;
    fixed_sysdate TIMESTAMP;    
BEGIN
    set nls_date_format='YYYY-MM-DD HH24:MI:SS';
    fixed_sysdate := sysdate;

    IF next_date >= fixed_sysdate THEN
        now_future := TRUE;
    END IF;

    SELECT current_user INTO current_user_varchar;
    SELECT current_database() INTO current_database_name;
    SELECT current_setting('client_encoding') INTO client_encoding_varchar;
    SELECT current_setting('lc_collate') INTO lc_collate_varchar;
    SELECT current_setting('datestyle') INTO datestyle_varchar;

    nls_env_varchar := 'client_encoding: ' || current_setting('client_encoding') || ', lc_collate: ' || current_setting('lc_collate') || ', datestyle: ' || current_setting('datestyle');


    SELECT current_setting('ivorysql.enable_emptystring_to_null') INTO ivorysql_enable_emptystring_to_null_varchar;
        
    SELECT TO_CHAR(next_date, 'YYYY-MM-DD HH24:MI:SS') INTO next_date_to_text;

    set ivorysql.enable_emptystring_to_null=off;

    has_keyword := interval::text ILIKE '%FREQ%' OR 
                   interval::text ILIKE '%INTERVAL%' OR
                   interval::text ILIKE '%ByMONTH%' OR
                   interval::text ILIKE '%BYMONTHDAY%' OR
                   interval::text ILIKE '%BYHOUR%' OR
                   interval::text ILIKE '%BYMINUTE%' OR
                   interval::text ILIKE '%BYSECOND%';

    IF has_keyword THEN
        IF now_future THEN
            result_datetime := evaluate_calendar_string(interval::text, next_date_to_text::timestamp, next_date_to_text::timestamp, true);
        ELSE
            result_datetime := evaluate_calendar_string(interval::text, next_date_to_text::timestamp, NULL, true);
        END IF;    

	    /* BEGIN - BUG#M0000412 */
	    /*
	     * call sys.insert_pgagent_tables2 procedure to operator the tables in schema pgagent.
	     * because sys.insert_pgagent_tables is runed with the Owner rights (DEFINER), if a user
	     * call the package dbms_job to create a job, the user can't operator the objects
	     * in schema pgagent.
	     */
        sys.insert_pgagent_tables2(true,
		                         result_datetime::pg_catalog.timestamptz,
		                         no_parse,
		                         current_user_varchar,
		                         0,
		                         nls_env_varchar,
								 what,
								 current_database_name,
								 interval,
								 NULL,
								 true,
								 job);
        /* END - BUG#M0000412 */
    ELSE

        BEGIN
            result_datetime := sys.evaluate_next_schedule_expression(interval::text);
        EXCEPTION
            WHEN others THEN
                
            RAISE EXCEPTION 'IVY-27465: invalid value %s for attribute REPEAT_INTERVAL',interval;
        END;

        IF result_datetime <= fixed_sysdate THEN
            interval_expression_jobenabled := false;
            interval_expression_failures := 1;
        END IF;

        IF now_future OR ((interval IS NULL OR interval ILIKE 'null') AND next_date < fixed_sysdate /* BUG#Z4760 */) THEN
            result_datetime  := next_date::timestamp;
        END IF;

	    /* BEGIN - BUG#M0000412 */
	    /*
	     * call sys.insert_pgagent_tables2 procedure to operator the tables in schema pgagent.
	     * because sys.insert_pgagent_tables is runed with the Owner rights (DEFINER), if a user
	     * call the package dbms_job to create a job, the user can't operator the objects
	     * in schema pgagent.
	     */
        sys.insert_pgagent_tables2(interval_expression_jobenabled,
		                         result_datetime::pg_catalog.timestamptz,
		                         no_parse,
		                         current_user_varchar,
		                         interval_expression_failures,
		                         nls_env_varchar,
								 what,
								 current_database_name,
								 interval,
								 next_date::text::pg_catalog.timestamptz,
								 false,
								 job);
        /* END - BUG#M0000412 */
    END IF;

    EXECUTE 'SET ivorysql.enable_emptystring_to_null = ' || quote_literal(ivorysql_enable_emptystring_to_null_varchar);
    
END isubmit;



PROCEDURE user_export (
    job IN INTEGER,
    mycall IN OUT VARCHAR2
) IS
    v_what VARCHAR2(4000);
    v_next_date VARCHAR2(100);
    v_interval VARCHAR2(100);
    v_no_parse VARCHAR2(10);
    v_job_count NUMBER;
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.user_export(job,
                  mycall);
  /* END - BUG#M0000412 */
END user_export;



PROCEDURE user_export (
    job IN INTEGER,
    mycall IN OUT VARCHAR2,
    myinst IN OUT VARCHAR2
) IS
BEGIN
  /* BEGIN - BUG#M0000412 */
  sys.user_export(job,
                  mycall,
		          myinst);
  /* END - BUG#M0000412 */
END user_export;

END;



/* CREATE VIEW */
CREATE VIEW SYS.DBA_JOBS AS
SELECT
    j.jobid AS JOB,
    j.LOG_USER,
    j.PRIV_USER,
    j.SCHEMA_USER,
    j.joblastrun AS LAST_DATE,
    SUBSTR(TO_CHAR(j.joblastrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS LAST_SEC,
    (SELECT MAX(jl.jlgstart)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_DATE,
    (SELECT SUBSTR(TO_CHAR(MAX(jl.jlgstart), 'YYYY-MM-DD HH24:MI:SS'),12,8)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_SEC,
    j.jobnextrun AS NEXT_DATE,
    SUBSTR(TO_CHAR(j.jobnextrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS NEXT_SEC,
    (SELECT SUM(jl.jlgduration)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid) AS TOTAL_TIME,
    CASE WHEN j.jobenabled = 't' THEN 'N' ELSE 'Y' END AS BROKEN, /* BUG#Z4563 */
    s.dbmsjob_interval AS INTERVAL,
    j.failures AS FAILURES,
    js.jstcode AS WHAT,
    j.NLS_ENV,
    j.MISC_ENV,
    j.dbmsjob_INSTANCE AS INSTANCE
FROM
    pgagent.pga_job j
JOIN
    pgagent.pga_schedule s ON s.jscjobid = j.jobid
JOIN
    pgagent.pga_jobstep js ON js.jstjobid = j.jobid;

/* GRANT SELECT PRIVILEGE TO PUBLIC */
GRANT SELECT ON SYS.DBA_JOBS TO PUBLIC;


/* CREATE VIEW */
CREATE VIEW SYS.DBA_JOBS_RUNNING AS
SELECT
    j.jobagentid AS SID,
    j.jobid AS JOB,
    j.failures AS FAILURES,
    j.joblastrun AS LAST_DATE,
    TO_CHAR(j.joblastrun, 'HH24:MI') AS LAST_SEC,
    jl.jlgstart AS THIS_DATE,
    TO_CHAR(jl.jlgstart, 'HH24:MI') AS THIS_SEC,
    j.dbmsjob_instance AS INSTANCE
FROM
    pgagent.pga_job j
JOIN
    pgagent.pga_joblog jl ON j.jobid = jl.jlgjobid
WHERE
    jl.jlgstatus = 'r';

/* GRANT SELECT PRIVILEGE TO PUBLIC */
GRANT SELECT ON SYS.DBA_JOBS_RUNNING TO PUBLIC;


/* CREATE VIEW */
CREATE VIEW SYS.USER_JOBS AS
SELECT
    j.jobid AS JOB,
    j.LOG_USER,
    j.PRIV_USER,
    j.SCHEMA_USER,
    j.joblastrun AS LAST_DATE,
    SUBSTR(TO_CHAR(j.joblastrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS LAST_SEC,
    (SELECT MAX(jl.jlgstart)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_DATE,
    (SELECT SUBSTR(TO_CHAR(MAX(jl.jlgstart), 'YYYY-MM-DD HH24:MI:SS'),12,8)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_SEC,
    j.jobnextrun AS NEXT_DATE,
    SUBSTR(TO_CHAR(j.jobnextrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS NEXT_SEC,
    (SELECT SUM(jl.jlgduration)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid) AS TOTAL_TIME,
    CASE WHEN j.jobenabled = 't' THEN 'N' ELSE 'Y' END AS BROKEN, /* BUG#Z4563 */
    s.dbmsjob_interval AS INTERVAL,
    j.failures AS FAILURES,
    js.jstcode AS WHAT,
    j.NLS_ENV,
    j.MISC_ENV,
    j.dbmsjob_INSTANCE AS INSTANCE
FROM
    pgagent.pga_job j
JOIN
    pgagent.pga_schedule s ON s.jscjobid = j.jobid
JOIN
    pgagent.pga_jobstep js ON js.jstjobid = j.jobid
WHERE
    j.LOG_USER = current_user;


/* GRANT SELECT PRIVILEGE TO PUBLIC */
GRANT SELECT ON SYS.USER_JOBS TO PUBLIC;

/* CREATE VIEW */
CREATE VIEW SYS.ALL_JOBS AS
SELECT
    j.jobid AS JOB,
    j.LOG_USER,
    j.PRIV_USER,
    j.SCHEMA_USER,
    j.joblastrun AS LAST_DATE,
    SUBSTR(TO_CHAR(j.joblastrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS LAST_SEC,
    (SELECT MAX(jl.jlgstart)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_DATE,
    (SELECT SUBSTR(TO_CHAR(MAX(jl.jlgstart), 'YYYY-MM-DD HH24:MI:SS'),12,8)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid AND jl.jlgstatus = 'r') AS THIS_SEC,
    j.jobnextrun AS NEXT_DATE,
    SUBSTR(TO_CHAR(j.jobnextrun, 'YYYY-MM-DD HH24:MI:SS'),12,8) AS NEXT_SEC,
    (SELECT SUM(jl.jlgduration)
     FROM pgagent.pga_joblog jl
     WHERE jl.jlgjobid = j.jobid) AS TOTAL_TIME,
    CASE WHEN j.jobenabled = 't' THEN 'N' ELSE 'Y' END AS BROKEN, /* BUG#Z4563 */
    s.dbmsjob_interval AS INTERVAL,
    j.failures AS FAILURES,
    js.jstcode AS WHAT,
    j.NLS_ENV,
    j.MISC_ENV,
    j.dbmsjob_INSTANCE AS INSTANCE
FROM
    pgagent.pga_job j
JOIN
    pgagent.pga_schedule s ON s.jscjobid = j.jobid
JOIN
    pgagent.pga_jobstep js ON js.jstjobid = j.jobid
WHERE
    j.LOG_USER = current_user;

/* GRANT SELECT PRIVILEGE TO PUBLIC */
GRANT SELECT ON SYS.ALL_JOBS TO PUBLIC;
/* End - ReqID:SRS-PLSQL-DBMS_JOB */
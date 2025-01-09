/* Begin - ReqID:SRS-PLSQL-DBMS_METADATA-GET_DDL */
/* contrib/ivorysql_ora/src/builtin_packages/dbms_metadata/dbms_metadata--1.0.sql */
CREATE OR REPLACE PACKAGE SYS.DBMS_METADATA  AUTHID CURRENT_USER AS
  FUNCTION GET_OBJNAME(ori_name IN VARCHAR(63)) RETURN VARCHAR(63);
  FUNCTION CHECK_PRIVILEGE(owneroid IN OID) RETURN BOOLEAN;
  FUNCTION is_supper_user(username IN VARCHAR2(63)) RETURN BOOLEAN;
  FUNCTION GET_VIEW_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_FUNC_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_PROCEDURE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_TRIGGER_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT;
  FUNCTION GET_TABLESPACE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_INDEX_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT;
  FUNCTION GET_CONSTRAINT_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT;
  FUNCTION GET_REF_CONSTRAINT_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT;
  PROCEDURE GET_ATTR(TABOID IN OID, ATTRCNT IN INT, BUF OUT VARCHAR2(8000));
  FUNCTION GET_TABLE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), PARENTOBJNAME VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_PACKAGE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_PACKAGE_BODY_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_DBLINK_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_SEQUENCE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT;
  FUNCTION GET_DDL(OBJTYPE VARCHAR2(63), OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63) DEFAULT NULL, MODEL VARCHAR2(63) DEFAULT NULL, VERSION VARCHAR2(63) DEFAULT NULL, TRANSFORM VARCHAR2(63) DEFAULT NULL) RETURN CLOB;
END;


CREATE OR REPLACE PACKAGE BODY SYS.DBMS_METADATA  IS
 --get the output object name with double quotation marks
  FUNCTION GET_OBJNAME(ori_name IN VARCHAR(63)) RETURN VARCHAR(63)
  AS
  DECLARE
    resname    VARCHAR(63);
	uppername	 VARCHAR(63);
	lowername    VARCHAR(63);
  BEGIN
    resname := ori_name;
 	uppername = UPPER(ori_name);
	lowername = LOWER(ori_name);
	IF ori_name = uppername THEN
	   resname =  lowername;
	ELSE
	  IF ori_name = lowername THEN
	     resname =  uppername;
	  END IF;
	END IF;
	return resname;
  END;

  --function CHECK_PRIVILEGE
  FUNCTION CHECK_PRIVILEGE(owneroid IN OID) RETURN BOOLEAN
  AS
  DECLARE
    res BOOLEAN := FALSE;
  BEGIN
    IF(owneroid = 0) THEN
      RETURN FALSE;
    END IF;

	SELECT TRUE INTO res
	  FROM pg_roles u, pg_database d
	  WHERE (u.oid = owneroid)
	  AND (u.rolname = current_user OR u.rolsuper IS TRUE)
	  AND (d.datname = current_database);

	RETURN res;
  END;

  FUNCTION is_supper_user(username IN VARCHAR2(63)) RETURN BOOLEAN
  AS
  DECLARE
    user_super BOOLEAN := FALSE;
    CURSOR mycur1 FOR
    SELECT u.rolsuper
    FROM pg_roles u where u.rolname = username;
  BEGIN
    OPEN mycur1;
    FETCH mycur1 INTO user_super;
    IF mycur1%NOTFOUND THEN
      CLOSE mycur1;
      RAISE EXCEPTION 'USER DOES NOT EXIST!';
	END IF;

	CLOSE mycur1;
	RETURN user_super;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur1%ISOPEN THEN
	CLOSE mycur1;
      END IF;
      RAISE;
  END;

  --function GET_VIEW_DDL
  FUNCTION GET_VIEW_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT cl.oid, cl.relowner, cl.relkind
    FROM pg_class cl, pg_namespace nsp
    WHERE (cl.relkind = 'v' OR cl.relkind = 'm')
    AND cl.relname = OBJNAME
    AND nsp.nspname = SCHEMA
    AND cl.relnamespace = nsp.oid;

    CURSOR mycur2 FOR
    SELECT f.source
	FROM pg_class cl, pg_force_view f
	WHERE cl.oid = f.fvoid;

    objtoid      OID;
    viewddl      TEXT;
    tmpddl       TEXT;
	relkind      TEXT;

    obj_owneroid OID;
	source       TEXT;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid, relkind;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;

    IF SCHEMA LIKE 'PG_TEMP_%' THEN
      viewddl := 'CREATE OR REPLACE TEMPORARY VIEW '||'"'||GET_OBJNAME(OBJNAME)||'"'||' AS ';
    ELSEIF relkind = 'v' THEN
	  OPEN mycur2;
	  FETCH mycur2 INTO source;
	  IF mycur2%NOTFOUND THEN
        viewddl := 'CREATE OR REPLACE VIEW '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(OBJNAME)||'"'||' AS ';
	  END IF;
	  CLOSE mycur2;
    ELSEIF relkind = 'm' THEN
      viewddl := 'CREATE MATERIALIZED VIEW '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(OBJNAME)||'"'||' AS ';
    END IF;

    --get view context
    SET quote_all_identifiers = true;
    SELECT pg_get_viewdef(objtoid, true) INTO tmpddl FROM dual;
	RESET quote_all_identifiers;
    viewddl := viewddl || tmpddl;

    CLOSE mycur;
    RETURN viewddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      IF mycur2%ISOPEN THEN
	    CLOSE mycur2;
      END IF;
      RAISE;
  END;

 --function GET_FUNC_DDL
  FUNCTION GET_FUNC_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT proc.oid, proc.proowner
    FROM pg_proc proc, pg_namespace nsp
    WHERE nsp.oid = proc.pronamespace
    AND nsp.nspname = SCHEMA
    AND proc.proname = OBJNAME
	AND proc.prokind = 'f';

    objtoid         OID;
    parameter       TEXT;
    paratype        OID;
    paratypename    TEXT;
    rettype         OID;
    rettypename     TEXT;
    probody         TEXT;
    paranum         INTEGER;
    i               INTEGER;
    parameterall    TEXT;
    funcddl         TEXT;
    pargmode        CHAR;
    pargmodetext    TEXT;
    obj_owneroid     OID;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF;

    funcddl := '';

    WHILE (mycur%FOUND) LOOP
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;

      parameterall := '(';
      SELECT proc.pronargs INTO paranum FROM pg_proc proc WHERE proc.oid = objtoid;
      FOR i IN 1 .. paranum LOOP
      -- get parameter name
        SELECT proc.proargnames[i], proc.proargtypes[i - 1], proc.proargmodes[i]
        INTO parameter, paratype, pargmode
        FROM pg_proc proc WHERE proc.oid = objtoid;
        -- get parameter type name
        SELECT typname INTO paratypename FROM pg_type WHERE oid = paratype;

        CASE
		WHEN pargmode = 'o' THEN
          pargmodetext := ' OUT ';
        WHEN pargmode = 'b' THEN
          pargmodetext := ' INOUT ';
        WHEN pargmode = 'i' THEN
          pargmodetext := ' IN ';
        ELSE
          pargmodetext := ' ';
        END CASE;

        IF i < paranum THEN
          parameterall := parameterall||pargmodetext||parameter||' '||paratypename||',';
        ELSE
          parameterall := parameterall||pargmodetext||parameter||' '||paratypename;
        END IF;
      END LOOP;
      parameterall := parameterall||')';

      -- get return type name
      SELECT proc.prorettype INTO rettype FROM pg_proc proc WHERE proc.oid = objtoid;
      SELECT typname INTO rettypename FROM pg_type WHERE oid=rettype;
      -- get function body
      SELECT proc.prosrc INTO probody FROM pg_proc proc WHERE proc.oid = objtoid;

      funcddl := funcddl||'CREATE OR REPLACE FUNCTION '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(OBJNAME)|| '"' || parameterall||' RETURN '||rettypename||' AS' ||probody||chr(10);
      FETCH mycur INTO objtoid, obj_owneroid;
    END LOOP;

  CLOSE mycur;
  RETURN funcddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

   --function GET_PROCEDURE_DDL
  FUNCTION GET_PROCEDURE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT proc.oid, proc.proowner
    FROM pg_proc proc, pg_namespace nsp
    WHERE proc.prokind='p'
    AND nsp.oid = proc.pronamespace
    AND nsp.nspname = SCHEMA
    AND proc.proname = OBJNAME;

    objtoid       OID;
    parameter     TEXT;
    paratype      INT;
    paratypename  TEXT;
    rettype       INT;
    rettypename   TEXT;
    probody       TEXT;
    paranum       INTEGER;
    i             INTEGER;
    parameterall  TEXT;
    procdureddl   TEXT;
    pargmode      CHAR;
    pargmodetext  TEXT;
    obj_owneroid   OID;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF;

    procdureddl := '';

    WHILE (mycur%FOUND) LOOP
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;

      parameterall := '(';
      SELECT proc.pronargs INTO paranum FROM pg_proc proc WHERE proc.oid = objtoid;
      FOR i IN 1 .. paranum LOOP
        -- get parameter name
        SELECT proc.proargnames[i], proc.proargtypes[i - 1], proc.proargmodes[i]
        INTO parameter, paratype, pargmode
        FROM pg_proc proc WHERE proc.oid = objtoid;

        CASE
        WHEN pargmode = 'o' THEN
          pargmodetext := ' OUT ';
        WHEN pargmode = 'b' THEN
          pargmodetext := ' INOUT ';
        WHEN pargmode = 'i' THEN
          pargmodetext := ' IN ';
        ELSE
          pargmodetext := ' ';
        END CASE;

        -- get parameter type name
        SELECT typname INTO paratypename FROM pg_type WHERE oid=paratype;

        IF i < paranum THEN
          parameterall := parameterall||pargmodetext||parameter||' '||paratypename||',';
        ELSE
          parameterall := parameterall||pargmodetext||parameter||' '||paratypename;
        END IF;
      END LOOP;
      parameterall := parameterall||')';

      -- get procdure body
      SELECT proc.prosrc INTO probody FROM pg_proc proc WHERE proc.oid = objtoid;

      procdureddl := procdureddl||'CREATE OR REPLACE PROCEDURE '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(OBJNAME)||'"' ||parameterall||' AS' ||probody||';'||chr(10);
      FETCH mycur INTO objtoid, obj_owneroid;
    END LOOP;

    CLOSE mycur;
    RETURN procdureddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  --function GET_TRIGGER_DDL
  FUNCTION GET_TRIGGER_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT
  AS
  DECLARE
    objtoid      OID;
    schemaoid    OID;
    stmt         TEXT;
    tmpddl       TEXT;
    triggerddl   TEXT;
    mycur        SYS_REFCURSOR;

    obj_owneroid  OID;
    obj_valid     INT := 0;
    obj_invalid   INT := 0;
	funname       TEXT = NULL;
  BEGIN
    stmt := 'SELECT tg.oid 
    FROM pg_trigger tg, pg_namespace nsp
    WHERE tg.tgname = '||''''||OBJNAME||''''||' AND nsp.nspname = '||''''||SCHEMA||'''';
    IF TABOID > 0 THEN
      stmt := stmt||' AND tg.tgrelid = '||TABOID;
    END IF;
    stmt := stmt||';';

    OPEN mycur FOR EXECUTE stmt;
    FETCH mycur INTO objtoid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF;

	triggerddl := '';
	WHILE mycur%FOUND LOOP
		  SELECT relowner INTO obj_owneroid FROM pg_class WHERE pg_class.oid = (SELECT tgrelid FROM pg_trigger WHERE pg_trigger.oid = objtoid);
		  IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
			obj_invalid := obj_invalid + 1;
		  ELSE
			obj_valid := obj_valid + 1;
			SELECT pg_get_triggerdef(objtoid, false) INTO tmpddl;
			triggerddl := triggerddl||tmpddl||chr(10);
		  END IF;
		  FETCH mycur INTO objtoid;
	END LOOP;
	CLOSE mycur;

	IF(obj_valid > 0) THEN
		RETURN triggerddl;
	ELSE
	   IF(obj_invalid > 0) THEN
		RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
	   END IF;
	END IF;
	RETURN triggerddl;
   EXCEPTION
     WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  --function GET_TABLESPACE_DDL 
  FUNCTION GET_TABLESPACE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT t.oid, t.spcowner
    FROM pg_tablespace t
    WHERE t.spcname = OBJNAME;

    objtoid      OID;
    retddl       TEXT;
    result       RECORD;

    obj_owneroid  OID;
	loc_name	  TEXT;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF; 

    --get tablespace context
    SELECT * INTO result FROM pg_tablespace t WHERE t.spcname = OBJNAME;
	SELECT pg_tablespace_location INTO loc_name FROM pg_tablespace_location(objtoid);
    retddl := 'CREATE TABLESPACE ' ||'"'|| result.spcname || '"';

	IF (loc_name IS NOT NULL) THEN
		retddl := retddl||' LOCATION '|| '''' || loc_name || '''';
	END IF;
    retddl := retddl||';';

    CLOSE mycur;
    RETURN retddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  FUNCTION GET_INDEX_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT
  AS
  DECLARE
    objtoid      OID;
    tbsoid       OID;
    isprimary    BOOLEAN;
    isunique     BOOLEAN;
    isclustered  BOOLEAN;
    isvalid      BOOLEAN;
    retddl       TEXT;
    tmpddl       TEXT;
    tabspace     TEXT;
    stmt         TEXT;
    mycur        SYS_REFCURSOR;

    obj_owneroid  OID;
    obj_valid     INT := 0;
    obj_invalid   INT := 0;
  BEGIN
    stmt := 'SELECT c.oid, c.reltablespace, i.indisprimary, i.indisunique, i.indisclustered, i.indisvalid, c.relowner
    FROM pg_catalog.pg_class c, pg_catalog.pg_namespace n, pg_catalog.pg_index i
    WHERE c.relkind=''i'' AND i.indexrelid = c.oid AND n.oid = c.relnamespace AND c.relname= '||''''||OBJNAME||''''||' AND n.nspname = '||''''||SCHEMA||'''';
    IF TABOID > 0 THEN
      stmt := stmt||' AND i.indrelid = '||TABOID;
    END IF;
    stmt := stmt || ' ORDER BY i.indisprimary DESC, i.indisunique DESC, c.relname;';
    OPEN mycur FOR EXECUTE stmt;

    FETCH mycur INTO objtoid, tbsoid, isprimary, isunique, isclustered, isvalid, obj_owneroid;
    IF mycur%NOTFOUND THEN
      CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF;
 
    retddl := '';
    WHILE mycur%FOUND LOOP
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
        obj_invalid := obj_invalid + 1;
      ELSE
        obj_valid := obj_valid + 1;
		  SET quote_all_identifiers = true;
		  SELECT pg_get_indexdef(objtoid) INTO tmpddl;
		  RESET quote_all_identifiers;
        retddl := retddl || tmpddl ||';'||chr(10);
      END IF;
      FETCH mycur INTO objtoid, tbsoid, isprimary, isunique, isclustered, isvalid, obj_owneroid;
    END LOOP;
    CLOSE mycur;

    IF(obj_valid > 0) THEN
      RETURN retddl;
    ELSE
      IF(obj_invalid > 0) THEN
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;
   EXCEPTION
     WHEN OTHERS THEN
       IF mycur%ISOPEN THEN
	    CLOSE mycur;
       END IF;
     RAISE;
  END;

  FUNCTION GET_CONSTRAINT_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT
  AS
  DECLARE
    objtoid      OID;
    retddl       TEXT;
    rsconname    TEXT;
    rscondef     TEXT;
    rstabname    TEXT;
    stmt         TEXT;
    mycur        SYS_REFCURSOR;

    obj_owneroid  OID;
    obj_valid     INT := 0;
    obj_invalid   INT := 0;
  BEGIN
    stmt := 'SELECT c.oid FROM pg_constraint c, pg_namespace nsp
    WHERE nsp.oid = c.connamespace
    AND nsp.nspname = '||''''||SCHEMA||''''||' AND c.conname = '||''''||OBJNAME||'''';
    IF TABOID > 0 THEN
      stmt := stmt||' AND c.conrelid = '||TABOID;
    END IF;
    stmt := stmt||';';

    OPEN mycur FOR EXECUTE stmt;
    FETCH mycur INTO objtoid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF; 

    retddl := '';
    --get constraint context
    WHILE mycur%FOUND LOOP
      SELECT relowner INTO obj_owneroid FROM pg_class WHERE pg_class.oid = (SELECT conrelid FROM pg_constraint WHERE pg_constraint.oid = objtoid);
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
        obj_invalid := obj_invalid + 1;
      ELSE
        obj_valid := obj_valid + 1;
		SET quote_all_identifiers = true;

        SELECT r.conname, pg_catalog.pg_get_constraintdef(r.oid, true)
        INTO rsconname, rscondef
        FROM pg_catalog.pg_constraint r
        WHERE r.oid = objtoid;

		RESET quote_all_identifiers;

        SELECT relname INTO rstabname FROM pg_class c WHERE c.oid = (SELECT conrelid FROM pg_constraint con WHERE con.oid = objtoid);

        retddl := retddl||'ALTER TABLE '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(rstabname)||'"'||' ADD CONSTRAINT '||'"'||GET_OBJNAME(rsconname)||'"'||' '||rscondef||';'||chr(10);
      END IF;
      FETCH mycur INTO objtoid;
    END LOOP;
    CLOSE mycur;

   IF(obj_valid > 0) THEN
      RETURN retddl;
    ELSE
      IF(obj_invalid > 0) THEN
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;
   EXCEPTION
     WHEN OTHERS THEN
       IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  --function GET_REF_CONSTRAINT_DDL
  FUNCTION GET_REF_CONSTRAINT_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), TABOID OID DEFAULT 0) RETURN TEXT
  AS
  DECLARE
    objtoid       OID;
    retddl        TEXT;
    rsconname     TEXT;
    rscondef      TEXT;
    rstabname     TEXT;
    stmt          TEXT;
    mycur         SYS_REFCURSOR;

    obj_owneroid   OID;
    obj_valid      INT := 0;
    obj_invalid    INT := 0;
  BEGIN
    stmt := 'SELECT c.oid FROM pg_constraint c, pg_namespace nsp
    WHERE c.contype = ''f''
    AND nsp.oid = c.connamespace
    AND nsp.nspname = '||''''||SCHEMA||''''||' AND c.conname = '||''''||OBJNAME||'''';
    IF TABOID > 0 THEN
      stmt := stmt||' AND c.conrelid = '||TABOID;
    END IF;
    stmt := stmt||'ORDER BY c.oid;';

    OPEN mycur FOR EXECUTE stmt;
    FETCH mycur INTO objtoid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    END IF;

    retddl := '';
    --get constraint context
    WHILE mycur%FOUND LOOP
      SELECT relowner INTO obj_owneroid FROM pg_class WHERE pg_class.oid = (SELECT conrelid FROM pg_constraint WHERE pg_constraint.oid = objtoid);
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
        obj_invalid := obj_invalid + 1;
      ELSE
        obj_valid := obj_valid + 1;
		SET quote_all_identifiers = true;

        SELECT r.conname, pg_catalog.pg_get_constraintdef(r.oid, true)
        INTO rsconname, rscondef
        FROM pg_catalog.pg_constraint r
        WHERE r.oid = objtoid;

        RESET quote_all_identifiers;

        SELECT relname
        INTO rstabname
        FROM pg_class c 
        WHERE c.oid = (SELECT conrelid FROM pg_constraint con WHERE con.oid = objtoid);

        retddl := retddl||'ALTER TABLE '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(rstabname)||'"'||' ADD CONSTRAINT '||'"'||GET_OBJNAME(rsconname)||'"'||' '||rscondef||';'||chr(10);
      END IF;
      FETCH mycur INTO objtoid;
    END LOOP;
    CLOSE mycur;

   IF(obj_valid > 0) THEN
      RETURN retddl;
    ELSE
      IF(obj_invalid > 0) THEN
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  --procedure GET_ATTR
  PROCEDURE GET_ATTR(TABOID IN OID, ATTRCNT IN INT, BUF OUT VARCHAR2(8000))
  AS
  DECLARE
    rsattname       TEXT;
    rsattinfo       TEXT;
    rsnotnull       BOOLEAN;
    rsnum           SMALLINT;
    rscnt           INT;
    hasindex        BOOLEAN;
    tmpddl          TEXT;
    mycur           SYS_REFCURSOR;
	rsattgenerated  TEXT;
	refobjid        OID;
	attidentity     CHAR;
	seqrelcursor    SYS_REFCURSOR;
	seqrelid        OID;
	seq_record	 RECORD;
    stmt         TEXT;
	seqname      TEXT;
	seqnamespace TEXT;
	seqvalue     RECORD;
	upperpname    TEXT;
	lowerpname    TEXT;
	uppername    TEXT;
	lowername    TEXT;
	attisvisible BOOLEAN;
	attgenerated CHAR;
	virexpr      TEXT;
	atthasdef    BOOLEAN;
	defaultexpr  TEXT;

	CURSOR relcursor FOR
	    SELECT d.refobjid FROM pg_index i, pg_depend d, pg_constraint c
		WHERE i.indrelid = TABOID AND i.indisprimary  = 't'AND d.objid = i.indexrelid
		AND d.refobjid = c.oid and c.conrelid = TABOID;
  BEGIN
    OPEN mycur FOR
    SELECT a.attname, pg_catalog.format_type(a.atttypid, a.atttypmod::INTEGER),
    a.attnotnull, a.attgenerated, a.attnum, a.attidentity, a.attisvisible, a.attgenerated, a.atthasdef
    FROM pg_catalog.pg_attribute a
    WHERE a.attrelid = TABOID AND a.attnum > 0 AND NOT a.attisdropped ORDER BY a.attnum;

    FETCH mycur INTO rsattname, rsattinfo, rsnotnull, rsattgenerated, rsnum, attidentity, attisvisible, attgenerated, atthasdef;

    CASE
    WHEN mycur%FOUND THEN
      BUF := '( '||chr(10);
      rscnt := 1;
      WHILE mycur%FOUND LOOP
        BUF := BUF ||' '|| '"' || GET_OBJNAME(rsattname) || '"' ||' '||rsattinfo;

		--check if the colum is INVISIBLE
		IF attisvisible = FALSE THEN
		   BUF := BUF ||' INVISIBLE ';
		END IF;

		--check if the colum is identity column
        IF attidentity = 'i' OR  attidentity = 'o' OR attidentity = 'n' THEN
		  IF attidentity = 'i' THEN
		    BUF := BUF ||' GENERATED ALWAYS AS IDENTITY ';
		  ELSEIF attidentity = 'o' THEN
		    BUF := BUF ||' GENERATED BY DEFAULT AS IDENTITY ';
		  ELSEIF attidentity = 'n' THEN
		     BUF := BUF ||' GENERATED BY DEFAULT ON NULL AS IDENTITY ';
		  END IF;

          OPEN seqrelcursor FOR SELECT c.oid, c.relname, n.nspname
          FROM pg_depend d, pg_class c, pg_sequence q, pg_namespace n
          WHERE d.refobjid = TABOID AND d.classid = 1259
          AND c.oid = d.objid AND q.seqrelid = d.objid
          AND c.relkind = 'S' AND n.oid = c.relnamespace;
 
          FETCH seqrelcursor INTO seqrelid, seqname, seqnamespace;
          IF seqrelcursor%FOUND THEN
	        stmt := 'SELECT * FROM pg_sequence where seqrelid =' || seqrelid;
	        EXECUTE stmt INTO seq_record;
            BUF := BUF ||' MINVALUE '||seq_record.seqmin||' MAXVALUE '||seq_record.seqmax;
            BUF := BUF ||' INCREMENT BY '||seq_record.seqincrement ||chr(10);

			upperpname = UPPER(seqnamespace);
			lowerpname = LOWER(seqnamespace);
			IF seqnamespace = upperpname THEN
			  seqnamespace =  lowerpname;
			ELSEIF seqnamespace = lowerpname THEN
			  seqnamespace =  upperpname;
			END IF;

			uppername = UPPER(seqname);
			lowername = LOWER(seqname);
			IF seqname = uppername THEN
			  seqname =  lowername;
			ELSEIF seqname = lowername THEN
			  seqname =  uppername;
			END IF;

            stmt := 'SELECT * FROM '||'"'||seqnamespace||'" '||'.'|| '"'||seqname||'" ';
            EXECUTE stmt INTO seqvalue;

            IF seqvalue.is_called THEN
              BUF := BUF ||'START WITH '||seqvalue.last_value + seq_record.seqincrement ||' CACHE '||seq_record.seqcache;
            ELSE
              BUF := BUF ||'START WITH '||seq_record.seqstart ||' CACHE '||seq_record.seqcache;
            END IF;
            
            IF seq_record.seqcycle THEN
              BUF := BUF || ' CYCLE '||chr(10);
            ELSE
              BUF := BUF || ' NOCYCLE '||chr(10);
            END IF;
          END IF;
          CLOSE seqrelcursor;
		ELSEIF attgenerated = 'v' THEN --virtual column
		  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid, true) INTO virexpr FROM pg_catalog.pg_attrdef d WHERE d.adrelid = TABOID AND d.adnum = rscnt;
		  BUF := BUF || ' GENERATED ALWAYS AS (' || virexpr || ')' ||chr(10);
		ELSEIF atthasdef = TRUE THEN --default value
		  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid, true) INTO defaultexpr FROM pg_catalog.pg_attrdef d WHERE d.adrelid = TABOID AND d.adnum = rscnt;
		  BUF := BUF || ' DEFAULT ' || defaultexpr ||chr(10);
        END IF;

        IF rsnotnull IS TRUE THEN
          BUF := BUF ||' NOT NULL';
        END IF;

        IF (rscnt <> ATTRCNT) THEN
          BUF := BUF ||', '||chr(10);
        END IF;
 
        FETCH mycur INTO rsattname, rsattinfo, rsnotnull, rsattgenerated, rsnum, attidentity, attisvisible, attgenerated, atthasdef;
        rscnt := rscnt + 1;
     END LOOP;

     SELECT relhasindex INTO hasindex FROM pg_class WHERE oid = TABOID;
     IF (hasindex) THEN
        BUF := BUF || ', '||chr(10);
        --get primary key information
		OPEN relcursor;
        FETCH relcursor INTO refobjid;
        IF relcursor%NOTFOUND THEN
	      CLOSE relcursor;
        ELSE
		  SET quote_all_identifiers = true;

          SELECT pg_get_constraintdef(refobjid) INTO tmpddl;

		  RESET quote_all_identifiers;
	      CLOSE relcursor;
        END IF;

        IF tmpddl IS NOT NULL THEN
          BUF := BUF||tmpddl||chr(10);
        END IF;
     ELSE
        BUF := BUF||chr(10);
     END IF;

     BUF := BUF || ') ';
     ELSE
        BUF := '';	 
    END CASE;
    CLOSE mycur;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      IF seqrelcursor%ISOPEN THEN
	    CLOSE seqrelcursor;
      END IF;
      RAISE;
  END;

 --function GET_TABLE_DDL
 FUNCTION GET_TABLE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63), PARENTOBJNAME VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    relid           OID;
    relname         TEXT;
    relnatts        INT;
    stmt            TEXT;
    relchecks       INT;
	relhastriggers  BOOLEAN;
    relkind         CHAR;
    relhasindex     BOOLEAN;
    relhasrules     BOOLEAN;
    relhasrowid      BOOLEAN;
    tablespaceid    OID;
    ddltmp          TEXT;
    idxrelname      TEXT;
	unconstraintname TEXT;
    chkname         TEXT;
    fkname          TEXT;
    tabspacename    TEXT;
    tabtype    TEXT;
	globaloptions TEXT;
	relispartition  BOOLEAN;
	relpartbound  pg_node_tree;

    CURSOR relcursor FOR
    SELECT c.oid, c.relname, c.relnatts, c.relowner,
    c.relhasindex, c.relkind, c.relchecks, c.relhastriggers, 
	c.relhasrules, c.relhasrowid, c.reltablespace, c.relpersistence, c.reloptions[1], c.relispartition, c.relpartbound
    FROM pg_catalog.pg_class c, pg_catalog.pg_namespace n
    WHERE (c.relkind = 'r' OR c.relkind = 'p')
    AND n.oid = c.relnamespace
    AND c.relname = OBJNAME
    AND n.nspname = SCHEMA
    ORDER BY 2;

    myindexcur        SYS_REFCURSOR;
	myunconstraintscur SYS_REFCURSOR;
    mycheckcur        SYS_REFCURSOR;
    result            TEXT;
    obj_owneroid      OID;
	myfkeycur         SYS_REFCURSOR;
	partrelcursor     SYS_REFCURSOR;
	objnspname	  	  TEXT;
	partrelid         OID;
  BEGIN
    SET quote_all_identifiers = true;

    OPEN relcursor;
    FETCH relcursor INTO relid, relname, relnatts, obj_owneroid, relhasindex, relkind, 
	  relchecks, relhastriggers, relhasrules, relhasrowid,tablespaceid, tabtype,globaloptions,relispartition,relpartbound;
    IF relcursor%NOTFOUND THEN
	  CLOSE relcursor;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE relcursor;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;
    CLOSE relcursor;

	IF tabtype = 'g' THEN
	  result := 'CREATE GLOBAL TEMPORARY TABLE '|| '"' || GET_OBJNAME(SCHEMA) || '"' ||'.' || '"' || GET_OBJNAME(OBJNAME) || '"';
	ELSEIF tabtype = 't' THEN
      result := 'CREATE TERPORARY TABLE '|| '"' || GET_OBJNAME(SCHEMA) || '"';
	ELSEIF tabtype = 'u' THEN
      result := 'CREATE UNLOGGED TABLE '|| '"' || GET_OBJNAME(SCHEMA) || '"';
    ELSE
      result := 'CREATE TABLE '|| '"' || GET_OBJNAME(SCHEMA) || '"' ||'.' || '"' || GET_OBJNAME(OBJNAME) || '"';
    END IF;

	IF relispartition = TRUE THEN
	  result := result || ' PARTITION OF ' || '"' || GET_OBJNAME(SCHEMA) || '"' ||'.' || '"' || GET_OBJNAME(PARENTOBJNAME) || '" ' || pg_catalog.pg_get_expr(relpartbound, relid) || chr(10);
	ELSE
      --attributes
      DBMS_METADATA.GET_ATTR(relid, relnatts, ddltmp);
      IF ddltmp IS NULL THEN
        RAISE EXCEPTION 'THE ATTRIBUTES INFORMATION DOSE NOT MATCH!';
      ELSE
        result := result || ddltmp;
      END IF;
	END IF;

	--partition
	IF relkind = 'p' THEN
	  result := result || 'PARTITION BY ' || pg_get_partkeydef(relid) || chr(10);
    END IF;

    --tablespace
	IF tabtype = 'p' THEN
	  IF tablespaceid = 0 THEN
		result := result||'TABLESPACE '||'"'|| 'PG_DEFAULT' || '"'||chr(10);
	  ELSE
        SELECT ts.spcname INTO tabspacename FROM pg_tablespace ts, pg_class c WHERE ts.oid = c.reltablespace 
	    AND c.relpersistence = 'p' AND c.oid = relid AND c.reltablespace != 0;
        
	    IF tabspacename IS NOT NULL THEN
          result := result||'TABLESPACE '||'"'||GET_OBJNAME(tabspacename)|| '"'||chr(10);
	    END IF;
	  END IF;
    END IF;

    --index
    IF (relhasindex) THEN
      OPEN myindexcur FOR SELECT c.relname, n.nspname FROM pg_class c, pg_index i, pg_namespace n WHERE c.oid = i.indexrelid AND i.indrelid = relid
			                AND n.oid = c.relnamespace AND i.indisunique = 'f' AND i.indisprimary = 'f';
      FETCH myindexcur INTO idxrelname, objnspname;
      WHILE myindexcur%FOUND LOOP
        ddltmp := DBMS_METADATA.GET_INDEX_DDL(idxrelname, objnspname, relid);
        IF ddltmp IS NULL THEN
		  CLOSE myindexcur;
          RAISE EXCEPTION 'THE INDEX INFORMATION DOSE NOT MATCH!';
        ELSE
          result := result || ddltmp;
        END IF;
        FETCH myindexcur INTO idxrelname, objnspname;
      END LOOP;
      CLOSE myindexcur;
    END IF;

	--unique constraint
	IF (relhasindex) THEN
      OPEN myunconstraintscur FOR SELECT c.relname, n.nspname FROM pg_class c, pg_index i, pg_namespace n WHERE c.oid = i.indexrelid AND i.indrelid = relid
			                AND n.oid = c.relnamespace AND i.indisunique = 't' AND i.indisprimary  = 'f' ;
      FETCH myunconstraintscur INTO unconstraintname, objnspname;
      WHILE myunconstraintscur%FOUND LOOP
        ddltmp := DBMS_METADATA.GET_CONSTRAINT_DDL(unconstraintname, objnspname, relid);
        IF ddltmp IS NULL THEN
		  CLOSE myunconstraintscur;
          RAISE EXCEPTION 'THE UNIQUE INFORMATION DOSE NOT MATCH!';
        ELSE
          result := result || ddltmp;
        END IF;
        FETCH myunconstraintscur INTO unconstraintname, objnspname;
      END LOOP;
      CLOSE myunconstraintscur;
    END IF;

    --check
    IF (relchecks > 0) THEN
      OPEN mycheckcur FOR SELECT r.conname, n.nspname FROM pg_catalog.pg_constraint r , pg_namespace n WHERE r.conrelid = relid AND r.contype = 'c' AND n.oid = r.connamespace ORDER BY 1;
      FETCH mycheckcur INTO chkname, objnspname;
      WHILE mycheckcur%FOUND LOOP
        ddltmp := DBMS_METADATA.GET_CONSTRAINT_DDL(chkname, objnspname, relid);
        IF ddltmp IS NULL THEN
		  CLOSE mycheckcur;
          RAISE EXCEPTION 'THE CHECK INFORMATION DOSE NOT MATCH!';
        ELSE
          result := result || ddltmp;
        END IF;
        FETCH mycheckcur INTO chkname, objnspname;
      END LOOP;
      CLOSE mycheckcur;
    END IF;

	--Foreign key constraint
      OPEN myfkeycur FOR SELECT r.conname, n.nspname  FROM pg_catalog.pg_constraint r, pg_namespace n  WHERE r.conrelid = relid AND r.contype = 'f'  AND n.oid = r.connamespace ORDER BY 1;
      FETCH myfkeycur INTO fkname, objnspname;
      WHILE myfkeycur%FOUND LOOP
        ddltmp := DBMS_METADATA.GET_CONSTRAINT_DDL(fkname, objnspname, relid);
        IF ddltmp IS NULL THEN
		  CLOSE myfkeycur;
          RAISE EXCEPTION 'THE FOREIGN KEY CONSTRAINT DOSE NOT MATCH!';
        ELSE
          result := result || ddltmp;
        END IF;
        FETCH myfkeycur INTO fkname, objnspname;
      END LOOP;
      CLOSE myfkeycur;

      IF tabtype = 'g' then
	    if instr(globaloptions,'true') = 0 THEN
			result := result || ' ON COMMIT PRESERVE ROWS';
		else
			result := result || ' ON COMMIT DELETE ROWS';
		end if;
	  END IF;

	--partition
	IF relkind = 'p' THEN
      OPEN partrelcursor FOR SELECT inhrelid FROM pg_inherits WHERE inhparent = relid;
      FETCH partrelcursor INTO partrelid;
      WHILE partrelcursor%FOUND LOOP
	    SET quote_all_identifiers = FALSE;
        ddltmp := DBMS_METADATA.GET_TABLE_DDL(partrelid::regclass::varchar2, SCHEMA, OBJNAME);
		SET quote_all_identifiers = TRUE;
        IF ddltmp IS NULL THEN
		  CLOSE partrelcursor;
          RAISE EXCEPTION 'THE PARTITION DOSE NOT EXISTS!';
        ELSE
          result := result || ddltmp;
        END IF;
        FETCH partrelcursor INTO partrelid;
      END LOOP;
      CLOSE partrelcursor;
    END IF;

    RESET quote_all_identifiers;

    RETURN result;
  EXCEPTION
    WHEN OTHERS THEN
      IF relcursor%ISOPEN THEN
	    CLOSE relcursor;
      END IF;
      IF myindexcur%ISOPEN THEN
	    CLOSE myindexcur;
      END IF;
      IF mycheckcur%ISOPEN THEN
	    CLOSE mycheckcur;
      END IF;
      IF myfkeycur%ISOPEN THEN
	    CLOSE myfkeycur;
      END IF;
      IF partrelcursor%ISOPEN THEN
	    CLOSE partrelcursor;
      END IF;
	  RESET quote_all_identifiers;
      RAISE;
  END;

--function GET_PACKAGE_DDL
  FUNCTION GET_PACKAGE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT p.oid, p.pkgowner FROM pg_package p, pg_catalog.pg_namespace n
    WHERE p.pkgname = OBJNAME
    AND n.nspname=SCHEMA
    AND n.oid = p.pkgnamespace;

    objtoid      OID;
    retddl       TEXT;
	headddl      TEXT;
    tmpddl       TEXT;
    obj_owneroid  OID;
	bodyddl      TEXT;
	len1         INT;
	len2         INT;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;

	SET quote_all_identifiers = true;
	SELECT pg_get_package_head(objtoid) INTO headddl;
	RESET quote_all_identifiers;

    --get package context
    SELECT pkgsrc INTO tmpddl FROM pg_package WHERE oid=objtoid;
    retddl := headddl || chr(10) || 'AS' || tmpddl || ';'|| chr(10);

    CLOSE mycur;
	--Is there a package body?
	--SELECT pg_get_package_body_for_getddl(objtoid) INTO bodyddl;
	SELECT bodysrc INTO bodyddl FROM pg_package_body WHERE pkgoid = objtoid;
    IF bodyddl IS NOT NULL THEN
	  len1 = LENGTH(headddl);
	  len2 = instr(headddl, 'PACKAGE');
	  retddl := retddl || LEFT(headddl, len2 + LENGTH('PACKAGE')) || 'BODY ' || RIGHT(headddl, len1 - len2 - LENGTH('PACKAGE')) || ' AS' || chr(10) || bodyddl || ';' || chr(10);
	END IF;

	RETURN retddl;
  EXCEPTION
	WHEN OTHERS THEN
	  IF mycur%ISOPEN THEN
	    CLOSE mycur;
	  END IF;
	  RAISE;
  END;

  --function GET_PACKAGE_BODY_DDL
  FUNCTION GET_PACKAGE_BODY_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT p.oid, p.pkgowner FROM pg_package p, pg_catalog.pg_namespace n
    WHERE p.pkgname = OBJNAME
    AND n.nspname=SCHEMA
    AND n.oid = p.pkgnamespace;

    objtoid      OID;
    retddl       TEXT;
    tmpddl       TEXT;
    obj_owneroid  OID;
	headddl      TEXT;
	bodyddl      TEXT;
	len1         INT;
	len2         INT;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;

    --get package body context
	SELECT bodysrc INTO bodyddl FROM pg_package_body WHERE pkgoid = objtoid;

	IF bodyddl IS NOT NULL THEN
	  SET quote_all_identifiers = true;
	  SELECT pg_get_package_head(objtoid) INTO headddl;
	  RESET quote_all_identifiers;
	  len1 = LENGTH(headddl);
	  len2 = instr(headddl, 'PACKAGE');
	  retddl := retddl || LEFT(headddl, len2 + LENGTH('PACKAGE')) || 'BODY ' || RIGHT(headddl, len1 - len2 - LENGTH('PACKAGE')) || ' AS' || chr(10) || bodyddl || ';' || chr(10);
    END IF;

    CLOSE mycur;
    RETURN retddl;
  EXCEPTION
    WHEN OTHERS THEN
	  IF mycur%ISOPEN THEN
	    CLOSE mycur;
	  END IF;
	  RAISE;
  END;

  --function GET_DBLINK_DDL
  FUNCTION GET_DBLINK_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT s.oid, s.srvowner, s.olnkisshared,s.olnkispublic, s.olnkconntype, s.olnkusingstring
	FROM pg_foreign_server s, pg_user u
    WHERE s.srvname = OBJNAME AND u.usesysid = s.srvowner;

    CURSOR mycur2 FOR
	SELECT umoptions
	FROM pg_foreign_server s, pg_user_mappings m
	WHERE s.srvname = OBJNAME  AND s.oid = m.srvid;

    objtoid      OID;
    retddl       TEXT;
    obj_owneroid  OID;
	olnkisshared BOOLEAN;
	olnkispublic BOOLEAN;
	olnkconntype INT;
	umoptions    TEXT;
	username     TEXT;
	userpwd      TEXT;
	olnkusingstring	TEXT;
	authentusername TEXT;
	authentuserpwd  TEXT;
	pos       INT;
	i            INT;
	str			 TEXT;
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid, olnkisshared, olnkispublic, olnkconntype, olnkusingstring;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;

	IF olnkconntype = 2 THEN
        OPEN mycur2;
        FETCH mycur2 INTO umoptions;
        IF mycur2%NOTFOUND THEN
	      CLOSE mycur2;
          RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
        END IF;

    	--get the username from umoptions in pg_user_mapping table
    	pos := 0;
    	SELECT POSITION('user' IN umoptions) INTO pos;
    	IF pos != 0 THEN
    	  i := pos;
    	  LOOP
    	    str := substr(umoptions, i + 5, 1);
    		IF str = ',' OR str = '}'THEN
    		  EXIT;
    		ELSE
    		  i := i + 1;
    		END IF;
    	  END LOOP;
    	  username = SUBSTR(umoptions, pos + 5, i - pos);
    	END IF;
    
    	--get the password from umoptions in pg_user_mapping table
    	pos := 0;
    	SELECT POSITION('password' IN umoptions) INTO pos;
    	IF pos != 0 THEN
    	  i := pos;
    	  LOOP
    	    str := substr(umoptions, i + 9, 1);
    		IF str = ',' OR str = '}'THEN
    		  EXIT;
    		ELSE
    		  i := i + 1;
    		END IF;
    	  END LOOP;
    	  userpwd = SUBSTR(umoptions, pos + 9, i - pos);
    	END IF;
    
    	--get the authuser from umoptions in pg_user_mapping table
    	pos := 0;
    	SELECT POSITION('authuser' IN umoptions) INTO pos;
    	IF pos != 0 THEN
    	  i := pos;
    	  LOOP
    	    str := substr(umoptions, i + 9, 1);
    		IF str = ',' OR str = '}'THEN
    		  EXIT;
    		ELSE
    		  i := i + 1;
    		END IF;
    	  END LOOP;
    	  authentusername = SUBSTR(umoptions, pos + 9, i - pos);
    	END IF;
    
    	--get the authpassword from umoptions in pg_user_mapping table
    	pos := 0;
    	SELECT POSITION('authpassword' IN umoptions) INTO pos;
    	IF pos != 0 THEN
    	  i := pos;
    	  LOOP
    	    str := substr(umoptions, i + 13, 1);
    		IF str = ',' OR str = '}'THEN
    		  EXIT;
    		ELSE
    		  i := i + 1;
    		END IF;
    	  END LOOP;
    	  authentuserpwd = SUBSTR(umoptions, pos + 13, i - pos);
    	END IF;
		CLOSE mycur2;
	END IF;

	IF olnkisshared = TRUE AND olnkispublic = TRUE THEN
      retddl := 'CREATE SHARED PUBLIC DATABASE LINK '||'"'||GET_OBJNAME(OBJNAME)||'"' || chr(10);
    ELSEIF olnkisshared = TRUE THEN
	  retddl := 'CREATE SHARED DATABASE LINK '||'"'||GET_OBJNAME(OBJNAME)||'"' || chr(10);
	ELSEIF olnkispublic = TRUE THEN
	  retddl := 'CREATE PUBLIC DATABASE LINK '||'"'||GET_OBJNAME(OBJNAME)||'"' || chr(10);
	ELSE
	  retddl := 'CREATE DATABASE LINK '||'"'||GET_OBJNAME(OBJNAME)||'"' || chr(10);
	END IF;

	IF olnkconntype = 2 THEN
	   retddl := retddl || ' CONNECT TO '||'"'||GET_OBJNAME(username)||'"' || ' IDENTIFIED BY '|| '"'|| userpwd ||'"' || chr(10);
	   IF olnkisshared = TRUE THEN
	     retddl := retddl || ' AUTHENTICATED BY  '||'"'||GET_OBJNAME(authentusername)||'"' || ' IDENTIFIED BY '|| '"'|| authentuserpwd ||'"' || chr(10);
	   END IF;
	ELSEIF olnkconntype = 3 THEN
	   retddl := retddl || ' CONNECT TO CURRENT_USER'|| chr(10);   
	END IF;

    IF olnkusingstring IS NOT NULL THEN
	    retddl := retddl || 'USING ' || '''' || olnkusingstring || ''''|| chr(10);
	END IF;

    CLOSE mycur;
	RETURN retddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      IF mycur2%ISOPEN THEN
	    CLOSE mycur2;
      END IF;
      RAISE;
  END;

--function GET_SEQUENCE_DDL
  FUNCTION GET_SEQUENCE_DDL(OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63)) RETURN TEXT
  AS
  DECLARE
    CURSOR mycur FOR
    SELECT c.oid, c.relowner FROM pg_class c, pg_catalog.pg_namespace n
    WHERE c.relname = OBJNAME
    AND c.relkind = 'S'
    AND n.nspname = SCHEMA
    AND n.oid = c.relnamespace;

    objtoid      OID;
    retddl       TEXT;
    result       RECORD;
	seq_record	 RECORD;
    stmt         TEXT;

    obj_owneroid  OID;
	uppername	 VARCHAR(63);
	lowername    VARCHAR(63);
	upperschema  VARCHAR(63);
	lowerschema  VARCHAR(63);
	objectname   VARCHAR2(63);
	schemaname   VARCHAR(63);
  BEGIN
    OPEN mycur;
    FETCH mycur INTO objtoid, obj_owneroid;
    IF mycur%NOTFOUND THEN
	  CLOSE mycur;
      RAISE EXCEPTION 'OBJECT % DOES NOT EXIST!', OBJNAME;
    ELSE
      IF(DBMS_METADATA.CHECK_PRIVILEGE(obj_owneroid) = FALSE) THEN
	    CLOSE mycur;
        RAISE EXCEPTION 'PERMISSION DENIED FOR OBJECT %! MUST BE OWNER OR DBA!',OBJNAME;
      END IF;
    END IF;

    IF SCHEMA LIKE 'pg_TEMP_%' THEN
      retddl := 'CREATE TEMPORARY SEQUENCE '||'"'||GET_OBJNAME(OBJNAME)||'"'||chr(10);
    ELSE
      retddl := 'CREATE SEQUENCE '||'"'||GET_OBJNAME(SCHEMA)||'"'||'.'||'"'||GET_OBJNAME(OBJNAME)||'"'||chr(10);
    END IF;

	uppername = UPPER(OBJNAME);
	lowername = LOWER(OBJNAME);
	IF OBJNAME = uppername THEN
	   objectname =  lowername;
	ELSE
	  IF OBJNAME = lowername THEN
	     objectname =  uppername;
	  END IF;
	END IF;

	upperschema = UPPER(SCHEMA);
	lowerschema = LOWER(SCHEMA);
    IF SCHEMA = upperschema THEN
	  schemaname =  lowerschema;
	ELSE
	  IF SCHEMA = lowerschema THEN
	    schemaname =  upperschema;
	  END IF;
	END IF;

    stmt := 'SELECT * FROM '||'"'||schemaname||'" '||'.'|| '"'||objectname||'" ';
    EXECUTE stmt INTO result;

	stmt := 'SELECT * FROM pg_sequence where seqrelid =' || objtoid;

	EXECUTE stmt INTO seq_record;
    retddl := retddl ||' MINVALUE '||seq_record.seqmin||' MAXVALUE '||seq_record.seqmax;
    retddl := retddl ||' INCREMENT BY '||seq_record.seqincrement ||chr(10);

    IF result.is_called THEN
      retddl := retddl ||'START WITH '||result.last_value + seq_record.seqincrement ||' CACHE '||seq_record.seqcache;
    ELSE
      retddl := retddl ||'START WITH '||seq_record.seqstart ||' CACHE '||seq_record.seqcache;
    END IF;

    IF seq_record.seqcycle THEN
      retddl := retddl || ' CYCLE '||chr(10);
    ELSE
      retddl := retddl || ' NOCYCLE '||chr(10);
    END IF;

    retddl := retddl || 'NOORDER NOKEEP NOSCALE;'||chr(10);

    CLOSE mycur;
    RETURN retddl;
  EXCEPTION
    WHEN OTHERS THEN
      IF mycur%ISOPEN THEN
	    CLOSE mycur;
      END IF;
      RAISE;
  END;

  FUNCTION GET_DDL(OBJTYPE VARCHAR2(63), OBJNAME VARCHAR2(63), SCHEMA VARCHAR2(63) DEFAULT NULL, MODEL VARCHAR2(63) DEFAULT NULL, VERSION VARCHAR2(63) DEFAULT NULL, TRANSFORM VARCHAR2(63) DEFAULT NULL) 
  RETURN CLOB IS
    ddl          CLOB;
    SCHNAME      VARCHAR2(63);
	username	 VARCHAR2(63);
	uppername	 VARCHAR(63);
	lowername    VARCHAR(63);
	upperschema  VARCHAR(63);
	lowerschema  VARCHAR(63);
	objectname      VARCHAR(63);
  BEGIN
    IF SCHEMA IS NULL THEN
	   SELECT current_user INTO username;
	   IF username = 'sys'OR username = 'system' THEN
	     SCHNAME = 'public';
	   ELSEIF is_supper_user(username) THEN
	     SCHNAME = 'public';
	   ELSE
		 SELECT current_schema INTO SCHNAME;
	   END IF;
    ELSE
      SCHNAME := SCHEMA;
	  upperschema = UPPER(SCHNAME);
	  lowerschema = LOWER(SCHNAME);
      IF SCHNAME = upperschema THEN
	    SCHNAME =  lowerschema;
	  ELSE
	    IF SCHNAME = lowerschema THEN
	      SCHNAME =  upperschema;
	     END IF;
	  END IF;
    END IF;

	uppername = UPPER(OBJNAME);
	lowername = LOWER(OBJNAME);
	IF OBJNAME = uppername THEN
	   objectname =  lowername;
	ELSE
	  IF OBJNAME = lowername THEN
	     objectname =  uppername;
	  END IF;
	END IF;

    CASE
    WHEN  OBJTYPE = 'VIEW' THEN
      ddl := DBMS_METADATA.GET_VIEW_DDL(objectname, SCHNAME);
	WHEN  OBJTYPE = 'FUNCTION' THEN
      ddl := DBMS_METADATA.GET_FUNC_DDL(objectname, SCHNAME);
    WHEN  OBJTYPE = 'PROCEDURE' THEN
      ddl := DBMS_METADATA.GET_PROCEDURE_DDL(objectname, SCHNAME);
    WHEN  OBJTYPE = 'TRIGGER' THEN
      ddl := DBMS_METADATA.GET_TRIGGER_DDL(objectname, SCHNAME);
    WHEN OBJTYPE = 'TABLESPACE' THEN
      ddl := DBMS_METADATA.GET_TABLESPACE_DDL(objectname, SCHEMA);
    WHEN OBJTYPE = 'INDEX' THEN
      ddl := DBMS_METADATA.GET_INDEX_DDL(objectname, SCHNAME);	
    WHEN OBJTYPE = 'CONSTRAINT' THEN
      ddl := DBMS_METADATA.GET_CONSTRAINT_DDL(objectname, SCHNAME);
    WHEN OBJTYPE = 'REF_CONSTRAINT' THEN
      ddl := DBMS_METADATA.GET_REF_CONSTRAINT_DDL(objectname, SCHNAME);
    WHEN OBJTYPE = 'SEQUENCE' THEN
      ddl := DBMS_METADATA.GET_SEQUENCE_DDL(objectname, SCHNAME);
    WHEN  OBJTYPE = 'TABLE' THEN
      ddl := DBMS_METADATA.GET_TABLE_DDL(objectname, SCHNAME, NULL);	  
    WHEN OBJTYPE = 'PACKAGE' THEN
      ddl := DBMS_METADATA.GET_PACKAGE_DDL(objectname, SCHNAME);
    WHEN OBJTYPE = 'PACKAGE_BODY' THEN
      ddl := DBMS_METADATA.GET_PACKAGE_BODY_DDL(objectname, SCHNAME);
    WHEN OBJTYPE = 'DB_LINK' THEN
      ddl := DBMS_METADATA.GET_DBLINK_DDL(objectname, SCHNAME);
    ELSE
      RAISE EXCEPTION 'INVALID INPUT VALUE % IN FUNCTION DBMS_METADATA.GET_DDL', OBJTYPE;
    END CASE;
    RETURN ddl;
  END;
END;

GRANT EXECUTE ON package SYS.DBMS_METADATA TO public;
/* End - ReqID:SRS-PLSQL-DBMS_METADATA-GET_DDL */
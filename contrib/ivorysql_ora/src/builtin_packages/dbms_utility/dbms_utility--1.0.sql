CREATE OR REPLACE FUNCTION sys.dbmsutility_expand_sql_text(
    input_sql_text IN VARCHAR2
) RETURNS VARCHAR2
AS 'MODULE_PATHNAME', 'dbms_utility_expand_sql_text'
LANGUAGE C;

CREATE OR REPLACE FUNCTION sys.dbmsutility_get_cpu_time() RETURNS INTEGER
AS 'MODULE_PATHNAME', 'dbms_utility_get_cpu_time'
LANGUAGE C;

CREATE OR REPLACE FUNCTION sys.dbmsutility_get_endianness() RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'dbms_utility_get_endianness'
LANGUAGE C;

CREATE OR REPLACE FUNCTION sys.dbmsutility_get_error_stack() RETURNS TEXT
AS 'MODULE_PATHNAME', 'dbms_utility_get_error_stack'
LANGUAGE C;

CREATE OR REPLACE FUNCTION sys.dbmsutility_exec_ddl_statement(
    parse_string IN VARCHAR2
) RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'dbms_utility_exec_ddl_statement'
LANGUAGE C;

CREATE OR REPLACE PACKAGE SYS.DBMS_UTILITY AUTHID CURRENT_USER AS
    -- Begin - ReqID: OA-2455
    TYPE INSTANCE_RECORD IS RECORD (
        inst_number NUMBER,
        inst_name VARCHAR2(60)
    );
    TYPE DBLINK_ARRAY IS TABLE OF VARCHAR2(128) INDEX BY INTEGER;
    TYPE INDEX_TABLE_TYPE IS TABLE OF BINARY_INTEGER INDEX BY INTEGER;
    TYPE INSTANCE_TABLE IS TABLE OF INSTANCE_RECORD INDEX BY INTEGER;
    TYPE LNAME_ARRAY IS TABLE OF VARCHAR2(4000) INDEX BY INTEGER;
    TYPE NAME_ARRAY IS TABLE OF VARCHAR2(30) INDEX BY INTEGER;
    TYPE NUMBER_ARRAY IS TABLE OF NUMBER INDEX BY INTEGER;
    TYPE UNCL_ARRAY IS TABLE OF VARCHAR2(227) INDEX BY INTEGER;
    -- End - ReqID: OA-2455

    PROCEDURE ANALYZE_DATABASE(method IN VARCHAR2 DEFAULT NULL, estimate_rows IN NUMBER DEFAULT NULL,
                            estimate_percent IN NUMBER DEFAULT NULL, method_opt IN VARCHAR2 DEFAULT NULL);
    PROCEDURE ANALYZE_PART_OBJECT(schema IN VARCHAR2 DEFAULT NULL, object_name IN VARCHAR2 DEFAULT NULL,
                                object_type IN CHAR DEFAULT 'T', command_type IN CHAR DEFAULT 'E',
                                command_opt IN VARCHAR2 DEFAULT NULL, sample_clause IN VARCHAR2 DEFAULT 'sample 5 percent');
    PROCEDURE ANALYZE_SCHEMA(schema IN VARCHAR2, method IN VARCHAR2 DEFAULT NULL, estimate_rows IN NUMBER DEFAULT NULL,
                            estimate_percent IN NUMBER DEFAULT NULL, method_opt IN VARCHAR2 DEFAULT NULL);
    PROCEDURE CANONICALIZE(name IN VARCHAR2, canon_name OUT VARCHAR2, canon_len IN BINARY_INTEGER);
    PROCEDURE COMMA_TO_TABLE(list IN VARCHAR2, tablen OUT BINARY_INTEGER, tab OUT VARCHAR2[]);
    PROCEDURE COMPILE_SCHEMA(schema IN VARCHAR2, compile_all IN BOOLEAN DEFAULT TRUE, reuse_settins IN BOOLEAN DEFAULT FALSE);
    PROCEDURE CREATE_ALTER_TYPE_ERROR_TABLE(schema_name IN VARCHAR2, table_name IN VARCHAR2);
    PROCEDURE DB_VERSION(version OUT VARCHAR2, compatibility OUT VARCHAR2);
    PROCEDURE EXEC_DDL_STATEMENT(parse_string IN VARCHAR2);
    PROCEDURE EXPAND_SQL_TEXT(input_sql_text IN VARCHAR2, output_sql_text OUT VARCHAR2);
    PROCEDURE GET_DEPENDENCY(object_type IN VARCHAR2, object_owner IN VARCHAR2, object_name IN VARCHAR2);
    PROCEDURE INVALIDATE(p_object_id NUMBER, p_plsql_object_settings VARCHAR2 DEFAULT NULL, p_option_flags INTEGER DEFAULT 0);
    PROCEDURE NAME_RESOLVE(name IN VARCHAR2, context IN NUMBER, schema OUT VARCHAR2, part1 OUT VARCHAR2,
                            part2 OUT VARCHAR2, dblink OUT VARCHAR2, part1_type OUT NUMBER, object_number OUT NUMBER);
    PROCEDURE NAME_TOKENIZE(name IN VARCHAR2, a OUT VARCHAR2, b OUT VARCHAR2, c OUT VARCHAR2, dblink OUT VARCHAR2, nextpos OUT BINARY_INTEGER);
    PROCEDURE TABLE_TO_COMMA(tab IN VARCHAR2[], tablen OUT BINARY_INTEGER, list OUT VARCHAR2);
    PROCEDURE VALIDATE(object_id NUMBER);
    PROCEDURE VALIDATE(owner VARCHAR2, objname VARCHAR2, namespace NUMBER, edition_name VARCHAR2 DEFAULT NULL);

    FUNCTION CURRENT_INSTANCE RETURN NUMBER;
    FUNCTION FORMAT_CALL_STACK RETURN VARCHAR2;
    FUNCTION FORMAT_ERROR_BACKTRACE RETURN VARCHAR2;
    FUNCTION FORMAT_ERROR_STACK RETURN VARCHAR2;
    FUNCTION GET_CPU_TIME RETURN NUMBER;
    FUNCTION GET_ENDIANNESS RETURN NUMBER;
    FUNCTION GET_HASH_VALUE(name VARCHAR2, base NUMBER, hash_size NUMBER) RETURN NUMBER;
    FUNCTION GET_PARAMETER_VALUE(parnam IN VARCHAR2, intval IN OUT BINARY_INTEGER, strval IN OUT VARCHAR2, listno IN BINARY_INTEGER DEFAULT 1) RETURN BINARY_INTEGER;
    FUNCTION GET_SQL_HASH(name IN VARCHAR2, hash OUT RAW, pre10ihash OUT NUMBER) RETURN NUMBER;
    FUNCTION GET_TIME RETURN NUMBER;
    FUNCTION IS_BIT_SET(r IN RAW, n IN NUMBER) RETURN NUMBER;
    FUNCTION IS_CLUSTER_DATABASE RETURN BOOLEAN;
    FUNCTION OLD_CURRENT_SCHEMA RETURN VARCHAR2;
    FUNCTION OLD_CURRENT_USER RETURN VARCHAR2;
    FUNCTION PORT_STRING RETURN VARCHAR2;
    FUNCTION SQLID_TO_SQLHASH(sql_id IN VARCHAR2) RETURN NUMBER;
    FUNCTION WAIT_ON_PENDING_DML(tables IN VARCHAR2, timeout IN BINARY_INTEGER, txnid IN OUT INTEGER) RETURN BOOLEAN;

    -- Begin - ReqID: OA-2455
    PROCEDURE COMMA_TO_TABLE(list IN VARCHAR2, tablen OUT BINARY_INTEGER, tab OUT LNAME_ARRAY);
    PROCEDURE TABLE_TO_COMMA(tab IN LNAME_ARRAY, tablen OUT BINARY_INTEGER, list OUT VARCHAR2);
    -- End - ReqID: OA-2455
END;


CREATE OR REPLACE PACKAGE BODY SYS.DBMS_UTILITY IS
    /*-----------------------------------sub procudure-----------------------------------*/
    PROCEDURE ANALYZE_DATABASE(method IN VARCHAR2 DEFAULT NULL, estimate_rows IN NUMBER DEFAULT NULL,
                            estimate_percent IN NUMBER DEFAULT NULL, method_opt IN VARCHAR2 DEFAULT NULL) AS
    DECLARE
        tup RECORD;
    BEGIN
        FOR tup IN
            SELECT oid, relname, relowner FROM pg_class WHERE oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer AND relkind IN ('r', 'p')
        LOOP
            IF tup.relowner = current_user::regrole THEN
                EXECUTE FORMAT('ANALYZE %I', tup.relname);
            ELSE
                RAISE NOTICE 'You have insufficient privileges for object "%" in this database, skip it.', tup.relname;
            END IF;
        END LOOP;
    END;

    PROCEDURE ANALYZE_PART_OBJECT(schema IN VARCHAR2 DEFAULT NULL, object_name IN VARCHAR2 DEFAULT NULL,
                                object_type IN CHAR DEFAULT 'T', command_type IN CHAR DEFAULT 'E',
                                command_opt IN VARCHAR2 DEFAULT NULL, sample_clause IN VARCHAR2 DEFAULT 'sample 5 percent') AS
    DECLARE
        obj_type CHAR;
        tabowner VARCHAR2;
        object_schema VARCHAR2;
        schName VARCHAR2 := ORA_CASE_TRANS(schema);
        tabName VARCHAR2 := ORA_CASE_TRANS(object_name);
    BEGIN
        IF tabName IS NOT NULL THEN
            IF (NOT EXISTS(SELECT 1 FROM pg_class WHERE relname = tabName)) THEN
                RAISE EXCEPTION 'Object does not exist or is not partitioned: %.%', schName, tabName;
            ELSE
                IF schName IS NULL THEN
                    object_schema := current_schema;
                ELSE
                    object_schema := schName;
                END IF;

                IF (NOT EXISTS(SELECT 1 FROM pg_ora_part_tables WHERE partrelid =
                        (SELECT oid FROM pg_class WHERE relname = tabName AND relnamespace = (SELECT oid FROM pg_namespace WHERE nspname = object_schema))))
                    AND ((NOT EXISTS(SELECT 1 FROM pg_tables WHERE tablename = tabName AND schemaname = object_schema)) OR
                        (NOT EXISTS(SELECT 1 FROM pg_class WHERE relname = tabName AND relkind = 'p'))) THEN
                    RAISE EXCEPTION 'Object does not exist or is not partitioned: %.%', object_schema, tabName;
                END IF;

                SELECT tableowner INTO tabowner FROM pg_tables WHERE tablename = tabName AND schemaname = object_schema;

                IF tabowner = current_user THEN
                    EXECUTE FORMAT('ANALYZE %I.%I', object_schema, tabName);
                ELSE
                    RAISE EXCEPTION 'insufficient privileges';
                END IF;
            END IF;
        ELSE
            RAISE EXCEPTION 'Object does not exist or is not partitioned';
        END IF;
    END;

    PROCEDURE ANALYZE_SCHEMA(schema IN VARCHAR2, method IN VARCHAR2 DEFAULT NULL, estimate_rows IN NUMBER DEFAULT NULL,
                            estimate_percent IN NUMBER DEFAULT NULL, method_opt IN VARCHAR2 DEFAULT NULL) AS
    DECLARE
        tup RECORD;
        schName VARCHAR2 := ORA_CASE_TRANS(schema);
    BEGIN
        IF NOT EXISTS(SELECT 1 FROM pg_namespace WHERE nspname = schName) THEN      -- BUG #z7926
            RAISE EXCEPTION 'invalid schema: %', schName;
        END IF;

        /* Begin - BUG #z7923 */
         FOR tup IN
            SELECT oid, relname, relowner FROM pg_class WHERE oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer AND relkind IN ('r', 'p')
                                            AND relnamespace = (SELECT oid FROM pg_namespace WHERE nspname = ORA_CASE_TRANS(schema))      -- BUG #z7926
        LOOP
            /* End - BUG #z7923 */
            IF PG_GET_USERBYID(tup.relowner) = current_user THEN
                EXECUTE FORMAT('ANALYZE %I.%I', schName, tup.relname);
            ELSE
                RAISE NOTICE 'You have insufficient privileges for object "%.%" in this schema, skip it.', schName, tup.relname;
            END IF;
        END LOOP;
    END;

    PROCEDURE CANONICALIZE(name IN VARCHAR2, canon_name OUT VARCHAR2, canon_len IN BINARY_INTEGER) AS
    DECLARE
        parts TEXT[];
        res TEXT;
        part TEXT;
        i INT;
        is_single BOOLEAN;
    BEGIN
        -- 1. Handle edge cases
        IF name IS NULL THEN
            canon_name := NULL;
            RETURN;
        END IF;

        IF TRIM(name) IS NULL THEN
            canon_name := name;
            RETURN;
        END IF;

        IF name = '""' THEN
            RAISE EXCEPTION 'illegal zero-length identifier';
        END IF;

        -- Begin - BUG #z7925
        IF name !~ '^\s*[a-zA-Z]' AND name !~ '^\s*"' THEN
            RAISE EXCEPTION 'unexpected name string "%"', name;
        END IF;
        -- End - BUG #z7925

        -- ------------------------------------------------------------------
        -- 2. Handle whole quoted identifier (e.g., "MySchema.MyTable", "Hello" )
        -- "MySchema"."MyTable" is not include here
        -- ------------------------------------------------------------------
        IF name ~ '^"[^"]*"$' THEN
            -- Extract inner content (remove surrounding quotes)
            res := substring(name FROM 2 FOR length(name) - 2);
            IF res = '' THEN
                RAISE EXCEPTION 'Empty quoted identifier';
            END IF;
            canon_name := (left(res, canon_len))::VARCHAR2;
            RETURN;
        END IF;

        -- ------------------------------------------------------------------
        -- 3. Split unquoted or qualified name by '.' into components
        -- ------------------------------------------------------------------
        parts := string_to_array(name, '.');

        -- Defensive check: string_to_array should not return NULL for non-NULL input,
        -- but guard against unexpected edge cases.
        IF parts IS NULL THEN
            RAISE EXCEPTION 'Empty component in qualified name';
        END IF;

        is_single := (array_length(parts, 1) = 1);

        -- ------------------------------------------------------------------
        -- 4. Process each component individually
        -- ------------------------------------------------------------------
        FOR i IN 1..array_length(parts, 1) LOOP
            part := parts[i];

            -- Case A: Component is quoted (e.g., "MyCol")
            IF part ~ '^"[^"]*"$' THEN
                part := substring(part FROM 2 FOR length(part) - 2);  -- Unquote
                IF part = '' THEN
                    RAISE EXCEPTION 'Empty quoted identifier';
                END IF;
                -- Quoted components retain original case and content (no uppercasing)

            -- Case B: Component is unquoted
            ELSE
                -- Check for forbidden characters: space or comma
                IF part ~ '[ ,]' THEN
                    -- Subcase: Pure whitespace (e.g., '   ')
                    IF part ~ '^[ ]+$' THEN
                        -- Oracle allows pure spaces only in single-component identifiers
                        IF NOT is_single THEN
                            RAISE EXCEPTION 'missing period';
                        END IF;
                        -- Keep as-is (do NOT uppercase)
                    ELSE
                        -- Contains comma or non-whitespace space --> always invalid
                        IF position(',' IN part) > 0 THEN
                            RAISE EXCEPTION 'unexpected name string "%"', name;
                        ELSE
                            RAISE EXCEPTION 'missing period';
                        END IF;
                    END IF;
                ELSE
                    -- Valid unquoted identifier: convert to uppercase (Oracle default)
                    part := UPPER(part);
                END IF;
            END IF;

            -- Store processed component back
            parts[i] := part;
        END LOOP;

        -- ------------------------------------------------------------------
        -- 5. Final validation: reject empty components in qualified names
        --    (e.g., 'a..b' -> ['a', '', 'b'] --> '' is invalid in multi-part context)
        -- ------------------------------------------------------------------
        IF NOT is_single THEN
            FOR i IN 1..array_length(parts, 1) LOOP
                IF (length(parts[i]) < 1) THEN
                    RAISE EXCEPTION 'missing identifier';
                END IF;
            END LOOP;
        END IF;

        -- ------------------------------------------------------------------
        -- 6. Reassemble result
        -- ------------------------------------------------------------------
        IF is_single THEN
            -- Single-component: return as-is (already processed)
            canon_name := (left(parts[1], canon_len))::VARCHAR2;
        ELSE
            -- Multi-component: quote each part and join with '.'
            FOR i IN 1..array_length(parts, 1) LOOP
                parts[i] := '"' || parts[i] || '"';
            END LOOP;
            res := array_to_string(parts, '.');
            canon_name := (left(res, canon_len))::VARCHAR2;
        END IF;
    END;

    PROCEDURE TABLE_TO_COMMA(tab IN VARCHAR2[], tablen OUT BINARY_INTEGER, list OUT VARCHAR2) AS
    DECLARE
        i integer;
        n integer;
    BEGIN
        tablen := 0;
        list := '';
        n := array_length(tab, 1);
        IF n IS NULL THEN
            RETURN;
        END IF;

        FOR i IN 1..n LOOP
            IF tab[i] IS NULL THEN
                EXIT;
            END IF;
            tablen := tablen + 1;
            IF i = 1 THEN
                list := tab[1];
            ELSE
                list := list || ',' || tab[i];
            END IF;
        END LOOP;
    END;

    PROCEDURE COMPILE_SCHEMA(schema IN VARCHAR2, compile_all IN BOOLEAN DEFAULT TRUE, reuse_settins IN BOOLEAN DEFAULT FALSE) AS
    DECLARE
        ret BOOLEAN;
        schema_id OID;
        tup RECORD;
        schName VARCHAR2 := ORA_CASE_TRANS(schema);     -- BUG #z7923
    BEGIN
        -- Begin - BUG #z7923
        IF ((schName IS NULL) OR (NOT EXISTS(SELECT 1 FROM pg_namespace WHERE nspname = schName))) THEN
            RAISE EXCEPTION 'invalid schema';
        ELSE
            SELECT oid INTO schema_id FROM pg_namespace WHERE nspname = schName;
            -- 1. handle FUNCTION
            FOR tup IN (SELECT objname FROM pg_object WHERE objkind = 'f' AND schemaoid = schema_id AND obj_oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer) LOOP
                EXECUTE FORMAT('ALTER FUNCTION %I.%I COMPILE', schName, tup.objname);
            END LOOP;
            -- 2. handle PROCEDURE
            FOR tup IN (SELECT objname FROM pg_object WHERE objkind = 'p' AND schemaoid = schema_id AND obj_oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer) LOOP
                EXECUTE FORMAT('ALTER PROCEDURE %I.%I COMPILE', schName, tup.objname);
            END LOOP;
            -- 3. handle FUNCTION
            FOR tup IN (SELECT objname FROM pg_object WHERE objkind = 't' AND obj_oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer) LOOP
                EXECUTE FORMAT('ALTER TRIGGER %I.%I COMPILE', schName, tup.objname);
            END LOOP;
            -- 4. handle PACKAGE
            FOR tup IN (SELECT pkgname FROM pg_package WHERE pkgnamespace = schema_id AND oid::integer >= (get_macro_value('FirstNormalObjectId'))::integer) LOOP
                EXECUTE FORMAT('ALTER PACKAGE %I.%I COMPILE', schName, tup.pkgname);
            END LOOP;
        END IF;
        -- End - BUG #z7923
    END;

    PROCEDURE CREATE_ALTER_TYPE_ERROR_TABLE(schema_name IN VARCHAR2, table_name IN VARCHAR2) AS
    DECLARE
        -- Begin - BUG #z7922
        schName VARCHAR2 := ORA_CASE_TRANS(schema_name);
        tabName VARCHAR2 := ORA_CASE_TRANS(table_name);
    BEGIN
        IF NOT EXISTS(SELECT 1 FROM pg_namespace WHERE nspname = schName) THEN
            RAISE EXCEPTION 'schema [%] does not exist', schName;
        END IF;
        IF EXISTS(SELECT 1 FROM pg_class WHERE relname = tabName and relnamespace = (SELECT oid FROM pg_namespace WHERE nspname = schName)) THEN
            RAISE EXCEPTION 'name [%] is already used by an existing object', tabName;
        END IF;
        EXECUTE FORMAT('CREATE TABLE %I.%I(owner VARCHAR2, object_name VARCHAR2, sequence# NUMBER, text_length NUMBER, error_text VARCHAR2)', schName, tabName);
        -- End -  BUG #z7922
    END;

    PROCEDURE DB_VERSION(version OUT VARCHAR2, compatibility OUT VARCHAR2) AS
    BEGIN
        version := hgdb_version();
        compatibility := current_setting('ivorysql.compatible_mode');
    END;

    PROCEDURE EXEC_DDL_STATEMENT(parse_string IN VARCHAR2) AS
    DECLARE
        ret BOOLEAN;
        cmd VARCHAR2;
    BEGIN
        cmd := trim(parse_string);
        IF cmd = '' OR cmd IS NULL THEN
            RAISE EXCEPTION 'invalid SQL statement';
        END IF;
        ret := dbmsutility_exec_ddl_statement(cmd);
        IF ret THEN
            EXECUTE cmd;
        END If;
    END;

    PROCEDURE EXPAND_SQL_TEXT(input_sql_text IN VARCHAR2, output_sql_text OUT VARCHAR2) AS
    BEGIN
        IF position(';' IN input_sql_text) > 0 THEN
            RAISE EXCEPTION 'SQL command not properly ended';
        END IF;
        IF LENGTH(input_sql_text) > 1 THEN
            output_sql_text := sys.dbmsutility_expand_sql_text(input_sql_text);
        ELSE
            output_sql_text := NULL;
        END IF;
    END;

    PROCEDURE GET_DEPENDENCY(object_type IN VARCHAR2, object_owner IN VARCHAR2, object_name IN VARCHAR2) AS
    DECLARE
        header TEXT;
        rec RECORD;
        visited TEXT[];
        current_key TEXT;
    BEGIN
        header := 'DEPENDENCIES ON ' || UPPER(QUOTE_IDENT(object_owner)) || '.' || QUOTE_IDENT(object_name);
        DBMS_OUTPUT.PUT_LINE(header);
        DBMS_OUTPUT.PUT_LINE(REPEAT('-', LENGTH(header)));
        visited := ARRAY[]::TEXT[];
        DBMS_OUTPUT.PUT_LINE(FORMAT('*%s    %s.%s()', UPPER(object_type), object_owner, UPPER(object_name)));

        FOR rec IN (
            WITH RECURSIVE dep_tree AS (
                SELECT
                    d.owner, d.name, d.type, 1 AS level, ARRAY[ROW(d.type, d.owner, d.name)::TEXT] AS path
                FROM all_dependencies d
                WHERE
                    UPPER(d.referenced_type) = UPPER(object_type)
                    AND d.referenced_owner = object_owner
                    AND d.referenced_name = object_name

                UNION ALL

                SELECT
                    d.owner, d.name, d.type, dt.level + 1, dt.path || ROW(d.type, d.owner, d.name)::TEXT
                FROM all_dependencies d
                JOIN dep_tree dt ON
                    d.referenced_owner = dt.owner
                    AND d.referenced_name = dt.name
                    AND d.referenced_type = dt.type
                WHERE
                    dt.level < 20
                    AND NOT ROW(d.type, d.owner, d.name)::TEXT = ANY(dt.path)
            )
            SELECT DISTINCT
                owner, name, type, min(level) AS min_level
            FROM dep_tree
            GROUP BY owner, name, type
            ORDER BY min_level, type, owner, name
        ) LOOP
            current_key := rec.type || '|' || rec.owner || '|' || rec.name;
            IF NOT current_key = ANY(visited) THEN
                visited := visited || current_key;
                DBMS_OUTPUT.PUT_LINE(FORMAT('*%s%s    %s.%s()', REPEAT(' ', (rec.min_level) * 4), rec.type, rec.owner, rec.name));
            END IF;
        END LOOP;
    END;

    PROCEDURE INVALIDATE(p_object_id NUMBER, p_plsql_object_settings VARCHAR2 DEFAULT NULL, p_option_flags INTEGER DEFAULT 0) AS
    DECLARE
        tup RECORD;
        objName VARCHAR2;
    BEGIN
        IF NOT EXISTS(SELECT 1 FROM pg_proc WHERE oid = p_object_id) THEN
            RAISE EXCEPTION 'object id argument passed to DBMS_UTILITY.INVALIDATE is not legal, object id: %', p_object_id;
        END IF;

        IF p_object_id::integer < (get_macro_value('FirstNormalObjectId'))::integer THEN
            RAISE NOTICE 'Not support to invalidate system object';
            RETURN;
        END IF;

        FOR tup IN (SELECT proname, prokind FROM pg_proc WHERE oid = p_object_id) LOOP
            IF LOWER(tup.proname) = tup.proname THEN
                objName := tup.proname;
            ELSE
                objName = '"' || tup.proname || '"';
            END IF;
            IF tup.prokind = 'p' THEN
                EXECUTE FORMAT('ALTER PROCEDURE %s INVALIDATE', objName);
            ELSE
                EXECUTE FORMAT('ALTER FUNCTION %s INVALIDATE', objName);
            END IF;
        END LOOP;
    END;

    PROCEDURE NAME_RESOLVE(name IN VARCHAR2, context IN NUMBER, schema OUT VARCHAR2, part1 OUT VARCHAR2,
                            part2 OUT VARCHAR2, dblink OUT VARCHAR2, part1_type OUT NUMBER, object_number OUT NUMBER) AS
    DECLARE
        v_full_name VARCHAR2 := trim(name);
        v_dblink_pos INT;
        v_obj_part VARCHAR2;
        v_schema_part VARCHAR2 := NULL;
        v_parts  VARCHAR[];
        rec RECORD;
    BEGIN
        schema := NULL;
        part1 := NULL;
        part2 := NULL;
        dblink := NULL;
        part1_type := NULL;
        object_number := NULL;

        -- extract dblink
        v_dblink_pos := position('@' IN v_full_name);
        IF v_dblink_pos > 0 THEN
            dblink := substring(v_full_name FROM v_dblink_pos + 1);
            v_obj_part := left(v_full_name, v_dblink_pos - 1);
        ELSE
            v_obj_part := v_full_name;
        END IF;

        v_obj_part := lower(v_obj_part);

        -- split schema_name and object name
        IF position('.' IN v_obj_part) > 0 THEN
            v_parts := string_to_array(v_obj_part, '.');
            v_schema_part := v_parts[1];
            part1 := v_parts[2];
            IF array_length(v_parts, 1) > 3 THEN
                RAISE EXCEPTION 'invalid name';
            END IF;
            IF array_length(v_parts, 1) = 3 THEN
                part2 := v_parts[3];
            END IF;
        ELSE
            v_schema_part := current_schema;
            part1 := v_obj_part;
        END IF;

        IF context = 0 THEN     -- TABLE, [MATERIALIZED] VIEW
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    c.relname AS obj_name,
                    c.oid AS obj_oid,
                    c.relkind
                FROM pg_class c
                JOIN pg_namespace n ON c.relnamespace = n.oid
                WHERE c.relname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part) AND c.relkind <> 'S'
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                IF NOT (HAS_TABLE_PRIVILEGE(rec.obj_oid, 'SELECT')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid,'INSERT')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'UPDATE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'DELETE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'TRUNCATE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'TRIGGER')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'REFERENCES')) THEN
                    RAISE EXCEPTION 'insufficient privileges';
                END IF;
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                CASE rec.relkind
                    WHEN 'r' THEN part1_type := 2;      -- TABLE
                    WHEN 'v' THEN part1_type := 4;      -- VIEW
                    WHEN 'm' THEN part1_type := 4;      -- MATERIALIZED VIEW
                    ELSE part1_type := -1;
                END CASE;
                RETURN;
            END LOOP;
        ELSIF context = 1 THEN          -- PLSQL
            IF v_parts IS NULL THEN
                FOR rec IN          -- PROCEDURE or FUNCTION
                    SELECT
                        n.nspname AS schema_name,
                        proc.proname AS obj_name,
                        proc.oid AS obj_oid,
                        proc.prokind
                    FROM pg_proc proc
                    JOIN pg_namespace n ON proc.pronamespace = n.oid
                    WHERE proc.proname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_FUNCTION_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part1 := NULL;
                    part2 := rec.obj_name;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    CASE rec.prokind
                        WHEN 'p' THEN part1_type := 7;      -- PROCEDURE
                        WHEN 'f' THEN part1_type := 8;      -- FUNCTION
                    END CASE;
                    RETURN;
                END LOOP;

                FOR rec IN      -- PACKAGE
                    SELECT
                        n.nspname AS schema_name,
                        pkg.pkgname AS obj_name,
                        pkg.oid AS obj_oid
                    FROM pg_package pkg
                    JOIN pg_namespace n ON pkg.pkgnamespace = n.oid
                    WHERE pkg.pkgname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_PACKAGE_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part1 := rec.obj_name;
                    part2 := NULL;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    part1_type := 9;        -- PACKAGE
                    RETURN;
                END LOOP;
            ELSIF array_length(v_parts, 1) = 2 THEN
                FOR rec IN          -- case1: schema.function
                    SELECT
                        n.nspname AS schema_name,
                        proc.proname AS obj_name,
                        proc.oid AS obj_oid,
                        proc.prokind
                    FROM pg_proc proc
                    JOIN pg_namespace n ON proc.pronamespace = n.oid
                    WHERE proc.proname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_FUNCTION_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part1 := NULL;
                    part2 := rec.obj_name;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    CASE rec.prokind
                        WHEN 'p' THEN part1_type := 7;      -- procedure
                        WHEN 'f' THEN part1_type := 8;      -- function
                    END CASE;
                    RETURN;
                END LOOP;

                FOR rec IN      -- case2: pkg.subfunction
                    SELECT
                        n.nspname AS schema_name,
                        pkg.pkgname AS obj_name,
                        pkg.oid AS obj_oid
                    FROM pg_package pkg
                    JOIN pg_namespace n ON pkg.pkgnamespace = n.oid
                    WHERE pkg.pkgname = v_schema_part
                            AND EXISTS(SELECT 1 FROM pg_package, LATERAL regexp_split_to_table(pkgsrc, ';') AS stmt WHERE pkgname = v_schema_part AND
                                        stmt ~* ('\m(FUNCTION|PROCEDURE)\s+' || part1 || '\M'))
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_PACKAGE_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part2 := part1;
                    part1 := rec.obj_name;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    part1_type := 9;        -- package
                    RETURN;
                END LOOP;

                FOR rec IN      -- case3: schema.package
                    SELECT
                        n.nspname AS schema_name,
                        pkg.pkgname AS obj_name,
                        pkg.oid AS obj_oid
                    FROM pg_package pkg
                    JOIN pg_namespace n ON pkg.pkgnamespace = n.oid
                    WHERE pkg.pkgname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_PACKAGE_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part1 := rec.obj_name;
                    part2 := NULL;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    part1_type := 9;        -- package
                    RETURN;
                END LOOP;

            ELSE            -- schema.pkg.subfunc
                FOR rec IN      -- subfunction, subprocedure in package
                    SELECT
                        n.nspname AS schema_name,
                        pkg.pkgname AS obj_name,
                        pkg.oid AS obj_oid
                    FROM pg_package pkg
                    JOIN pg_namespace n ON pkg.pkgnamespace = n.oid
                    WHERE pkg.pkgname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                            AND EXISTS(SELECT 1 FROM pg_package, LATERAL regexp_split_to_table(pkgsrc, ';') AS stmt WHERE pkgname = part1 AND
                                        stmt ~* ('\m(FUNCTION|PROCEDURE)\s+' || part2 || '\M'))
                    ORDER BY
                        CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                        array_position(current_schemas(true), n.nspname)
                    LIMIT 1
                LOOP
                    IF NOT HAS_PACKAGE_PRIVILEGE(rec.obj_oid, 'EXECUTE') THEN
                        RAISE EXCEPTION 'insufficient privileges';
                    END IF;
                    schema := rec.schema_name;
                    part1 := rec.obj_name;
                    object_number := rec.obj_oid::TEXT::NUMBER;
                    part1_type := 9;        -- package
                    RETURN;
                END LOOP;
            END IF;
        ELSIF context = 2 THEN          -- SEQUENCE
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    c.relname AS obj_name,
                    c.oid AS obj_oid,
                    c.relkind
                FROM pg_class c
                JOIN pg_namespace n ON c.relnamespace = n.oid
                WHERE c.relname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part) AND c.relkind = 'S'
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                IF NOT (HAS_TABLE_PRIVILEGE(rec.obj_oid, 'SELECT')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid,'INSERT')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'UPDATE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'DELETE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'TRUNCATE')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'TRIGGER')
                        OR HAS_TABLE_PRIVILEGE(rec.obj_oid, 'REFERENCES')) THEN
                    RAISE EXCEPTION 'insufficient privileges';
                END IF;
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                part1_type := 6;
                RETURN;
            END LOOP;
        ELSIF context = 3 THEN          -- TRIGGER
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    tg.tgname AS obj_name,
                    tg.oid AS obj_oid
                FROM pg_trigger tg
                JOIN pg_namespace n ON n.oid = (SELECT relnamespace FROM pg_class WHERE oid = tg.tgrelid)
                WHERE tg.tgname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                part1_type := 12;
                RETURN;
            END LOOP;
        ELSIF context = 4 THEN          -- SYNONYM
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    syn.synname AS obj_name,
                    syn.oid AS obj_oid
                FROM pg_synonym syn
                JOIN pg_namespace n ON syn.synnamespace = n.oid
                WHERE syn.synname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                part1_type := 5;
                RETURN;
            END LOOP;
        ELSIF context = 7 THEN          -- TYPE
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    typ.typname AS obj_name,
                    typ.oid AS obj_oid
                FROM pg_type typ
                JOIN pg_namespace n ON typ.typnamespace = n.oid
                WHERE typ.typname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part)
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                IF NOT HAS_TYPE_PRIVILEGE(rec.obj_oid, 'USAGE') THEN
                    RAISE EXCEPTION 'insufficient privileges';
                END IF;
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                part1_type := 13;
                RETURN;
            END LOOP;
        ELSIF context = 9 THEN          -- INDEX
            FOR rec IN
                SELECT
                    n.nspname AS schema_name,
                    c.relname AS obj_name,
                    c.oid AS obj_oid,
                    c.relkind
                FROM pg_class c
                JOIN pg_namespace n ON c.relnamespace = n.oid
                WHERE c.relname = part1 AND (v_schema_part IS NULL OR n.nspname = v_schema_part) AND c.relkind = 'i'
                ORDER BY
                    CASE WHEN v_schema_part IS NOT NULL THEN 0 ELSE 1 END,
                    array_position(current_schemas(true), n.nspname)
                LIMIT 1
            LOOP
                schema := rec.schema_name;
                part1 := rec.obj_name;
                object_number := rec.obj_oid::TEXT::NUMBER;
                part1_type := 1;
                RETURN;
            END LOOP;
        ELSE
            RAISE EXCEPTION 'context argument must be integral, valid only in [0,1,2,3,4,7,9]';
        END IF;
        RAISE EXCEPTION 'object % does not exists for context %', name, context;
    END;

    PROCEDURE NAME_TOKENIZE(name IN VARCHAR2, a OUT VARCHAR2, b OUT VARCHAR2, c OUT VARCHAR2, dblink OUT VARCHAR2, nextpos OUT BINARY_INTEGER) AS
    DECLARE
        v_name VARCHAR2;
        v_at_pos INT;
        v_obj_part VARCHAR2;
        v_dblink VARCHAR2;
        v_parts VARCHAR2[];
        part VARCHAR2;
    BEGIN
        a := NULL;
        b := NULL;
        c := NULL;
        dblink := NULL;
        nextpos := 0;

        IF name IS NULL OR TRIM(name) = '' THEN
            nextpos := 1;
            RETURN;
        END IF;

        v_name := TRIM(name);

        -- 1. split dblink
        v_at_pos := LENGTH(v_name) - POSITION('@' IN REVERSE(v_name)) + 1;
        IF v_at_pos > LENGTH(v_name) OR v_at_pos <= 0 THEN
            v_obj_part := v_name;
            v_dblink := NULL;
        ELSE
            v_dblink := SUBSTRING(v_name FROM v_at_pos + 1);
            v_obj_part := SUBSTRING(v_name FROM 1 FOR v_at_pos - 1);

            IF v_dblink IS NOT NULL AND (LENGTH(v_dblink) >= 2) AND
                LEFT(TRIM(v_dblink), 1) = '"' AND RIGHT(TRIM(v_dblink), 1) = '"' THEN
                dblink := SUBSTRING(TRIM(v_dblink) FROM 2 FOR LENGTH(TRIM(v_dblink)) - 2);
            ELSE
                dblink := UPPER(TRIM(v_dblink));
            END IF;
        END IF;

        -- 2. split a.b.c
        v_obj_part := TRIM(v_obj_part);
        IF v_obj_part = '' THEN
            -- do nothing
        ELSE
            v_parts := ARRAY[]::VARCHAR2[];
            FOR part IN SELECT TRIM(unnest_elem) FROM UNNEST(STRING_TO_ARRAY(v_obj_part, '.')) AS unnest_elem LOOP
                IF part IS NOT NULL THEN
                    v_parts := ARRAY_APPEND(v_parts, part);
                END IF;
            END LOOP;

            -- get a
            IF ARRAY_LENGTH(v_parts, 1) >= 1 THEN
                part := v_parts[1];
                IF LENGTH(part) >= 2 AND (LEFT(part, 1) = '"') AND (RIGHT(part, 1) = '"') THEN
                    a := SUBSTRING(part FROM 2 FOR LENGTH(part) - 2);
                ELSE
                    a := UPPER(part);
                END IF;
            END IF;

            -- get b
            IF ARRAY_LENGTH(v_parts, 1) >= 2 THEN
                part := v_parts[2];
                IF LENGTH(part) >= 2 AND (LEFT(part, 1) = '"') AND (RIGHT(part, 1) = '"') THEN
                    b := SUBSTRING(part FROM 2 FOR LENGTH(part) - 2);
                ELSE
                    b := UPPER(part);
                END IF;
            END IF;

            --get c
            IF ARRAY_LENGTH(v_parts, 1) >= 3 THEN
                part := v_parts[3];
                IF LENGTH(part) >= 2 AND (LEFT(part, 1) = '"') AND (RIGHT(part, 1) = '"') THEN
                    c := SUBSTRING(part FROM 2 FOR LENGTH(part) - 2);
                ELSE
                    c := UPPER(part);
                END IF;
            END IF;
        END IF;
        nextpos := LENGTH(name);
    END;

    PROCEDURE COMMA_TO_TABLE(list IN VARCHAR2, tablen OUT BINARY_INTEGER, tab OUT VARCHAR2[]) AS
    BEGIN
        IF list IS NULL OR trim(list) = '' THEN
            RAISE EXCEPTION 'comma-separated list invalid';     -- BUG #z7924
        END IF;

        SELECT
            COALESCE(array_agg(unnest)::VARCHAR2[], '{}'),
            COALESCE(COUNT(*), 0)::BINARY_INTEGER
        INTO tab, tablen
        FROM regexp_split_to_table(list, ',(?=(?:[^"]*"[^"]*")*[^"]*$)') AS unnest;

        tab := ARRAY(
            SELECT unnest FROM unnest(tab) WITH ORDINALITY AS t(unnest, idx) ORDER BY idx
        );

        IF tab IS NULL THEN
            tab := '{}';
            tablen := 0;
        END IF;
    END;

    PROCEDURE VALIDATE(object_id NUMBER) AS
    DECLARE
        tup RECORD;
        procName VARCHAR2;
    BEGIN
        IF NOT EXISTS(SELECT 1 FROM pg_proc WHERE oid = object_id) THEN
            RAISE EXCEPTION 'object id argument passed to DBMS_UTILITY.VALIDATE is not legal, object id: %', object_id;
        END IF;

        IF object_id::integer < (get_macro_value('FirstNormalObjectId'))::integer THEN
            RAISE NOTICE 'Not support to validate system object';
            RETURN;
        END IF;

        FOR tup IN (SELECT proname, prokind FROM pg_proc WHERE oid = object_id) LOOP
            IF LOWER(tup.proname) = tup.proname THEN
                procName := tup.proname;
            ELSE
                procName := '"' || tup.proname || '"';
            END IF;
            IF tup.prokind = 'p' THEN
                EXECUTE FORMAT('ALTER PROCEDURE %s COMPILE', procName);
            ELSE
                EXECUTE FORMAT('ALTER FUNCTION %s COMPILE', procName);
            END IF;
        END LOOP;
    END;

    PROCEDURE VALIDATE(owner VARCHAR2, objname VARCHAR2, namespace NUMBER, edition_name VARCHAR2 DEFAULT NULL) AS
    DECLARE
        tup RECORD;
        procName VARCHAR2 := ORA_CASE_TRANS(objname);
        -- Begin - BUG #z7789
        powner VARCHAR2 := ORA_CASE_TRANS(owner);
        schName VARCHAR2;
    BEGIN
        IF NOT EXISTS (SELECT 1 FROM pg_proc WHERE proname = procName AND pronamespace = namespace
                    AND proowner = (SELECT oid FROM pg_authid WHERE rolname = powner)) THEN
            RAISE EXCEPTION 'argument passed to DBMS_UTILITY.VALIDATE is not legal';
        END IF;

        IF (SELECT oid FROM pg_proc WHERE proname = procName AND pronamespace = namespace AND proowner = (SELECT oid FROM pg_authid WHERE rolname = powner)) 
            < (get_macro_value('FirstNormalObjectId'))::integer THEN
            RAISE NOTICE 'Not support to validate system object';
            RETURN;
        END IF;

        SELECT nspname INTO schName FROM pg_namespace WHERE oid = namespace;

        FOR tup IN (SELECT proname, prokind FROM pg_proc WHERE proname = procName AND pronamespace = namespace AND proowner = (SELECT oid FROM pg_authid WHERE rolname = powner)) LOOP
            IF LOWER(tup.proname) = tup.proname THEN
                procName := tup.proname;
            ELSE
                procName := '"' || tup.proname || '"';
            END IF;
            IF tup.prokind = 'p' THEN
                EXECUTE FORMAT('ALTER PROCEDURE %s.%s COMPILE', schName, procName);
            ELSE
                EXECUTE FORMAT('ALTER FUNCTION %s.%s COMPILE', schName, procName);
            END IF;
        END LOOP;
        -- End - BUG #z7789
    END;

    /*-----------------------------------sub function-----------------------------------*/
    FUNCTION CURRENT_INSTANCE RETURN NUMBER IS
    BEGIN
        RETURN 1::NUMBER;
    END;

    FUNCTION FORMAT_CALL_STACK RETURN VARCHAR2 IS
    DECLARE
        raw_stack TEXT;
        formatted_stack TEXT := '';
        line_record RECORD;
        func_oid OID;
        match_result TEXT[];
    BEGIN
        GET DIAGNOSTICS raw_stack := PG_CONTEXT;
        formatted_stack := FORMAT(
                            '%13s    %8s    %s',
                            'object handle',
                            'line number',
                            'object name'
                        ) || E'\n';

        IF raw_stack LIKE '%PL/iSQL function%' THEN
            FOR line_record IN
                SELECT regexp_split_to_table(raw_stack, E'\n') AS line
            LOOP
                IF line_record.line ~ 'function .* line \d+' THEN
                    match_result := regexp_match(
                        line_record.line,
                        'function ([\w\.\(\)\*]+).*line (\d+)'
                    );
                    IF match_result IS NOT NULL THEN
                        SELECT COALESCE(
                                    COALESCE
                                    (
                                        (
                                            SELECT oid FROM pg_proc WHERE proname = (SELECT TRIM(TRAILING '()' FROM SPLIT_PART(match_result[1], '.', 2))) AND
                                                pronamespace = (SELECT oid FROM pg_namespace WHERE nspname = SPLIT_PART(match_result[1], '.', 1))
                                        ),
                                        (
                                            SELECT oid FROM pg_package WHERE pkgname = SPLIT_PART(match_result[1], '.', 2) AND
                                                pkgnamespace = (SELECT oid FROM pg_namespace WHERE nspname = SPLIT_PART(match_result[1], '.', 1))
                                        )
                                    ),
                            0) INTO func_oid;
                        formatted_stack := formatted_stack || FORMAT(
                            '0x%s    %8s          %s',
                            lpad(to_hex(func_oid::bigint), 8, '0'),
                            match_result[2],
                            match_result[1]
                        ) || E'\n';
                    END IF;
                END IF;
            END LOOP;
        ELSE
            formatted_stack := '0x00000000    1    anonymous';
        END IF;
        RETURN (COALESCE(formatted_stack, ''));
    END;

    FUNCTION FORMAT_ERROR_BACKTRACE RETURN VARCHAR2 IS
    DECLARE
        target_schema TEXT := 'public';
        lines TEXT[];
        current_block TEXT := '';
        blocks TEXT[] := '{}';
        formatted_backtrace TEXT;
        raw_stack TEXT;
        match_result TEXT[];
    BEGIN
        raw_stack := sys.dbmsutility_get_error_stack();
        IF raw_stack IS NULL OR trim(raw_stack) = '' THEN
            RETURN '';
        END IF;

        lines := string_to_array(raw_stack, E'\n');
        FOR i IN 1..array_length(lines, 1) LOOP
            IF lines[i] LIKE '%ERROR MESSAGE:%' THEN
                IF current_block IS NOT NULL THEN
                    blocks := array_append(blocks, current_block);
                END IF;
                current_block := lines[i];
            ELSE
                IF current_block IS NOT NULL THEN
                    current_block := current_block || E'\n' || lines[i];
                END IF;
            END IF;
        END LOOP;

        IF current_block IS NOT NULL THEN
            blocks := array_append(blocks, current_block);
        END IF;

        FOR i IN REVERSE array_length(blocks, 1)..1 LOOP
            lines := string_to_array(blocks[i], E'\n');
            FOR j IN 1..array_length(lines, 1) LOOP
                IF lines[j] ~ 'function .* line \d+' THEN
                    match_result := regexp_match(
                        lines[j],
                        'function ([\w\.\(\)\*]+).*line (\d+)'
                    );
                    IF match_result IS NOT NULL THEN
                        formatted_backtrace := formatted_backtrace || FORMAT(
                            'at "%s", line %s',
                            match_result[1],
                            match_result[2]
                        ) || E'\n';
                    END IF;
                END IF;
            END LOOP;
        END LOOP;
        RETURN formatted_backtrace;
    END;

    FUNCTION FORMAT_ERROR_STACK RETURN VARCHAR2 IS
    DECLARE
        target_schema TEXT := 'public';
        lines TEXT[];
        current_block TEXT := '';
        blocks TEXT[] := '{}';
        formatted_stack TEXT;
        raw_stack TEXT;
        match_result TEXT[];
    BEGIN
        raw_stack := sys.dbmsutility_get_error_stack();
        IF raw_stack IS NULL OR length(raw_stack) = 0 THEN
            RETURN '';
        END IF;

        lines := string_to_array(raw_stack, E'\n');
        FOR i IN 1..array_length(lines, 1) LOOP
            IF lines[i] LIKE '%ERROR MESSAGE:%' THEN
                IF current_block IS NOT NULL THEN
                    blocks := array_append(blocks, current_block);
                END IF;
                current_block := lines[i];
            ELSE
                IF current_block IS NOT NULL THEN
                    current_block := current_block || E'\n' || lines[i];
                END IF;
            END IF;
        END LOOP;

        IF current_block IS NOT NULL THEN
            blocks := array_append(blocks, current_block);
        END IF;

        FOR i IN REVERSE array_length(blocks, 1)..1 LOOP
            lines := string_to_array(blocks[i], E'\n');
            FOR j IN 1..array_length(lines, 1) LOOP
                IF lines[j] LIKE '%ERROR MESSAGE:%' THEN
                    formatted_stack := formatted_stack || substring(lines[j] FROM 'ERROR MESSAGE:\s*(.*)') || E'\n';
                ELSE
                    IF lines[j] ~ 'function .* line \d+' THEN
                        match_result := regexp_match(
                            lines[j],
                            'function ([\w\.\(\)\*]+).*line (\d+)'
                        );
                        IF match_result IS NOT NULL THEN
                            formatted_stack := formatted_stack || FORMAT(
                                'at "%s", line %s',
                                match_result[1],
                                match_result[2]
                            ) || E'\n';
                        END IF;
                    END IF;
                END IF;
            END LOOP;
        END LOOP;
        RETURN formatted_stack;
    END;

    FUNCTION GET_CPU_TIME RETURN NUMBER IS
    DECLARE
        time NUMBER;
    BEGIN
        time := (sys.dbmsutility_get_cpu_time())::NUMBER;
        RETURN time;
    END;

    FUNCTION GET_ENDIANNESS RETURN NUMBER IS
    DECLARE
        func_ret BOOLEAN;
        ret NUMBER;
    BEGIN
        func_ret := sys.dbmsutility_get_endianness();
        IF func_ret THEN
            ret := 2::NUMBER;
        ELSE
            ret := 1::NUMBER;
        END IF;
        RETURN ret;
    END;

    FUNCTION GET_HASH_VALUE(name VARCHAR2, base NUMBER, hash_size NUMBER) RETURN NUMBER IS
    BEGIN
        RETURN ((COALESCE(hashtext(name::text), 0) + 2147483648) % GREATEST(hash_size, 1) + base);  -- BUG #z7935
    END;

    FUNCTION GET_PARAMETER_VALUE(parnam IN VARCHAR2, intval OUT BINARY_INTEGER, strval OUT VARCHAR2, listno IN BINARY_INTEGER DEFAULT 1) RETURN BINARY_INTEGER IS
    DECLARE
        param_val TEXT;
        param_type TEXT;
        ret NUMBER;
    BEGIN
        IF NOT EXISTS(SELECT 1 FROM pg_settings WHERE name = parnam) THEN
            RAISE EXCEPTION 'get_parameter_value: invalid or unsupported parameter "%"', parnam;
        END IF;
        SELECT setting, vartype INTO param_val, param_type FROM pg_settings WHERE name = parnam;
        CASE
            WHEN UPPER(param_type) = 'INTEGER' THEN
                ret := 0;
                intval := param_val::BINARY_INTEGER;
            WHEN UPPER(param_type) = 'BOOL' THEN
                ret := 0;
                IF UPPER(param_val) = 'ON' THEN
                    intval := 1::BINARY_INTEGER;
                ELSE
                    intval := 0::BINARY_INTEGER;
                END IF;
            ELSE
                ret := 1;
                intval := LENGTH(param_val)::BINARY_INTEGER;
                strval := param_val;
        END CASE;
        RETURN ret;
    END;

    FUNCTION GET_SQL_HASH(name IN VARCHAR2, hash OUT RAW, pre10ihash OUT NUMBER) RETURN NUMBER IS
    BEGIN
        hash := md5(name)::RAW;
        pre10ihash := NULL::NUMBER;
        RETURN ('x' || RIGHT(md5(name), 8))::bit(32)::bigint::number;
    END;

    FUNCTION GET_TIME RETURN NUMBER IS
    BEGIN
        RETURN floor(EXTRACT(EPOCH FROM (clock_timestamp() - pg_postmaster_start_time())*100));
    END;

    FUNCTION IS_BIT_SET(r IN RAW, n IN NUMBER) RETURN NUMBER IS
    DECLARE
        ret NUMBER;
    BEGIN
        IF n::integer < 0 OR n::integer >= length(r) * 8 THEN
            RAISE EXCEPTION 'index out of range';
        END IF;
        SELECT
            CASE
                WHEN r IN NULL THEN 0
                ELSE (get_byte(r, length(r) - 1 - (n::integer / 8)::integer) >> (n::integer % 8)) & 1
            END CASE
        INTO ret;
        RETURN ret;
    END;

    FUNCTION IS_CLUSTER_DATABASE RETURN BOOLEAN IS
    BEGIN
        RETURN FALSE;
    END;

    FUNCTION OLD_CURRENT_SCHEMA RETURN VARCHAR2 IS
    BEGIN
        RETURN current_schema;
    END;

    FUNCTION OLD_CURRENT_USER RETURN VARCHAR2 IS
    BEGIN
        RETURN current_user;
    END;

    FUNCTION PORT_STRING RETURN VARCHAR2 IS
    DECLARE
        ret VARCHAR2;
        ver VARCHAR2;
        platform_part VARCHAR2;
        v_arch VARCHAR2;
        v_os VARCHAR2;
    BEGIN
        ver := version();
        platform_part := TRIM(SUBSTRING(ver FROM 'on ([^,]+),'));
        IF platform_part IS NULL THEN
            RETURN 'unknown/unknownOS';
        END IF;

        IF platform_part LIKE '%x86_64%' OR platform_part LIKE '%amd64%' THEN v_arch := 'x86_64';
        ELSIF platform_part LIKE '%aarch64%' OR platform_part LIKE '%arm64%' THEN v_arch := 'aarch64';
        ELSIF platform_part LIKE '%i686%' OR platform_part LIKE '%i386%' THEN v_arch := 'x86';
        ELSIF platform_part LIKE '%ppc64%' THEN v_arch := 'ppc';
        ELSIF platform_part LIKE '%s390x%' THEN v_arch := 's390x';
        ELSE v_arch := 'unknown';
        END IF;

        IF platform_part LIKE '%linux%' THEN v_os := 'Linux';
        ELSIF platform_part LIKE '%darwin%' THEN v_os := 'Darwin';
        ELSIF platform_part LIKE '%win%' THEN v_os := 'Windows';
        ELSIF platform_part LIKE '%freebsd%' THEN v_os := 'FreeBSD';
        ELSIF platform_part LIKE '%openbsd%' THEN v_os := 'OpenBSD';
        ELSIF platform_part LIKE '%sunos%' THEN v_os := 'Solaris';
        ELSE v_os := 'unknownOS';
        END IF;
        ret := v_arch || '/' || v_os;
        RETURN ret;
    END;

    FUNCTION SQLID_TO_SQLHASH(sql_id IN VARCHAR2) RETURN NUMBER IS
    BEGIN
        RAISE NOTICE 'This function is not supported yet';
        RETURN 0::NUMBER;
    END;

    FUNCTION WAIT_ON_PENDING_DML(tables IN VARCHAR2, timeout IN BINARY_INTEGER, txnid IN OUT INTEGER) RETURN BOOLEAN IS
    DECLARE
        tab_list VARCHAR2[];
        tab_cnd BINARY_INTEGER;
        schema VARCHAR2;
        part1 VARCHAR2;
        part2 VARCHAR2;
        dblink VARCHAR2;
        part1_type NUMBER;
        object_number NUMBER;       -- BUG #z7936
        wait_time NUMBER := 0;
    BEGIN
        COMMA_TO_TABLE(list => tables, tablen => tab_cnd, tab => tab_list);
        FOR i IN 1..array_length(tab_list, 1) LOOP
            tab_list[i] := trim(tab_list[i]);
            NAME_RESOLVE(tab_list[i], 0, schema, part1, part2, dblink, part1_type, object_number);
        END LOOP;

        IF txnid < 0 OR txnid > txid_current() THEN
            txnid := txid_current();
        END IF;

        LOOP
            IF NOT EXISTS(
                SELECT 1 FROM pg_locks
                WHERE relation IN (SELECT oid FROM pg_class c JOIN unnest(tab_list) AS t(tabname) ON c.relname = t.tabname)
                    AND locktype = 'relation'
                    AND granted = true
                    AND pid != pg_backend_pid()
                    AND mode = 'RowExclusiveLock'
                    AND transactionid::TEXT::INTEGER < txnid
            ) THEN
                RETURN true;
            ELSE
                wait_time := wait_time + 0.5;
                PERFORM pg_sleep(0.5);
                IF wait_time >= timeout THEN
                    RETURN false;
                END IF;
            END IF;
        END LOOP;
        RETURN false;
    END;

    -- Begin - ReqID: OA-2455
    PROCEDURE COMMA_TO_TABLE(list IN VARCHAR2, tablen OUT BINARY_INTEGER, tab OUT LNAME_ARRAY) AS
    DECLARE
        tables VARCHAR2[];
    BEGIN
        IF list IS NULL OR trim(list) = '' THEN
            RAISE EXCEPTION 'comma-separated list invalid';
        END IF;

        SELECT
            COALESCE(array_agg(unnest)::VARCHAR2[], '{}'),
            COALESCE(COUNT(*), 0)::BINARY_INTEGER
        INTO tables, tablen
        FROM regexp_split_to_table(list, ',(?=(?:[^"]*"[^"]*")*[^"]*$)') AS unnest;

        tables := ARRAY(
            SELECT unnest FROM unnest(tables) WITH ORDINALITY AS t(unnest, idx) ORDER BY idx
        );

        IF tables IS NULL THEN
            tables := '{}';
            tablen := 0;
        END IF;

        tab.delete;
        FOR i IN 1 .. tablen LOOP
            tab(i) = tables[i];
        END LOOP;
    END;

    PROCEDURE TABLE_TO_COMMA(tab IN LNAME_ARRAY, tablen OUT BINARY_INTEGER, list OUT VARCHAR2) AS
    DECLARE
        v_idx INTEGER;
        v_first BOOLEAN := TRUE;
    BEGIN
        tablen := 0;
        list := '';
        tablen := tab.count;

        IF tab IS NULL OR tablen = 0 THEN
            RETURN;
        END IF;

        FOR v_idx IN 1 .. tablen LOOP
            IF v_first THEN
                list := tab(v_idx);
                v_first := FALSE;
            ELSE
                list := list || ',' || tab(v_idx);
            END IF;
        END LOOP;
    END;
    -- End - ReqID: OA-2455
END;

GRANT EXECUTE ON PACKAGE SYS.DBMS_UTILITY TO PUBLIC;

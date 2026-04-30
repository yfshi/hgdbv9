CREATE OR REPLACE NONEDITIONABLE PACKAGE SYS.DBMS_ROWID IS
	rowid_type_restricted constant integer := 0;
	rowid_tupe_extended constant integer := 1;

	rowid_object_undefined constant integer := 0;

	rowid_convert_internal constant integer := 0;
	rowid_convert_extended constant integer := 1;
	function rowid_create(rowid_type in number,
						  object_number in number,
						  relative_fno in number,
						  block_number in number,
						  row_number in number) return rowid;
	procedure rowid_info(row_id in rowid,
						 rowid_type out number,
						 object_number out number,
						 relative_fno out number,
						 block_number out number,
						 row_number out number,
						 ts_type in varchar2 default 'SMALLFILE');
	function rowid_type(row_id in rowid) return number;
	function rowid_object(row_id in rowid) return number;
	function rowid_relative_fno(row_id in rowid, ts_type in varchar2 default 'smallfile')return number;
	function rowid_block_number(row_id in rowid,ts_type in varchar2 default 'smallfile')return number;
	function rowid_row_number(row_id in rowid)return number;
	function rowid_to_absolute_fno(row_id in rowid,schema_name in varchar2, object_name in varchar2)return number;
	function rowid_to_extended(old_rowid in rowid,schema_name in varchar2,object_name in varchar2,conversion_type in integer)return rowid;
	function rowid_to_restricted(old_rowid in rowid,conversion_type in integer)return rowid;
	function rowid_verify(rowid_in in rowid,schema_name in varchar2,object_name in varchar2,conversion_type in integer)return number;
END;


CREATE OR REPLACE NONEDITIONABLE PACKAGE BODY SYS.DBMS_ROWID IS
	function rowid_create(rowid_type in number,
						  object_number in number,
						  relative_fno in number,
						  block_number in number,
						  row_number in number) return rowid
	AS
	DECLARE
		return_rowid rowid;
	BEGIN
		-- check input param validate
		IF rowid_type IS NULL THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;

		IF rowid_type != 0 AND rowid_type != 1 THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;

		--Begin BUG#Z7666
		IF object_number < 0 OR object_number != floor(object_number) THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;

		IF row_number < 0 OR row_number != floor(row_number) THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;
		--End Bug#Z7666

		--Begin BUG#Z7667
		IF object_number IS NULL OR row_number IS NULL THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;
		--End Bug#Z7667

		return_rowid.rowoid = object_number;
		return_rowid.rowno = row_number;

		RETURN return_rowid;
	END;

	procedure rowid_info(row_id in rowid,
						 rowid_type out number,
						 object_number out number,
						 relative_fno out number,
						 block_number out number,
						 row_number out number,
						 ts_type in varchar2 default 'SMALLFILE')
	AS
	DECLARE
		v_schema name;
		v_name name;
		v_sql text;
		my_ctid tid;
	BEGIN
		rowid_type := 1;
		object_number := row_id.rowoid;
		row_number := row_id.rowno;
		select n.nspname, c.relname, c.relfilenode into v_schema, v_name, relative_fno from pg_class c JOIN pg_namespace n on n.oid = c.relnamespace where c.oid = row_id.rowoid;
		IF NOT FOUND THEN
			block_number := -1;
			relative_fno := -1;
			RETURN;
		END IF;

		-- get ctid information
		v_sql := format('SELECT ctid FROM %I.%I WHERE rowid = %L', v_schema, v_name, row_id::text);
		EXECUTE v_sql INTO my_ctid;
		block_number := split_part(trim(both'()' from my_ctid::text), ',', 1)::number;
		IF block_number IS NULL THEN
			block_number := -1;
			relative_fno := -1;
		END IF;
		EXCEPTION
			WHEN OTHERS THEN
				block_number := -1;
				relative_fno := -1;
	END;

	function rowid_type(row_id in rowid) return number
	AS
	BEGIN
		RETURN 1;
	END;

	function rowid_object(row_id in rowid) return number
	AS
	BEGIN
		IF row_id IS NULL OR row_id = '' THEN
			RETURN 0;
		END IF;

		RETURN row_id.rowoid;
	END;

	function rowid_relative_fno(row_id in rowid, ts_type in varchar2 default 'smallfile') return number
	AS
	DECLARE
		relative_fno number;
	BEGIN
		IF row_id IS NULL OR row_id = '' THEN
			RETURN 0;
		END IF;

		select relfilenode into relative_fno from pg_class where oid = row_id.rowoid;
		RETURN relative_fno;
		EXCEPTION
			WHEN OTHERS THEN
				RETURN row_id.rowoid;
	END;

	function rowid_block_number(row_id in rowid,ts_type in varchar2 default 'smallfile') return number
	AS
	DECLARE
		rowid_no text;
	BEGIN
		IF row_id IS NULL OR row_id = '' THEN
			RETURN 0;
		END IF;

		EXECUTE FORMAT(
			'SELECT ctid::text FROM %s where rowid = (%s,%s)', quote_ident(row_id.rowoid::regclass::text), row_id.rowoid, row_id.rowno
		)INTO rowid_no;
		RETURN split_part(trim(both'()' from rowid_no), ',', 1)::number;
		EXCEPTION
			WHEN OTHERS THEN
				RETURN -1;
	END;

	function rowid_row_number(row_id in rowid)return number
	AS
	BEGIN
		--Begin Bug#Z7671
		IF row_id IS NULL OR row_id = '' THEN
			RETURN 0;
		END IF;
		--End Bug#Z7671

		RETURN row_id.rowno;
	END;

	function rowid_to_absolute_fno(row_id in rowid,schema_name in varchar2, object_name in varchar2) return number
	AS
	DECLARE
		table_exists BOOLEAN := FALSE;
		relative_fno number;
		v_obj_id number;
		schemename varchar2 := ora_case_trans(schema_name);
		objectname varchar2 := ora_case_trans(object_name);
		query text;
		result record;
	BEGIN
		--validate input parameters
		IF schemename IS NULL OR objectname IS NULL THEN
			RAISE EXCEPTION 'table or view does not exist';
		END IF;

		IF schemename IS NOT NULL AND objectname IS NOT NULL THEN
			SELECT EXISTS (SELECT 1 FROM information_schema.tables
				WHERE table_schema = schemename AND table_name = objectname)
				INTO table_exists;
			IF NOT table_exists THEN
				RAISE EXCEPTION 'table or view does not exist';
			END IF;
		END IF;

		--check whether the schemaname and objectname are enclosed in quotes
		IF left(schemename, 1) = '"'  OR left(objectname, 1) = '"' THEN
			RAISE EXCEPTION 'table or view does not exist';
		END IF;

		IF row_id IS NULL OR row_id = '' THEN
			RAISE EXCEPTION 'invalid ROWID';
		END IF;

		--Begin Bug#Z7679
		query := format('SELECT 1 FROM %I.%I WHERE rowid = row(%L, %L);', schemename, objectname, row_id.rowoid, row_id.rowno);
		BEGIN
			EXECUTE query into result;
		EXCEPTION
			WHEN NO_DATA_FOUND THEN
				RAISE EXCEPTION 'invalid ROWID';
		END;
		--End Bug#Z7679
		begin
			select c.relfilenode into relative_fno from pg_class c join pg_namespace n on n.oid = c.relnamespace where c.relname = objectname  and n.nspname= schemename;
			--Begin Bug#Z7692
			IF relative_fno = 0 THEN
				RAISE EXCEPTION 'invalid ROWID';
			END IF;
			--End Bug#Z7692

			RETURN relative_fno;
			EXCEPTION
				WHEN NO_DATA_FOUND THEN
					RAISE EXCEPTION 'table or view does not exist';
		end;
	END;

	function rowid_to_extended(old_rowid in rowid,schema_name in varchar2,object_name in varchar2,conversion_type in integer) return rowid
	AS
	DECLARE
		table_exists BOOLEAN := FALSE;
		schemename varchar2 := ora_case_trans(schema_name);
		objectname varchar2 := ora_case_trans(object_name);
		query text;
		result record;
	BEGIN
		IF  schemename IS NULL AND objectname IS NULL THEN
			RETURN old_rowid;
		END IF;

		IF old_rowid IS NULL OR old_rowid = '' THEN
			RETURN NULL;
		END IF;

		--validate input parameters
		IF schemename IS NOT NULL AND objectname IS NULL THEN
			RAISE EXCEPTION 'invalid table name';
		END IF;

		IF schemename IS NULL AND objectname IS NOT NULL THEN
			RAISE EXCEPTION 'invalid table name';
		END IF;

		IF schemename IS NOT NULL AND objectname IS NOT NULL THEN
			SELECT EXISTS (SELECT 1 FROM information_schema.tables
				WHERE table_schema = schemename AND table_name = objectname)
				INTO table_exists;
			IF NOT table_exists THEN
				RAISE EXCEPTION 'table or view does not exist';
			END IF;
		END IF;

		--Begin Bug#Z7668
		query := format('SELECT 1 FROM %I.%I WHERE rowid = row(%L, %L);', schemename, objectname, old_rowid.rowoid, old_rowid.rowno);
		BEGIN
			EXECUTE query into result;
		EXCEPTION
			WHEN NO_DATA_FOUND THEN
				RAISE EXCEPTION 'invalid ROWID';
		--End Bug#Z7668
		END;
		--check whether the schemaname and objectname are enclosed in quotes
		IF left(schemename, 1) = '"'  OR left(objectname, 1) = '"' THEN
			RAISE EXCEPTION 'table or view does not exist';
		END IF;

		RETURN old_rowid;
	END;

	function rowid_to_restricted(old_rowid in rowid,conversion_type in integer)return rowid
	AS
	BEGIN
		RAISE NOTICE 'There only has extended rowid';
		IF old_rowid IS NULL OR old_rowid = '' THEN
			RETURN NULL;--Bug#Z7669
		END IF;

		return old_rowid;
	END;

	function rowid_verify(rowid_in in rowid,schema_name in varchar2,object_name in varchar2,conversion_type in integer)return number
	AS
	DECLARE
		table_exists BOOLEAN := FALSE;
		exists_flag BOOLEAN := FALSE;
		tbl_name TEXT;
		row_exists BOOLEAN := FALSE;
		schemename varchar2 := ora_case_trans(schema_name);
		objectname varchar2 := ora_case_trans(object_name);
		query text;
		result record;
	BEGIN
		--Beign Bug#Z7676
		--IF schema_name AND object_name both NULL OR '',return 0
		IF schemename IS NULL AND objectname IS NULL THEN
			return 0;
		ELSIF schemename = '' AND objectname = '' THEN
			return 0;
		ELSIF (schemename IS NOT NULL AND schemename = '' AND objectname IS NULL)
			OR (schemename IS NULL AND objectname IS NOT NULL AND objectname = '') THEN
			return 0;
		END IF;
		--End Bug#Z7676

		--validate input parameters
		IF schemename IS NOT NULL AND objectname IS NULL THEN
			RAISE EXCEPTION 'invalid table name';
		END IF;

		IF schemename IS NULL AND objectname IS NOT NULL THEN
			RAISE EXCEPTION 'invalid table name';
		END IF;

		IF schemename IS NOT NULL AND objectname IS NOT NULL THEN
			SELECT EXISTS (SELECT 1 FROM information_schema.tables
				WHERE table_schema = schemename AND table_name = objectname)
				INTO table_exists;
			IF NOT table_exists THEN
				RAISE EXCEPTION 'table or view does not exist';
			END IF;
		END IF;

		--check whether the schemaname and objectname are enclosed in quotes
		IF left(schemename, 1) = '"'  OR left(objectname, 1) = '"' THEN
			RAISE EXCEPTION 'table or view does not exist';
		END IF;

		--Begin Bug#Z7674
		IF rowid_in IS NULL OR rowid_in = '' THEN
			RETURN 0;
		END IF;
		--End Bug#Z7674

		SELECT EXISTS(
			SELECT 1 FROM pg_class WHERE oid = rowid_in.rowoid AND relkind = 'r'
			) INTO exists_flag;
		IF NOT exists_flag THEN
			RETURN 1;
		END IF;

		--Begin Bug#7672&7673&7675&7677
		query := format('SELECT 1 FROM %I.%I WHERE rowid = row(%L, %L);', schemename, objectname, rowid_in.rowoid, rowid_in.rowno);
		BEGIN
			EXECUTE query into result;
		EXCEPTION
			WHEN OTHERS THEN
				RETURN 1;
		END;
		--End Bug#7672&7673&7675&7677

		RETURN 0;
	END;
END;

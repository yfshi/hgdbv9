/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
CREATE OR REPLACE PROCEDURE sys.dbmsoutput_enable(buffer_size integer default 20000)
AS 'MODULE_PATHNAME','dbms_output_enable'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_disable()
AS 'MODULE_PATHNAME','dbms_output_disable'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_put(item anyelement)
AS 'MODULE_PATHNAME','dbms_output_put'
LANGUAGE C;

/* for unkonwn type */
CREATE OR REPLACE PROCEDURE sys.dbmsoutput_put(item text)
AS 'MODULE_PATHNAME','dbms_output_put'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_putline(item anyelement)
AS 'MODULE_PATHNAME','dbms_output_put_line'
LANGUAGE C;

/* for unkonwn type */
CREATE OR REPLACE PROCEDURE sys.dbmsoutput_putline(item text)
AS 'MODULE_PATHNAME','dbms_output_put_line'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_newline()
AS 'MODULE_PATHNAME','dbms_output_new_line'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_getline(OUT line varchar2, OUT status integer)
AS 'MODULE_PATHNAME','dbms_output_get_line'
LANGUAGE C;

CREATE OR REPLACE PROCEDURE sys.dbmsoutput_getlines(OUT lines text[], IN OUT numlines integer)
AS 'MODULE_PATHNAME','dbms_output_get_lines'
LANGUAGE C;

/*
 * currently, since the collection type is not supported,
 * the GET_LINES procedure temporarily uses text[] instead.
 *
 * The advantage of anyelement is that it is more general,
 * whether it is for existing data types or types created
 * in the future by create type or create domain, or other
 * compatible Oracle data types implemented in the future.
 *
 * The PUT/PUT_LINE procedure for text type parameters is a
 * ugly behavior. It is only used to solve the problem that
 * anyelement does not support unknown types.Note that this
 * is not universal, but it is OK here.
 */
CREATE OR REPLACE PACKAGE SYS.DBMS_OUTPUT AUTHID DEFINER
AS
	PROCEDURE ENABLE(buffer_size in integer default 20000);
	PROCEDURE DISABLE();
	PROCEDURE PUT(item text);
	PROCEDURE PUT(item anyelement);
	PROCEDURE PUT_LINE(item text);
	PROCEDURE PUT_LINE(item anyelement);
	PROCEDURE NEW_LINE();
	PROCEDURE GET_LINE(line out varchar2, status out integer);
	PROCEDURE GET_LINES(lines out text[], numlines in out integer);
END;

CREATE OR REPLACE PACKAGE BODY SYS.DBMS_OUTPUT
AS
	/* ENABLE */
	PROCEDURE ENABLE(buffer_size in integer default 20000)
	AS
	BEGIN
		sys.dbmsoutput_enable(buffer_size);
	END;

	/* DISABLE */
	PROCEDURE DISABLE()
	AS
	BEGIN
		sys.dbmsoutput_disable();
	END;

	/* PUT */
	PROCEDURE PUT(item text)
	AS
	BEGIN
		sys.dbmsoutput_put(item);
	END;

	PROCEDURE PUT(item anyelement)
	AS
	BEGIN
		sys.dbmsoutput_put(item);
	END;

	/* PUT_LINE */
	PROCEDURE PUT_LINE(item text)
	AS
	BEGIN
		sys.dbmsoutput_putline(item);
	END;

	PROCEDURE PUT_LINE(item anyelement)
	AS
	BEGIN
		sys.dbmsoutput_putline(item);
	END;

	/* NEW_LINE */
	PROCEDURE NEW_LINE()
	AS
	BEGIN
		sys.dbmsoutput_newline();
	END;

	/* GET_LINE */
	PROCEDURE GET_LINE(line out varchar2, status out integer)
	AS
	BEGIN
		sys.dbmsoutput_getline(line, status);
	END;

	/* GET_LINES */
	PROCEDURE GET_LINES(lines out text[], numlines in out integer)
	AS
	BEGIN
		sys.dbmsoutput_getlines(lines, numlines);
	END;
END;
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

GRANT EXECUTE ON PACKAGE SYS.DBMS_OUTPUT TO public; /* BUG#Z6218 */

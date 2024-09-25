--standard package
create or replace package sys.STANDARD AUTHID CURRENT_USER IS
 /********** Predefined constants **********/

  BINARY_FLOAT_NAN constant BINARY_FLOAT DEFAULT 'Nan';
  BINARY_FLOAT_INFINITY constant BINARY_FLOAT DEFAULT 'Inf';
  BINARY_FLOAT_MAX_NORMAL constant BINARY_FLOAT DEFAULT '3.40282347E+038';
  BINARY_FLOAT_MIN_NORMAL constant BINARY_FLOAT DEFAULT '1.17549435E-038';
  BINARY_FLOAT_MAX_SUBNORMAL constant BINARY_FLOAT DEFAULT '1.17549421E-038';
  BINARY_FLOAT_MIN_SUBNORMAL constant BINARY_FLOAT DEFAULT '1.40129846E-045';
  BINARY_DOUBLE_NAN constant BINARY_DOUBLE DEFAULT 'Nan';
  BINARY_DOUBLE_INFINITY constant BINARY_DOUBLE DEFAULT 'Inf';
  BINARY_DOUBLE_MAX_NORMAL constant BINARY_DOUBLE DEFAULT '1.7976931348623157E+308';
  BINARY_DOUBLE_MIN_NORMAL constant BINARY_DOUBLE DEFAULT '2.2250738585072009E-308';
  BINARY_DOUBLE_MAX_SUBNORMAL constant BINARY_DOUBLE DEFAULT '2.2250738585072014E-308';
  BINARY_DOUBLE_MIN_SUBNORMAL constant BINARY_DOUBLE DEFAULT '4.9406564584124654E-324';

  /********** Add new constants here **********/

/********** Predefined exceptions **********/
  CURSOR_ALREADY_OPEN exception;
   pragma EXCEPTION_INIT(CURSOR_ALREADY_OPEN, -6511);

  DUP_VAL_ON_INDEX exception;
    pragma EXCEPTION_INIT(DUP_VAL_ON_INDEX, -0001);

  TIMEOUT_ON_RESOURCE exception;
    pragma EXCEPTION_INIT(TIMEOUT_ON_RESOURCE, -0051);

  INVALID_CURSOR exception;
    pragma EXCEPTION_INIT(INVALID_CURSOR, -1001);

  NOT_LOGGED_ON exception;
    pragma EXCEPTION_INIT(NOT_LOGGED_ON, -1012);

  LOGIN_DENIED exception;
    pragma EXCEPTION_INIT(LOGIN_DENIED, -1017);

  NO_DATA_FOUND exception;
    pragma EXCEPTION_INIT(NO_DATA_FOUND, 100);

  ZERO_DIVIDE exception;
    pragma EXCEPTION_INIT(ZERO_DIVIDE, -1476);

  INVALID_NUMBER exception;
    pragma EXCEPTION_INIT(INVALID_NUMBER, -1722);

  TOO_MANY_ROWS exception;
    pragma EXCEPTION_INIT(TOO_MANY_ROWS, -1422);

  STORAGE_ERROR exception;
    pragma EXCEPTION_INIT(STORAGE_ERROR, -6500);

  PROGRAM_ERROR exception;
    pragma EXCEPTION_INIT(PROGRAM_ERROR, -6501);

  VALUE_ERROR exception;
    pragma EXCEPTION_INIT(VALUE_ERROR, -6502);

  ACCESS_INTO_NULL exception;
    pragma EXCEPTION_INIT(ACCESS_INTO_NULL, -6530);

  COLLECTION_IS_NULL exception;
    pragma EXCEPTION_INIT(COLLECTION_IS_NULL , -6531);

  SUBSCRIPT_OUTSIDE_LIMIT exception;
    pragma EXCEPTION_INIT(SUBSCRIPT_OUTSIDE_LIMIT,-6532);

  SUBSCRIPT_BEYOND_COUNT exception;
    pragma EXCEPTION_INIT(SUBSCRIPT_BEYOND_COUNT ,-6533);

  -- exception for ref cursors
  ROWTYPE_MISMATCH exception;
  pragma EXCEPTION_INIT(ROWTYPE_MISMATCH, -6504);

  SYS_INVALID_ROWID  EXCEPTION;
  PRAGMA EXCEPTION_INIT(SYS_INVALID_ROWID, -1410);

  -- The object instance i.e. SELF is null
  SELF_IS_NULL exception;
    pragma EXCEPTION_INIT(SELF_IS_NULL, -30625);

  CASE_NOT_FOUND exception;
    pragma EXCEPTION_INIT(CASE_NOT_FOUND, -6592);

  -- Added for USERENV enhancement, bug 1622213.
  USERENV_COMMITSCN_ERROR exception;
    pragma EXCEPTION_INIT(USERENV_COMMITSCN_ERROR, -1725);

  -- Parallel and pipelined support
  NO_DATA_NEEDED exception;
    pragma EXCEPTION_INIT(NO_DATA_NEEDED, -6548);
  -- End of 8.2 parallel and pipelined support

  /********** Add new exceptions here **********/

  /********** Function, operators and procedures **********/
  function SQLCODE return integer;

  function SQLERRM return varchar2;

  function SQLERRM (code number) return varchar2;
end;

create or replace package body sys.STANDARD IS
	FUNCTION SQLCODE RETURN integer IS
	BEGIN
		RETURN plisql_sqlcode();
	end;
	FUNCTION sqlerrm RETURN varchar2 IS
	BEGIN
		RETURN plisql_sqlerrm();
	end;
	FUNCTION sqlerrm(code number) RETURN varchar2 IS
	BEGIN
		RETURN plisql_sqlerrm(code::integer);
	end;
end;

GRANT EXECUTE ON package SYS.STANDARD TO public;

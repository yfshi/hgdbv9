--dbms_standard
CREATE OR REPLACE NONEDITIONABLE PACKAGE SYS.DBMS_STANDARD IS
	 procedure raise_application_error(num integer, msg varchar2,
      keeperrorstack boolean default FALSE);
end;


CREATE OR REPLACE PACKAGE BODY SYS.DBMS_STANDARD IS
	 procedure raise_application_error(num integer, msg varchar2,
      keeperrorstack boolean default FALSE) IS
	 BEGIN
		-- num valid is -20000 and -20999
		IF (num < -20999 or num > -20000) then
			plisql_raise_application_error(-21000, 'IVY-21000: error number argument to raise_application_error of ' || num || ' is out of range', false);
		else
			plisql_raise_application_error(num, msg, keeperrorstack);
		END if;
	 end;
end;


GRANT EXECUTE ON package SYS.DBMS_STANDARD TO public;

/***************************************************************
 *
 * Character datatype functions. 
 *
 ***************************************************************/
/* length/lengthb for CHAR(n char/byte) */
/* begin - bug m230 */
CREATE FUNCTION sys.length(text)
RETURNS integer
AS 'MODULE_PATHNAME','oracharlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;
/* end - bug m230 */

CREATE FUNCTION sys.length(sys.oracharchar)
RETURNS integer
AS 'MODULE_PATHNAME','oracharlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.length(sys.oracharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','oracharlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.lengthb(sys.oracharchar)
RETURNS integer
AS 'MODULE_PATHNAME','oracharoctetlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.lengthb(sys.oracharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','oracharoctetlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* length/lengthb for VARCHAR2(n char/byte) */
CREATE FUNCTION sys.length(sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','oravarcharlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.length(sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','oravarcharlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.lengthb(sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','oravarcharoctetlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.lengthb(sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','oravarcharoctetlen'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/* Begin - bug0000540 */
CREATE FUNCTION sys.length(integer)
RETURNS integer AS
$$
	SELECT sys.length(cast($1 as sys.oravarcharchar));
$$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
VOLATILE;
/* End - bug0000540 */

create function sys.lengthb(bytea) returns int as
$$
begin
  return octet_length($1);
end;
$$
language plpgsql
PARALLEL SAFE; -- BUG#M0000422


/* trim/ltrim/rtrim functions */
/* Begin - Bug#796 */
CREATE FUNCTION sys.rtrim(sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','rtrim1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION sys.rtrim(sys.oravarcharchar, sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','rtrim2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION sys.ltrim(sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','ltrim1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION sys.ltrim(sys.oravarcharchar, sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','ltrim2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
/* End - Bug#796 */

/* Begin - BUG#833 */
CREATE FUNCTION sys.trim(sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','trim1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.trim(sys.oravarcharchar, sys.oravarcharchar)
RETURNS oravarcharchar
AS 'MODULE_PATHNAME','trim2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

--bytea use pg_catalog
CREATE FUNCTION sys.trim(bytea, bytea)
RETURNS bytea
AS $$ SELECT pg_catalog.btrim($1, $2);$$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;
/* End - BUG#833 */

/* Begin - SRS-CKC-Oracle-SupportOra_Regexp_245 */
/* regexp_replace */
CREATE FUNCTION sys.regexp_replace(varchar2, varchar2, varchar2, integer,integer,varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_replace(varchar2, varchar2, varchar2, integer,integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_replace(varchar2, varchar2, varchar2, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

/* Begin - bug0000698 */
CREATE FUNCTION sys.regexp_replace(varchar2, varchar2, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_replace(text, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;
/* End - bug0000698 */

CREATE FUNCTION sys.regexp_replace(varchar2, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_replace(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_replace'
LANGUAGE C
PARALLEL SAFE
STABLE;

/*regexp_substr*/
CREATE FUNCTION sys.regexp_substr(varchar2, varchar2, integer, integer, varchar2, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.regexp_substr(varchar2, varchar2, integer, integer, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.regexp_substr(varchar2, varchar2, integer, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.regexp_substr(varchar2, varchar2, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.regexp_substr(varchar2, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION sys.regexp_substr(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_substr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

/*regexp_instr*/
CREATE FUNCTION sys.regexp_instr(varchar2, varchar2, integer, integer, integer, varchar2, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2, varchar2, integer, integer, integer, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2, varchar2, integer, integer, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2, varchar2, integer, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2, varchar2, integer)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

CREATE FUNCTION sys.regexp_instr(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME','ora_regexp_instr'
LANGUAGE C
PARALLEL SAFE
STABLE;

/* regexp_like */
CREATE FUNCTION sys.regexp_like(varchar2, varchar2)
RETURNS bool
AS 'MODULE_PATHNAME','ora_regexp_like_no_flags'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION sys.regexp_like(varchar2, varchar2, varchar2)
RETURNS bool
AS 'MODULE_PATHNAME','ora_regexp_like'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

/* Begin - BUG:0000891 */
/* Begin - ReqID:SRS-SQL-SYSFUNC: modify text to varchar2, int to number */
CREATE FUNCTION sys.regexp_count(varchar2, varchar2, number default 1, varchar2 default 'g')
RETURNS int
AS 'MODULE_PATHNAME','ora_regexp_count'
LANGUAGE C
PARALLEL SAFE
STABLE;
/* End - BUG:0000891 */
/* End - ReqID:SRS-SQL-SYSFUNC */
/* End - SRS-CKC-Oracle-SupportOra_Regexp_245 */

/* SR */
CREATE FUNCTION sys.substrb(varchar, number)
RETURNS text
AS 'MODULE_PATHNAME','ora_substrb_no_length'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.substrb(varchar, number, number)
RETURNS text
AS 'MODULE_PATHNAME','ora_substrb'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.replace(varchar, varchar, varchar default NULL)
RETURNS text
AS 'MODULE_PATHNAME','ora_replace'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
IMMUTABLE;

/* Begin - ReqID:SRS-SQL-SYSFUNC: modify text and varchar to varchar2 */
CREATE FUNCTION sys.instrb(varchar2, varchar2, number default 1, number default 1)
RETURNS int
AS 'MODULE_PATHNAME','ora_instrb'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
IMMUTABLE;
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - BUG#997 */
--lpad
CREATE FUNCTION sys.lpad(varchar2, number) returns varchar2 AS
$$ select pg_catalog.lpad($1::text, $2::integer); $$
LANGUAGE SQL
PARALLEL SAFE -- BUG#M0000422, for pg_catalog.lpad() is parallel safe
STRICT
IMMUTABLE;

CREATE FUNCTION sys.lpad(varchar2, number, varchar2) returns varchar2 AS
$$ select pg_catalog.lpad($1::text, $2::integer, $3::text); $$
LANGUAGE SQL
PARALLEL SAFE -- BUG#M0000422, for pg_catalog.lpad() is parallel safe
STRICT
IMMUTABLE;

--rpad
CREATE FUNCTION sys.rpad(varchar2, number) returns varchar2 AS
$$ select pg_catalog.rpad($1::text, $2::integer); $$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.rpad(varchar2, number, varchar2) returns varchar2 AS
$$ select pg_catalog.rpad($1::text, $2::integer, $3::text); $$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;
/* End - BUG#997 */

/***************************************************************
 *
 * Datetime datatype functions. 
 *
 ***************************************************************/
CREATE FUNCTION sys.sysdate()
RETURNS sys.oradate
AS 'MODULE_PATHNAME','sysdate'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.current_date()
RETURNS sys.oradate
AS 'MODULE_PATHNAME','ora_current_date'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.systimestamp()
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','ora_current_timestamp'
LANGUAGE C
STRICT
STABLE;

/* Begin - bug0000415 */
CREATE FUNCTION sys.current_timestamp()
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','ora_current_timestamp'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.current_timestamp(integer)
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','ora_current_timestamp'
LANGUAGE C
STRICT
STABLE;
/* End - bug0000415 */

/* Begin - bug0000415 */
CREATE FUNCTION sys.localtimestamp()
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','ora_local_timestamp'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.localtimestamp(integer)
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','ora_local_timestamp'
LANGUAGE C
STRICT
STABLE;
/* End - bug0000415 */

CREATE FUNCTION sys.last_day(sys.oradate)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','last_day'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

/* Begin - BUGID:0000414 */
CREATE FUNCTION sys.add_months(sys.oradate,sys.number)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','add_months'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;
/* End - BUGID:0000414 */

CREATE FUNCTION sys.round(sys.oradate,text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','ora_round'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.round(sys.oradate)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','ora_round'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;

--bug#894 change trunc to immutable
CREATE FUNCTION sys.trunc(sys.oradate,text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','ora_trunc'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION sys.trunc(sys.oradate)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','ora_trunc'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION sys.next_day(sys.oradate,integer)
RETURNS sys.oradate
AS 'MODULE_PATHNAME', 'next_day_by_index'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION sys.next_day(sys.oradate,text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME', 'next_day'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION sys.new_time(sys.oradate,text,text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME', 'ora_new_time'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.tz_offset(text)
RETURNS text
AS 'MODULE_PATHNAME', 'ora_tz_offset'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.months_between(sys.oradate, sys.oradate)
RETURNS double precision
AS 'MODULE_PATHNAME', 'months_between'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION sys.from_tz(sys.oratimestamp,text)
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','ora_from_tz'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.sys_extract_utc(sys.oratimestamptz)
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','ora_sys_extract_utc'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;

/* Begin - bug0000427 */
CREATE FUNCTION sys.sessiontimezone()
RETURNS text
AS 'MODULE_PATHNAME','ora_sessiontimezone'
LANGUAGE C
PARALLEL SAFE -- BUG#M0000422
STRICT
STABLE;
/* End - bug0000427 */

CREATE FUNCTION sys.to_date(text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','to_oradate1'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.to_date(text,text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','to_oradate2'
LANGUAGE C
STRICT
STABLE;
 
CREATE FUNCTION sys.to_date(text, text, text)
RETURNS sys.oradate
AS 'MODULE_PATHNAME','to_oradate3'
LANGUAGE C
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp(text)
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','to_oratimestamp1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp(text, text)
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','to_oratimestamp2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp(text, text, text)
RETURNS sys.oratimestamp
AS 'MODULE_PATHNAME','to_oratimestamp3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp_tz(text)
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','to_oratimestamptz1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp_tz(text, text)
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','to_oratimestamptz2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_timestamp_tz(text, text, text)
RETURNS sys.oratimestamptz
AS 'MODULE_PATHNAME','to_oratimestamptz3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
STABLE;

CREATE FUNCTION sys.to_char(sys.oradate)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradate_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;


CREATE FUNCTION sys.to_char(sys.oradate, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradate_to_char2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oradate, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradate_to_char3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamp)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamp_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamp, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamp_to_char2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamp, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamp_to_char3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamptz)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamptz_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamptz, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamptz_to_char2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestamptz, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestamptz_to_char3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestampltz)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestampltz_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestampltz, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestampltz_to_char2'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.oratimestampltz, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oratimestampltz_to_char3'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.dsinterval)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradsinterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.dsinterval, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradsinterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.dsinterval, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','oradsinterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.yminterval)
RETURNS varchar2
AS 'MODULE_PATHNAME','orayminterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.yminterval, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','orayminterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_char(sys.yminterval, text, text)
RETURNS varchar2
AS 'MODULE_PATHNAME','orayminterval_to_char1'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_yminterval(text)
RETURNS sys.yminterval
AS 'MODULE_PATHNAME','to_yminterval'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.numtoyminterval(float8, text)
RETURNS sys.yminterval
AS 'MODULE_PATHNAME','numtoyminterval'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_dsinterval(text)
RETURNS sys.dsinterval
AS 'MODULE_PATHNAME','to_dsinterval'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

CREATE FUNCTION sys.numtodsinterval(float8, text)
RETURNS sys.dsinterval
AS 'MODULE_PATHNAME','numtodsinterval'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;


/***************************************************************
 *
 * Numeric datatype functions. 
 *
 ***************************************************************/
/* Begin - bug0000457 */
CREATE FUNCTION sys.bitand(number,number)
RETURNS number
AS 'MODULE_PATHNAME','number_bitand'
LANGUAGE C
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;
/* End - bug0000457 */

/* Begin - bug0000478 */
CREATE FUNCTION sys.bitand(text, text)
RETURNS number
AS
$$
declare
  v1 number = cast($1 as number);
  v2 number = cast($2 as number);
begin
	return sys.bitand(v1, v2);
end;
$$
LANGUAGE plpgsql
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;
/* End - bug0000478 */

/* Begin - ReqID:SRS-SQL-SYSFUNC */
CREATE FUNCTION sys.to_number(boolean, boolean, text)
RETURNS NUMBER
AS 'MODULE_PATHNAME','ora_to_number_w'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_number(boolean, boolean, text, text)
RETURNS NUMBER
AS 'MODULE_PATHNAME','ora_to_number_w'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_number(boolean, boolean, text, text, text)
RETURNS NUMBER
AS 'MODULE_PATHNAME','ora_to_number_w'
LANGUAGE C
STRICT
IMMUTABLE;

CREATE FUNCTION sys.to_number(boolean, boolean, text, text, text, text)
RETURNS NUMBER
AS 'MODULE_PATHNAME','ora_to_number_w'
LANGUAGE C
STRICT
IMMUTABLE;
/* End - ReqID:SRS-SQL-SYSFUNC */
/***************************************************************
 *
 * RAW BLOB CLOB datatype functions. 
 *
 ***************************************************************/
--bug#834 support to_clob
CREATE OR REPLACE FUNCTION sys.to_clob(varchar2)
RETURNS clob AS 
$$ 
	SELECT $1::clob;
$$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;

/* Begin - BUG#999 */
CREATE OR REPLACE FUNCTION sys.to_blob(varchar2)
RETURNS blob
AS $$ SELECT $1::blob;$$
LANGUAGE SQL
PARALLEL SAFE	-- BUG#M0000422
STRICT
IMMUTABLE;
/* End - BUG#999 */

/* support hextoraw function for oracle compatibility */
CREATE or REPLACE FUNCTION sys.hextoraw(text)
RETURNS bytea
AS $$ SELECT pg_catalog.decode($1, 'hex'); $$
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;


/***************************************************************
 *
 * Misc functions. 
 *
 ***************************************************************/
/* Begin - bug0000459 */
CREATE FUNCTION sys.uid()
RETURNS int4
AS 'MODULE_PATHNAME','uid'
LANGUAGE C
STRICT
IMMUTABLE;
/* End - bug0000459 */

/* 
 * Oracle USERENV support
 */
/* SESSIONID */
CREATE SEQUENCE sys.userenv_sessionid_sequence;
GRANT ALL ON SEQUENCE sys.userenv_sessionid_sequence to public;
   
CREATE OR REPLACE FUNCTION sys.get_sessionid() RETURNS number AS $$     
DECLARE
	res int8;     
BEGIN    
	SELECT currval('sys.userenv_sessionid_sequence') into res;     
RETURN res;     
EXCEPTION     
    WHEN sqlstate '55000' THEN      
		SELECT nextval('sys.userenv_sessionid_sequence') into res;     
		RETURN res;      
    WHEN sqlstate '42P01' THEN      
		CREATE SEQUENCE sys.userenv_sessionid_sequence;    
		SELECT nextval('sys.userenv_sessionid_sequence') into res;     
		RETURN res;     
END;     
$$ LANGUAGE plpgsql STRICT SET client_min_messages to error; 

/* LANG */
CREATE OR REPLACE FUNCTION sys.get_lang() RETURNS varchar2 AS $$     
	SELECT (regexp_split_to_array(current_setting('lc_messages'), '\.'))[1];    
$$ 
LANGUAGE sql 
PARALLEL SAFE -- BUG#M0000422
STRICT;

/* LANGUAGE */
CREATE OR REPLACE FUNCTION sys.get_language() RETURNS varchar2 AS $$     
	SELECT (regexp_split_to_array(current_setting('lc_monetary'), '\.'))[1]||'.'||pg_client_encoding();  
$$ LANGUAGE sql 
PARALLEL SAFE	-- BUG#M0000422
STRICT;

/* CLIENT_INFO */
CREATE OR REPLACE FUNCTION sys.get_client_info() RETURNS varchar2 AS $$     
DECLARE     
BEGIN       
	RETURN NULL;         
END;     
$$ LANGUAGE plpgsql 
PARALLEL SAFE	-- BUG#M0000422
STRICT
SET client_min_messages to error;


/* ENTRYID */
CREATE OR REPLACE FUNCTION sys.get_entryid() RETURNS number AS $$     
DECLARE     
BEGIN       
	RETURN NULL;         
END;     
$$ LANGUAGE plpgsql
PARALLEL SAFE	-- BUG#M0000422
STRICT
SET client_min_messages to error; 

/* TERMINAL */
CREATE OR REPLACE FUNCTION sys.get_terminal() RETURNS number AS $$     
DECLARE     
BEGIN       
	RETURN NULL;         
END;     
$$ LANGUAGE plpgsql 
PARALLEL SAFE -- BUG#M0000422
STRICT 
SET client_min_messages to error; 

/* SID */
CREATE OR REPLACE FUNCTION sys.get_sid() RETURNS number AS $$     
DECLARE
	res int;
BEGIN
	SELECT pg_backend_pid() into res;
	RETURN res;         
END;     
$$ LANGUAGE plpgsql 
PARALLEL RESTRICTED	-- BUG#M0000422
STRICT 
SET client_min_messages to error;

/* ISDBA */
CREATE OR REPLACE FUNCTION sys.get_isdba() RETURNS VARCHAR2 AS $$     
DECLARE
	res boolean;
BEGIN
	SELECT rolsuper into res FROM pg_roles WHERE rolname=current_user;
	IF res = true THEN
		RETURN 'TRUE';
	ELSIF res = false THEN
		RETURN 'FALSE';
	ELSE
		RAISE 'Failed to obtain user attributes.';
	END IF;
END;     
$$ LANGUAGE plpgsql STRICT SET client_min_messages to error;


/* Begin - BUG:Z475 */
CREATE OR REPLACE FUNCTION sys.length(smallint)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(bigint)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;
/* End - BUG:Z475 */

/* Begin - ReqID:SRS-SQL-SYSFUNC: using numeric because float point number 
 * and big number, them can't recognized by number type
 */
CREATE OR REPLACE FUNCTION sys.length(numeric)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.number)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.binary_double)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.oradate)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.oradate)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.oratimestamp)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.length(sys.oratimestamptz)
RETURNS integer
AS 'select sys.length($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;


--lengthb
CREATE OR REPLACE FUNCTION sys.lengthb(integer)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.lengthb(numeric)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.lengthb(sys.number)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL 
IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.lengthb(sys.oradate)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.lengthb(sys.oratimestamp)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.lengthb(sys.oratimestamptz)
RETURNS integer
AS 'select sys.lengthb($1::sys.oravarcharchar)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION sys.lengthb(text)
RETURNS integer
AS 'MODULE_PATHNAME','oravarcharoctetlen'
LANGUAGE C
STRICT
IMMUTABLE
PARALLEL SAFE; -- BUG#M0000422

--round
CREATE OR REPLACE FUNCTION sys.round(text)
RETURNS sys.number
AS 'select pg_catalog.round($1::pg_catalog.numeric)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.round(text, integer)
RETURNS sys.number
AS 'select pg_catalog.round($1::pg_catalog.numeric, $2)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION sys.round(double precision, integer)
RETURNS numeric
AS $$SELECT pg_catalog.round($1::numeric,$2);$$
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

--trunc
CREATE OR REPLACE FUNCTION sys.trunc(text)
RETURNS sys.number
AS 'select pg_catalog.trunc($1::pg_catalog.numeric)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.trunc(text, integer)
RETURNS sys.number
AS 'select pg_catalog.trunc($1::pg_catalog.numeric, $2)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.trunc(double precision, integer)
RETURNS sys.number
AS 'select pg_catalog.trunc($1::pg_catalog.numeric, $2)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

--to_date
CREATE OR REPLACE FUNCTION sys.to_date(bigint)
RETURNS sys.oradate
AS 'select sys.to_date($1::text)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION sys.to_date(bigint, text)
RETURNS sys.oradate
AS 'select sys.to_date($1::text, $2)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;

--to_char
CREATE OR REPLACE FUNCTION sys.to_char(number)
RETURNS sys.oravarcharchar
AS 'select sys.to_char($1::text)'
LANGUAGE SQL IMMUTABLE PARALLEL SAFE STRICT;
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - BUG:M0000084 */
CREATE FUNCTION sys.substr(str text, sta int, len int)
RETURNS varchar
AS 'MODULE_PATHNAME','oracle_substr3'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.substr(str text, sta int)
RETURNS varchar
AS 'MODULE_PATHNAME','oracle_substr2'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.substr(str varchar2, sta number, len number)
RETURNS varchar
AS $$
  select sys.substr(str::text, sta::integer, len::integer);
$$ LANGUAGE SQL 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.substr(str varchar2, sta number)
RETURNS varchar
AS $$
 select sys.substr(str::text, sta::integer);
$$ LANGUAGE SQL 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str text, patt text, sta int, nth int)
RETURNS int
AS 'MODULE_PATHNAME','oracle_instr4'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str text, patt text, sta int)
RETURNS int
AS 'MODULE_PATHNAME','oracle_instr3'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str text, patt text)
RETURNS int
AS 'MODULE_PATHNAME','oracle_instr2'
LANGUAGE C 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str varchar2, patt varchar2, sta number, nth number)
RETURNS int
AS $$
  select sys.instr(str::text, patt::text, sta::integer, nth::integer);
$$ LANGUAGE SQL 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str varchar2, patt varchar2, sta number)
RETURNS int
AS $$
  select sys.instr(str::text, patt::text, sta::integer);
$$ LANGUAGE SQL 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.instr(str varchar2, patt varchar2)
RETURNS int
AS $$
  select sys.instr(str::text, patt::text);
$$ LANGUAGE SQL 
PARALLEL SAFE -- BUG#M0000422
IMMUTABLE STRICT;
/* End - BUG:M0000084 */

/* Begin - BUG#M0000194 */
CREATE FUNCTION sys.to_multi_byte(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'orafce_to_multi_byte'
LANGUAGE C 
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.to_single_byte(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'orafce_to_single_byte'
LANGUAGE C 
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.asciistr(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'ora_asciistr'
LANGUAGE C 
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.compose(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'ora_compose'
LANGUAGE C
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.decompose(varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'ora_decompose'
LANGUAGE C
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.decompose(varchar2, varchar2)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'ora_decompose'
LANGUAGE C
PARALLEL SAFE  -- BUG#M0000422
IMMUTABLE STRICT;

CREATE FUNCTION sys.convert(varchar2, name, name)
RETURNS varchar2
AS 'MODULE_PATHNAME', 'ora_convert'
LANGUAGE C IMMUTABLE STRICT;
/* End - BUG#M0000194 */

/* Begin - Bug#Z1064 */
/* get macro value */
CREATE FUNCTION sys.get_macro_value(text)
RETURNS text
AS 'MODULE_PATHNAME','get_macro_value'
LANGUAGE C
PARALLEL SAFE  -- BUG#M0000422
STRICT
IMMUTABLE;
/* End - Bug#Z1064 */

/* Begin - BUG#M0000457 */
CREATE FUNCTION sys.listagg1_transfn(internal, text)
RETURNS internal
AS 'MODULE_PATHNAME','orafce_listagg1_transfn'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION sys.listagg2_transfn(internal, text, text)
RETURNS internal
AS 'MODULE_PATHNAME','orafce_listagg2_transfn'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION sys.listagg_finalfn(internal)
RETURNS text
AS 'MODULE_PATHNAME','orafce_listagg_finalfn'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE sys.listagg(varchar2) (
  SFUNC=sys.listagg1_transfn,
  STYPE=internal,
  FINALFUNC=sys.listagg_finalfn
);

CREATE AGGREGATE sys.listagg(varchar2, varchar2) (
  SFUNC=sys.listagg2_transfn,
  STYPE=internal,
  FINALFUNC=sys.listagg_finalfn
);
/* End - BUG#M0000457 */
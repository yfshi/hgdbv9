/*--------------------redaction table------------------------
 *
 * create data redaction table hg_redaction_policy, and index
 */
create table hg_security.hg_redaction_policy(polid oid, polname name, tabrelid oid, schema name, tabname name, enable bool, poloptions text[]);
create unique index hg_redaciton_policy_oid_index on hg_security.hg_redaction_policy using btree(polid);
create index hg_redaciton_table_oid_index on hg_security.hg_redaction_policy using btree(tabrelid);
create unique index hg_redaciton_policy_tabrelid_polname_index on hg_security.hg_redaction_policy using btree(tabrelid, polname);

/*--------------------redaction table------------------------
 *
 * create data redaction table hg_redaction_column, and index
 */
create table hg_security.hg_redaction_column(polid oid not null, polname name not null, tabrelid oid not null, attname name not null, procid oid not null, procname name not null, polscope smallint, polexception smallint);
create index hg_redaciton_column_polid_index on hg_security.hg_redaction_column using btree (polid);
create unique index hg_redaciton_column_tabrelid_attname_index on hg_security.hg_redaction_column using btree (tabrelid, attname);
create unique index hg_redaciton_column_polname_attname_index on hg_security.hg_redaction_column using btree (polname, tabrelid, attname);


/*--------------------redact_bankcard------------------------
 *
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT.
 * For text data in credit card format
 * '4880-9898-4545-2525' will be redactioned as 'xxxx-xxxx-xxxx-2525',
 * This function redactions the numbers before the last four digits.
 */
create or replace function hg_security.redact_bankcard(col text,letter char default 'x') RETURNS text AS $$
declare 
	size INTEGER := 4;
begin
	return CASE WHEN length(col) >= size THEN
		REGEXP_REPLACE(left(col, size*(-1)), '[\d+]', letter, 'g') || right(col, size)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_emailname------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for the text data in email format
 * 'abcd@gmail.com' will be redactioned as'xxxx@gmail.com',
 * redaction the text before the first @.
 */
create or replace function hg_security.redact_emailname(col text, letter char default 'x') RETURNS text AS $$
declare 
	pos INTEGER := position('@' in col);
begin
	return CASE WHEN pos > 1 THEN
	repeat(letter, pos - 1) || substring(col, pos, length(col) - pos +1)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_emailfull------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for the text data in email format
 * 'abcd@gmail.com' will be redactioned as 'xxxx@xxxxx.com',
 * redaction the text before the last '.', except'@'.
 */
create or replace function hg_security.redact_emailfull(col text, letter char default 'x') RETURNS text AS $$
declare 
	pos INTEGER := position('@' in col);
	dot_pos INTEGER := length(col) - position('.' in reverse(col)) + 1;
begin
	return CASE WHEN pos > 2 and dot_pos > pos THEN
	repeat(letter, pos - 1) || '@' || repeat(letter,  dot_pos - pos - 1) || substring(col, dot_pos, length(col) - dot_pos +1)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_digits------------------------
 *
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * only for text type numbers.
 * 'alex123alex' will be redactioned as 'alex000alex',
 * redaction only for numbers in text.
 */
create or replace function hg_security.redact_digits(col text, letter char default '0') RETURNS text AS $$
begin
	return REGEXP_REPLACE(col, '[\d+]', letter, 'g');
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_shuffle------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * only for text type numbers.
 * 'hello word' will be randomly sorted as 'hlwoeor dl'
 * The function is implenmented by arranging characters
 * in random order,which is a weak redaction function.
 *
 * !!! not recommended to use this function.
 */
create or replace function hg_security.redact_shuffle(col text) RETURNS text AS $$
declare 
	index INTEGER := 0;
	rd INTEGER;
	size INTEGER := length(col) -1;
	tmp text := col;
	res text;
begin
	rd := floor(random() * length(tmp) + 1);
	res := right(left(tmp, rd), 1);
	tmp := left(tmp, rd - 1) || right(tmp, length(tmp) - rd);
	while size > 0 loop 
		rd := floor(random() * length(tmp) + 1);
		res := res || right(left(tmp, rd), 1);
		tmp := left(tmp, rd - 1) || right(tmp, length(tmp) - rd);
		size := size - 1;
	END loop;
	return res;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_random------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for text type data.
 * 'hello word' will be redactioned as 'ad5f5ghdf5'random numbers like this,
 * randomly redaction the text by character.
 */
create or replace function hg_security.redact_random(col text) RETURNS text AS $$	
begin
	return left(MD5(random()::text), length(col));
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_idcard------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for text type data.
 * Redaction the ID number.130623202101010623' will be redactioned as
 '130623********0623', reserve the first six digits and the last four
 * digits of the ID card number, and replace all the middle digits
 * with *.
 */
create or replace function hg_security.redact_idcard(col text, letter char default '*') RETURNS text AS $$
declare 
	size INTEGER := 4;
begin
	return CASE WHEN length(col) >= size THEN
	substring(col, 0, size + 3) || repeat(letter, length(col) - size -size - 2) || substring(col, length(col) - size + 1, size + 1)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_name------------------------
 * 
 * Supported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for text type data.
 * Perform redaction on the name, replace the first chinese character with *
 * for names with two chinese characters, and replace the penultimate character
 * with * for names with more than three chinese characters.
 * such as “张三” will be redactioned as “张**”，“张三丰” will be redactioned as “张**”，
 * “富兰克林” will be redactioned as “富**”
 */
create or replace function hg_security.redact_name(col text, letter char default '*') RETURNS text AS $$
declare 
	size INTEGER := 2;
begin
	return CASE WHEN length(col) >= size THEN
	substring(col, 0, size) || repeat(letter, 2)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_phone------------------------
 * 
 * upported data type: BPCHAR, VARCHAR, NVARCHAR, TEXT
 * Only for text type data.
 * Perform redaction in phone numbers. applicable to mobile
 * phone numbers and fixed-line telephone numbers, retain the
 * first three and last four digits, redaction on the middle
 * number. such as “13534556093” will be redactioned as “135****6093”
 */
create or replace function hg_security.redact_phone(col text, letter char default '*') RETURNS text AS $$
declare 
	size INTEGER := 4;
begin
	return CASE WHEN length(col) >= size THEN
	substring(col, 0, size) || repeat(letter, length(col) - size -size + 1) || substring(col, length(col) - size + 1, size + 1)
	ELSE
		col
	end;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_all------------------------
 * 
 * upported data type: 
   RELTIME, TIME, TIMETZ, INTERVAL, TIMESTAMP, TIMESTAMPTZ, SMALLDATETIME, ABSTIME,
   TEXT, BPCHAR, VARCHAR, NVARCHAR2, NAME, 
   INT8, INT4, INT2, INT1, NUMRIC,
   FLOAT4, FLOAT8, 
   CASH,
   BOOL, 
 * Almost all data types are supported.
 * '4880-9898-4545-2525' will be redactioned as 'xxxxxxxxxxxxxxxxxxx'
 * redaction all characters
 */
create or replace function hg_security.redact_all(col text) RETURNS text AS $$	
begin
	return col;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

/*--------------------redact_cash------------------------
 * 
 * upported data type: cash type data.
 * Perform redaction in cash. applicable to money.
 */
create or replace function hg_security.redact_cash(col money) RETURNS money AS $$
begin
	return $1 - $1;
end;
$$ LANGUAGE plpgsql
SET pg_proc.redaction to 1;

CREATE OR REPLACE FUNCTION hg_security.policy_num()
RETURNS bigint
AS 'MODULE_PATHNAME','all_redaction_policy_num'
LANGUAGE C
IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hg_security.redact_all_name(col name)
RETURNS name
AS 'MODULE_PATHNAME','redaction_all_name'
LANGUAGE C
IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hg_security.redact_all_name(col name)
RETURNS name
AS 'MODULE_PATHNAME','redaction_all_name'
LANGUAGE C
IMMUTABLE PARALLEL SAFE;

/*--------------------redaction function view------------------------
 *
 * the view as redaction functions view,
 *	display all redactio functions.
 */
CREATE OR REPLACE VIEW hg_security.hg_redaction_functions as 
	SELECT 
		proname,
		proargtypes,
		prorettype,
		pronargs
	FROM pg_proc
	WHERE proconfig = '{pg_proc.redaction=1}';

/*--------------------redaction view------------------------
 *
 * the view as redaction functions view,
 *	display all redactio functions.
 */
CREATE OR REPLACE VIEW hg_security.hg_redaction as
	SELECT
		a.polid   as polid,
		a.polname  as polname,
		a.enable  as enable,
		a.tabrelid as tabrelid,
		a.schema as schema,
		a.tabname as tabname,
		b.attname as colname,
		b.procname as procname,
		a.poloptions as poloptions,
		b.polscope polscope,
		b.polexception as polexception
	from hg_security.hg_redaction_policy a, hg_security.hg_redaction_column b
	where a.polid = b.polid;

grant select on hg_security.hg_redaction_policy to public;
grant select on hg_security.hg_redaction_column to public;
grant select on hg_security.hg_redaction_functions to public;
grant select on hg_security.hg_redaction to public;

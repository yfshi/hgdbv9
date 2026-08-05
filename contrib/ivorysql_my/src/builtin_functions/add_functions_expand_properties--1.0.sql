-- comparison / predicate operator implementation functions
ALTER FUNCTION ivymy.anyenum_eq(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_eq(text, anyenum) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_ne(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_ne(text, anyenum) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_lt(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_lt(text, anyenum) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_le(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_le(text, anyenum) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_gt(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_gt(text, anyenum) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_ge(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.anyenum_ge(text, anyenum) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.text_seq(text, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.varbinary_eq(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_ne(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_lt(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_le(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_gt(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_ge(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.varbinary_seq(varbinary, varbinary) EXPAND 'convert_encoding';

ALTER FUNCTION pg_catalog.texteq(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textne(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.text_lt(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.text_le(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.text_gt(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.text_ge(text, text) EXPAND 'convert_encoding';

ALTER FUNCTION pg_catalog.bpchareq(character, character) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.bpcharne(character, character) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.bpcharlt(character, character) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.bpcharle(character, character) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.bpchargt(character, character) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.bpcharge(character, character) EXPAND 'convert_encoding';

ALTER FUNCTION pg_catalog.nameeqtext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.namenetext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.namelttext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.nameletext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.namegttext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.namegetext(name, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.texteqname(text, name) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textnename(text, name) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textltname(text, name) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textlename(text, name) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textgtname(text, name) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.textgename(text, name) EXPAND 'convert_encoding';


-- ivymy MySQL-compatible functions
ALTER FUNCTION ivymy.concat(VARIADIC "any") EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."field"(VARIADIC "any") EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."field"(VARIADIC arr text[]) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy."elt"(bigint, VARIADIC text[]) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."elt"(double precision, VARIADIC text[]) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."elt"(numeric, VARIADIC text[]) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."elt"(text, VARIADIC text[]) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."elt"(uint8, VARIADIC text[]) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.export_set(bigint, text, text, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.export_set(bigint, text, text, numeric, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.export_set(bigint, text, text, text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.export_set(bigint, text, text, text, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.export_set(text, text, text, text, integer) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.find_in_set(anyenum, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.find_in_set(integer, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.find_in_set(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.find_in_set(uint8, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.insert(text, bigint, bigint, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.insert(text, bigint, text, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.insert(text, text, bigint, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.insert(text, text, text, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.instr(bigint, bigint) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.instr(binary, binary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.instr(integer, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.instr(numeric, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.instr(text, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.lcase(integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.lcase(numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.lcase(text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy."left"(blob, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."left"(boolean, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."left"(numeric, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."left"(text, boolean) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."left"(text, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."left"(text, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.locate(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.locate(text, text, bigint) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.locate(text, text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.locate(text, text, numeric) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy."right"(boolean, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."right"(numeric, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."right"(text, boolean) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."right"(text, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy."right"(text, text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.soundex(boolean) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.soundex(mysbit) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.soundex(numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.soundex(text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.strcmp(bigint, bigint) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(binary, binary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(date, date) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(double precision, double precision) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(integer, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(numeric, numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.strcmp(varbinary, varbinary) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy."substring"(text, integer, bigint) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(text, text, bigint) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(text, text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(text, text, numeric) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.ucase(integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.ucase(numeric) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.ucase(text) EXPAND 'convert_encoding';

ALTER FUNCTION ivymy.concat_ws(varbinary, varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.export_set(integer, varbinary, varbinary, varbinary, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.find_in_set(varbinary, text) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.find_in_set(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.insert(varbinary, integer, integer, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.locate(varbinary, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.lpad(varbinary, integer, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.repeat(varbinary, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.rpad(varbinary, integer, varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.soundex(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(text, varbinary, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.lcase(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.ltrim(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.ucase(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.rtrim(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(varbinary, text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.substring_index(varbinary, varbinary, integer) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.upper(varbinary) EXPAND 'convert_encoding';
ALTER FUNCTION ivymy.lower(varbinary) EXPAND 'convert_encoding';

-- pg_catalog string functions used directly by parser/function lookup
ALTER FUNCTION pg_catalog.concat_ws(text, VARIADIC "any") EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."left"(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.lower(text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.upper(text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.lpad(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.lpad(text, integer, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.rpad(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.rpad(text, integer, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.ltrim(text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.ltrim(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.rtrim(text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.rtrim(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.repeat(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.replace(text, text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.reverse(text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."right"(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.substr(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog.substr(text, integer, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."substring"(text, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."substring"(text, integer, integer) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."substring"(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."substring"(text, text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."position"(text, text) EXPAND 'convert_encoding';
ALTER FUNCTION pg_catalog."position"(varbinary, varbinary) EXPAND 'convert_encoding';
-- pg_collation
select * from pg_collation where collprovider='o';

-- collation id and name
select nls_collation_id('using_nls_comp')       from dual; -- 16382
select nls_collation_id('using_nls_sort')       from dual; -- 16381
select nls_collation_id('using_nls_sort_ai')    from dual; -- 16379
select nls_collation_id('using_nls_sort_ci')    from dual; -- 16380
select nls_collation_id('using_nls_sort_cs')    from dual; -- 16378
select nls_collation_id('binary')               from dual; -- 16383
select nls_collation_id('binary_ai')            from dual; -- 81919
select nls_collation_id('binary_ci')            from dual; -- 147455
select nls_collation_id('generic_m_ci')         from dual; -- 135169
select nls_collation_id('schinese_pinyin_m')    from dual; -- 4160
select nls_collation_id('schinese_radical_m')   from dual; -- 4163
select nls_collation_id('schinese_stroke_m')    from dual; -- 4161
select nls_collation_id('uca0700_schinese')     from dual; -- 208941
select nls_collation_id('uca0700_schinese1')    from dual; -- 208942
select nls_collation_id('uca0700_schinese2')    from dual; -- 208943
select nls_collation_id('uca0700_schinese_s4_vs_bn_ny_en_fn_hn_dn_mn')     from dual; -- 208941
select nls_collation_id('uca0700_schinese1_s4_vs_bn_ny_en_fn_hn_dn_mn')    from dual; -- 208942
select nls_collation_id('uca0700_schinese2_s4_vs_bn_ny_en_fn_hn_dn_mn')    from dual; -- 208943

select nls_collation_name(16382)  from dual; -- using_nls_comp
select nls_collation_name(16381)  from dual; -- using_nls_sort
select nls_collation_name(16379)  from dual; -- using_nls_sort_ai
select nls_collation_name(16380)  from dual; -- using_nls_sort_ci
select nls_collation_name(16378)  from dual; -- using_nls_sort_cs
select nls_collation_name(16383)  from dual; -- binary
select nls_collation_name(81919)  from dual; -- binary_ai
select nls_collation_name(147455) from dual; -- binary_ci
select nls_collation_name(135169) from dual; -- generic_m_ci
select nls_collation_name(4160)   from dual; -- schinese_pinyin_m
select nls_collation_name(4163)   from dual; -- schinese_radical_m
select nls_collation_name(4161)   from dual; -- schinese_stroke_m

select nls_collation_name(208941) from dual; -- uca0700_schinese_s4_vs_bn_ny_en_fn_hn_dn_mn
select nls_collation_name(208942) from dual; -- uca0700_schinese1_s4_vs_bn_ny_en_fn_hn_dn_mn
select nls_collation_name(208943) from dual; -- uca0700_schinese2_s4_vs_bn_ny_en_fn_hn_dn_mn

select nls_collation_name(208941, 's') from dual; -- uca0700_schinese
select nls_collation_name(208942, 's') from dual; -- uca0700_schinese1
select nls_collation_name(208943, 's') from dual; -- uca0700_schinese2

select nls_collation_name(208941, 'l') from dual; -- uca0700_schinese_s4_vs_bn_ny_en_fn_hn_dn_mn
select nls_collation_name(208942, 'l') from dual; -- uca0700_schinese1_s4_vs_bn_ny_en_fn_hn_dn_mn
select nls_collation_name(208943, 'l') from dual; -- uca0700_schinese2_s4_vs_bn_ny_en_fn_hn_dn_mn

select '1' as aaa from dual order by aaa collate uca0700_schinese_s4_vs_bn_ny_en_fn_hn_dn_mn;  -- test alias
select '1' as aaa from dual order by aaa collate uca0700_schinese1_s4_vs_bn_ny_en_fn_hn_dn_mn; -- test alias
select '1' as aaa from dual order by aaa collate uca0700_schinese2_s4_vs_bn_ny_en_fn_hn_dn_mn; -- test alias

-- test 0 argument
select nlssort() from dual; -- error

-- test pseudo-collation in NLSSORT() function
select nlssort('highgo', 'NLS_SORT=USING_NLS_COMP') from dual;    -- error
select nlssort('highgo', 'NLS_SORT=USING_NLS_SORT') from dual;    -- error
select nlssort('highgo', 'NLS_SORT=USING_NLS_SORT_AI') from dual; -- error
select nlssort('highgo', 'NLS_SORT=USING_NLS_SORT_CI') from dual; -- error
select nlssort('highgo', 'NLS_SORT=USING_NLS_SORT_CS') from dual; -- error

-- test string
select nlssort('highgo') from dual;
select nlssort('highgo', 'NLS_SORT=BINARY') from dual;
select nlssort('highgo', 'nls_sort=BINARY') from dual;
select nlssort('highgo', 'nls_sort=binary') from dual;
select nlssort('highgo', 'nls_lang=US')  from dual;
select nlssort('highgo', 'nls_lang=ZHS') from dual;
select nlssort('highgo', 'nls_language=AMERICAN') from dual;

--  test space trim
select nlssort('')                              from dual; -- null
select nlssort(' ')                             from dual; -- 0x20
select nlssort(' highgo ')                      from dual; -- trim
select nlssort('highgo  ')                      from dual; -- trim
select nlssort(CAST('highgo ' as char(20)))     from dual; -- trim
select nlssort(CAST('highgo ' as varchar(20)))  from dual; -- no trim
select nlssort(CAST('highgo ' as varchar2(20))) from dual; -- no trim

select nlssort('',         'nls_sort=binary')                      from dual; -- null
select nlssort(' ',        'nls_sort=binary')                      from dual; -- 0x20
select nlssort(' highgo ', 'nls_sort=binary')                      from dual; -- trim
select nlssort('highgo  ', 'nls_sort=binary')                      from dual; -- trim
select nlssort(CAST('highgo ' as char(20)),     'nls_sort=binary') from dual; -- trim
select nlssort(CAST('highgo ' as varchar(20)),  'nls_sort=binary') from dual; -- no trim
select nlssort(CAST('highgo ' as varchar2(20)), 'nls_sort=binary') from dual; -- no trim

select 1 from  dual where nlssort('highgo  ') = nlssort('highgo        '); -- true
select 1 from  dual where nlssort('highgo  ') = nlssort('highgo        ', 'nls_sort=binary'); -- true
select 1 from  dual where nlssort('highgo  ', 'nls_sort=binary') = nlssort('highgo        '); -- true
select 1 from  dual where nlssort('highgo  ', 'nls_sort=binary') = nlssort('highgo        ', 'nls_sort=binary'); -- true

create table nlssort_space_test(v_char char(64), v_varchar varchar(64), v_varchar2 varchar2(64), v_raw raw(64), v_blob blob);
insert into nlssort_space_test values ('highgo  ', 'highgo  ', 'highgo  ', '2020', '2020');
select  nlssort(v_char),     -- trim
        nlssort(v_varchar),  -- no trim
        nlssort(v_varchar2), -- no trim
        nlssort(v_blob),     -- no trim
        nlssort(v_raw)       -- no trim
        from nlssort_space_test;

select  nlssort(v_char,     'nls_sort=binary'),     -- trim
        nlssort(v_varchar,  'nls_sort=binary'),     -- no trim
        nlssort(v_varchar2, 'nls_sort=binary'),     -- no trim
        nlssort(v_blob,     'nls_sort=binary'),     -- no trim
        nlssort(v_raw,      'nls_sort=binary')      -- no trim
        from nlssort_space_test;

drop table nlssort_space_test;

-- test exception
select nlssort('highgo', 'xxx') from dual;          -- error
select nlssort('highgo', 'nls_sort=xxx') from dual; -- error

-- BEGIN test environment variable NLS_COMP & NLS_SORT
-- arguments available check
set nls_comp=binary;
set nls_comp=linguistic;
set nls_comp=ansi;
set nls_comp=xxx; -- error

set nls_sort=binary;
set nls_sort=schinese_pinyin_m;
set nls_sort=schinese_stroke_m;
set nls_sort=schinese_radical_m;
set nls_sort=generic_m_ci;
set nls_sort=uca0700_schinese;
set nls_sort=uca0700_schinese1;
set nls_sort=uca0700_schinese2;
set nls_sort=xxx; -- error

-- binary + x = binary
set nls_comp=binary;
set nls_sort=binary;
select nlssort('毛') from dual;

set nls_sort=schinese_pinyin_m;
select nlssort('毛') from dual;

set nls_sort=schinese_stroke_m;
select nlssort('毛') from dual;

set nls_sort=schinese_radical_m;
select nlssort('毛') from dual;

set nls_sort=generic_m_ci;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese1;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese2;
select nlssort('毛') from dual;

-- linguistic + x = x
set nls_comp=linguistic;
set nls_sort=binary;
select nlssort('毛') from dual;

set nls_sort=schinese_pinyin_m;
select nlssort('毛') from dual;

set nls_sort=schinese_stroke_m;
select nlssort('毛') from dual;

set nls_sort=schinese_radical_m;
select nlssort('毛') from dual;

set nls_sort=generic_m_ci;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese毛;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese2;
select nlssort('毛') from dual;

-- ansi + x = x
set nls_comp=ansi;
set nls_sort=binary;
select nlssort('毛') from dual;

set nls_sort=schinese_pinyin_m;
select nlssort('毛') from dual;

set nls_sort=schinese_stroke_m;
select nlssort('毛') from dual;

set nls_sort=schinese_radical_m;
select nlssort('毛') from dual;

set nls_sort=generic_m_ci;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese毛;
select nlssort('毛') from dual;

set nls_sort=uca0700_schinese2;
select nlssort('毛') from dual;
-- END

-- test return value's truncate
select nlssort('北国风光，千里冰封，万里雪飘。望长城内外，惟余莽莽；大河上下，顿失滔滔。山舞银蛇，原驰蜡象，欲与天公试比高。须晴日，看红装素裹，分外妖娆。江山如此多娇，引无数英雄竞折腰。惜秦皇汉武，略输文采；唐宗宋祖，稍逊风骚。一代天骄，成吉思汗，只识弯弓射大雕。俱往矣，数风流人物，还看今朝。', 'nls_sort=SCHINESE_PINYIN_M') from dual;

-- test RAW type data
create table raw_test(val  raw(64));
insert into  raw_test(val) values('0');
insert into  raw_test(val) values('1');
insert into  raw_test(val) values('2');
insert into  raw_test(val) values('3');
insert into  raw_test(val) values('4');
insert into  raw_test(val) values('5');
insert into  raw_test(val) values('6');
insert into  raw_test(val) values('7');
insert into  raw_test(val) values('8');
insert into  raw_test(val) values('9');
insert into  raw_test(val) values('a');
insert into  raw_test(val) values('b');
insert into  raw_test(val) values('c');
insert into  raw_test(val) values('d');
insert into  raw_test(val) values('e');
insert into  raw_test(val) values('f');
insert into  raw_test(val) values('g'); -- error
insert into  raw_test(val) values('20');
insert into  raw_test(val) values('ff20');
insert into  raw_test(val) values('ff2020');
select       nlssort(val)  from raw_test;
drop   table raw_test;

-- test BLOB type data
create table blob_test(val  raw(64));
insert into  blob_test(val) values('0');
insert into  blob_test(val) values('1');
insert into  blob_test(val) values('2');
insert into  blob_test(val) values('3');
insert into  blob_test(val) values('4');
insert into  blob_test(val) values('5');
insert into  blob_test(val) values('6');
insert into  blob_test(val) values('7');
insert into  blob_test(val) values('8');
insert into  blob_test(val) values('9');
insert into  blob_test(val) values('a');
insert into  blob_test(val) values('b');
insert into  blob_test(val) values('c');
insert into  blob_test(val) values('d');
insert into  blob_test(val) values('e');
insert into  blob_test(val) values('f');
insert into  blob_test(val) values('g'); -- error
insert into  blob_test(val) values('20');
insert into  blob_test(val) values('ff20');
insert into  blob_test(val) values('ff2020');
insert into  blob_test(val) values('ff200020');
select       nlssort(val)   from blob_test;
drop table   blob_test;

-- BUG#M0000436, test collation inherit
---- When ivorysql.enable_ora_default_collation = on
set ivorysql.enable_ora_default_collation = on;
create table ora_default_collation_test(ora_default_collation_col text);
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
drop table ora_default_collation_test;

set default_collation to 'DEFAULT';
create table ora_default_collation_test(ora_default_collation_col text);
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
drop table ora_default_collation_test;

set default_collation to DEFAULT;
create table ora_default_collation_test(ora_default_collation_col text);
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
drop table ora_default_collation_test;

---- When ivorysql.enable_ora_default_collation = off
set ivorysql.enable_ora_default_collation = off;
create table ora_default_collation_test(ora_default_collation_col text);
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;


-- BUG#M0000436
---- GUC default_collation can set libc or icu collation
set ivorysql.enable_ora_default_collation = on;
set default_collation to 'DEFAULT';
set default_collation to 'C';
set default_collation to 'POSIX';
reset default_collation;

---- CREATE TABLE DEFAULT COLLATION can set libc or icu collation
set ivorysql.enable_ora_default_collation = on;
CREATE TABLE ora_default_collation_test(ora_default_collation_col text) DEFAULT COLLATION schinese_pinyin_m;
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;

CREATE TABLE ora_default_collation_test(ora_default_collation_col text) DEFAULT COLLATION DEFAULT;
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;

CREATE TABLE ora_default_collation_test(ora_default_collation_col text) DEFAULT COLLATION C;
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;

CREATE TABLE ora_default_collation_test(ora_default_collation_col text) DEFAULT COLLATION POSIX;
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;

---- if ivorysql.enable_ora_default_collation is off, CREATE TABLE DEFAULT COLLATION invalid
set ivorysql.enable_ora_default_collation = off;
CREATE TABLE ora_default_collation_test(ora_default_collation_col text) DEFAULT COLLATION schinese_pinyin_m;
select attcollation from pg_attribute where attname = 'ora_default_collation_col';
DROP TABLE ora_default_collation_test;

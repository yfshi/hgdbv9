-- CREATE EXTENSION restricted_dba;
set search_path to 'hg_security';

select is_sysdba(10);

select is_syssso(10);

select is_syssao(10);

select is_syssso('syssso'::regrole);

select is_syssao('syssao'::regrole);

--pass
alter system set se_feature.mode to on;
alter system reset se_feature.mode;

-- set se_feature.mode to on and check membership again;
set se_feature.mode to on;

select is_sysdba(10);

select is_syssso(10);

select is_syssao(10);

select is_syssso('syssso'::regrole);

select is_syssao('syssao'::regrole);

--fail
alter system set se_feature.mode to on;
alter system reset se_feature.mode;

--fail, these test case depends on other module
alter system set credcheck.enable to on;
alter system reset audit.enable;

reset search_path;

/* Begin bug#z6859 */
\c - syssso
set se_feature.mode to on;
set se_feature.restricted_dba to on;
set search_path to 'hg_security';

select is_sysdba(10);

select is_syssso(10);

select is_syssao(10);

select is_syssso('syssso'::regrole);

select is_syssao('syssao'::regrole);

reset se_feature.mode;
reset se_feature.restricted_dba;
reset search_path;
/* End - bug#z6859 */
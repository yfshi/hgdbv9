-- CREATE EXTENSION restricted_dba;
select is_sysdba(10);

select is_syssso(10);

select is_syssao(10);

select is_syssso('syssso'::regrole);

select is_syssao('syssao'::regrole);

-- set se_feature.mode to on and check membership again;
set se_feature.mode to on;

select is_sysdba(10);

select is_syssso(10);

select is_syssao(10);

select is_syssso('syssso'::regrole);

select is_syssao('syssao'::regrole);
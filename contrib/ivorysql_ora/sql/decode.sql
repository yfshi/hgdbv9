--Begin - bugz1409
create table test_case_number(a number,b varchar2(50));
insert into test_case_number values(1,'bbb'),(2,'ccc'),(null,'ddd');
create or replace view v_test_case_number as select decode(a,0,'') from test_case_number;
\d+ v_test_case_number
select * from v_test_case_number;
select * from test_case_number;
create view aa as SELECT
        CASE
            WHEN
            CASE
                WHEN test_case_number.a IS NULL AND 0 IS NULL THEN true
                WHEN test_case_number.a IS NOT NULL AND 0 IS NOT NULL AND test_case_number.a = 0::number THEN true
                ELSE false
            END THEN NULL::varchar2
            ELSE NULL::varchar2
        END AS "decode"
   FROM test_case_number;
SELECT
        CASE
            WHEN
            CASE
                WHEN test_case_number.a IS NULL AND 0 IS NULL THEN true
                WHEN test_case_number.a IS NOT NULL AND 0 IS NOT NULL AND test_case_number.a = 0::number THEN true
                ELSE false
            END THEN NULL::varchar2
            ELSE NULL::varchar2
        END AS "decode"
   FROM test_case_number;
select * from aa;
drop view v_test_case_number;
drop view aa;
drop table test_case_number;
--End - bugz1409

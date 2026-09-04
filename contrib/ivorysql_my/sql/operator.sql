/* Begin - bug#z5412 */
create table t_precedence (
    a int4,
    b integer default 10,
    c integer signed
);

insert into t_precedence (a,c) values (5,2);

-- precedence of & is higher than |
select (a << 1) | (b >> 1) & c from t_precedence; -- 10
select (a << 1) | ((b >> 1) & c) from t_precedence; -- 10
drop table t_precedence;
/* End - bug#z5412 */

/*Begin OA#2701 */
SELECT 1 AS result WHERE -1!=+1;
SELECT 1 AS result WHERE -1!=1;
SELECT 1 AS result WHERE 1!=-1;
/*End OA#2701 */
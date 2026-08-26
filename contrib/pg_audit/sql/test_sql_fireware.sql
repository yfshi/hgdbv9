--1 initdb
./initdb -D ../data -m oracle

--2 edit ../data/postgres.conf
--
audit.enable = on


audit.sql_injection_detection = on
audit.sql_injection_threshold = 60

audit.xss_detection = on
audit.xss_columns = 'public.sql_security_v01_test.plain_text=plain_text,public.sql_security_v01_test.rich_html=safe_html'
audit.xss_safe_tags = 'p,strong,em,b,i,u,a,ul,ol,li,blockquote,code,pre,br'

--3 start server
./pg_ctl -D ../data -l test.LOG start

--4 psql login
./psql -dpostgres

--5 audit dml statement
SELECT hg_security.pg_audit_stmt('select', 'fail');
SELECT hg_security.pg_audit_stmt('insert', 'fail');
SELECT hg_security.pg_audit_stmt('update', 'fail');


--6 common sql
SELECT '正常查询通过' AS result;
--target list not
SELECT 2 = 1 + 1 AS ordinary_constant_expression;

--all false
SELECT 1 AS no_rows_expected WHERE 2 = 3;

-- 预期阻断：OR 分支中的 2=1+1 会被常量折叠为真，命中 constant_tautology。
SELECT 1 WHERE false OR 2 = 1 + 1;

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：敏感文件读取函数命中 sensitive_function，文件不会被读取。
SELECT pg_read_file('postgresql.conf', 0, 10);

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：恒真条件与注释组合，审计错误中应同时包含对应规则。
SELECT 1 WHERE false OR 'a' = 'a' /* 模拟注释截断特征 */;

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

--xss test
CREATE TABLE public.sql_security_v01_test
(
    id          integer PRIMARY KEY,
    plain_text  text,
    rich_html   text
);

-- plain_text 不含 HTML，rich_html 只使用允许的标签和安全链接，应当写入成功。
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES
(
    1,
    'Alice',
    '<p>Hello <strong>world</strong> <a href="https://example.com">link</a></p>'
);

-- 安全更新应当成功。
UPDATE public.sql_security_v01_test
   SET plain_text = 'Alice Updated'
 WHERE id = 1;

 -- 预期阻断：plain_text 字段不允许出现 HTML 标签。
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (2, '<b>Alice</b>', '<p>安全内容</p>');

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：img 不在默认安全标签列表中，同时包含事件处理属性。
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (3, 'Alice', '<img src="x" onerror="alert(1)">');

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：允许的 p 标签中出现 onclick 事件处理属性。
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (4, 'Alice', '<p onclick="alert(1)">点击</p>');

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：href 使用 javascript 危险协议。
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (5, 'Alice', '<a href="javascript:alert(1)">点击</a>');

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

-- 预期阻断：UPDATE 的最终写入值同样会被检查，原值应当保持不变。
UPDATE public.sql_security_v01_test
   SET plain_text = '<script>alert(1)</script>'
 WHERE id = 1;

select sqltext,errtext, result from  hg_security.pg_audit_log WHERE result = 'FAIL';

--clean data
DROP TABLE public.sql_security_v01_test;

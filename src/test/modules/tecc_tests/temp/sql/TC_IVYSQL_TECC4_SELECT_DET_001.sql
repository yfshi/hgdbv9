-- 依赖前置建表 _query_setup.sql 中的 string_type_test / tecc_string_type_test
-- 前置条件已建好KEK/DEK和各表

-- 测试步骤
select * from string_type_test where txt_col='hello' and vc_col='hello' and bp_col='hello';
select * from tecc_string_type_test where txt_col='hello' and vc_col='hello' and bp_col='hello';

select * from string_type_test where txt_col<>'llo' or vc_col<>'hlo' or bp_col<>'hel';
select * from tecc_string_type_test where txt_col<>'llo' or vc_col<>'hlo' or bp_col<>'hel';

SELECT txt_col = 'Hello' as text,vc_col = 'Hello' as varchar,bp_col = 'Hello' as bpchar FROM string_type_test WHERE txt_col IN ('hello', 'Hello');
SELECT txt_col = 'Hello' as text,vc_col = 'Hello' as varchar,bp_col = 'Hello' as bpchar FROM tecc_string_type_test WHERE txt_col IN ('hello', 'Hello');

SELECT txt_col = '  hello  ' as text,vc_col = '  hello  ' as varchar,bp_col = '  hello  ' as bpchar FROM string_type_test WHERE txt_col = '  hello  ';
SELECT txt_col = '  hello  ' as text,vc_col = '  hello  ' as varchar,bp_col = '  hello  ' as bpchar FROM tecc_string_type_test WHERE txt_col = '  hello  ';

SELECT bp_col = 'hello' as equals,bp_col = 'hello     ' as spaces FROM string_type_test WHERE bp_col = 'hello';
SELECT bp_col = 'hello' as equals,bp_col = 'hello     ' as spaces FROM tecc_string_type_test WHERE bp_col = 'hello';

SELECT txt_col IS NULL as text,vc_col IS NULL as varchar,bp_col IS NULL as bpchar, txt_col = vc_col as te_vc, vc_col = bp_col as vc_bp FROM string_type_test WHERE txt_col IS NULL;
SELECT txt_col IS NULL as text,vc_col IS NULL as varchar,bp_col IS NULL as bpchar, txt_col = vc_col as te_vc, vc_col = bp_col as vc_bp FROM tecc_string_type_test WHERE txt_col IS NULL;

SELECT txt_col = '' as text,vc_col = '' as varchar,bp_col = '' as bpchar, txt_col = vc_col as te_vc,vc_col = bp_col as va_bp FROM string_type_test WHERE txt_col = '';
SELECT txt_col = '' as text,vc_col = '' as varchar,bp_col = '' as bpchar, txt_col = vc_col as te_vc,vc_col = bp_col as va_bp FROM tecc_string_type_test WHERE txt_col = '';

SELECT txt_col IS DISTINCT FROM vc_col as te_va,vc_col IS DISTINCT FROM bp_col as va_bp FROM string_type_test;
SELECT txt_col IS DISTINCT FROM vc_col as te_va,vc_col IS DISTINCT FROM bp_col as va_bp FROM tecc_string_type_test;

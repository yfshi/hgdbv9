/* ReqID: SRS-MYSQL-NUMBERICTYPE-UINT123 */
UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysuint4_out')
WHERE oid = 9210;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysuint4_send')
WHERE oid = 9210;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodin')
WHERE oid = 9210;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodout')
WHERE oid = 9210;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodin')
WHERE oid = 9211;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodout')
WHERE oid = 9211;

alter domain ivymy.uint3 add constraint uint3_check check(value >= 0 and value <= 16777215);


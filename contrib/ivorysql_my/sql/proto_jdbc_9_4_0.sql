/* Begin - BUG#7733 */
set ivorysql.compatible_mode = PG;

CREATE USER proto_user WITH PASSWORD 'ivorysql@123';
GRANT ALL ON SCHEMA public TO proto_user;
ALTER USER proto_user WITH PASSWORD 'ivorysql@123';

set ivorysql.compatible_mode = MYSQL;

-- compile java source file
\! javac -classpath proto_test/class/junixsocket-common-2.6.2.jar:proto_test/class/junixsocket-mysql-2.6.2.jar proto_test/class/*.java

-- wrong password
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcPrepareTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

select * from t3;

\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcAutoCommitTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

select relname from pg_class where relname like '%autocommit_test%' order by relname;

select * from autocommit_test6;

\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/junixsocket-common-2.6.2.jar:proto_test/class/junixsocket-mysql-2.6.2.jar:proto_test/class/junixsocket-native-common-2.6.2.jar:proto_test/class/. MySQLJdbcAutoIncrementTest 127.0.0.1 $MYSPORT db_uds $USER

-- prepare
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLLongDataExample 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLPreparedStatementExample 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcStmtPrepareTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcStmtPrepareTest_2 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcStmtResetTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- /* Begin - ReqID:SRS-MYSQL-SQL-LOADDATA */
-- load data
-- allowLoadLocalInfile default is on
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcLoadDataLocalTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- allowLoadLocalInfile set to false manual
-- return an error
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcLoadDataLocalOffTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- set server local_infile to off
show local_infile;
alter system set local_infile = off;
select pg_reload_conf();
-- return an error
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcLoadDataLocalTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- set server local_infile to on
show local_infile;
alter system set local_infile = on;
select pg_reload_conf();
-- success
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcLoadDataLocalTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- select into test
-- return an error as proto_user is not superuser, so could not write file on server
\du proto_user
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcSelectIntoFileTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- give super priv to proto_user
ALTER USER proto_user WITH SUPERUSER;
\du proto_user
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcSelectIntoFileTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
ALTER USER proto_user WITH NOSUPERUSER;
\du proto_user
-- /* End - ReqID:SRS-MYSQL-SQL-LOADDATA */

-- fetch
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcFetchTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- very long sql
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLInsertLongData 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- test DatabaseMetaData.getColumns
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcGetColumnsTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

-- test DatabaseMetaData.getColumns
\! java -cp proto_test/class/mysql-connector-java-5.1.47.jar:proto_test/class/. MySQLJdbcTestIntroducer 127.0.0.1 $MYSPORT public proto_user ivorysql@123

-- TIME/DATETIME/TIMESTAMP fractional seconds
\! java -cp proto_test/class/mysql-connector-j-9.4.0.jar:proto_test/class/. MySQLJdbcTimeDecimalTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- final clean
RESET ROLE;
DROP USER IF EXISTS proto_user CASCADE;
\! rm proto_test/class/*.class
/* END - BUG#7733 */
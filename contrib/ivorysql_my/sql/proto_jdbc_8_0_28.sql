set ivorysql.compatible_mode = PG;

CREATE USER proto_user WITH PASSWORD 'ivorysql@123';
GRANT ALL ON SCHEMA public TO proto_user;
ALTER USER proto_user WITH PASSWORD 'ivorysql@123';

set ivorysql.compatible_mode = MYSQL;

-- compile java source file
\! javac -classpath proto_test/class/junixsocket-common-2.6.2.jar:proto_test/class/junixsocket-mysql-2.6.2.jar proto_test/class/*.java

-- wrong password
\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcPrepareTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

select * from t3;

\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcAutoCommitTest 127.0.0.1 $MYSPORT public proto_user ivorysql@123

select relname from pg_class where relname like '%autocommit_test%' order by relname;

select * from autocommit_test6;

\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/junixsocket-common-2.6.2.jar:proto_test/class/junixsocket-mysql-2.6.2.jar:proto_test/class/junixsocket-native-common-2.6.2.jar:proto_test/class/. MySQLJdbcAutoIncrementTest 127.0.0.1 $MYSPORT db_uds $USER

-- prepare
\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLLongDataExample 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLPreparedStatementExample 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcStmtPrepareTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123
\! java -cp proto_test/class/mysql-connector-java-8.0.28.jar:proto_test/class/. MySQLJdbcStmtResetTest 127.0.0.1 $MYSPORT public proto_user wrong_pass@123

-- final clean
RESET ROLE;
DROP USER IF EXISTS proto_user CASCADE;
\! rm proto_test/class/*.class
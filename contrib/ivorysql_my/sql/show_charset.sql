show charset;
show charset like 'utf8';
show charset like 'UTF8';
show charset where charset = 'utf8';
show charset where charset = 'UTF8';
show charset where maxlen > 1;

show character set;
show character set like 'utf8';
show character set like 'UTF8';
show character set where charset = 'utf8';
show character set where charset = 'UTF8';
show character set where maxlen > 1;

show collation;

CREATE USER mysql_show_test_user WITH superuser;

-- unix regress
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8mb3
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8mb4
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=ascii
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=binary
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1250
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1251
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1256
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1257
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp866
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=euckr
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gb18030
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gb2312
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gbk
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=greek
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=hebrew
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=koi8r
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=koi8u
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin1
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin2
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin5
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin7
\! psql -p $PGPORT -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=ujis

-- windows regress
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8mb3
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=utf8mb4
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=ascii
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=binary
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1250
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1251
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1256
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp1257
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=cp866
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=euckr
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gb18030
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gb2312
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=gbk
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=greek
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=hebrew
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=koi8r
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=koi8u
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin1
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin2
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin5
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=latin7
\! psql -p %PGPORT% -d postgres -U mysql_show_test_user -f ./sql/collation.sql -v charset=ujis

DROP USER mysql_show_test_user;

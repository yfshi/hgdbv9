CREATE TYPE mys_show_charset_results AS ("Charset" TEXT, 
                                         "Description" TEXT, 
                                         "Default collation" TEXT, 
                                         "Maxlen" INT4);

CREATE FUNCTION pg_catalog.show_mysql_charsets()
RETURNS setof mys_show_charset_results 
AS '$libdir/ivorysql_my','show_mysql_charsets' 
LANGUAGE C
PARALLEL SAFE;

CREATE TYPE mys_show_collation_results AS ("Collation" TEXT, 
                                           "Charset" TEXT, 
                                           "Id" INT4,
                                           "Default" TEXT, 
                                           "Compiled" TEXT, 
                                           "Sortlen" INT4);

CREATE FUNCTION pg_catalog.show_mysql_collations()
RETURNS setof mys_show_collation_results 
AS '$libdir/ivorysql_my','show_mysql_collations' 
LANGUAGE C
PARALLEL SAFE;

CREATE TYPE mys_show_charset_results AS ("Charset" VARCHAR, 
                                         "Description" VARCHAR, 
                                         "Default collation" VARCHAR, 
                                         "Maxlen" INT8);

CREATE FUNCTION pg_catalog.show_mysql_charsets()
RETURNS setof mys_show_charset_results 
AS '$libdir/ivorysql_my','show_mysql_charsets' 
LANGUAGE C
PARALLEL SAFE;

CREATE TYPE mys_show_collation_results AS ("Collation" VARCHAR, 
                                           "Charset" VARCHAR, 
                                           "Id" INT8,
                                           "Default" VARCHAR, 
                                           "Compiled" VARCHAR, 
                                           "Sortlen" INT8);

CREATE FUNCTION pg_catalog.show_mysql_collations()
RETURNS setof mys_show_collation_results 
AS '$libdir/ivorysql_my','show_mysql_collations' 
LANGUAGE C
PARALLEL SAFE;

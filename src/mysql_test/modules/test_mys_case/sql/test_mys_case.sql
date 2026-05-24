/* Begin - ReqID:SRS-MYSQL-CASE */
--
-- Tests for mys_case_switch control file parameter.
--
SET ivorysql.compatible_mode TO mysql;

SET lower_case_table_names = 0;

-- mys_case_switch=false: 
-- MysExtOffTable should be stored as 'mysextofftable' (lowercased by PG)
CREATE EXTENSION mys_ext_case_off;
SELECT relname FROM pg_class WHERE relname IN ('mysextofftable', 'MysExtOffTable') ORDER BY 1;
DROP EXTENSION mys_ext_case_off;

-- mys_case_switch=true:
-- MysExtOnTable should be stored as 'MysExtOnTable' (case preserved, lctn=0)
CREATE EXTENSION mys_ext_case_on;
SELECT relname FROM pg_class WHERE relname IN ('mysextontable', 'MysExtOnTable') ORDER BY 1;
DROP EXTENSION mys_ext_case_on;

-- ----------------------------------------------------------------
-- lower_case_table_names = 1 (all-lowercase mode)
-- In this mode MySQL always lowercases identifiers.
-- ----------------------------------------------------------------
SET lower_case_table_names = 1;

-- mys_case_switch=false: case handling OFF during install → lowercased
CREATE EXTENSION mys_ext_case_off;
SELECT relname FROM pg_class WHERE relname IN ('mysextofftable', 'MysExtOffTable') ORDER BY 1;
DROP EXTENSION mys_ext_case_off;

-- mys_case_switch=true: case handling ON during install; lctn=1 → still lowercase
CREATE EXTENSION mys_ext_case_on;
SELECT relname FROM pg_class WHERE relname IN ('mysextontable', 'MysExtOnTable') ORDER BY 1;
DROP EXTENSION mys_ext_case_on;

-- cleanup
RESET ivorysql.compatible_mode;
RESET lower_case_table_names;

/* End - ReqID:SRS-MYSQL-CASE */

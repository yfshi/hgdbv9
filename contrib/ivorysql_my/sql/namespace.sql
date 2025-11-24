/* Begin - bug#6338 */
select current_database();
create database test_database with encoding 'latin1' template template0 lc_collate 'C' lc_ctype 'C';
\c test_database
show ivorysql.compatible_mode;
create table t(a tinytext);
\d t
/* End - bug#6338 */
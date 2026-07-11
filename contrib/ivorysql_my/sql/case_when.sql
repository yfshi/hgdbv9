/* Begin - bug#8556 */
CREATE TABLE t(id int, age int, collation int);
SELECT collation, CASE WHEN id = 1 && age = 2 THEN 1 END FROM t;
/* End - bug#8556 */

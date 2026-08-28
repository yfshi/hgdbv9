create collation mysql_collation_copy from utf8_general_ci;
select 'a' = 'A' collate mysql_collation_copy;


/* Begin - BUG#9071 */
CREATE COLLATION utf8mb4_0900_ai_ci3 (provider = mysql, locale = 'utf8mb3_general_ci');
SELECT
    collname,
    collnamespace::regnamespace,
    collprovider,
    collisdeterministic,
    collencoding,
    collcollate,
    collctype,
    collversion
FROM pg_collation
WHERE collname = 'utf8mb4_0900_ai_ci3';
DROP COLLATION utf8mb4_0900_ai_ci3;
/* End - BUG#9071 */
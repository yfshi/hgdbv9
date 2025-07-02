CREATE EXTENSION fuzzystrmatch;


SELECT pg_catalog.soundex('hello world!');

SELECT pg_catalog.soundex('Anne'), pg_catalog.soundex('Ann'), difference('Anne', 'Ann');
SELECT pg_catalog.soundex('Anne'), pg_catalog.soundex('Andrew'), difference('Anne', 'Andrew');
SELECT pg_catalog.soundex('Anne'), pg_catalog.soundex('Margaret'), difference('Anne', 'Margaret');
SELECT pg_catalog.soundex(''), difference('', '');


SELECT levenshtein('GUMBO', 'GAMBOL');
SELECT levenshtein('GUMBO', 'GAMBOL', 2, 1, 1);
SELECT levenshtein_less_equal('extensive', 'exhaustive', 2);
SELECT levenshtein_less_equal('extensive', 'exhaustive', 4);


SELECT metaphone('GUMBO', 4);


SELECT dmetaphone('gumbo');
SELECT dmetaphone_alt('gumbo');

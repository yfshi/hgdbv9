--
-- encoding-sensitive tests for json and jsonb
--

-- We provide expected-results files for UTF8 (json_encoding.out)
-- and for SQL_ASCII (json_encoding_1.out).  Skip otherwise.
SELECT getdatabaseencoding() NOT IN ('UTF8', 'SQL_ASCII')
       AS skip_test \gset
\if :skip_test
\quit
\endif

SELECT getdatabaseencoding();           -- just to label the results files

-- first json

-- basic unicode input
SELECT '"\u"'::json;			-- ERROR, incomplete escape
SELECT '"\u00"'::json;			-- ERROR, incomplete escape
SELECT '"\u000g"'::json;		-- ERROR, g is not a hex digit
SELECT '"\u0000"'::json;		-- OK, legal escape
SELECT '"\uaBcD"'::json;		-- OK, uppercase and lower case both OK

-- handling of unicode surrogate pairs

select CAST('{ "a":  "\ud83d\ude04\ud83d\udc36" }' AS json) -> 'a' as correct_in_utf8;
select CAST('{ "a":  "\ud83d\ud83d" }' AS json) -> 'a'; -- 2 high surrogates in a row
select CAST('{ "a":  "\ude04\ud83d" }' AS json) -> 'a'; -- surrogates in wrong order
select CAST('{ "a":  "\ud83dX" }' AS json) -> 'a'; -- orphan high surrogate
select CAST('{ "a":  "\ude04X" }' AS json) -> 'a'; -- orphan low surrogate

--handling of simple unicode escapes

select CAST('{ "a":  "the Copyright \u00a9 sign" }' AS json) as correct_in_utf8;
select CAST('{ "a":  "dollar \u0024 character" }' AS json) as correct_everywhere;
select CAST('{ "a":  "dollar \\u0024 character" }' AS json) as not_an_escape;
select CAST('{ "a":  "null \u0000 escape" }' AS json) as not_unescaped;
select CAST('{ "a":  "null \\u0000 escape" }' AS json) as not_an_escape;

select CAST('{ "a":  "the Copyright \u00a9 sign" }' AS json) ->> 'a' as correct_in_utf8;
select CAST('{ "a":  "dollar \u0024 character" }' AS json) ->> 'a' as correct_everywhere;
select CAST('{ "a":  "dollar \\u0024 character" }' AS json) ->> 'a' as not_an_escape;
select CAST('{ "a":  "null \u0000 escape" }' AS json) ->> 'a' as fails;
select CAST('{ "a":  "null \\u0000 escape" }' AS json) ->> 'a' as not_an_escape;

-- then jsonb

-- basic unicode input
SELECT '"\u"'::jsonb;			-- ERROR, incomplete escape
SELECT '"\u00"'::jsonb;			-- ERROR, incomplete escape
SELECT '"\u000g"'::jsonb;		-- ERROR, g is not a hex digit
SELECT '"\u0045"'::jsonb;		-- OK, legal escape
SELECT '"\u0000"'::jsonb;		-- ERROR, we don't support U+0000
-- use octet_length here so we don't get an odd unicode char in the
-- output
SELECT octet_length('"\uaBcD"'::jsonb::text); -- OK, uppercase and lower case both OK

-- handling of unicode surrogate pairs

SELECT octet_length((CAST('{ "a":  "\ud83d\ude04\ud83d\udc36" }' AS jsonb) -> 'a')::text) AS correct_in_utf8;
SELECT CAST('{ "a":  "\ud83d\ud83d" }' AS jsonb) -> 'a'; -- 2 high surrogates in a row
SELECT CAST('{ "a":  "\ude04\ud83d" }' AS jsonb) -> 'a'; -- surrogates in wrong order
SELECT CAST('{ "a":  "\ud83dX" }' AS jsonb) -> 'a'; -- orphan high surrogate
SELECT CAST('{ "a":  "\ude04X" }' AS jsonb) -> 'a'; -- orphan low surrogate

-- handling of simple unicode escapes

SELECT CAST('{ "a":  "the Copyright \u00a9 sign" }' AS jsonb) as correct_in_utf8;
SELECT CAST('{ "a":  "dollar \u0024 character" }' AS jsonb) as correct_everywhere;
SELECT CAST('{ "a":  "dollar \\u0024 character" }' AS jsonb) as not_an_escape;
SELECT CAST('{ "a":  "null \u0000 escape" }' AS jsonb) as fails;
SELECT CAST('{ "a":  "null \\u0000 escape" }' AS jsonb) as not_an_escape;

SELECT CAST('{ "a":  "the Copyright \u00a9 sign" }' AS jsonb) ->> 'a' as correct_in_utf8;
SELECT CAST('{ "a":  "dollar \u0024 character" }' AS jsonb) ->> 'a' as correct_everywhere;
SELECT CAST('{ "a":  "dollar \\u0024 character" }' AS jsonb) ->> 'a' as not_an_escape;
SELECT CAST('{ "a":  "null \u0000 escape" }' AS jsonb) ->> 'a' as fails;
SELECT CAST('{ "a":  "null \\u0000 escape" }' AS jsonb) ->> 'a' as not_an_escape;

SET ivorysql.enable_emptystring_to_null to off;

--
-- SM3 family
--

-- SM3
SELECT encode(digest('', 'sm3'), 'hex');
SELECT encode(digest('a', 'sm3'), 'hex');
SELECT encode(digest('abc', 'sm3'), 'hex');
SELECT encode(digest('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq', 'sm3'), 'hex');
SELECT encode(digest('12345678901234567890123456789012345678901234567890123456789012345678901234567890', 'sm3'), 'hex');

RESET ivorysql.enable_emptystring_to_null;
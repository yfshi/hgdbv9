--
-- SHA1 message digest
--

SET ivorysql.enable_emptystring_to_null to off;
SELECT encode(digest('', 'sha1'), 'hex');
RESET ivorysql.enable_emptystring_to_null;
SELECT encode(digest('a', 'sha1'), 'hex');
SELECT encode(digest('abc', 'sha1'), 'hex');
SELECT encode(digest('message digest', 'sha1'), 'hex');
SELECT encode(digest('abcdefghijklmnopqrstuvwxyz', 'sha1'), 'hex');
SELECT encode(digest('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789', 'sha1'), 'hex');
SELECT encode(digest('12345678901234567890123456789012345678901234567890123456789012345678901234567890', 'sha1'), 'hex');

--
-- SM4 cipher
--
-- ensure consistent test output regardless of the default bytea format
SET bytea_output TO escape;

-- some standard Rijndael testvalues
SELECT encode(encrypt(
decode('00112233445566778899aabbccddeeff', 'hex'),
decode('000102030405060708090a0b0c0d0e0f', 'hex'),
'sm4-ecb/pad:none'), 'hex');

SELECT encode(encrypt(
decode('00112233445566778899aabbccddeeff', 'hex'),
decode('000102030405060708090a0b0c0d0e0f1011121314151617', 'hex'),
'sm4-ecb/pad:none'), 'hex');

SELECT encode(encrypt(
decode('00112233445566778899aabbccddeeff', 'hex'),
decode('000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f', 'hex'),
'sm4-ecb/pad:none'), 'hex');

-- cbc
SELECT encode(encrypt(
decode('00112233445566778899aabbccddeeff', 'hex'),
decode('000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f', 'hex'),
'sm4-cbc/pad:none'), 'hex');

-- key padding

SELECT encode(encrypt(
decode('0011223344', 'hex'),
decode('000102030405', 'hex'),
'sm4-cbc'), 'hex');

SELECT encode(encrypt(
decode('0011223344', 'hex'),
decode('000102030405060708090a0b0c0d0e0f10111213', 'hex'),
'sm4-cbc'), 'hex');

SELECT encode(encrypt(
decode('0011223344', 'hex'),
decode('000102030405060708090a0b0c0d0e0f101112131415161718191a1b', 'hex'),
'sm4-cbc'), 'hex');

-- empty data
SET ivorysql.enable_emptystring_to_null to off;
select encode(encrypt('', 'foo', 'sm4'), 'hex');
-- 10 bytes key
select encode(encrypt('foo', '0123456789', 'sm4'), 'hex');
-- 22 bytes key
select encode(encrypt('foo', '0123456789012345678901', 'sm4'), 'hex');

-- decrypt
select decrypt(encrypt('foo', '0123456', 'sm4'), '0123456', 'sm4');

-- iv
select encode(encrypt_iv('foo', '0123456', 'abcd', 'sm4'), 'hex');
select decrypt_iv(decode('c9e8290bec346d69a12cb3e0d3c3a783', 'hex'),
		'0123456', 'abcd', 'sm4');

-- long message
select encode(encrypt('Lets try a longer message.', '0123456789', 'sm4'), 'hex');
select decrypt(encrypt('Lets try a longer message.', '0123456789', 'sm4'), '0123456789', 'sm4');

-- 中文
select convert_from(decrypt('\x06d2283c71227bbf3ac9708f4848a0c7', '0123456789', 'sm4'), 'utf8');

RESET ivorysql.enable_emptystring_to_null;

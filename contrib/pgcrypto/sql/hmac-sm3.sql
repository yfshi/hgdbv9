--
-- HMAC-SHA1
--

SELECT encode(hmac(
'Hi There',
decode('0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b', 'hex'),
'sm3'), 'hex');

-- 2
SELECT encode(hmac(
'Jefe',
'what do ya want for nothing?',
'sm3'), 'hex');

-- 3
SELECT encode(hmac(
decode('dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd', 'hex'),
decode('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa', 'hex'),
'sm3'), 'hex');

-- 4
SELECT encode(hmac(
decode('cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd', 'hex'),
decode('0102030405060708090a0b0c0d0e0f10111213141516171819', 'hex'),
'sm3'), 'hex');

-- 5
SELECT encode(hmac(
'Test With Truncation',
decode('0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c', 'hex'),
'sm3'), 'hex');

-- 6
SELECT encode(hmac(
'Test Using Larger Than Block-Size Key - Hash Key First',
decode('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa', 'hex'),
'sm3'), 'hex');

-- 7
SELECT encode(hmac(
'Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data',
decode('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa', 'hex'),
'sm3'), 'hex');

/* contrib/pgcrypto/pgcrypto--1.3.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgcrypto" to load this file. \quit

CREATE FUNCTION digest(text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_digest'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION digest(bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_digest'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hmac(text, text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_hmac'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hmac(bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_hmac'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION crypt(text, text)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_crypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION gen_salt(text)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_gen_salt'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION gen_salt(text, int4)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_gen_salt_rounds'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION encrypt(bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_encrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION decrypt(bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_decrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION encrypt_iv(bytea, bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_encrypt_iv'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION decrypt_iv(bytea, bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_decrypt_iv'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION gen_random_bytes(int4)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_random_bytes'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE FUNCTION gen_random_uuid()
RETURNS uuid
AS 'MODULE_PATHNAME', 'pg_random_uuid'
LANGUAGE C VOLATILE PARALLEL SAFE;

--
-- pgp_sym_encrypt(data, key)
--
CREATE FUNCTION pgp_sym_encrypt(text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_encrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_sym_encrypt_bytea(bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_encrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_sym_encrypt(data, key, args)
--
CREATE FUNCTION pgp_sym_encrypt(text, text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_encrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_sym_encrypt_bytea(bytea, text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_encrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_sym_decrypt(data, key)
--
CREATE FUNCTION pgp_sym_decrypt(bytea, text)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_sym_decrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_sym_decrypt_bytea(bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_decrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_sym_decrypt(data, key, args)
--
CREATE FUNCTION pgp_sym_decrypt(bytea, text, text)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_sym_decrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_sym_decrypt_bytea(bytea, text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_sym_decrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_pub_encrypt(data, key)
--
CREATE FUNCTION pgp_pub_encrypt(text, bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_encrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_pub_encrypt_bytea(bytea, bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_encrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_pub_encrypt(data, key, args)
--
CREATE FUNCTION pgp_pub_encrypt(text, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_encrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_pub_encrypt_bytea(bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_encrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_pub_decrypt(data, key)
--
CREATE FUNCTION pgp_pub_decrypt(bytea, bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_pub_decrypt_bytea(bytea, bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_pub_decrypt(data, key, psw)
--
CREATE FUNCTION pgp_pub_decrypt(bytea, bytea, text)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_pub_decrypt_bytea(bytea, bytea, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp_pub_decrypt(data, key, psw, arg)
--
CREATE FUNCTION pgp_pub_decrypt(bytea, bytea, text, text)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_text'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_pub_decrypt_bytea(bytea, bytea, text, text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pgp_pub_decrypt_bytea'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- PGP key ID
--
CREATE FUNCTION pgp_key_id(bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'pgp_key_id_w'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- pgp armor
--
CREATE FUNCTION armor(bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_armor'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION armor(bytea, text[], text[])
RETURNS text
AS 'MODULE_PATHNAME', 'pg_armor'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION dearmor(text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'pg_dearmor'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION pgp_armor_headers(text, key OUT text, value OUT text)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'pgp_armor_headers'
LANGUAGE C STRICT PARALLEL SAFE;

--
-- HSM api
--
CREATE FUNCTION hsm_generate_key(algname cstring, lenbit int, kekid int)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_generate_key'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_import_key(algname cstring, kekid int, rkey bytea)
RETURNS int
AS 'MODULE_PATHNAME', 'hsm_import_key'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_generate_key_pair(algname cstring, lenbit int)
RETURNS TABLE(key bytea)
AS 'MODULE_PATHNAME', 'hsm_generate_key_pair'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_import_key_pair(algname cstring, pubkey bytea, prikey bytea, mode cstring default 'all')
RETURNS int
AS 'MODULE_PATHNAME', 'hsm_import_key_pair'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_destroy_key(id int)
RETURNS void
AS 'MODULE_PATHNAME', 'hsm_destroy_key'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE TYPE hsm_keys_record AS (id int, algname text, mode text, kekid int, lenbit int);
CREATE FUNCTION hsm_show_keys()
RETURNS SETOF hsm_keys_record
AS 'MODULE_PATHNAME', 'hsm_show_keys'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_encrypt(algname cstring, id int, plaintext bytea, iv bytea default '')
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_encrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_decrypt(algname cstring, id int, ciphertext bytea, iv bytea default '')
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_decrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_gen_random_bytes(len int)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_gen_random_bytes'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_hash(algname cstring, datatext bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_hash'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_pub_encrypt(algname cstring, id int, plaintext bytea, keylenbits int default 0)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_pub_encrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_pub_decrypt(algname cstring, id int, ciphertext bytea, 
                                passwd cstring default '', keylenbits int default 0)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_pub_decrypt'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_sign(algname cstring, id int, datatext bytea, 
                         passwd cstring default '', keylenbits int default 0)
RETURNS bytea
AS 'MODULE_PATHNAME', 'hsm_sign'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION hsm_verify(algname cstring, id int, datatext bytea, signtext bytea, 
                           keylenbits int default 0)
RETURNS void
AS 'MODULE_PATHNAME', 'hsm_verify'
LANGUAGE C STRICT PARALLEL SAFE;

/*-------------------------------------------------------------------------
 *
 * tecc_sm2.h
 *    SM2 envelope encryption for TECC key management
 *
 * Provides SM2 asymmetric encryption of HE (homomorphic encryption)
 * private keys. The KEK is an SM2 key pair: its public key (65-byte
 * uncompressed point) encrypts the HE private key, and its private key
 * (PKCS#8 DER) decrypts it.
 *
 * Portions Copyright (c) 2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2021, HighGo Software Co.,Ltd.
 *
 * src/include/common/tecc_key/tecc_sm2.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_SM2_H
#define TECC_SM2_H

/* SM2 error codes */
#define TECC_SM2_SUCCESS			0
#define TECC_SM2_ERR_NULL			-1
#define TECC_SM2_ERR_MEMORY			-2
#define TECC_SM2_ERR_CRYPTO			-3
#define TECC_SM2_ERR_INVALID		-4

/*
 * Encrypt a plaintext buffer with an SM2 public key (envelope encryption).
 *
 * @param pub_point  65-byte uncompressed EC point (04 || X || Y) of SM2 pubkey
 * @param pub_len    Length of pub_point (65)
 * @param plain      Plaintext to encrypt (e.g. HE private key bytes)
 * @param plain_len  Length of plaintext
 * @param envelope   Output: SM2 ciphertext (OPENSSL allocated, caller OPENSSL_free)
 * @param env_len    Output: ciphertext length
 * @return TECC_SM2_SUCCESS on success, negative on error
 */
int tecc_sm2_envelope_encrypt(const unsigned char *pub_point, size_t pub_len,
                              const unsigned char *plain, size_t plain_len,
                              unsigned char **envelope, size_t *env_len);

/*
 * Decrypt an SM2 envelope with an SM2 private key (PKCS#8 DER).
 *
 * @param priv_der  PKCS#8 DER encoded SM2 private key
 * @param priv_len  Length of priv_der
 * @param envelope  SM2 ciphertext from tecc_sm2_envelope_encrypt
 * @param env_len   Length of envelope
 * @param plain     Output: decrypted plaintext (OPENSSL allocated, caller OPENSSL_free)
 * @param plain_len Output: plaintext length
 * @return TECC_SM2_SUCCESS on success, negative on error
 */
int tecc_sm2_envelope_decrypt(const unsigned char *priv_der, size_t priv_len,
                              const unsigned char *envelope, size_t env_len,
                              unsigned char **plain, size_t *plain_len);

/*
 * Derive the 65-byte uncompressed public point from a PKCS#8 DER SM2
 * private key. Used by ALTER KEK to re-envelope HE private keys with the
 * new KEK's public key.
 *
 * @param priv_der  PKCS#8 DER encoded SM2 private key
 * @param priv_len  Length of priv_der
 * @param pub_point Output: 65-byte uncompressed EC point (OPENSSL alloc)
 * @param pub_len   Output: pub_point length
 * @return TECC_SM2_SUCCESS on success, negative on error
 */
int tecc_sm2_derive_pubkey(const unsigned char *priv_der, size_t priv_len,
                           unsigned char **pub_point, size_t *pub_len);

/*
 * ReqID:TECC4.0 - HE 私钥信封的分块容器（密码机托管 KEK 下 he-mul 必需）。
 *
 * 背景：KEK 托管在密码机时，HE 私钥信封只能由设备内 SM2 私钥解封，而设备
 * ECCCipher.C 只有 136 字节（真机实测：C2=136 可用、137 起返回 0x01000804，
 * 且 SDF_InternalDecrypt_BigData_ECC 并不扩容）。he-mul 的 FF-ElGamal 私钥
 * 是 256 字节（2048 位群标量，参数不可缩），单个信封放不下，故拆成 ≤128
 * 字节的分块、各自独立封装后串联存放。
 *
 * 容器格式（存于 hg_dek_args.he_*_privkey 的 hex 文本）：
 *   单信封（legacy）：<hex>
 *   分块容器       ："v2:" <hex> (":" <hex>)*
 * 前缀 'v2:' 含非十六进制字符，与 legacy 的纯 hex 天然可区分；legacy 数据
 * （本端 KEK 为 localkms 时，以及历史数据）读取路径保持不变。
 */
#define TECC_HE_ENV_CHUNK_PREFIX "v2:"
#define TECC_HE_ENV_CHUNK_MAX    128

/* 是否分块容器（"v2:" 前缀）：1 是 / 0 否 */
int tecc_sm2_env_is_chunked(const char *env_hex);

/* 分块数：容器为块数（>=1），legacy 恒为 1，非法/空为 0 */
int tecc_sm2_env_chunk_count(const char *env_hex);

/*
 * 取第 idx 块的 hex 文本（malloc 分配，调用方 free）。
 * legacy 时 idx 必须为 0，返回整体的一份副本。
 * @return TECC_SM2_SUCCESS / TECC_SM2_ERR_INVALID / TECC_SM2_ERR_MEMORY
 */
int tecc_sm2_env_chunk_hex(const char *env_hex, int idx, char **out_hex);

/*
 * 用 SM2 公钥封装明文并输出为 hex 文本。
 *
 * max_plain_per_chunk > 0 且明文长度超过该值时输出分块容器（"v2:..."），
 * 否则输出单个信封的裸 hex（与历史行为逐字节一致）。
 * 传 0 表示"不分块"（KEK 为 localkms 时用，避免制造与本端无关的新格式）。
 *
 * @return TECC_SM2_SUCCESS on success, negative on error
 */
int tecc_sm2_envelope_encrypt_hex(const unsigned char *pub_point, size_t pub_len,
                                  const unsigned char *plain, size_t plain_len,
                                  size_t max_plain_per_chunk, char **out_hex);

#endif /* TECC_SM2_H */

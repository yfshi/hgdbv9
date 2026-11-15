/*
 * kek_file.c
 *   KEK file read/write functions — shared between libpq and tecc_ktool
 *
 *   Provides the same logic as kms_adapter.c encrypt_kek_for_storage/
 *   decrypt_kek_from_storage, but accepts MEK as explicit parameter
 *   instead of FIXED_MASTER_KEY.
 *
 *   File format:
 *     magic(7) + ver(1) + reserved(8) + algo_id(4)
 *     + salt(16) + iv(16) + SM4-CBC ciphertext + HMAC-SHA256(32)
 *
 *   src/include/common/tecc_key/kek_file.h
 */

#ifndef KEK_FILE_H
#define KEK_FILE_H

#include "c.h"
#include <stddef.h>
#include <stdint.h>

#define KEK_FILE_MAGIC      "TECCKEK"
#define KEK_FILE_MAGIC_LEN  7
#define KEK_FILE_SALT_SIZE  16
#define KEK_FILE_IV_SIZE    16
#define KEK_FILE_HMAC_SIZE  32
#define KEK_FILE_HEADER_SIZE 16

/*
 * Encrypt a plaintext KEK (32B symmetric key or SM2 private key DER)
 * and write to a file, using MEK for protection.
 *
 *   mek:      32-byte master key
 *   plain:    plaintext key data
 *   plain_len: length of plaintext key data
 *   path:     output file path
 *
 * Returns 0 on success, -1 on error.
 */
extern int tecc_kek_encrypt_file(
    const unsigned char *mek, size_t mek_len,
    const unsigned char *plain, size_t plain_len,
    const char *path);

/*
 * Decrypt a KEK file and return the plaintext key data.
 *
 *   mek:      32-byte master key
 *   path:     input file path
 *   plain:    output pointer — allocated by the function, caller must pfree
 *   plain_len:output length
 *
 * Returns 0 on success, -1 on error.
 */
extern int tecc_kek_decrypt_file(
    const unsigned char *mek, size_t mek_len,
    const char *path,
    unsigned char **plain, size_t *plain_len);

/*
 * Verify HMAC of a KEK file using MEK.
 * Returns 0 if HMAC matches, -1 if mismatch or error.
 */
extern int tecc_kek_verify_hmac(
    const unsigned char *mek, size_t mek_len,
    const char *path);

#endif /* KEK_FILE_H */

/*
 * kek_file.c — KEK file read/write (shared between libpq and tecc_ktool)
 *
 *   Encrypt/decrypt KEK files with MEK, PBKDF2-HMAC-SHA256, SM4-CBC, and
 *   HMAC-SHA256 integrity.
 *
 *   File format:
 *     TECCKEK(7) + ver(1) + reserved(8) + algo_id(4)
 *     + salt(16) + iv(16) + SM4-CBC ciphertext + HMAC-SHA256(32)
 *
 *   This is binary-compatible with kms_adapter.c encrypt_kek_for_storage()
 *   and decrypt_kek_from_storage(), except the MEK is passed explicitly
 *   instead of using FIXED_MASTER_KEY.
 *
 *   src/common/tecc_key/kek_file.c
 */

#include "postgres_fe.h"
#include "kek_file.h"
#include "security.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

/* Algorithm constants — matches kms_adapter.c */
#define PBKDF2_DERIVED_KEY_LEN  32
#define SM4_KEY_LEN              16
#define KEK_FILE_ALGO_ID_SIZE     4

/* Buffer large enough for any KEK file */
#define KEK_FILE_BUF_SIZE       65536

int
tecc_kek_encrypt_file(const unsigned char *mek, size_t mek_len,
                      const unsigned char *plain, size_t plain_len,
                      const char *path)
{
    unsigned char salt[KEK_FILE_SALT_SIZE];
    unsigned char iv[KEK_FILE_IV_SIZE];
    unsigned char *derived_key = NULL;
    size_t       derived_key_len = 0;
    unsigned char *ciphertext = NULL;
    size_t       ciphertext_len = 0;
    unsigned char *hmac_key = NULL;
    size_t       hmac_key_len = 0;
    unsigned char file_buf[KEK_FILE_BUF_SIZE];
    size_t       file_len;
    unsigned char hmac[KEK_FILE_HMAC_SIZE];
    int          ret;
    int          fd;
    unsigned char *ptr = file_buf;

    /* Step 1: Generate random salt and IV */
    if (security_generate_random_bytes(salt, KEK_FILE_SALT_SIZE) != SECURITY_SUCCESS)
        return -1;
    if (security_generate_random_bytes(iv, KEK_FILE_IV_SIZE) != SECURITY_SUCCESS)
        return -1;

    /* Step 2: Derive encryption key from MEK using PBKDF2 */
    ret = security_derive_key_from_kek(mek, mek_len,
                                       salt, KEK_FILE_SALT_SIZE,
                                       "KEK_ENCRYPTION",
                                       &derived_key, &derived_key_len);
    if (ret != SECURITY_SUCCESS)
        return -1;

    /* Step 3: Encrypt with SM4-CBC using first 16 bytes of derived key */
    if (derived_key_len < SM4_KEY_LEN)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        return -1;
    }

    ret = security_sm4_cbc_encrypt(plain, plain_len,
                                   derived_key, SM4_KEY_LEN,
                                   &ciphertext, &ciphertext_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        return -1;
    }

    /* Step 4: Build file buffer */
    /* Header: magic(7) + version(1) + reserved(8) = 16 bytes */
    memcpy(ptr, KEK_FILE_MAGIC, KEK_FILE_MAGIC_LEN);
    ptr += KEK_FILE_MAGIC_LEN;
    *ptr++ = 1;  /* version */
    memset(ptr, 0, 8); /* reserved */
    ptr += 8;

    /* algo_id: encrypt_algo(2)=SM4-CBC, kdf_algo(2)=PBKDF2-HMAC-SHA256 */
    ptr[0] = 0x00; ptr[1] = 0x01;
    ptr[2] = 0x00; ptr[3] = 0x01;
    ptr += KEK_FILE_ALGO_ID_SIZE;

    /* salt + iv */
    memcpy(ptr, salt, KEK_FILE_SALT_SIZE);
    ptr += KEK_FILE_SALT_SIZE;
    memcpy(ptr, iv, KEK_FILE_IV_SIZE);
    ptr += KEK_FILE_IV_SIZE;

    /* ciphertext */
    memcpy(ptr, ciphertext, ciphertext_len);
    ptr += ciphertext_len;

    /* HMAC placeholder — computed over entire file_buf so far, then written back */
    file_len = ptr - file_buf;

    /* Derive separate HMAC key */
    ret = security_derive_hmac_key(derived_key, derived_key_len,
                                  &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        free(ciphertext);
        return -1;
    }

    ret = security_generate_kek_file_hmac(file_buf, file_len,
                                          hmac_key, hmac_key_len,
                                          hmac);
    if (ret != SECURITY_SUCCESS)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        free(ciphertext);
        if (hmac_key) { memset(hmac_key, 0, hmac_key_len); free(hmac_key); }
        return -1;
    }
    if (hmac_key) { memset(hmac_key, 0, hmac_key_len); free(hmac_key); hmac_key = NULL; }

    /* Append HMAC */
    memcpy(ptr, hmac, KEK_FILE_HMAC_SIZE);
    ptr += KEK_FILE_HMAC_SIZE;
    file_len = ptr - file_buf;

    /* Write file */
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        free(ciphertext);
        return -1;
    }

    ret = (int)write(fd, file_buf, file_len);
    close(fd);

    memset(derived_key, 0, derived_key_len);
    free(derived_key);
    free(ciphertext);

    return (ret == (int)file_len) ? 0 : -1;
}

int
tecc_kek_decrypt_file(const unsigned char *mek, size_t mek_len,
                      const char *path,
                      unsigned char **plain, size_t *plain_len)
{
    unsigned char file_buf[KEK_FILE_BUF_SIZE];
    unsigned char *derived_key = NULL;
    size_t       derived_key_len = 0;
    unsigned char *hmac_key = NULL;
    size_t       hmac_key_len = 0;
    int          fd;
    ssize_t      nread;
    size_t       file_len;
    size_t       header_size;
    size_t       cipher_len;
    unsigned char hmac[KEK_FILE_HMAC_SIZE];
    int          ret;
    const unsigned char *salt_ptr;
    const unsigned char *iv_ptr;
    const unsigned char *cipher_ptr;
    const unsigned char *hmac_ptr;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    nread = read(fd, file_buf, KEK_FILE_BUF_SIZE);
    close(fd);

    if (nread < (ssize_t)(KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE +
                          KEK_FILE_SALT_SIZE + KEK_FILE_IV_SIZE + KEK_FILE_HMAC_SIZE + 1))
        return -1;

    file_len = (size_t)nread;

    /* Check magic */
    if (memcmp(file_buf, KEK_FILE_MAGIC, KEK_FILE_MAGIC_LEN) != 0)
        return -1;

    /* Parse layout */
    header_size = KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE;
    salt_ptr = file_buf + header_size;
    iv_ptr = salt_ptr + KEK_FILE_SALT_SIZE;
    cipher_ptr = iv_ptr + KEK_FILE_IV_SIZE;
    cipher_len = file_len - (header_size + KEK_FILE_SALT_SIZE +
                             KEK_FILE_IV_SIZE + KEK_FILE_HMAC_SIZE);
    hmac_ptr = file_buf + file_len - KEK_FILE_HMAC_SIZE;

    /* Derive key from salt + MEK */
    ret = security_derive_key_from_kek(mek, mek_len,
                                       salt_ptr, KEK_FILE_SALT_SIZE,
                                       "KEK_ENCRYPTION",
                                       &derived_key, &derived_key_len);
    if (ret != SECURITY_SUCCESS)
        return -1;

    /* Derive separate HMAC key */
    ret = security_derive_hmac_key(derived_key, derived_key_len,
                                  &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        return -1;
    }

    /* Verify HMAC (over everything except the HMAC itself) */
    ret = security_generate_kek_file_hmac(file_buf, file_len - KEK_FILE_HMAC_SIZE,
                                          hmac_key, hmac_key_len,
                                          hmac);
    if (ret != SECURITY_SUCCESS)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        memset(hmac_key, 0, hmac_key_len);
        free(hmac_key);
        return -1;
    }

    memset(hmac_key, 0, hmac_key_len);
    free(hmac_key);

    if (memcmp(hmac, hmac_ptr, KEK_FILE_HMAC_SIZE) != 0)
    {
        memset(derived_key, 0, derived_key_len);
        free(derived_key);
        return -1;
    }

    /* Decrypt */
    ret = security_sm4_cbc_decrypt(cipher_ptr, cipher_len,
                                   derived_key, SM4_KEY_LEN,
                                   plain, plain_len);
    memset(derived_key, 0, derived_key_len);
    free(derived_key);

    return (ret == SECURITY_SUCCESS) ? 0 : -1;
}

int
tecc_kek_verify_hmac(const unsigned char *mek, size_t mek_len,
                     const char *path)
{
    unsigned char file_buf[KEK_FILE_BUF_SIZE];
    unsigned char *derived_key = NULL;
    size_t       derived_key_len = 0;
    unsigned char *hmac_key = NULL;
    size_t       hmac_key_len = 0;
    int          fd;
    ssize_t      nread;
    size_t       file_len;
    unsigned char hmac[KEK_FILE_HMAC_SIZE];
    const unsigned char *salt_ptr;
    const unsigned char *hmac_ptr;
    int          ret;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    nread = read(fd, file_buf, KEK_FILE_BUF_SIZE);
    close(fd);

    if (nread < (ssize_t)(KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE +
                          KEK_FILE_SALT_SIZE + KEK_FILE_IV_SIZE + KEK_FILE_HMAC_SIZE + 1))
        return -1;

    file_len = (size_t)nread;

    /* Check magic */
    if (memcmp(file_buf, KEK_FILE_MAGIC, KEK_FILE_MAGIC_LEN) != 0)
        return -1;

    salt_ptr = file_buf + KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE;
    hmac_ptr = file_buf + file_len - KEK_FILE_HMAC_SIZE;

    ret = security_derive_key_from_kek(mek, mek_len,
                                       salt_ptr, KEK_FILE_SALT_SIZE,
                                       "KEK_ENCRYPTION",
                                       &derived_key, &derived_key_len);
    if (ret != SECURITY_SUCCESS)
        return -1;

    /* Derive separate HMAC key — must match encrypt/decrypt paths */
    ret = security_derive_hmac_key(derived_key, derived_key_len,
                                  &hmac_key, &hmac_key_len);
    memset(derived_key, 0, derived_key_len);
    free(derived_key);
    if (ret != SECURITY_SUCCESS)
        return -1;

    ret = security_generate_kek_file_hmac(file_buf, file_len - KEK_FILE_HMAC_SIZE,
                                          hmac_key, hmac_key_len,
                                          hmac);
    memset(hmac_key, 0, hmac_key_len);
    free(hmac_key);

    if (ret != SECURITY_SUCCESS)
        return -1;

    return (memcmp(hmac, hmac_ptr, KEK_FILE_HMAC_SIZE) == 0) ? 0 : -1;
}

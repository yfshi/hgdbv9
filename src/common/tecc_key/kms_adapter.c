/*-------------------------------------------------------------------------
 *
 * kms_adapter.c
 *    KMS adapter layer implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/kms_adapter.c
 *
 * ReqID:TECC4.0 - Key management KMS adapter layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "c.h"
#include "common/fe_memutils.h"
#include "port/pg_bswap.h"
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include "kms_adapter.h"
#include "security.h"
#include "utils.h"

/* Constants */
#define LOCAL_KMS_DEFAULT_BASE_PATH "/var/lib/tecc/keys"
#define LOCAL_KMS_KEY_FILE_PATTERN "%s.key"
#define LOCAL_KMS_FILE_MODE 0600  /* rw------- */
#define MAX_KEY_NAME_LENGTH 256
#define MAX_PATH_LENGTH 1024

/* KEK file format constants */
#define KEK_FILE_MAGIC "TECCKEK"
#define KEK_FILE_MAGIC_LENGTH 7
#define KEK_FILE_VERSION 0x01
#define KEK_FILE_HEADER_SIZE 16  /* magic(7) + version(1) + reserved(8) */
#define KEK_FILE_ALGO_ID_SIZE 4   /* encrypt_algo(2) + kdf_algo(2) */
#define KEK_FILE_SALT_SIZE 16
#define KEK_FILE_IV_SIZE 16
#define KEK_FILE_HMAC_SIZE 32

/* Algorithm identifiers */
#define ENCRYPT_ALGO_SM4_CBC 0x0001
#define KDF_ALGO_PBKDF2_HMAC_SHA256 0x0001

/* PBKDF2 parameters */
#define PBKDF2_ITERATIONS 10000
#define PBKDF2_DERIVED_KEY_LENGTH 32  /* 256-bit */

/* Fixed master key (for testing - should be from secure configuration) */
static const unsigned char FIXED_MASTER_KEY[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
#define FIXED_MASTER_KEY_LENGTH 32

/* Thread-local error buffer */
static pthread_key_t kms_error_key;
static pthread_once_t kms_error_key_once = PTHREAD_ONCE_INIT;

/* KMS factory registry */
static struct {
    KmsType type;
    KmsFactoryFunc factory;
} kms_factories[] = {
    {KMS_TYPE_LOCAL, NULL},
    {KMS_TYPE_HSM, NULL},
    {KMS_TYPE_HSM_SWXA, NULL}
};
static const int kms_factories_count = sizeof(kms_factories) / sizeof(kms_factories[0]);
static pthread_mutex_t kms_factory_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * create_kms_error_key
 *    Create thread-specific error key for KMS module
 */
static void
create_kms_error_key(void)
{
    pthread_key_create(&kms_error_key, free);
}

/*
 * get_kms_error_buffer
 *    Get thread-specific error buffer for KMS module
 */
static char *
get_kms_error_buffer(void)
{
    char *buffer;

    pthread_once(&kms_error_key_once, create_kms_error_key);

    buffer = pthread_getspecific(kms_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(kms_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_kms_error
 *    Set KMS error message
 */
static void
set_kms_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_kms_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}

/*
 * get_kms_error
 *    Get KMS error message
 */
static const char *
get_kms_error(void)
{
    char *buffer = get_kms_error_buffer();
    if (buffer == NULL)
        return "Unknown error (no error buffer)";

    return buffer;
}

/*
 * kms_type_to_string
 *    Convert KMS type to string
 */
const char *
kms_type_to_string(KmsType kms_type)
{
    switch (kms_type)
    {
        case KMS_TYPE_LOCAL:
            return "localkms";
        case KMS_TYPE_HSM:
            return "hsmkms";
        case KMS_TYPE_HSM_SWXA:
            return "swxakms";
        default:
            return "UNKNOWN";
    }
}

/*
 * string_to_kms_type
 *    Convert string to KMS type
 */
KmsType
string_to_kms_type(const char *kms_type_str)
{
    if (kms_type_str == NULL)
        return KMS_TYPE_LOCAL;

    if (strcasecmp(kms_type_str, "localkms") == 0)
        return KMS_TYPE_LOCAL;
    else if (strcasecmp(kms_type_str, "hsmkms") == 0)
        return KMS_TYPE_HSM;
    else if (strcasecmp(kms_type_str, "swxakms") == 0)
        return KMS_TYPE_HSM_SWXA;
    else
        return KMS_TYPE_LOCAL;  /* Default to LOCAL */
}

/*
 * kms_type_supported
 *    Check if KMS type is supported
 */
bool
kms_type_supported(KmsType kms_type)
{
    switch (kms_type)
    {
        case KMS_TYPE_LOCAL:
            return true;  /* Always supported */
        case KMS_TYPE_HSM:
            return false; /* TODO: Implement HSM support */
        case KMS_TYPE_HSM_SWXA:
            return true;  /* SWXA HSM adapter */
        default:
            return false;
    }
}

/*
 * kms_config_create
 *    Create KMS configuration
 */
KmsConfig *
kms_config_create(KmsType kms_type, const char *kms_path, const char *kms_args)
{
    KmsConfig *config = NULL;

    config = (KmsConfig *)pg_malloc0(sizeof(KmsConfig));
    if (config == NULL)
    {
        set_kms_error("Failed to allocate memory for KMS config");
        return NULL;
    }

    config->kms_type = kms_type;

    if (kms_path != NULL)
        config->kms_path = pg_strdup(kms_path);
    else
        config->kms_path = pg_strdup(LOCAL_KMS_DEFAULT_BASE_PATH);

    if (kms_args != NULL)
        config->kms_args = pg_strdup(kms_args);

    /* Set defaults */
    config->algorithm = pg_strdup("SM4-CBC");
    config->key_length = 16;  /* 128-bit for SM4 */
    config->require_hmac = true;
    config->hmac_algorithm = pg_strdup("HMAC-SHA256");
    config->hmac_key_length = 32;  /* 256-bit for SHA256 */

    return config;
}

/*
 * kms_config_destroy
 *    Destroy KMS configuration
 */
void
kms_config_destroy(KmsConfig *config)
{
    if (config == NULL)
        return;

    if (config->kms_path != NULL)
        pg_free(config->kms_path);
    if (config->kms_args != NULL)
        pg_free(config->kms_args);
    if (config->algorithm != NULL)
        pg_free(config->algorithm);
    if (config->hmac_algorithm != NULL)
        pg_free(config->hmac_algorithm);

    pg_free(config);
}

/*
 * kms_config_validate
 *    Validate KMS configuration
 */
int
kms_config_validate(const KmsConfig *config)
{
    if (config == NULL)
    {
        set_kms_error("KMS configuration is NULL");
        return -1;
    }

    /* Check KMS type */
    if (!kms_type_supported(config->kms_type))
    {
        set_kms_error("Unsupported KMS type: %s", kms_type_to_string(config->kms_type));
        return -1;
    }

    /* Check KMS path */
    if (config->kms_path == NULL || strlen(config->kms_path) == 0)
    {
        set_kms_error("KMS path is empty");
        return -1;
    }

    /* For local KMS, check if path exists or can be created */
    if (config->kms_type == KMS_TYPE_LOCAL)
    {
        struct stat st;
        if (stat(config->kms_path, &st) != 0)
        {
            /* Path doesn't exist, check if parent directory exists */
            char parent_path[MAX_PATH_LENGTH];
            char *last_slash;
#ifdef _WIN32
            {
                const char *p_fwd = strrchr(config->kms_path, '/');
                const char *p_bwd = strrchr(config->kms_path, '\\');
                last_slash = (p_fwd > p_bwd) ? (char *)p_fwd : (char *)p_bwd;
            }
#else
            last_slash = strrchr(config->kms_path, '/');
#endif
            if (last_slash != NULL)
            {
                size_t parent_len = last_slash - config->kms_path;
                if (parent_len >= sizeof(parent_path))
                {
                    set_kms_error("Parent path too long");
                    return -1;
                }
                strncpy(parent_path, config->kms_path, parent_len);
                parent_path[parent_len] = '\0';

                if (stat(parent_path, &st) != 0)
                {
                    set_kms_error("Parent directory doesn't exist: %s", parent_path);
                    return -1;
                }
            }
        }
        else if (!S_ISDIR(st.st_mode))
        {
            set_kms_error("KMS path is not a directory: %s", config->kms_path);
            return -1;
        }
    }

    /* Check algorithm */
    if (config->algorithm == NULL || strlen(config->algorithm) == 0)
    {
        set_kms_error("Encryption algorithm is empty");
        return -1;
    }

    /* Check key length */
    if (config->key_length <= 0 || config->key_length > 64)  /* Max 512-bit */
    {
        set_kms_error("Invalid key length: %d", config->key_length);
        return -1;
    }

    /* Check HMAC settings if required */
    if (config->require_hmac)
    {
        if (config->hmac_algorithm == NULL || strlen(config->hmac_algorithm) == 0)
        {
            set_kms_error("HMAC algorithm is empty");
            return -1;
        }
        if (config->hmac_key_length <= 0 || config->hmac_key_length > 64)
        {
            set_kms_error("Invalid HMAC key length: %d", config->hmac_key_length);
            return -1;
        }
    }

    return 0;
}

/*
 * kms_operation_result_create
 *    Create KMS operation result
 */
KmsOperationResult *
kms_operation_result_create(KmsOperationType op_type)
{
    KmsOperationResult *result = NULL;

    result = (KmsOperationResult *)pg_malloc0(sizeof(KmsOperationResult));
    if (result == NULL)
    {
        set_kms_error("Failed to allocate memory for operation result");
        return NULL;
    }

    result->op_type = op_type;
    result->error_code = 0;
    result->error_message = NULL;
    result->result_data = NULL;
    result->result_size = 0;
    result->operation_time = time(NULL);

    return result;
}

/*
 * kms_operation_result_destroy
 *    Destroy KMS operation result
 */
void
kms_operation_result_destroy(KmsOperationResult *result)
{
    if (result == NULL)
        return;

    if (result->error_message != NULL)
        pg_free(result->error_message);

    if (result->result_data != NULL)
    {
        /* Securely erase result data */
        memset(result->result_data, 0, result->result_size);
        pg_free(result->result_data);
    }

    pg_free(result);
}

/*
 * kms_operation_result_set_error
 *    Set error in KMS operation result
 */
void
kms_operation_result_set_error(KmsOperationResult *result,
                              int error_code, const char *error_message)
{
    if (result == NULL)
        return;

    result->error_code = error_code;
    result->operation_time = time(NULL);

    if (result->error_message != NULL)
        pg_free(result->error_message);

    if (error_message != NULL)
        result->error_message = pg_strdup(error_message);
    else
        result->error_message = NULL;

    /* Clear any previous result data */
    if (result->result_data != NULL)
    {
        memset(result->result_data, 0, result->result_size);
        pg_free(result->result_data);
        result->result_data = NULL;
        result->result_size = 0;
    }
}

/*
 * kms_operation_result_set_success
 *    Set success in KMS operation result
 */
void
kms_operation_result_set_success(KmsOperationResult *result,
                                void *data, size_t size)
{
    if (result == NULL)
        return;

    result->error_code = 0;
    result->operation_time = time(NULL);

    if (result->error_message != NULL)
    {
        pg_free(result->error_message);
        result->error_message = NULL;
    }

    /* Clear any previous result data */
    if (result->result_data != NULL)
    {
        memset(result->result_data, 0, result->result_size);
        pg_free(result->result_data);
    }

    /* Copy new result data */
    if (data != NULL && size > 0)
    {
        result->result_data = pg_malloc(size);
        if (result->result_data != NULL)
        {
            memcpy(result->result_data, data, size);
            result->result_size = size;
        }
    }
    else
    {
        result->result_data = NULL;
        result->result_size = 0;
    }
}

/*
 * kms_register_factory
 *    Register KMS factory function
 */
int
kms_register_factory(KmsType kms_type, KmsFactoryFunc factory)
{
    int i;
    int ret = -1;

    if (factory == NULL)
    {
        set_kms_error("Factory function is NULL");
        return -1;
    }

    pthread_mutex_lock(&kms_factory_lock);

    for (i = 0; i < kms_factories_count; i++)
    {
        if (kms_factories[i].type == kms_type)
        {
            kms_factories[i].factory = factory;
            ret = 0;
            break;
        }
    }

    pthread_mutex_unlock(&kms_factory_lock);

    if (ret != 0)
        set_kms_error("Unknown KMS type: %d", kms_type);

    return ret;
}

/*
 * kms_unregister_factory
 *    Unregister KMS factory function
 */
int
kms_unregister_factory(KmsType kms_type)
{
    int i;
    int ret = -1;

    pthread_mutex_lock(&kms_factory_lock);

    for (i = 0; i < kms_factories_count; i++)
    {
        if (kms_factories[i].type == kms_type)
        {
            kms_factories[i].factory = NULL;
            ret = 0;
            break;
        }
    }

    pthread_mutex_unlock(&kms_factory_lock);

    if (ret != 0)
        set_kms_error("Unknown KMS type: %d", kms_type);

    return ret;
}

/*
 * kms_get_factory
 *    Get KMS factory function
 */
KmsFactoryFunc
kms_get_factory(KmsType kms_type)
{
    int i;
    KmsFactoryFunc factory = NULL;

    pthread_mutex_lock(&kms_factory_lock);

    for (i = 0; i < kms_factories_count; i++)
    {
        if (kms_factories[i].type == kms_type)
        {
            factory = kms_factories[i].factory;
            break;
        }
    }

    pthread_mutex_unlock(&kms_factory_lock);

    return factory;
}

/*
 * local_kms_get_key_path
 *    Get full path for key file in local KMS
 */
char *
local_kms_get_key_path(LocalKmsContext *context, const char *key_name)
{
    char *key_path = NULL;

    if (context == NULL || key_name == NULL)
        return NULL;

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path ENTER: context=%p, key_name='%s'",
                   (void*)context, key_name);

    /* Use the extracted filename from the original key_path */
    if (context->key_filename == NULL)
    {
        set_kms_error("Key filename not set in local KMS context");
        return NULL;
    }

    /* Debug logging */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path: base_path='%s', key_filename='%s', key_name='%s'",
                   context->base_path, context->key_filename, key_name);

    /* Additional debug: print actual values */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path: context=%p, context->key_filename=%p ('%s')",
                   (void*)context, (void*)context->key_filename, context->key_filename);

    /* TEST: Add a very clear marker to confirm this code is executed */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "TEST MARKER: local_kms_get_key_path called! key_filename='%s', key_name='%s'",
                   context->key_filename, key_name);

    /* Construct full path using the extracted filename */
    key_path = (char *)pg_malloc(MAX_PATH_LENGTH);
    if (key_path == NULL)
        return NULL;

    /* Construct full path using the stored key_filename from context.
     * This ensures that multiple KEKs using the same KEY_PATH share the same file. */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path: Building path with base_path='%s', key_filename='%s'",
                   context->base_path, context->key_filename);
    snprintf(key_path, MAX_PATH_LENGTH, "%s/%s", context->base_path, context->key_filename);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path: After snprintf: key_path='%s', key_filename='%s'",
                   key_path, context->key_filename);

    /* Debug logging */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                   "local_kms_get_key_path: returning path='%s' (built from key_filename)", key_path);

    return key_path;
}

/*
 * encrypt_kek_for_storage
 *    Encrypt KEK for storage using fixed master key and PBKDF2
 */
static int
encrypt_kek_for_storage(const unsigned char *plain_kek, size_t kek_len,
                       unsigned char **encrypted_data, size_t *encrypted_len)
{
    unsigned char salt[KEK_FILE_SALT_SIZE];
    unsigned char iv[KEK_FILE_IV_SIZE];
    unsigned char *derived_key = NULL;
    size_t derived_key_len = 0;
    unsigned char *ciphertext = NULL;
    size_t ciphertext_len = 0;
    unsigned char *file_data = NULL;
    size_t file_data_len = 0;
    unsigned char hmac[KEK_FILE_HMAC_SIZE];
    unsigned char *hmac_key = NULL;
    size_t hmac_key_len = 0;
    unsigned char *ptr = NULL;
    uint16_t encrypt_algo;
    uint16_t kdf_algo;
    int ret = -1;
    size_t sm4_key_len = 16;

    /* Step 1: Generate random salt and IV */
    if (security_generate_random_bytes(salt, KEK_FILE_SALT_SIZE) != SECURITY_SUCCESS ||
        security_generate_random_bytes(iv, KEK_FILE_IV_SIZE) != SECURITY_SUCCESS) {
        set_kms_error("Failed to generate random salt/IV");
        return -1;
    }

    /* Step 2: Derive encryption key from fixed master key using PBKDF2 */
    /* Use salt as PBKDF2 salt, and "KEK_ENCRYPTION" as context */
    ret = security_derive_key_from_kek(FIXED_MASTER_KEY, FIXED_MASTER_KEY_LENGTH,
                                      salt, KEK_FILE_SALT_SIZE,
                                      "KEK_ENCRYPTION",
                                      &derived_key, &derived_key_len);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to derive encryption key using PBKDF2");
        return -1;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: Derived encryption key using PBKDF2");

    /* Step 3: Encrypt KEK using SM4-CBC */
    /* SM4 requires 16-byte key, use first 16 bytes of derived key */
    if (derived_key_len < sm4_key_len) {
        set_kms_error("Derived key too short for SM4");
        memset(derived_key, 0, derived_key_len);
        pg_free(derived_key);
        return -1;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: Calling security_sm4_cbc_encrypt, kek_len=%zu, using first %zu bytes of derived key", kek_len, sm4_key_len);
    ret = security_sm4_cbc_encrypt(plain_kek, kek_len,
                                  derived_key, sm4_key_len,
                                  &ciphertext, &ciphertext_len);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: security_sm4_cbc_encrypt returned %d", ret);
    if (ret != SECURITY_SUCCESS) {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: SM4 encryption failed");
        set_kms_error("Failed to encrypt KEK with SM4-CBC");
        memset(derived_key, 0, derived_key_len);
        pg_free(derived_key);
        return -1;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: Encrypted KEK with SM4-CBC, ciphertext_len=%zu", ciphertext_len);

    /* Step 4: Prepare file data (header + salt + iv + ciphertext) */
    file_data_len = KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE +
                   KEK_FILE_SALT_SIZE + KEK_FILE_IV_SIZE + ciphertext_len;
    file_data = (unsigned char *)pg_malloc(file_data_len);
    if (file_data == NULL) {
        set_kms_error("Failed to allocate memory for file data");
        memset(derived_key, 0, derived_key_len);
        pg_free(derived_key);
        memset(ciphertext, 0, ciphertext_len);
        pg_free(ciphertext);
        return -1;
    }

    ptr = file_data;

    /* File header: magic + version + reserved */
    memcpy(ptr, KEK_FILE_MAGIC, KEK_FILE_MAGIC_LENGTH);
    ptr += KEK_FILE_MAGIC_LENGTH;
    *ptr++ = KEK_FILE_VERSION;
    memset(ptr, 0, 8);  /* reserved bytes */
    ptr += 8;

    /* Algorithm identifiers */
    encrypt_algo = htons(ENCRYPT_ALGO_SM4_CBC);
    kdf_algo = htons(KDF_ALGO_PBKDF2_HMAC_SHA256);
    memcpy(ptr, &encrypt_algo, 2);
    ptr += 2;
    memcpy(ptr, &kdf_algo, 2);
    ptr += 2;

    /* Salt and IV */
    memcpy(ptr, salt, KEK_FILE_SALT_SIZE);
    ptr += KEK_FILE_SALT_SIZE;
    memcpy(ptr, iv, KEK_FILE_IV_SIZE);
    ptr += KEK_FILE_IV_SIZE;

    /* Ciphertext */
    memcpy(ptr, ciphertext, ciphertext_len);

    /* Step 5: Derive HMAC key and generate HMAC */
    ret = security_derive_hmac_key(derived_key, derived_key_len,
                                  &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to derive HMAC key");
        goto cleanup;
    }

    ret = security_generate_kek_file_hmac(file_data, file_data_len,
                                         hmac_key, hmac_key_len, hmac);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to generate HMAC for KEK file");
        goto cleanup;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: Generated HMAC");
    tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "encrypt_kek_for_storage: HMAC", hmac, KEK_FILE_HMAC_SIZE);

    /* Step 6: Combine file data and HMAC */
    *encrypted_len = file_data_len + KEK_FILE_HMAC_SIZE;
    *encrypted_data = (unsigned char *)pg_malloc(*encrypted_len);
    if (*encrypted_data == NULL) {
        set_kms_error("Failed to allocate memory for encrypted data");
        ret = -1;
        goto cleanup;
    }

    memcpy(*encrypted_data, file_data, file_data_len);
    memcpy(*encrypted_data + file_data_len, hmac, KEK_FILE_HMAC_SIZE);

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "encrypt_kek_for_storage: Successfully encrypted KEK");
    ret = 0;

cleanup:
    /* Cleanup sensitive data */
    if (derived_key) {
        memset(derived_key, 0, derived_key_len);
        pg_free(derived_key);
    }
    if (ciphertext) {
        memset(ciphertext, 0, ciphertext_len);
        pg_free(ciphertext);
    }
    if (file_data) {
        memset(file_data, 0, file_data_len);
        pg_free(file_data);
    }
    if (hmac_key) {
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
    }
    memset(salt, 0, KEK_FILE_SALT_SIZE);
    memset(iv, 0, KEK_FILE_IV_SIZE);
    memset(hmac, 0, KEK_FILE_HMAC_SIZE);

    return ret;
}

/*
 * decrypt_kek_from_storage
 *    Decrypt KEK from storage using fixed master key and PBKDF2
 */
static int
decrypt_kek_from_storage(const unsigned char *encrypted_data, size_t encrypted_len,
                        unsigned char **plain_kek, size_t *plain_kek_len)
{
    int ret = -1;
    unsigned char *file_data = NULL;
    size_t file_data_len = 0;
    unsigned char *hmac_received = NULL;
    unsigned char *hmac_calculated = NULL;
    unsigned char *derived_key = NULL;
    size_t derived_key_len = 0;
    unsigned char *hmac_key = NULL;
    size_t hmac_key_len = 0;
    unsigned char *ciphertext = NULL;
    size_t ciphertext_len = 0;
    unsigned char *plaintext = NULL;
    size_t plaintext_len = 0;
    unsigned char version;
    unsigned short encrypt_algo;
    unsigned short kdf_algo;
    size_t offset;
    unsigned char salt[KEK_FILE_SALT_SIZE];
    unsigned char iv[KEK_FILE_IV_SIZE];
    size_t sm4_key_len = 16;

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Decrypting KEK (encrypted_len=%zu)", encrypted_len);

    /* Step 1: Verify minimum length */
    if (encrypted_len < KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE +
                       KEK_FILE_SALT_SIZE + KEK_FILE_IV_SIZE + KEK_FILE_HMAC_SIZE) {
        set_kms_error("Encrypted data too short");
        return -1;
    }

    /* Step 2: Split file data and HMAC */
    file_data_len = encrypted_len - KEK_FILE_HMAC_SIZE;
    file_data = (unsigned char *)pg_malloc(file_data_len);
    if (file_data == NULL) {
        set_kms_error("Failed to allocate memory for file data");
        return -1;
    }
    memcpy(file_data, encrypted_data, file_data_len);

    hmac_received = (unsigned char *)pg_malloc(KEK_FILE_HMAC_SIZE);
    if (hmac_received == NULL) {
        set_kms_error("Failed to allocate memory for HMAC");
        goto cleanup;
    }
    memcpy(hmac_received, encrypted_data + file_data_len, KEK_FILE_HMAC_SIZE);

    /* Step 3: Parse file header */
    if (memcmp(file_data, KEK_FILE_MAGIC, KEK_FILE_MAGIC_LENGTH) != 0) {
        set_kms_error("Invalid magic number in KEK file");
        goto cleanup;
    }

    version = file_data[7];
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: File version: 0x%02x (expected: 0x%02x)",
            version, KEK_FILE_VERSION);
    if (version != KEK_FILE_VERSION) {
        set_kms_error("Unsupported KEK file version: 0x%02x", version);
        goto cleanup;
    }

    /* Step 4: Parse algorithm identifiers */
    offset = KEK_FILE_HEADER_SIZE;
    encrypt_algo = (file_data[offset] << 8) | file_data[offset + 1];
    kdf_algo = (file_data[offset + 2] << 8) | file_data[offset + 3];
    offset += KEK_FILE_ALGO_ID_SIZE;

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Encrypt algo: 0x%04x (expected: 0x%04x)",
            encrypt_algo, ENCRYPT_ALGO_SM4_CBC);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: KDF algo: 0x%04x (expected: 0x%04x)",
            kdf_algo, KDF_ALGO_PBKDF2_HMAC_SHA256);

    if (encrypt_algo != ENCRYPT_ALGO_SM4_CBC) {
        set_kms_error("Unsupported encryption algorithm: 0x%04x", encrypt_algo);
        goto cleanup;
    }
    if (kdf_algo != KDF_ALGO_PBKDF2_HMAC_SHA256) {
        set_kms_error("Unsupported KDF algorithm: 0x%04x", kdf_algo);
        goto cleanup;
    }

    /* Step 5: Extract salt and IV */
    memcpy(salt, file_data + offset, KEK_FILE_SALT_SIZE);
    offset += KEK_FILE_SALT_SIZE;
    memcpy(iv, file_data + offset, KEK_FILE_IV_SIZE);
    offset += KEK_FILE_IV_SIZE;

    /* Step 6: Extract ciphertext */
    ciphertext_len = file_data_len - offset;
    ciphertext = (unsigned char *)pg_malloc(ciphertext_len);
    if (ciphertext == NULL) {
        set_kms_error("Failed to allocate memory for ciphertext");
        goto cleanup;
    }
    memcpy(ciphertext, file_data + offset, ciphertext_len);

    /* Step 7: Derive encryption key using PBKDF2 (same as encryption) */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Deriving encryption key using PBKDF2");
    ret = security_derive_key_from_kek(FIXED_MASTER_KEY, FIXED_MASTER_KEY_LENGTH,
                                      salt, KEK_FILE_SALT_SIZE,
                                      "KEK_ENCRYPTION",
                                      &derived_key, &derived_key_len);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to derive encryption key using PBKDF2");
        goto cleanup;
    }

    /* Step 8: Verify HMAC using derived_key (same key used during encryption) */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Verifying HMAC");

    /* Debug: print salt */
    tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "decrypt_kek_from_storage: Salt", salt, KEK_FILE_SALT_SIZE);

    ret = security_derive_hmac_key(derived_key, derived_key_len,
                                  &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to derive HMAC key");
        goto cleanup;
    }

    hmac_calculated = (unsigned char *)pg_malloc(KEK_FILE_HMAC_SIZE);
    if (hmac_calculated == NULL) {
        set_kms_error("Failed to allocate memory for calculated HMAC");
        goto cleanup;
    }

    ret = security_generate_kek_file_hmac(file_data, file_data_len,
                                         hmac_key, hmac_key_len, hmac_calculated);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to calculate HMAC");
        goto cleanup;
    }

    /* Debug: print HMAC values */
    tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "decrypt_kek_from_storage: Received HMAC", hmac_received, KEK_FILE_HMAC_SIZE);
    tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "decrypt_kek_from_storage: Calculated HMAC", hmac_calculated, KEK_FILE_HMAC_SIZE);

    if (memcmp(hmac_received, hmac_calculated, KEK_FILE_HMAC_SIZE) != 0) {
        set_kms_error("HMAC verification failed - data may be tampered");
        ret = -1;
        goto cleanup;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: HMAC verified successfully");

    /* Step 9: Decrypt ciphertext using SM4-CBC */
    /* SM4 requires 16-byte key, use first 16 bytes of derived key (matches encrypt path) */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Decrypting with SM4-CBC, using first %zu bytes of derived key", sm4_key_len);
    ret = security_sm4_cbc_decrypt(ciphertext, ciphertext_len,
                                  derived_key, sm4_key_len,
                                  &plaintext, &plaintext_len);
    if (ret != SECURITY_SUCCESS) {
        set_kms_error("Failed to decrypt KEK with SM4-CBC");
        goto cleanup;
    }

    /* Step 10: Return plaintext KEK */
    *plain_kek = plaintext;
    *plain_kek_len = plaintext_len;
    plaintext = NULL;  /* Ownership transferred to caller */

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "decrypt_kek_from_storage: Successfully decrypted KEK (len=%zu)", plaintext_len);
    {
        size_t dbg_len = (plaintext_len < 16) ? plaintext_len : 16;
        tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "decrypt_kek_from_storage: KEK plaintext", *plain_kek, dbg_len);
    }
    ret = 0;

cleanup:
    /* Cleanup sensitive data */
    if (file_data) {
        memset(file_data, 0, file_data_len);
        pg_free(file_data);
    }
    if (hmac_received) {
        memset(hmac_received, 0, KEK_FILE_HMAC_SIZE);
        pg_free(hmac_received);
    }
    if (hmac_calculated) {
        memset(hmac_calculated, 0, KEK_FILE_HMAC_SIZE);
        pg_free(hmac_calculated);
    }
    if (derived_key) {
        memset(derived_key, 0, derived_key_len);
        pg_free(derived_key);
    }
    if (hmac_key) {
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
    }
    if (ciphertext) {
        memset(ciphertext, 0, ciphertext_len);
        pg_free(ciphertext);
    }
    if (plaintext) {
        memset(plaintext, 0, plaintext_len);
        pg_free(plaintext);
    }
    memset(salt, 0, KEK_FILE_SALT_SIZE);
    memset(iv, 0, KEK_FILE_IV_SIZE);

    return ret;
}

/*
 * local_kms_save_key
 *    Save key to local file system with encryption
 */
int
local_kms_save_key(LocalKmsContext *context, const char *key_name,
                  const unsigned char *key_data, size_t key_len)
{
    char *key_path = NULL;
    FILE *key_file = NULL;
    size_t written;
    int ret = 0;
    int load_ret = 0;
    struct stat file_stat;
    unsigned char *encrypted_data = NULL;
    size_t encrypted_len = 0;
    unsigned char *test_key_data = NULL;
    size_t test_key_len = 0;

    if (context == NULL || key_name == NULL || key_data == NULL || key_len == 0)
    {
        set_kms_error("Invalid parameters for saving key");
        return -1;
    }

    /* Get key file path */
    key_path = local_kms_get_key_path(context, key_name);
    if (key_path == NULL)
    {
        set_kms_error("Failed to construct key path");
        return -1;
    }

    /* Debug: Print the actual key path */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: key_path='%s', base_path='%s', key_filename='%s'",
            key_path, context->base_path, context->key_filename);

    /* Check if file already exists */
    if (stat(key_path, &file_stat) == 0)
    {
        /* File exists */
        if (file_stat.st_size == 0)
        {
            /* File exists but is empty - this is an error */
            set_kms_error("Key file %s exists but is empty", key_path);
            pg_free(key_path);
            return -1;
        }
        else
        {
            /* File exists and is non-empty - verify file integrity before reusing */
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: Key file %s already exists, verifying integrity...", key_path);

            /* Try to load and decrypt the key to verify it's valid */
            load_ret = local_kms_load_key(context, key_name, &test_key_data, &test_key_len);

            if (load_ret != 0)
            {
                /* File exists but is invalid (tampered or corrupted) */
                tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: local_kms_load_key failed with error: %s", get_kms_error());
                set_kms_error("Key file %s exists but is invalid or tampered (HMAC verification failed)", key_path);
                pg_free(key_path);
                return -1;
            }

            /* File is valid, clean up test data and reuse existing file */
            if (test_key_data)
            {
                memset(test_key_data, 0, test_key_len);
                pg_free(test_key_data);
            }

            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: Key file %s is valid, reusing", key_path);
            pg_free(key_path);
            return 0;
        }
    }
    else if (errno != ENOENT)
    {
        /* stat failed for some reason other than file not existing */
        set_kms_error("Failed to check key file %s: %s",
                     key_path, strerror(errno));
        pg_free(key_path);
        return -1;
    }

    /* File doesn't exist, create it */

    /* Ensure directory exists */
    if (mkdir(context->base_path, 0700) != 0 && errno != EEXIST)
    {
        set_kms_error("Failed to create directory %s: %s",
                     context->base_path, strerror(errno));
        pg_free(key_path);
        return -1;
    }

    /* Encrypt the KEK before storage */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: Encrypting KEK before storage");
    ret = encrypt_kek_for_storage(key_data, key_len, &encrypted_data, &encrypted_len);
    if (ret != 0) {
        set_kms_error("Failed to encrypt KEK for storage");
        pg_free(key_path);
        return -1;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: KEK encrypted, encrypted_len=%zu", encrypted_len);

    /* Open key file */
    key_file = fopen(key_path, "wb");
    if (key_file == NULL)
    {
        set_kms_error("Failed to open key file %s: %s",
                     key_path, strerror(errno));
        memset(encrypted_data, 0, encrypted_len);
        pg_free(encrypted_data);
        pg_free(key_path);
        return -1;
    }

    /* Write encrypted key data */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: Writing %zu bytes to file", encrypted_len);
    written = fwrite(encrypted_data, 1, encrypted_len, key_file);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: fwrite wrote %zu bytes", written);
    if (written != encrypted_len)
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: fwrite failed, wrote %zu of %zu bytes", written, encrypted_len);
        set_kms_error("Failed to write encrypted key data to %s: wrote %zu of %zu bytes",
                     key_path, written, encrypted_len);
        ret = -1;
    }

    /* Set file permissions */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: Setting file permissions to %04o", context->file_mode);
#ifndef _WIN32
    if (fchmod(fileno(key_file), context->file_mode) != 0)
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_save_key: fchmod failed: %s", strerror(errno));
        set_kms_error("Failed to set file permissions on %s: %s",
                     key_path, strerror(errno));
        ret = -1;
    }
#endif

    /* Cleanup */
    fclose(key_file);
    if (encrypted_data) {
        memset(encrypted_data, 0, encrypted_len);
        pg_free(encrypted_data);
    }
    pg_free(key_path);

    return ret;
}

/*
 * local_kms_load_key
 *    Load key from local file system
 */
int
local_kms_load_key(LocalKmsContext *context, const char *key_name,
                  unsigned char **key_data, size_t *key_len)
{
    char *key_path = NULL;
    FILE *key_file = NULL;
    struct stat file_stat;
    unsigned char *data = NULL;
    size_t file_size;
    size_t read_bytes;
    unsigned char *plain_kek = NULL;
    size_t plain_kek_len = 0;
    int ret = 0;

    if (context == NULL || key_name == NULL || key_data == NULL || key_len == NULL)
    {
        set_kms_error("Invalid parameters for loading key");
        return -1;
    }

    /* Get key file path */
    key_path = local_kms_get_key_path(context, key_name);
    if (key_path == NULL)
    {
        set_kms_error("Failed to construct key path");
        return -1;
    }

    /* Check if file exists */
    if (stat(key_path, &file_stat) != 0)
    {
        set_kms_error("Key file %s not found: %s",
                     key_path, strerror(errno));
        pg_free(key_path);
        return -1;
    }

    /* Check file size */
    file_size = file_stat.st_size;
    if (file_size == 0 || file_size > 1024 * 1024)  /* Max 1MB */
    {
        set_kms_error("Invalid key file size: %zu bytes", file_size);
        pg_free(key_path);
        return -1;
    }

    /* Open key file */
    key_file = fopen(key_path, "rb");
    if (key_file == NULL)
    {
        set_kms_error("Failed to open key file %s: %s",
                     key_path, strerror(errno));
        pg_free(key_path);
        return -1;
    }

    /* Allocate memory for key data */
    data = (unsigned char *)pg_malloc(file_size);
    if (data == NULL)
    {
        set_kms_error("Failed to allocate memory for key data");
        fclose(key_file);
        pg_free(key_path);
        return -1;
    }

    /* Read encrypted key data */
    read_bytes = fread(data, 1, file_size, key_file);
    if (read_bytes != file_size)
    {
        set_kms_error("Failed to read encrypted key data from %s: read %zu of %zu bytes",
                     key_path, read_bytes, file_size);
        pg_free(data);
        ret = -1;
    }
    else
    {
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_load_key: Read encrypted data (len=%zu), decrypting...", file_size);

        /* Decrypt the KEK */
        plain_kek = NULL;
        plain_kek_len = 0;

        ret = decrypt_kek_from_storage(data, file_size, &plain_kek, &plain_kek_len);
        if (ret != 0) {
            set_kms_error("Failed to decrypt KEK from storage");
            ret = -1;
        } else {
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_load_key: Successfully decrypted KEK (len=%zu)", plain_kek_len);
            {
                size_t dbg_len = (plain_kek_len < 16) ? plain_kek_len : 16;
                tecc_key_log_hexdump(LOG_LEVEL_DEBUG, "local_kms_load_key: KEK plaintext", plain_kek, dbg_len);
            }

            /* Return plaintext KEK */
            *key_data = plain_kek;
            *key_len = plain_kek_len;
        }

        /* Free the encrypted data */
        memset(data, 0, file_size);
        pg_free(data);
    }

    /* Cleanup */
    fclose(key_file);
    pg_free(key_path);

    return ret;
}

/*
 * local_kms_delete_key
 *    Delete key from local file system
 */
int
local_kms_delete_key(LocalKmsContext *context, const char *key_name)
{
    char *key_path = NULL;
    int ret;

    if (context == NULL || key_name == NULL)
    {
        set_kms_error("Invalid parameters for deleting key");
        return -1;
    }

    /* Get key file path */
    key_path = local_kms_get_key_path(context, key_name);
    if (key_path == NULL)
    {
        set_kms_error("Failed to construct key path");
        return -1;
    }

    /* Delete key file */
    ret = unlink(key_path);
    if (ret != 0)
    {
        set_kms_error("Failed to delete key file %s: %s",
                     key_path, strerror(errno));
        pg_free(key_path);
        return -1;
    }

    pg_free(key_path);
    return 0;
}

/*
 * extract_dir_from_path
 *    Extract directory from full file path
 */
static char *
extract_dir_from_path(const char *file_path)
{
    char *dir_path = NULL;
    char *last_slash = NULL;

    if (file_path == NULL)
        return NULL;

    /* Find last separator ('/' on all platforms, '\\' on Windows) */
#ifdef _WIN32
    {
        const char *last_fwd = strrchr(file_path, '/');
        const char *last_bwd = strrchr(file_path, '\\');
        last_slash = (last_fwd > last_bwd) ? (char *)last_fwd : (char *)last_bwd;
    }
#else
    last_slash = strrchr(file_path, '/');
#endif
    if (last_slash == NULL)
    {
        /* No directory separator, use current directory */
        dir_path = pg_strdup(".");
    }
    else
    {
        /* Extract directory part */
        size_t dir_len = last_slash - file_path;
        if (dir_len == 0)
        {
            /* Root directory */
            dir_path = pg_strdup("/");
        }
        else
        {
            dir_path = (char *)pg_malloc(dir_len + 1);
            strncpy(dir_path, file_path, dir_len);
            dir_path[dir_len] = '\0';
        }
    }

    /* Debug logging */
    set_kms_error("DEBUG kms_adapter.c: extract_dir_from_path: file_path=%s, dir_path=%s",
                  file_path, dir_path);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "extract_dir_from_path: file_path=%s, dir_path=%s",
            file_path, dir_path);

    return dir_path;
}

/*
 * extract_filename_from_path
 *    Extract filename from full file path
 */
static char *
extract_filename_from_path(const char *file_path)
{
    char *filename = NULL;
    char *last_slash = NULL;

    if (file_path == NULL)
        return NULL;

    /* Find last separator ('/' on all platforms, '\\' on Windows) */
#ifdef _WIN32
    {
        const char *last_fwd = strrchr(file_path, '/');
        const char *last_bwd = strrchr(file_path, '\\');
        last_slash = (last_fwd > last_bwd) ? (char *)last_fwd : (char *)last_bwd;
    }
#else
    last_slash = strrchr(file_path, '/');
#endif
    if (last_slash == NULL)
    {
        /* No directory separator, use whole path as filename */
        filename = pg_strdup(file_path);
    }
    else
    {
        /* Extract filename part */
        filename = pg_strdup(last_slash + 1);
    }

    /* Debug logging */
    set_kms_error("DEBUG kms_adapter.c: extract_filename_from_path: file_path=%s, filename=%s",
                  file_path, filename);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "extract_filename_from_path: file_path=%s, filename=%s",
            file_path, filename);

    return filename;
}

/*
 * local_kms_adapter_init
 *    Initialize local KMS adapter
 */
int
local_kms_adapter_init(KmsAdapter *adapter, const KmsConfig *config)
{
    LocalKmsContext *context = NULL;
    char *dir_path = NULL;
    char *filename = NULL;
    char *clean_path = NULL;

    if (adapter == NULL || config == NULL)
    {
        set_kms_error("Invalid parameters for local KMS init");
        return -1;
    }

    /* Debug: Print config values */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_init: config->kms_path='%s'",
            config->kms_path ? config->kms_path : "(null)");
    fflush(stderr);

    /* Create local KMS context */
    context = (LocalKmsContext *)pg_malloc0(sizeof(LocalKmsContext));
    if (context == NULL)
    {
        set_kms_error("Failed to allocate memory for local KMS context");
        return -1;
    }

    /* Remove trailing slashes/backslashes from kms_path before parsing */
    clean_path = pg_strdup(config->kms_path);
    if (clean_path != NULL) {
        size_t len = strlen(clean_path);
        while (len > 0 && (clean_path[len - 1] == '/'
#ifdef _WIN32
            || clean_path[len - 1] == '\\'
#endif
            )) {
            clean_path[len - 1] = '\0';
            len--;
        }
    }

    /* Extract directory and filename from cleaned path */
    dir_path = extract_dir_from_path(clean_path);
    filename = extract_filename_from_path(clean_path);

    if (dir_path == NULL || filename == NULL)
    {
        set_kms_error("Failed to parse key file path: %s", config->kms_path);
        if (dir_path) pg_free(dir_path);
        if (filename) pg_free(filename);
        if (clean_path) pg_free(clean_path);
        pg_free(context);
        return -1;
    }

    /* Debug logging */
    set_kms_error("DEBUG kms_adapter.c: local_kms_adapter_init: kms_path='%s', dir_path='%s', filename='%s'",
                  config->kms_path, dir_path, filename);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_init: kms_path='%s', dir_path='%s', filename='%s'",
            config->kms_path, dir_path, filename);

    /* Store filename in context for later use */
    context->key_filename = filename;

    /* Debug: Print what we're storing */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_init: Storing base_path='%s', key_filename='%s'",
            dir_path, filename);
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_init: config->kms_path='%s', clean_path='%s'",
            config->kms_path, clean_path ? clean_path : "(null)");

    if (clean_path != NULL)
        pg_free(clean_path);

    /* Set context fields */
    /*
     * Cross-platform support: if TECC_KEK_PATH environment variable is set,
     * use it as the base directory instead of the stored absolute path.
     * This allows KEK files to be found on a different OS where the directory
     * structure differs from the path stored in the database.
     */
    {
        const char *env_path = getenv("TECC_KEK_PATH");
        if (env_path != NULL && env_path[0] != '\0')
        {
            pg_free(dir_path);
            dir_path = pg_strdup(env_path);
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "TECC_KEK_PATH=%s overrides base_path '%s' -> '%s'",
                        env_path, config->kms_path, dir_path);
        }
    }

    context->base_path = dir_path;  /* Directory where key file should be stored */
    context->key_file_pattern = pg_strdup(LOCAL_KMS_KEY_FILE_PATTERN);
    context->file_mode = LOCAL_KMS_FILE_MODE;
    context->encrypt_files = false;  /* TODO: Implement file encryption */
    context->encryption_key = NULL;

    /* Store context in adapter */
    adapter->kms_context = context;
    adapter->initialized = true;

    /* Ensure base directory exists */
    if (mkdir(context->base_path, 0700) != 0 && errno != EEXIST)
    {
        set_kms_error("Failed to create base directory %s: %s",
                     context->base_path, strerror(errno));
        pg_free(context->base_path);
        if (context->key_filename) pg_free(context->key_filename);
        pg_free(context->key_file_pattern);
        pg_free(context);
        adapter->kms_context = NULL;
        adapter->initialized = false;
        return -1;
    }

    return 0;
}

/*
 * local_kms_adapter_connect
 *    Connect to local KMS (for local KMS, just check directory)
 */
static int
local_kms_adapter_connect(KmsAdapter *adapter)
{
    LocalKmsContext *context = NULL;
    struct stat st;

    if (adapter == NULL || !adapter->initialized)
    {
        set_kms_error("Local KMS adapter not initialized");
        return -1;
    }

    context = (LocalKmsContext *)adapter->kms_context;
    if (context == NULL)
    {
        set_kms_error("Local KMS context is NULL");
        return -1;
    }

    /* Check if base directory exists and is accessible */
    if (stat(context->base_path, &st) != 0)
    {
        set_kms_error("Local KMS directory %s not accessible: %s",
                     context->base_path, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode))
    {
        set_kms_error("Local KMS path %s is not a directory",
                     context->base_path);
        return -1;
    }

    /* Check directory permissions */
    if (access(context->base_path, R_OK | W_OK | X_OK) != 0)
    {
        set_kms_error("Insufficient permissions for local KMS directory %s",
                     context->base_path);
        return -1;
    }

    adapter->connected = true;
    return 0;
}

/*
 * local_kms_adapter_disconnect
 *    Disconnect from local KMS
 */
static int
local_kms_adapter_disconnect(KmsAdapter *adapter)
{
    if (adapter == NULL)
        return 0;

    adapter->connected = false;
    return 0;
}

/*
 * local_kms_adapter_cleanup
 *    Cleanup local KMS adapter
 */
static void
local_kms_adapter_cleanup(KmsAdapter *adapter)
{
    LocalKmsContext *context = NULL;

    if (adapter == NULL)
        return;

    context = (LocalKmsContext *)adapter->kms_context;
    if (context != NULL)
    {
        if (context->base_path != NULL)
            pg_free(context->base_path);
        if (context->key_filename != NULL)
            pg_free(context->key_filename);
        if (context->key_file_pattern != NULL)
            pg_free(context->key_file_pattern);
        if (context->encryption_key != NULL)
        {
            memset(context->encryption_key, 0, strlen(context->encryption_key));
            pg_free(context->encryption_key);
        }
        pg_free(context);
        adapter->kms_context = NULL;
    }

    adapter->initialized = false;
    adapter->connected = false;
}

/*
 * local_kms_adapter_create_key
 *    Create key in local KMS
 */
static int
local_kms_adapter_create_key(KmsAdapter *adapter, const char *key_name,
                            const unsigned char *key_data, size_t key_len,
                            KmsOperationResult *result)
{
    LocalKmsContext *context = NULL;
    int ret;

    if (adapter == NULL || !adapter->initialized || key_name == NULL ||
        key_data == NULL || key_len == 0 || result == NULL)
    {
        set_kms_error("Invalid parameters for creating key");
        if (result != NULL)
            kms_operation_result_set_error(result, -1, "Invalid parameters");
        return -1;
    }

    context = (LocalKmsContext *)adapter->kms_context;
    if (context == NULL)
    {
        set_kms_error("Local KMS context is NULL");
        kms_operation_result_set_error(result, -1, "KMS context not initialized");
        return -1;
    }

    /* Debug logging */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_create_key: Creating key '%s' in local KMS", key_name);

    /* Save key to file system */
    ret = local_kms_save_key(context, key_name, key_data, key_len);
    if (ret != 0)
    {
        /* local_kms_save_key already set error message via set_kms_error */
        kms_operation_result_set_error(result, -1, get_kms_error());
        return ret;
    }

    kms_operation_result_set_success(result, NULL, 0);
    return 0;
}

/*
 * local_kms_adapter_delete_key
 *    Delete key from local KMS
 */
static int
local_kms_adapter_delete_key(KmsAdapter *adapter, const char *key_name,
                            KmsOperationResult *result)
{
    LocalKmsContext *context = NULL;
    int ret;

    if (adapter == NULL || !adapter->initialized || key_name == NULL || result == NULL)
    {
        set_kms_error("Invalid parameters for deleting key");
        if (result != NULL)
            kms_operation_result_set_error(result, -1, "Invalid parameters");
        return -1;
    }

    context = (LocalKmsContext *)adapter->kms_context;
    if (context == NULL)
    {
        set_kms_error("Local KMS context is NULL");
        kms_operation_result_set_error(result, -1, "KMS context not initialized");
        return -1;
    }

    /* Debug logging */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, "local_kms_adapter_delete_key: Deleting key '%s' from local KMS", key_name);

    /* Delete key from file system */
    ret = local_kms_delete_key(context, key_name);
    if (ret != 0)
    {
        set_kms_error("Failed to delete key %s from local KMS", key_name);
        kms_operation_result_set_error(result, -1, "Failed to delete key");
        return ret;
    }

    kms_operation_result_set_success(result, NULL, 0);
    return 0;
}

/*
 * kms_adapter_create - Create KMS adapter based on type
 */
KmsAdapter *
kms_adapter_create(KmsType kms_type, const KmsConfig *config)
{
    KmsAdapter *adapter = NULL;
    KmsFactoryFunc factory = NULL;

    if (!config)
    {
        set_kms_error("KMS configuration is NULL");
        return NULL;
    }

    /* Get factory function for this KMS type */
    factory = kms_get_factory(kms_type);
    if (!factory)
    {
        set_kms_error("No factory registered for KMS type: %s", kms_type_to_string(kms_type));
        return NULL;
    }

    /* Create adapter using factory */
    adapter = factory(config);
    if (!adapter)
    {
        set_kms_error("Factory failed to create adapter for KMS type: %s", kms_type_to_string(kms_type));
        return NULL;
    }

    return adapter;
}

/*
 * kms_adapter_destroy - Destroy KMS adapter
 */
void
kms_adapter_destroy(KmsAdapter *adapter)
{
    if (!adapter)
        return;

    /* Call adapter cleanup if available */
    if (adapter->cleanup)
        adapter->cleanup(adapter);

    /* Free configuration */
    if (adapter->config)
        kms_config_destroy(adapter->config);

    /* Free adapter structure */
    pg_free(adapter);
}

/*
 * local_kms_adapter_create - Create local KMS adapter
 */
KmsAdapter *
local_kms_adapter_create(const KmsConfig *config)
{
    KmsAdapter *adapter = NULL;

    if (!config || config->kms_type != KMS_TYPE_LOCAL)
    {
        set_kms_error("Invalid configuration for local KMS adapter");
        return NULL;
    }

    /* Allocate adapter structure */
    adapter = (KmsAdapter *)pg_malloc0(sizeof(KmsAdapter));
    if (!adapter)
    {
        set_kms_error("Failed to allocate memory for local KMS adapter");
        return NULL;
    }

    /* Set adapter fields */
    adapter->kms_type = KMS_TYPE_LOCAL;
    adapter->config = kms_config_create(config->kms_type, config->kms_path, config->kms_args);
    if (!adapter->config)
    {
        set_kms_error("Failed to create configuration for local KMS adapter");
        pg_free(adapter);
        return NULL;
    }

    /* Copy configuration values */
    if (config->algorithm)
    {
        pg_free(adapter->config->algorithm);
        adapter->config->algorithm = pg_strdup(config->algorithm);
    }
    adapter->config->key_length = config->key_length;
    adapter->config->require_hmac = config->require_hmac;
    if (config->hmac_algorithm)
    {
        pg_free(adapter->config->hmac_algorithm);
        adapter->config->hmac_algorithm = pg_strdup(config->hmac_algorithm);
    }
    adapter->config->hmac_key_length = config->hmac_key_length;

    /* Set operation methods */
    adapter->init = local_kms_adapter_init;
    adapter->connect = local_kms_adapter_connect;
    adapter->disconnect = local_kms_adapter_disconnect;
    adapter->cleanup = local_kms_adapter_cleanup;
    adapter->create_key = local_kms_adapter_create_key;
    adapter->delete_key = local_kms_adapter_delete_key;
    /* TODO: Implement other methods: load_key, list_keys, encrypt_data, decrypt_data */

    /* Initialize state */
    adapter->initialized = false;
    adapter->connected = false;
    adapter->kms_context = NULL;

    return adapter;
}

/*
 * hsm_kms_adapter_create - Create HSM KMS adapter (stub implementation)
 */
KmsAdapter *
hsm_kms_adapter_create(const KmsConfig *config)
{
    /* TODO: Implement HSM KMS adapter */
    set_kms_error("HSM KMS adapter not yet implemented");
    return NULL;
}

/*
 * kms_test_connection - Test KMS connection
 */
int
kms_test_connection(KmsAdapter *adapter)
{
    KmsOperationResult *result = NULL;
    int ret;

    if (!adapter || !adapter->initialized)
    {
        set_kms_error("KMS adapter not initialized");
        return -1;
    }

    result = kms_operation_result_create(KMS_OP_CREATE_KEY);
    if (!result)
    {
        set_kms_error("Failed to create operation result");
        return -1;
    }

    if (adapter->test_connection)
        ret = adapter->test_connection(adapter, result);
    else
        ret = -1;

    kms_operation_result_destroy(result);
    return ret;
}

/*
 * Module initialization - Register factory functions
 */
TECC_CONSTRUCTOR(kms_adapter_module_init)
{
    /* Initialize logging to stderr */
    const char *log_level_str = getenv("TECC_LOG_LEVEL");
    LogLevel min_level = LOG_LEVEL_WARNING;  /* Default to WARNING for production */

    if (log_level_str != NULL)
    {
        /* Parse log level from environment variable */
        if (strcmp(log_level_str, "DEBUG") == 0 || strcmp(log_level_str, "DEBUG1") == 0 ||
            strcmp(log_level_str, "DEBUG2") == 0 || strcmp(log_level_str, "DEBUG3") == 0 ||
            strcmp(log_level_str, "DEBUG4") == 0 || strcmp(log_level_str, "DEBUG5") == 0)
        {
            min_level = LOG_LEVEL_DEBUG;
        }
        else if (strcmp(log_level_str, "INFO") == 0 || strcmp(log_level_str, "NOTICE") == 0)
        {
            min_level = LOG_LEVEL_INFO;
        }
        else if (strcmp(log_level_str, "WARNING") == 0 || strcmp(log_level_str, "WARN") == 0)
        {
            min_level = LOG_LEVEL_WARNING;
        }
        else if (strcmp(log_level_str, "ERROR") == 0)
        {
            min_level = LOG_LEVEL_ERROR;
        }
    }

    /* Initialize logging to stderr (NULL log_file means use stderr) */
    tecc_key_log_init(NULL, min_level, false);

    /* Register local KMS factory */
    kms_register_factory(KMS_TYPE_LOCAL, local_kms_adapter_create);

    /* Note: HSM KMS factory will be registered when implemented */

    tecc_key_log(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__,
                "KMS adapter module initialized");
}
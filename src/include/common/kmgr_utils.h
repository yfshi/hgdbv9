/*-------------------------------------------------------------------------
 *
 * kmgr_utils.h
 *		Declarations for utility function for file encryption key
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/common/kmgr_utils.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#ifndef KMGR_UTILS_H
#define KMGR_UTILS_H

#include "common/cipher.h"

/* Current version number */
#define KMGR_VERSION 1

/*
 * Directories where cluster file encryption keys reside within PGDATA.
 */
#define KMGR_DIR			"pg_cryptokeys"
#define KMGR_DIR_PID		KMGR_DIR"/pg_alterckey.pid"
#define LIVE_KMGR_DIR		KMGR_DIR"/live"
/* used during cluster key rotation */
#define NEW_KMGR_DIR		KMGR_DIR"/new"
#define OLD_KMGR_DIR		KMGR_DIR"/old"

/* CryptoKey file name is keys id */
#define CryptoKeyFilePath(path, dir, id) \
	snprintf((path), MAXPGPATH, "%s/%s.wkey", (dir), (wkey_filenames[id]))

/*
 * Identifiers of internal keys.
 */
#define KMGR_KEY_ID_REL 		0
#define KMGR_KEY_ID_WAL 		1
#define KMGR_KEY_ID_EX			2
#define KMGR_NUM_DATA_KEYS		3

/* We always, today, use a 256-bit AES key. */
#define KMGR_CLUSTER_KEY_LEN 	PG_AES256_KEY_LEN

#define ALLOC_KMGR_CLUSTER_KEY_LEN	128

/* Maximum length of key the key manager can store */
#define KMGR_MAX_KEY_LEN			256
#define KMGR_PADDING_KEY_BYTES		16
#define KMGR_MAX_KEY_LEN_BYTES		(KMGR_MAX_KEY_LEN / 8) + KMGR_PADDING_KEY_BYTES

/* Maximum length of wrapped key, must bigger than (sizeof(CryptoKey) + pg_cipher_blocksize(ctx)) */
#define MAX_WRAPPED_KEY_LENGTH		128

#define KMGR_HSM_FLAG		"HSM_KEK_ID="
#define KMGR_HSM_FLAG_LEN	11
#define CHECK_KEY_HSM_FLAG(key, len) (len > KMGR_HSM_FLAG_LEN && \
			pg_strncasecmp(key, KMGR_HSM_FLAG, KMGR_HSM_FLAG_LEN) == 0)
#define INVALID_HSM_KEK_ID	0

#ifdef USE_SWSDS
#define KMGR_USE_HSM_MODE		(hsm_get_kek_id() != INVALID_HSM_KEK_ID)
#else
#define KMGR_USE_HSM_MODE false
#endif

/*
 * Cryptographic key data structure.
 *
 * This is the structure we use to write out the encrypted keys and
 * which we use to store the keys in shared memory.
 *
 * Note that wrapping this structure results in an encrypted byte
 * string which is what we actually write and then read back in.
 *
 * klen is the key length in bytes
 * key is the encryption key of klen length
 */
typedef struct CryptoKey
{
	unsigned int	kAlgId;			/* key wrapped algorithm */
	unsigned int	klen;			/* key length in bytes */
	unsigned char	key[KMGR_MAX_KEY_LEN_BYTES];
} CryptoKey;

/* Encryption method array */
typedef struct encryption_method
{
	const char *name;
	const int	bit_length;
	const int	cipher_encrypt_id;
} encryption_method;

/* Corresponding to encryption_methods */
#define DISABLED_ENCRYPTION_METHOD 0
#define DEFAULT_ENABLED_ENCRYPTION_METHOD 1
#define NUM_ENCRYPTION_METHODS	5

typedef struct encryption_target_mode_type
{
	const char *name;
	const int   mode;
} encryption_target_mode_type;

/* Corresponding to encryption_target_modes */
#define ENCRYPTION_MODE_NOTHING 0
#define ENCRYPTION_MODE_ALL		1
#define ENCRYPTION_MODE_WAL		2
#define ENCRYPTION_MODE_REL		3
#define ENCRYPTION_MODE_CUSTOM	4
#define NUM_ENCRYPTION_MODES	5

#ifdef USE_SWSDS
typedef struct hsm_encryption_algorithm_type
{
	const char	   *algName;
	unsigned int	algId;
} hsm_encryption_algorithm_type;
#define NUM_HSM_ENCRYPTION_ALGORITHMS		42
#define HSM_ENCRYPTION_ALGORITHM_HASH_BEGIN	31
#define HSM_ENCRYPTION_ALGORITHM_PUB_BEGIN	38

#define HSM_IS_SYM_ALG(algId) ( \
	HSM_GetIndexByAlgId(algId) > 0 && \
	HSM_GetIndexByAlgId(algId) < HSM_ENCRYPTION_ALGORITHM_HASH_BEGIN )
#define HSM_IS_HASH_ALG(algId) ( \
	HSM_GetIndexByAlgId(algId) >= HSM_ENCRYPTION_ALGORITHM_HASH_BEGIN && \
	HSM_GetIndexByAlgId(algId) < HSM_ENCRYPTION_ALGORITHM_PUB_BEGIN )
#define HSM_IS_PUB_ALG(algId) ( \
	HSM_GetIndexByAlgId(algId) >= HSM_ENCRYPTION_ALGORITHM_PUB_BEGIN && \
	HSM_GetIndexByAlgId(algId) <= NUM_HSM_ENCRYPTION_ALGORITHMS )

#endif

#ifndef WIN32
#define open_terminal_fd(fd) do { \
	fd = open("/dev/tty", O_RDWR, 0); \
} while(0)
#else 
#define open_terminal_fd(fd) do { \
	fd = open("CONOUT$", O_RDWR, 0); \
} while(0)
#endif 

#define close_terminal_fd(fd) do { \
	if (fd != -1) { \
		close(fd); \
		fd = -1; \
	} \
} while(0)

extern encryption_method encryption_methods[NUM_ENCRYPTION_METHODS];
extern encryption_target_mode_type encryption_target_modes[NUM_ENCRYPTION_MODES];
extern char *wkey_filenames[KMGR_NUM_DATA_KEYS];

extern bool kmgr_wrap_data_key(PgCipherCtx *ctx, CryptoKey *in, unsigned char *out, int *outlen);
extern bool kmgr_unwrap_data_key(PgCipherCtx *ctx, unsigned char *in, int inlen, CryptoKey *out);
extern bool kmgr_verify_cluster_key(unsigned char *cluster_key,
									unsigned char **in_keys, int *klens, CryptoKey *out_keys);
extern int	kmgr_run_cluster_key_command(char *cluster_key_command,
										 char *buf, int size, char *live_path, int terminal_fd);
extern void kmgr_read_wrapped_data_keys(const char *path, unsigned char **keys,
										int *key_lens);
extern bool kmgr_get_keycmd_conf(char *dataDir, char **cluster_key_command, 
						  		 int *file_encryption_method, int *encryption_target_mode);

extern bool kmgr_retrieve_cluster_key(char *cluster_key_cmd, unsigned char* cluster_key,
                                 	  char *live_path, int terminal_fd);
extern bool kmgr_retrieve_data_key(unsigned char *cluster_key, const char* key_path, CryptoKey *data_key);

#ifdef USE_SWSDS
extern unsigned int HSM_GetAlgID(int pg_cipher_id);
extern void* HSM_GetDeviceHandle(bool force_new);
extern void* HSM_GetSessionHandle(void* *phSessionHandle, bool force_new);
extern void* HSM_GenerateSessionKey(void* *phSessionHandle, unsigned int uiKeyBits,
										 unsigned int uiAlgID, unsigned int uiKEKIndex,
										 unsigned char *pucKey, unsigned int *puiKeyLength,
										 void* *phKeyHandle);
extern void* HSM_GetKeyHandle(void* *phSessionHandle, void* *phKeyHandle,
								   const CryptoKey *key, unsigned int kekid, bool force_new);
extern void HSM_DestroyKey(void* *phSessionHandle, void* hKeyHandle);
extern bool HSM_Encrypt(void* *phSessionHandle, void* *phKeyHandle,
						const CryptoKey *key, unsigned int kekid,
						unsigned int uiAlgID, unsigned char *iv, 
						unsigned char *plaintext, unsigned int inlen,
						unsigned char *ciphertext, unsigned int *outlen);
extern bool HSM_Decrypt(void* *phSessionHandle, void* *phKeyHandle,
						const CryptoKey *key, unsigned int kekid,
						unsigned int uiAlgID, unsigned char *iv, 
						unsigned char *ciphertext, unsigned int inlen,
						unsigned char *plaintext, unsigned int *outlen);
extern void HSM_Generate_Random(void* *phSessionHandle, unsigned int len, unsigned char *pucRandom);
extern void HSM_Hash(void* *phSessionHandle, unsigned int AlgId,
					 unsigned char *indata, unsigned int inlen,
					 unsigned char *outdata, unsigned int *outlen,
					 void *pubKey);
extern void HSM_Generate_Key_Pair(void* *phSessionHandle, unsigned int uiKeyBits,
								  unsigned int uiAlgID, void *pubKey, void *priKey);
extern void HSM_Pub_Encrypt(void* *phSessionHandle, unsigned int AlgId, 
							unsigned int keyIndex, void *key, 
							unsigned char *indata, unsigned int inlen,
							unsigned char *outdata, unsigned int *outlen);
extern void HSM_Pri_Decrypt(void* *phSessionHandle, unsigned int AlgId, 
							unsigned int keyIndex, void *key, 
							unsigned char *indata, unsigned int inlen,
							unsigned char *outdata, unsigned int *outlen,
							unsigned char *passwd);
extern void HSM_Sign(void* *phSessionHandle, unsigned int AlgId, 
					 unsigned int keyIndex, void *key, unsigned int keylen,
					 unsigned char *indata, unsigned int inlen,
					 unsigned char *signature, unsigned int *siglen,
					 unsigned char *passwd, void *pubKey);
extern void HSM_Verify(void* *phSessionHandle, unsigned int AlgId, 
					   unsigned int keyIndex, void *key, unsigned int keylen,
					   unsigned char *indata, unsigned int inlen,
					   unsigned char *signature, unsigned int siglen);
#endif
extern unsigned int hsm_get_kek_id(void);
extern void swsds_failure(void);
extern void hsm_generate_crypto_key(unsigned int kek_id, int len_bit, CryptoKey *key);
extern void hsm_verify_cluster_key(unsigned char **in_keys, int *key_lens, CryptoKey *out_keys);

extern unsigned int HSM_AlgName2Id(const char *algName);
extern const char* HSM_AlgId2Name(unsigned int algId);
extern int HSM_GetIndexByAlgId(unsigned int algId);

#endif							/* KMGR_UTILS_H */

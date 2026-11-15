/*-------------------------------------------------------------------------
 *
 * fe-hsm-adapter.h
 *	  Hardware Security Module (HSM) adapter interface for TECC
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-hsm-adapter.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef FE_HSM_ADAPTER_H
#define FE_HSM_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
/* Use common KmsType definition from tecc_common.h */
#include "../../../common/tecc_key/tecc_common.h"
/* KmsType is now defined in tecc_common.h */

#ifdef sm4_encrypt
#undef sm4_encrypt
#endif

#ifdef sm4_decrypt
#undef sm4_decrypt
#endif

/* HSM error codes */
typedef enum
{
	HSM_SUCCESS = 0,
	HSM_ERROR_INIT_FAILED,
	HSM_ERROR_CONNECT_FAILED,
	HSM_ERROR_KEY_NOT_FOUND,
	HSM_ERROR_KEY_ACCESS_DENIED,
	HSM_ERROR_INVALID_PARAM,
	HSM_ERROR_OPERATION_FAILED,
	HSM_ERROR_NOT_SUPPORTED,
	HSM_ERROR_INTERNAL
} HsmErrorCode;

/* HSM device handle */
typedef struct HsmDevice
{
	void *device_handle;		/* Device handle */
	void *session_handle;		/* Session handle */
	bool connected;			/* Connection status */
	char *config_file;		/* Configuration file path */
	/* ReqID:TECC4.0 - 适配器私有状态（由 init 分配、cleanup 释放）。
	 * swxa 适配器用它挂"连接级会话密钥句柄缓存"（见 fe-swxa-adapter.c），
	 * 其它适配器不使用。每个 HsmDevice 对应一个连接/一段会话，
	 * 故私有状态天然是连接级的，不会跨连接串用。 */
	void *adapter_private;
} HsmDevice;

/* HSM key information */
typedef struct HsmKeyInfo
{
	unsigned int key_index;		/* Key index in HSM */
	unsigned int key_bits;		/* Key length in bits */
	unsigned int key_type;		/* Key type (RSA, ECC, etc.) */
	char *key_name;			/* Key name (optional) */
	void *key_data;			/* Key data buffer */
	size_t key_len;			/* Key data length */
} HsmKeyInfo;

/* HSM adapter interface */
typedef struct HsmAdapterInterface
{
	/* Device management */
	HsmErrorCode (*init)(HsmDevice **device, const char *config_file);
	HsmErrorCode (*connect)(HsmDevice *device);
	HsmErrorCode (*disconnect)(HsmDevice *device);
	void (*cleanup)(HsmDevice *device);

	/* Key management */
	HsmErrorCode (*check_key_exists)(HsmDevice *device, unsigned int key_index);
	HsmErrorCode (*get_key_info)(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info);
	HsmErrorCode (*export_public_key)(HsmDevice *device, unsigned int key_index,
									 void **key_data, size_t *key_len);

	/* Key operations */
	HsmErrorCode (*encrypt)(HsmDevice *device, unsigned int key_index,
						   const void *plaintext, size_t plaintext_len,
						   void **ciphertext, size_t *ciphertext_len);
	HsmErrorCode (*decrypt)(HsmDevice *device, unsigned int key_index,
						   const void *ciphertext, size_t ciphertext_len,
						   void **plaintext, size_t *plaintext_len);
	HsmErrorCode (*sign)(HsmDevice *device, unsigned int key_index,
						const void *data, size_t data_len,
						void **signature, size_t *signature_len);
	HsmErrorCode (*verify)(HsmDevice *device, unsigned int key_index,
						  const void *data, size_t data_len,
						  const void *signature, size_t signature_len);

	/* SM4 operations for column data encryption/decryption through HSM */
	HsmErrorCode (*sm4_encrypt)(HsmDevice *device,
							   const unsigned char *key, size_t key_len,
							   const unsigned char *iv,
							   const unsigned char *plaintext, size_t plain_len,
							   unsigned char *ciphertext, size_t *cipher_len);
	HsmErrorCode (*sm4_decrypt)(HsmDevice *device,
							   const unsigned char *key, size_t key_len,
							   const unsigned char *iv,
							   const unsigned char *ciphertext, size_t cipher_len,
							   unsigned char *plaintext, size_t *plain_len);

	/* ReqID:TECC4.0 - SM2 envelope decryption with the device-internal SM2 private
	 * key (identified by key_index).  Used to unwrap the HE DEK private-key envelope
	 * when the KEK is an HSM-hosted SM2 key (key_store=swxakms/tzwykms, key_algo=sm2),
	 * so the SM2 private key never leaves the device. */
	HsmErrorCode (*sm2_decrypt)(HsmDevice *device, unsigned int key_index,
								const unsigned char *ciphertext, size_t cipher_len,
								void **plaintext, size_t *plaintext_len);

	/* Utility functions */
	const char *(*error_string)(HsmErrorCode error_code);
	bool (*is_connected)(HsmDevice *device);
} HsmAdapterInterface;

/* Tongzhi Weiye TZHSM-G002 adapter */
extern HsmAdapterInterface *get_tzhsm_adapter(void);

/* SanWeiXinAn SWXA adapter */
extern HsmAdapterInterface *get_swxa_adapter(void);

/* Mock adapter for testing (when HSM is not available) */
extern HsmAdapterInterface *get_mock_hsm_adapter(void);

/* Key path parsing */
extern bool parse_hsm_key_path(const char *key_path, unsigned int *key_index);

/* HSM KEK setup */
/* ReqID:TECC4.0 */
extern int setup_hsm_kek(const char *kek_name, const char *key_path);

#endif /* FE_HSM_ADAPTER_H */
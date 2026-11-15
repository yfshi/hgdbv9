/*-------------------------------------------------------------------------
 *
 * fe-hsm-adapter.c
 *	  Hardware Security Module (HSM) adapter implementation for TECC
 *
 * ReqID:TECC4.0 - 同智伟业（TZHSM）适配器通过 dlopen/dlsym 在运行时装载
 * libsdf.client.so，构建期不再依赖厂商 HSM 驱动库：未使用 TZHSM 密码机
 * 的客户无需安装厂商 SDK 即可完成构建，仅在运行时首次使用 TZHSM 设备时
 * 才需要装载该动态库（与 fe-swxa-adapter.c 的 swxa 适配器方式一致）。
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-hsm-adapter.c
 *
 *-------------------------------------------------------------------------
 */

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef _MSC_VER
/* Windows lacks these POSIX dlopen flags.  The PostgreSQL port layer
 * provides dlopen() on Windows via LoadLibrary, which ignores mode. */
#define RTLD_LAZY     0
#define RTLD_LOCAL    0
#define RTLD_DEEPBIND 0
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>  /* For offsetof */
#include <dlfcn.h>
#include <openssl/crypto.h>  /* ReqID:TECC4.0 - OPENSSL_malloc for sm2_decrypt output */

#include "fe-hsm-adapter.h"
#include "../tecc_common/tecc_logging.h"
#include "../../../include/hsm/sdf_client.h"

/* Forward declarations for Tongzhi Weiye adapter functions */
static HsmErrorCode tzhsm_init(HsmDevice **device, const char *config_file);
static HsmErrorCode tzhsm_connect(HsmDevice *device);
static HsmErrorCode tzhsm_disconnect(HsmDevice *device);
static void tzhsm_cleanup(HsmDevice *device);
static HsmErrorCode tzhsm_check_key_exists(HsmDevice *device, unsigned int key_index);
static HsmErrorCode tzhsm_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info);
static HsmErrorCode tzhsm_export_public_key(HsmDevice *device, unsigned int key_index,
										   void **key_data, size_t *key_len);
static HsmErrorCode tzhsm_encrypt(HsmDevice *device, unsigned int key_index,
								 const void *plaintext, size_t plaintext_len,
								 void **ciphertext, size_t *ciphertext_len);
static HsmErrorCode tzhsm_decrypt(HsmDevice *device, unsigned int key_index,
								 const void *ciphertext, size_t ciphertext_len,
								 void **plaintext, size_t *plaintext_len);
static HsmErrorCode tzhsm_sign(HsmDevice *device, unsigned int key_index,
							  const void *data, size_t data_len,
							  void **signature, size_t *signature_len);
static HsmErrorCode tzhsm_verify(HsmDevice *device, unsigned int key_index,
								const void *data, size_t data_len,
								const void *signature, size_t signature_len);
static HsmErrorCode tzhsm_sm4_encrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *plaintext, size_t plain_len,
									 unsigned char *ciphertext, size_t *cipher_len);
static HsmErrorCode tzhsm_sm4_decrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *ciphertext, size_t cipher_len,
									 unsigned char *plaintext, size_t *plain_len);
static HsmErrorCode tzhsm_sm2_decrypt(HsmDevice *device, unsigned int key_index,
									  const unsigned char *ciphertext, size_t cipher_len,
									  void **plaintext, size_t *plaintext_len);
static const char *tzhsm_error_string(HsmErrorCode error_code);
static bool tzhsm_is_connected(HsmDevice *device);

/* Helper function declaration */
static HsmErrorCode sdf_error_to_hsm_error(int sdf_error);

/* ReqID:TECC4.0 - TZHSM SDF API 函数指针表。
 * libsdf.client.so（同智伟业密码机 SDK）在运行时通过 dlopen/dlsym 装载，
 * 构建期不再依赖厂商驱动库：未使用 TZHSM 密码机（kms_type=tzwykms）的
 * 客户无需安装 SDK 即可完成构建；仅在首次使用 TZHSM 设备时才需要装载。 */
typedef struct TzhsmSdfApi
{
	void *handle;	/* dlopen handle，NULL 表示未装载 */

	int (*SDF_OpenDevice)(long long *phDeviceHandle);
	int (*SDF_OpenSession)(long long hDeviceHandle, long long *phSessionHandle);
	int (*SDF_CloseDevice)(long long hDeviceHandle);
	int (*SDF_CloseSession)(long long hSessionHandle);
	int (*SDF_ImportKeyWithKEK)(long long hSessionHandle, unsigned int uiAlgID,
								unsigned int uiKEKIndex, unsigned char *pucKey,
								unsigned int uiKeyLength, long long *phKeyHandle);
	int (*SDF_DestroyKey)(long long hSessionHandle, long long hKeyHandle);
	int (*SDF_ExportSignPublicKey_RSA)(long long hSessionHandle,
									   unsigned int uiKeyIndex,
									   RSArefPublicKey *pucPublicKey);
	int (*SDF_ExportSignPublicKey_ECC)(long long hSessionHandle,
									   unsigned int uiKeyIndex,
									   ECCrefPublicKey *pucPublicKey);
	int (*SDF_GenerateRandom)(long long hSessionHandle, unsigned int uiLength,
							  unsigned char *pucRandom);
	int (*SDF_Encrypt)(long long hSessionHandle, long long hKeyHandle,
					   unsigned int uiAlgID, unsigned char *pucIV,
					   unsigned char *pucData, unsigned int uiDataLength,
					   unsigned char *pucEncData, unsigned int *puiEncDataLength);
	int (*SDF_Decrypt)(long long hSessionHandle, long long hKeyHandle,
					   unsigned int uiAlgID, unsigned char *pucIV,
					   unsigned char *pucEncData, unsigned int uiEncDataLength,
					   unsigned char *pucData, unsigned int *puiDataLength);
	int (*SDF_ImportKey)(long long hSessionHandle, unsigned char *pucKey,
						 unsigned int uiKeyLength, long long *phKeyHandle);
	int (*SDF_InternalDecrypt_ECC)(long long hSessionHandle,
								   unsigned int uiISKIndex,
								   unsigned int uiAlgID, ECCCipher *pucEncData,
								   unsigned char *pucData,
								   unsigned int *uiDataLength);
} TzhsmSdfApi;

/* ReqID:TECC4.0 - TZHSM SDF API 单例（进程内仅装载一次） */
static TzhsmSdfApi tzhsm_sdf = {0};

/*
 * tzhsm_load_sdf_api - 运行时装载 TZHSM SDF 动态库（libsdf.client.so）
 *
 * 装载顺序：TECC_SDF_LIB 环境变量（测试可用 SoftSDF libteccsdf.so）→
 * /usr/lib64/libsdf.client.so → /usr/local/lib64/libsdf.client.so →
 * 按 soname "libsdf.client.so"（走 LD_LIBRARY_PATH / rpath）。
 * 使用 RTLD_LOCAL | RTLD_DEEPBIND，避免与已装载的 libswsds.so（三未信安）
 * 的同名 SDF 符号相互冲突。全部函数解析成功后返回 HSM_SUCCESS。
 */
static HsmErrorCode
tzhsm_load_sdf_api(void)
{
	const char *sdf_lib;

	if (tzhsm_sdf.handle != NULL)
		return HSM_SUCCESS;

	/* ReqID:TECC4.0 - 支持通过 TECC_SDF_LIB 环境变量切换/指定 SDF 动态库 */
	sdf_lib = getenv("TECC_SDF_LIB");
	if (sdf_lib != NULL && sdf_lib[0] != '\0')
	{
		tzhsm_sdf.handle = dlopen(sdf_lib, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (tzhsm_sdf.handle == NULL)
		{
			fprintf(stderr, "ERROR: Failed to load %s (TECC_SDF_LIB): %s\n",
					sdf_lib, dlerror());
			return HSM_ERROR_INIT_FAILED;
		}
	}
	else
	{
		/* 常见安装路径，最后按 soname 查找（LD_LIBRARY_PATH / rpath） */
		tzhsm_sdf.handle = dlopen("/usr/lib64/libsdf.client.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (tzhsm_sdf.handle == NULL)
			tzhsm_sdf.handle = dlopen("/usr/local/lib64/libsdf.client.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (tzhsm_sdf.handle == NULL)
			tzhsm_sdf.handle = dlopen("libsdf.client.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (tzhsm_sdf.handle == NULL)
		{
			fprintf(stderr, "ERROR: Failed to load libsdf.client.so: %s\n", dlerror());
			fprintf(stderr, "ERROR: TZHSM (Tongzhi Weiye) password machine support requires the vendor SDK.\n");
			fprintf(stderr, "ERROR: Install libsdf.client.so (e.g. /usr/lib64) or set TECC_SDF_LIB=/path/to/libsdf.client.so.\n");
			return HSM_ERROR_INIT_FAILED;
		}
	}

	/* 通过 dlsym 解析全部 SDF 函数 */
#define LOAD_TZHSM_FUNC(name) \
	do { \
		tzhsm_sdf.name = dlsym(tzhsm_sdf.handle, #name); \
		if (tzhsm_sdf.name == NULL) { \
			fprintf(stderr, "ERROR: Failed to resolve %s in libsdf.client.so: %s\n", \
					#name, dlerror()); \
			dlclose(tzhsm_sdf.handle); \
			tzhsm_sdf.handle = NULL; \
			return HSM_ERROR_INIT_FAILED; \
		} \
	} while (0)

	LOAD_TZHSM_FUNC(SDF_OpenDevice);
	LOAD_TZHSM_FUNC(SDF_OpenSession);
	LOAD_TZHSM_FUNC(SDF_CloseDevice);
	LOAD_TZHSM_FUNC(SDF_CloseSession);
	LOAD_TZHSM_FUNC(SDF_ImportKeyWithKEK);
	LOAD_TZHSM_FUNC(SDF_DestroyKey);
	LOAD_TZHSM_FUNC(SDF_ExportSignPublicKey_RSA);
	LOAD_TZHSM_FUNC(SDF_ExportSignPublicKey_ECC);
	LOAD_TZHSM_FUNC(SDF_GenerateRandom);
	LOAD_TZHSM_FUNC(SDF_Encrypt);
	LOAD_TZHSM_FUNC(SDF_Decrypt);
	LOAD_TZHSM_FUNC(SDF_ImportKey);
	LOAD_TZHSM_FUNC(SDF_InternalDecrypt_ECC);

#undef LOAD_TZHSM_FUNC

	return HSM_SUCCESS;
}

/* Static adapter instances */
static HsmAdapterInterface tzhsm_adapter = {0};
static pthread_once_t tzhsm_adapter_once = PTHREAD_ONCE_INIT;

static void tzhsm_adapter_init(void)
{
	tzhsm_adapter.init = tzhsm_init;
	tzhsm_adapter.connect = tzhsm_connect;
	tzhsm_adapter.disconnect = tzhsm_disconnect;
	tzhsm_adapter.cleanup = tzhsm_cleanup;
	tzhsm_adapter.check_key_exists = tzhsm_check_key_exists;
	tzhsm_adapter.get_key_info = tzhsm_get_key_info;
	tzhsm_adapter.export_public_key = tzhsm_export_public_key;
	tzhsm_adapter.encrypt = tzhsm_encrypt;
	tzhsm_adapter.decrypt = tzhsm_decrypt;
	tzhsm_adapter.sign = tzhsm_sign;
	tzhsm_adapter.verify = tzhsm_verify;
	tzhsm_adapter.error_string = tzhsm_error_string;
	tzhsm_adapter.is_connected = tzhsm_is_connected;
	tzhsm_adapter.sm4_encrypt = tzhsm_sm4_encrypt;
	tzhsm_adapter.sm4_decrypt = tzhsm_sm4_decrypt;
	tzhsm_adapter.sm2_decrypt = tzhsm_sm2_decrypt;
}

/*
 * get_tzhsm_adapter - Get Tongzhi Weiye TZHSM-G002 adapter instance
 */
HsmAdapterInterface *
get_tzhsm_adapter(void)
{
	pthread_once(&tzhsm_adapter_once, tzhsm_adapter_init);
	return &tzhsm_adapter;
}

/*
 * Tongzhi Weiye TZHSM-G002 adapter implementation
 * Note: These are stub implementations that need to be filled with actual
 * Tongzhi Weiye SDK calls based on sdf_client.h
 */

static HsmErrorCode
tzhsm_init(HsmDevice **device, const char *config_file)
{
	HsmDevice *dev;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	/* ReqID:TECC4.0 - 首次使用 TZHSM 设备时惰性装载 libsdf.client.so */
	if (tzhsm_load_sdf_api() != HSM_SUCCESS)
		return HSM_ERROR_INIT_FAILED;

	/* Allocate device structure */
	dev = (HsmDevice *) malloc(sizeof(HsmDevice));
	if (!dev)
		return HSM_ERROR_INTERNAL;

	memset(dev, 0, sizeof(HsmDevice));

	/* Store configuration file path if provided */
	if (config_file)
	{
		dev->config_file = strdup(config_file);
		if (!dev->config_file)
		{
			free(dev);
			return HSM_ERROR_INTERNAL;
		}
	}
	else
	{
		/* Use default configuration file */
		dev->config_file = strdup("/usr/lib64/cfg/client_config.xml");
		if (!dev->config_file)
		{
			free(dev);
			return HSM_ERROR_INTERNAL;
		}
	}

	dev->device_handle = NULL;
	dev->session_handle = NULL;
	dev->connected = false;

	*device = dev;
	return HSM_SUCCESS;
}

static HsmErrorCode
tzhsm_connect(HsmDevice *device)
{
	int ret;
	long long device_handle = 0;
	long long session_handle = 0;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	if (device->connected)
		return HSM_SUCCESS;

	/* Open device using Tongzhi Weiye SDK */
	ret = tzhsm_sdf.SDF_OpenDevice(&device_handle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "SDF_OpenDevice failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Open session */
	ret = tzhsm_sdf.SDF_OpenSession(device_handle, &session_handle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "SDF_OpenSession failed: 0x%08x\n", ret);
		tzhsm_sdf.SDF_CloseDevice(device_handle);
		return sdf_error_to_hsm_error(ret);
	}

	device->device_handle = (void *)device_handle;
	device->session_handle = (void *)session_handle;
	device->connected = true;

	return HSM_SUCCESS;
}

static HsmErrorCode
tzhsm_disconnect(HsmDevice *device)
{
	int ret;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	if (!device->connected)
		return HSM_SUCCESS;

	/* Close session */
	if (device->session_handle)
	{
		ret = tzhsm_sdf.SDF_CloseSession((long long)device->session_handle);
		if (ret != SDR_OK)
		{
			fprintf(stderr, "SDF_CloseSession failed: 0x%08x\n", ret);
		}
		device->session_handle = NULL;
	}

	/* Close device */
	if (device->device_handle)
	{
		ret = tzhsm_sdf.SDF_CloseDevice((long long)device->device_handle);
		if (ret != SDR_OK)
		{
			fprintf(stderr, "SDF_CloseDevice failed: 0x%08x\n", ret);
		}
		device->device_handle = NULL;
	}

	device->connected = false;
	return HSM_SUCCESS;
}

static void
tzhsm_cleanup(HsmDevice *device)
{
	if (!device)
		return;

	/* Disconnect if still connected */
	if (device->connected)
		tzhsm_disconnect(device);

	/* Free resources */
	if (device->config_file)
		free(device->config_file);

	free(device);
}

static HsmErrorCode
tzhsm_check_key_exists(HsmDevice *device, unsigned int key_index)
{
	int ret;
	long long key_handle = 0;
	unsigned char test_key[16] = {0};  /* Test key data */
	unsigned int test_key_len = sizeof(test_key);

	if (!device || !device->connected)
		return HSM_ERROR_CONNECT_FAILED;

	if (key_index < 1 || key_index > 65535)
		return HSM_ERROR_INVALID_PARAM;

	/* Try to import a test key using the specified KEK index
	 * This will fail with SDR_KEYNOTEXIST if the KEK doesn't exist
	 */
	ret = tzhsm_sdf.SDF_ImportKeyWithKEK((long long)device->session_handle,
							   SGD_SM4_ECB,  /* Use SM4 ECB as test algorithm */
							   key_index,
							   test_key,
							   test_key_len,
							   &key_handle);

	/* Clean up if key was created */
	if (key_handle != 0)
	{
		tzhsm_sdf.SDF_DestroyKey((long long)device->session_handle, key_handle);
	}

	/* Convert SDF error to HSM error */
	return sdf_error_to_hsm_error(ret);
}

static HsmErrorCode
tzhsm_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info)
{
	HsmKeyInfo *info;
	int ret;
	RSArefPublicKey rsa_pub_key;
	ECCrefPublicKey ecc_pub_key;

	if (!device || !device->connected || !key_info)
		return HSM_ERROR_INVALID_PARAM;

	/* Check if key exists first */
	if (tzhsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	/* Allocate key info structure */
	info = (HsmKeyInfo *) malloc(sizeof(HsmKeyInfo));
	if (!info)
		return HSM_ERROR_INTERNAL;

	memset(info, 0, sizeof(HsmKeyInfo));
	info->key_index = key_index;

	/* Try to get RSA key info */
	ret = tzhsm_sdf.SDF_ExportSignPublicKey_RSA((long long)device->session_handle,
									  key_index, &rsa_pub_key);
	if (ret == SDR_OK)
	{
		info->key_bits = rsa_pub_key.bits;
		info->key_type = SGD_RSA; /* RSA key type */
		*key_info = info;
		return HSM_SUCCESS;
	}

	/* Try to get ECC key info */
	ret = tzhsm_sdf.SDF_ExportSignPublicKey_ECC((long long)device->session_handle,
									  key_index, &ecc_pub_key);
	if (ret == SDR_OK)
	{
		info->key_bits = ecc_pub_key.bits;
		info->key_type = SGD_SM2; /* Assume SM2 for ECC */
		*key_info = info;
		return HSM_SUCCESS;
	}

	/* If we can't determine key type, use defaults */
	info->key_bits = 2048;
	info->key_type = SGD_RSA;
	*key_info = info;
	return HSM_SUCCESS;
}

static HsmErrorCode
tzhsm_export_public_key(HsmDevice *device, unsigned int key_index,
					   void **key_data, size_t *key_len)
{
	int ret;
	RSArefPublicKey rsa_pub_key;
	ECCrefPublicKey ecc_pub_key;
	size_t total_size;
	unsigned char *buffer;
	unsigned int *p;

	if (!device || !device->connected || !key_data || !key_len)
		return HSM_ERROR_INVALID_PARAM;

	/* Try to export RSA public key */
	ret = tzhsm_sdf.SDF_ExportSignPublicKey_RSA((long long)device->session_handle,
									  key_index, &rsa_pub_key);
	if (ret == SDR_OK)
	{
		/* Calculate total size: bits (4 bytes) + m + e */
		total_size = sizeof(unsigned int) +
				   RSAref_MAX_LEN + RSAref_MAX_LEN;
		buffer = malloc(total_size);
		if (!buffer)
			return HSM_ERROR_INTERNAL;

		/* Copy data to buffer */
		p = (unsigned int *)buffer;
		*p = rsa_pub_key.bits;
		memcpy(buffer + sizeof(unsigned int), rsa_pub_key.m, RSAref_MAX_LEN);
		memcpy(buffer + sizeof(unsigned int) + RSAref_MAX_LEN,
			   rsa_pub_key.e, RSAref_MAX_LEN);

		*key_data = buffer;
		*key_len = total_size;
		return HSM_SUCCESS;
	}

	/* Try to export ECC public key */
	ret = tzhsm_sdf.SDF_ExportSignPublicKey_ECC((long long)device->session_handle,
									  key_index, &ecc_pub_key);
	if (ret == SDR_OK)
	{
		/* Calculate total size: bits (4 bytes) + x + y */
		total_size = sizeof(unsigned int) +
					ECCref_MAX_LEN + ECCref_MAX_LEN;
		buffer = malloc(total_size);
		if (!buffer)
			return HSM_ERROR_INTERNAL;

		/* Copy data to buffer */
		p = (unsigned int *)buffer;
		*p = ecc_pub_key.bits;
		memcpy(buffer + sizeof(unsigned int), ecc_pub_key.x, ECCref_MAX_LEN);
		memcpy(buffer + sizeof(unsigned int) + ECCref_MAX_LEN,
			   ecc_pub_key.y, ECCref_MAX_LEN);

		*key_data = buffer;
		*key_len = total_size;
		return HSM_SUCCESS;
	}

	return HSM_ERROR_OPERATION_FAILED;
}

static HsmErrorCode
tzhsm_encrypt(HsmDevice *device, unsigned int key_index,
			 const void *plaintext, size_t plaintext_len,
			 void **ciphertext, size_t *ciphertext_len)
{
	int ret;
	long long hSessionHandle;
	long long hKeyHandle = 0;
	unsigned char iv[16]; /* IV for SM4 CBC mode */
	size_t padded_len;
	size_t total_len; /* Total length of ciphertext (IV + encrypted data) */
	unsigned char pad_value;
	unsigned char *padded_plaintext;
	unsigned char *encrypted_data;
	unsigned int encrypted_len;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!plaintext || plaintext_len == 0 || !ciphertext || !ciphertext_len)
		return HSM_ERROR_INVALID_PARAM;

	hSessionHandle = (long long)device->session_handle;

	/* For TECC: Encrypt DEK (plaintext) using KEK stored in HSM */
	/* KEK is a symmetric SM4 key stored in HSM with index = key_index */

	/* Step 1: Import the KEK to get a key handle */
	/* Note: SDF_ImportKeyWithKEK with NULL/0 (internal key) mode is not
	 * supported by this HSM SDK version, so we pass a dummy test key.
	 * The returned hKeyHandle is the actual KEK handle on this HSM. */
	hKeyHandle = 0;
	{
		unsigned char kek_test_key[16] = {0};
		unsigned int kek_test_key_len = sizeof(kek_test_key);
		ret = tzhsm_sdf.SDF_ImportKeyWithKEK(hSessionHandle,
								   SGD_SM4_CBC,   /* Use CBC for key import (matches encrypt/decrypt) */
								   key_index,    /* KEK index in HSM */
								   kek_test_key,
								   kek_test_key_len,
								   &hKeyHandle);
	}

	if (ret != SDR_OK)
	{
		fprintf(stderr, "SDF_ImportKeyWithKEK failed for key index %u: 0x%08x\n", key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 2: Generate random IV for SM4 CBC mode */
	ret = tzhsm_sdf.SDF_GenerateRandom(hSessionHandle, sizeof(iv), iv);
	if (ret != SDR_OK) {
		if (hKeyHandle != key_index && hKeyHandle != 0) {
			tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
		}
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 3: Prepare plaintext with PKCS#7 padding */
	/* HSM SDK requires plaintext to be block-aligned (16 bytes for SM4) */
	/* PKCS#7: If data length is multiple of block size, add a full block of padding */

	if (plaintext_len == 0) {
		/* Empty data: pad to one full block */
		padded_len = 16;
		pad_value = 16;
	} else {
		/* Calculate padding needed */
		size_t remainder = plaintext_len % 16;
		if (remainder == 0) {
			/* Data is already block-aligned: add a full block of padding */
			padded_len = plaintext_len + 16;
			pad_value = 16;
		} else {
			/* Data needs padding to reach block boundary */
			padded_len = plaintext_len + (16 - remainder);
			pad_value = (unsigned char)(16 - remainder);
		}
	}

	padded_plaintext = (unsigned char *)malloc(padded_len);
	if (!padded_plaintext) {
		if (hKeyHandle != key_index && hKeyHandle != 0) {
			tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
		}
		return HSM_ERROR_INTERNAL;
	}

	/* Copy original plaintext */
	memcpy(padded_plaintext, plaintext, plaintext_len);

	/* Apply PKCS#7 padding */
	{
		size_t i;
		for (i = plaintext_len; i < padded_len; i++) {
			padded_plaintext[i] = pad_value;
		}
	}


	/* Step 4: Encrypt the padded DEK using the KEK */
	/* Allocate buffer for ciphertext: IV + encrypted data */
	total_len = sizeof(iv) + padded_len; /* IV + encrypted data */
	encrypted_data = (unsigned char *)malloc(total_len);
	if (!encrypted_data) {
		free(padded_plaintext);
		if (hKeyHandle != key_index && hKeyHandle != 0) {
			tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
		}
		return HSM_ERROR_INTERNAL;
	}

	/* Copy IV to beginning of ciphertext */
	memcpy(encrypted_data, iv, sizeof(iv));

	/* Perform encryption (store encrypted data after IV) */
	ret = tzhsm_sdf.SDF_Encrypt(hSessionHandle,
					  hKeyHandle,      /* Key handle */
					  SGD_SM4_CBC,     /* SM4 CBC mode */
					  iv,
					  padded_plaintext, /* Use padded plaintext */
					  (unsigned int)padded_len,
					  encrypted_data + sizeof(iv), /* Store after IV */
					  &encrypted_len);


	/* Clean up padded plaintext */
	free(padded_plaintext);

	/* Clean up key handle if we created one */
	if (hKeyHandle != key_index && hKeyHandle != 0) {
		tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
	}

	if (ret == SDR_OK) {
		*ciphertext = encrypted_data;
		*ciphertext_len = sizeof(iv) + encrypted_len; /* Total length: IV + encrypted data */
		return HSM_SUCCESS;
	}

	/* Encryption failed */
	fprintf(stderr, "Encryption failed with error: 0x%08x\n", ret);
	free(encrypted_data);
	return sdf_error_to_hsm_error(ret);
}

static HsmErrorCode
tzhsm_decrypt(HsmDevice *device, unsigned int key_index,
			 const void *ciphertext, size_t ciphertext_len,
			 void **plaintext, size_t *plaintext_len)
{
	int ret;
	long long hSessionHandle;
	long long hKeyHandle = 0;
	unsigned char iv[16]; /* IV for SM4 CBC mode - extracted from ciphertext */
	unsigned char *decrypted_data;
	unsigned int decrypted_len;
	size_t encrypted_data_len;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!ciphertext || ciphertext_len == 0 || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	hSessionHandle = (long long)device->session_handle;

	/* Step 1: Import the KEK to get a key handle (same as encryption) */
	hKeyHandle = 0;
	{
		unsigned char kek_test_key[16] = {0};
		unsigned int kek_test_key_len = sizeof(kek_test_key);
		ret = tzhsm_sdf.SDF_ImportKeyWithKEK(hSessionHandle,
								   SGD_SM4_CBC,   /* SM4 CBC mode for key import (matches encrypt/decrypt) */
								   key_index,    /* KEK index in HSM */
								   kek_test_key,
								   kek_test_key_len,
								   &hKeyHandle);
	}

	if (ret != SDR_OK)
	{
		fprintf(stderr, "SDF_ImportKeyWithKEK failed for key index %u: 0x%08x\n", key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 2: Extract IV from ciphertext (first 16 bytes) */
	if (ciphertext_len < 16) {
		if (hKeyHandle != key_index && hKeyHandle != 0) {
			tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
		}
		return HSM_ERROR_INVALID_PARAM; /* Ciphertext too short to contain IV */
	}

	memcpy(iv, ciphertext, sizeof(iv));

	/* Step 3: Decrypt the ciphertext using the KEK */
	/* Calculate encrypted data length (excluding IV) */
	encrypted_data_len = ciphertext_len - sizeof(iv);

	/* Allocate buffer for plaintext */
	decrypted_data = (unsigned char *)malloc(encrypted_data_len); /* Max possible size */
	if (!decrypted_data) {
		if (hKeyHandle != key_index && hKeyHandle != 0) {
			tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
		}
		return HSM_ERROR_INTERNAL;
	}

	/* Perform decryption (skip IV at beginning of ciphertext) */
	ret = tzhsm_sdf.SDF_Decrypt(hSessionHandle,
					  hKeyHandle,      /* Key handle */
					  SGD_SM4_CBC,     /* SM4 CBC mode */
					  iv,
					  (unsigned char *)ciphertext + sizeof(iv), /* Skip IV */
					  (unsigned int)encrypted_data_len,
					  decrypted_data,
					  &decrypted_len);

	/* Clean up key handle if we created one */
	if (hKeyHandle != key_index && hKeyHandle != 0) {
		tzhsm_sdf.SDF_DestroyKey(hSessionHandle, hKeyHandle);
	}

	if (ret == SDR_OK) {
		/* Remove PKCS#7 padding from decrypted data */
		if (decrypted_len > 0) {
			unsigned char pad_value = decrypted_data[decrypted_len - 1];

			/* Validate padding */
			if (pad_value > 0 && pad_value <= 16 && decrypted_len >= pad_value) {
				/* Check all padding bytes */
				int valid_padding = 1;
				{
					unsigned int i;
					for (i = 1; i <= pad_value; i++) {
						if (decrypted_data[decrypted_len - i] != pad_value) {
							valid_padding = 0;
							break;
						}
					}
				}

				if (valid_padding) {
					/* Remove padding */
					size_t unpadded_len = decrypted_len - pad_value;
					unsigned char *unpadded_data = (unsigned char *)malloc(unpadded_len);
					if (!unpadded_data) {
						free(decrypted_data);
						return HSM_ERROR_INTERNAL;
					}

					memcpy(unpadded_data, decrypted_data, unpadded_len);
					free(decrypted_data);

					*plaintext = unpadded_data;
					*plaintext_len = unpadded_len;
					return HSM_SUCCESS;
				}
			}

			/* If padding is invalid, check if data might already be unpadded */
			/* This could happen if decrypting data from soft implementation without padding */
			fprintf(stderr, "Warning: Invalid PKCS#7 padding (pad_value=0x%02x, len=%u), returning data as-is\n",
					pad_value, decrypted_len);
		}

		*plaintext = decrypted_data;
		*plaintext_len = decrypted_len;
		return HSM_SUCCESS;
	}

	/* Decryption failed */
	free(decrypted_data);
	return sdf_error_to_hsm_error(ret);
}

/* ReqID:TECC4.0 - SM2 envelope decryption with the device-internal SM2 private key
 * (identified by key_index).  Input ciphertext is the HE private-key envelope produced
 * by local tecc_sm2_envelope_encrypt (OpenSSL SM2, C1||C3||C2), converted to the SDF
 * ECCCipher layout (x||y||M||L||C). */
static HsmErrorCode
tzhsm_sm2_decrypt(HsmDevice *device, unsigned int key_index,
				  const unsigned char *ciphertext, size_t cipher_len,
				  void **plaintext, size_t *plaintext_len)
{
	int ret;
	long long hSessionHandle;
	ECCCipher *ecc_cipher;
	unsigned char *plain_buf;
	unsigned int plain_len;
	size_t c2_len;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!ciphertext || cipher_len <= 96 || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	hSessionHandle = (long long)device->session_handle;

	/* C1(64: 04||x||y) || C3(32) || C2(rest) */
	c2_len = cipher_len - 96;

	ecc_cipher = (ECCCipher *)malloc(sizeof(ECCCipher) - 1 + c2_len);
	if (!ecc_cipher)
		return HSM_ERROR_INTERNAL;
	memset(ecc_cipher, 0, sizeof(ECCCipher) - 1 + c2_len);

	/* C1 point: skip 0x04 prefix, then x(32) and y(32) */
	memcpy(ecc_cipher->x, ciphertext + 1, ECCref_MAX_LEN);
	memcpy(ecc_cipher->y, ciphertext + 1 + ECCref_MAX_LEN, ECCref_MAX_LEN);
	/* C3 hash */
	memcpy(ecc_cipher->M, ciphertext + 64, 32);
	/* C2 ciphertext */
	ecc_cipher->L = (unsigned int)c2_len;
	memcpy(ecc_cipher->C, ciphertext + 96, c2_len);

	/* OPENSSL_malloc so the caller (tecc_algorithm_scheduler) can OPENSSL_free */
	plain_buf = (unsigned char *)OPENSSL_malloc(c2_len);
	if (!plain_buf)
	{
		free(ecc_cipher);
		return HSM_ERROR_INTERNAL;
	}
	plain_len = (unsigned int)c2_len;

	ret = tzhsm_sdf.SDF_InternalDecrypt_ECC(hSessionHandle, key_index, SGD_SM2,
								  ecc_cipher, plain_buf, &plain_len);
	free(ecc_cipher);

	if (ret != SDR_OK)
	{
		OPENSSL_free(plain_buf);
		return sdf_error_to_hsm_error(ret);
	}

	*plaintext = plain_buf;
	*plaintext_len = (size_t)plain_len;
	return HSM_SUCCESS;
}

static HsmErrorCode
tzhsm_sign(HsmDevice *device, unsigned int key_index,
		  const void *data, size_t data_len,
		  void **signature, size_t *signature_len)
{
	/* Suppress unused parameter warnings */
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	/* TODO: Fix SM2 signature implementation - parameter mismatch */
	/* For now, return not supported to allow compilation */
	return HSM_ERROR_NOT_SUPPORTED;
}

static HsmErrorCode
tzhsm_verify(HsmDevice *device, unsigned int key_index,
			const void *data, size_t data_len,
			const void *signature, size_t signature_len)
{
	/* Suppress unused parameter warnings */
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	/* TODO: Fix SM2 verification implementation - parameter mismatch */
	/* For now, return not supported to allow compilation */
	return HSM_ERROR_NOT_SUPPORTED;
}

static const char *
tzhsm_error_string(HsmErrorCode error_code)
{
	switch (error_code)
	{
		case HSM_SUCCESS:
			return "Success";
		case HSM_ERROR_INIT_FAILED:
			return "HSM initialization failed";
		case HSM_ERROR_CONNECT_FAILED:
			return "HSM connection failed";
		case HSM_ERROR_KEY_NOT_FOUND:
			return "Key not found in HSM";
		case HSM_ERROR_KEY_ACCESS_DENIED:
			return "Key access denied";
		case HSM_ERROR_INVALID_PARAM:
			return "Invalid parameter";
		case HSM_ERROR_OPERATION_FAILED:
			return "HSM operation failed";
		case HSM_ERROR_NOT_SUPPORTED:
			return "Operation not supported";
		case HSM_ERROR_INTERNAL:
			return "Internal error";
		default:
			return "Unknown error";
	}
}

static bool
tzhsm_is_connected(HsmDevice *device)
{
	return device && device->connected;
}

/*
 * tzhsm_sm4_encrypt - Encrypt data using HSM SM4-CBC with raw key
 *
 * Imports the raw SM4 key as a session key via SDF_ImportKey,
 * encrypts the plaintext with PKCS#7 padding using SDF_Encrypt (SGD_SM4_CBC),
 * then destroys the session key.
 */
static HsmErrorCode
tzhsm_sm4_encrypt(HsmDevice *device,
				  const unsigned char *key, size_t key_len,
				  const unsigned char *iv,
				  const unsigned char *plaintext, size_t plain_len,
				  unsigned char *ciphertext, size_t *cipher_len)
{
	int ret;
	long long hSession;
	long long hKey = 0;
	size_t padded_len;
	unsigned char *padded = NULL;
	unsigned char pad_val;
	unsigned int out_len;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;
	if (!key || key_len == 0 || !iv || !plaintext || plain_len == 0 ||
		!ciphertext || !cipher_len)
		return HSM_ERROR_INVALID_PARAM;

	hSession = (long long)device->session_handle;

	/* Import raw SM4 key as session key handle */
	ret = tzhsm_sdf.SDF_ImportKey(hSession, (unsigned char *)key, (unsigned int)key_len, &hKey);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "tzhsm_sm4_encrypt: SDF_ImportKey failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* PKCS#7 padding: pad to 16-byte block boundary */
	padded_len = ((plain_len / 16) + 1) * 16;
	pad_val = (unsigned char)(padded_len - plain_len);
	padded = (unsigned char *)malloc(padded_len);
	if (!padded)
	{
		tzhsm_sdf.SDF_DestroyKey(hSession, hKey);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(padded, plaintext, plain_len);
	memset(padded + plain_len, pad_val, padded_len - plain_len);

	/* SM4-CBC encrypt via HSM */
	out_len = (unsigned int)padded_len;
	ret = tzhsm_sdf.SDF_Encrypt(hSession, hKey, SGD_SM4_CBC,
					 (unsigned char *)iv,
					 padded, (unsigned int)padded_len,
					 ciphertext, &out_len);

	free(padded);
	tzhsm_sdf.SDF_DestroyKey(hSession, hKey);

	if (ret != SDR_OK)
	{
		fprintf(stderr, "tzhsm_sm4_encrypt: SDF_Encrypt failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	*cipher_len = out_len;
	return HSM_SUCCESS;
}

/*
 * tzhsm_sm4_decrypt - Decrypt data using HSM SM4-CBC with raw key
 *
 * Imports the raw SM4 key as a session key, decrypts using SDF_Decrypt,
 * removes PKCS#7 padding, then destroys the session key.
 */
static HsmErrorCode
tzhsm_sm4_decrypt(HsmDevice *device,
				  const unsigned char *key, size_t key_len,
				  const unsigned char *iv,
				  const unsigned char *ciphertext, size_t cipher_len,
				  unsigned char *plaintext, size_t *plain_len)
{
	int ret;
	long long hSession;
	long long hKey = 0;
	unsigned int out_len;
	unsigned char *decrypted = NULL;
	unsigned char pad_val;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;
	if (!key || key_len == 0 || !iv || !ciphertext || cipher_len == 0 ||
		!plaintext || !plain_len)
		return HSM_ERROR_INVALID_PARAM;

	hSession = (long long)device->session_handle;

	/* Import raw SM4 key as session key handle */
	ret = tzhsm_sdf.SDF_ImportKey(hSession, (unsigned char *)key, (unsigned int)key_len, &hKey);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "tzhsm_sm4_decrypt: SDF_ImportKey failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Allocate output buffer */
	decrypted = (unsigned char *)malloc(cipher_len);
	if (!decrypted)
	{
		tzhsm_sdf.SDF_DestroyKey(hSession, hKey);
		return HSM_ERROR_INTERNAL;
	}

	/* SM4-CBC decrypt via HSM */
	out_len = (unsigned int)cipher_len;
	ret = tzhsm_sdf.SDF_Decrypt(hSession, hKey, SGD_SM4_CBC,
					 (unsigned char *)iv,
					 (unsigned char *)ciphertext, (unsigned int)cipher_len,
					 decrypted, &out_len);

	tzhsm_sdf.SDF_DestroyKey(hSession, hKey);

	if (ret != SDR_OK)
	{
		fprintf(stderr, "tzhsm_sm4_decrypt: SDF_Decrypt failed: 0x%08x\n", ret);
		free(decrypted);
		return sdf_error_to_hsm_error(ret);
	}

	/* Remove PKCS#7 padding */
	if (out_len == 0)
	{
		free(decrypted);
		return HSM_ERROR_INVALID_PARAM;
	}
	pad_val = decrypted[out_len - 1];
	if (pad_val > 0 && pad_val <= 16 && out_len >= pad_val)
	{
		size_t unpadded_len = out_len - pad_val;
		memcpy(plaintext, decrypted, unpadded_len);
		*plain_len = unpadded_len;
	}
	else
	{
		/* Invalid padding, return data as-is */
		memcpy(plaintext, decrypted, out_len);
		*plain_len = out_len;
	}
	free(decrypted);
	return HSM_SUCCESS;
}


/*
 * Utility functions
 */

/*
 * sdf_error_to_hsm_error - Convert SDF error code to HSM error code
 */
static HsmErrorCode
sdf_error_to_hsm_error(int sdf_error)
{
	switch (sdf_error)
	{
		case SDR_OK:
			return HSM_SUCCESS;
		case SDR_KEYNOTEXIST:
			return HSM_ERROR_KEY_NOT_FOUND;
		case SDR_PARDENY:
			return HSM_ERROR_KEY_ACCESS_DENIED;
		case SDR_NOTSUPPORT:
		case SDR_ALGNOTSUPPORT:
			return HSM_ERROR_NOT_SUPPORTED;
		case SDR_OPENDEVICE:
		case SDR_OPENSESSION:
		case SDR_COMMFAIL:
			return HSM_ERROR_CONNECT_FAILED;
		case SDR_HARDFAIL:
			return HSM_ERROR_INIT_FAILED;
		default:
			return HSM_ERROR_OPERATION_FAILED;
	}
}

/*
 * parse_hsm_key_path - Parse HSM key path in format "hsm:id=<index>"
 */
bool
parse_hsm_key_path(const char *key_path, unsigned int *key_index)
{
	const char *prefix = "hsm:id=";
	size_t prefix_len = strlen(prefix);
	char *endptr;
	long index;

	if (!key_path || !key_index)
		return false;

	/* Check if path starts with "hsm:id=" */
	if (strncmp(key_path, prefix, prefix_len) != 0)
		return false;

	/* Parse the index */
	errno = 0;
	index = strtol(key_path + prefix_len, &endptr, 10);

	/* Check for conversion errors */
	if (errno != 0 || endptr == key_path + prefix_len || *endptr != '\0')
		return false;

	/* Check if index is valid (1-65535) */
	if (index < 1 || index > 65535)
		return false;

	*key_index = (unsigned int)index;
	return true;
}

/*
 * setup_hsm_kek - Setup HSM KEK by checking if key exists at specified index
 */
int
setup_hsm_kek(const char *kek_name, const char *key_path)
{
	unsigned int key_index;
	HsmAdapterInterface *adapter;
	HsmDevice *device = NULL;
	HsmErrorCode error;
	int result = -1;

	if (!kek_name || !key_path)
	{
		fprintf(stderr, "ERROR: KEK name or key path is NULL\n");
		return -1;
	}

	/* Parse key path */
	if (!parse_hsm_key_path(key_path, &key_index))
	{
		fprintf(stderr, "ERROR: Invalid HSM key path format: %s\n", key_path);
		fprintf(stderr, "Expected format: hsm:id=<index> where index is 1-65535\n");
		return -1;
	}

	/* ReqID:TECC4.0 - 原先用 printf 打 stdout，会混进使用方捕获的结果流；改走既有
	 * TECC_LOG（级别见 TECC_LOG_LEVEL，默认 ERROR 即静默）。 */
	TECC_LOG(TECC_LOG_DEBUG1, "Setting up HSM KEK: %s", kek_name);
	TECC_LOG(TECC_LOG_DEBUG1, "Key path: %s (index: %u)", key_path, key_index);

	/* Get HSM adapter - use Tongzhi Weiye adapter for real password machine */
	adapter = get_tzhsm_adapter();

	/* Initialize HSM device */
	error = adapter->init(&device, "/usr/lib64/cfg/client_config.xml");
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to initialize HSM: %s\n",
				adapter->error_string(error));
		goto cleanup;
	}

	/* Connect to HSM */
	error = adapter->connect(device);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to connect to HSM: %s\n",
				adapter->error_string(error));
		goto cleanup;
	}

	/* Check if key exists at specified index */
	error = adapter->check_key_exists(device, key_index);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Key not found in HSM at index %u: %s\n",
				key_index, adapter->error_string(error));
		fprintf(stderr, "Please generate the key in the password machine first\n");
		goto cleanup;
	}

	TECC_LOG(TECC_LOG_INFO, "SUCCESS: Key found in HSM at index %u", key_index);
	TECC_LOG(TECC_LOG_DEBUG1, "HSM KEK setup completed for: %s", kek_name);

	result = 0;

cleanup:
	if (device)
	{
		if (adapter->is_connected(device))
			adapter->disconnect(device);
		adapter->cleanup(device);
	}

	return result;
}
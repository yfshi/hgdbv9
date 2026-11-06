/*-------------------------------------------------------------------------
 *
 * fe-swxa-adapter.c
 *	  三未信安 (SWXA) HSM adapter implementation
 *
 * Implements HsmAdapterInterface for 三未信安 password machines
 * using the SDF interface loaded at runtime via dlopen/dlsym.
 *
 * Portions Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
/* Windows lacks these POSIX dlopen flags.  The PostgreSQL port layer
 * provides dlopen() on Windows via LoadLibrary, which ignores mode. */
#define RTLD_LAZY     0
#define RTLD_LOCAL    0
#define RTLD_DEEPBIND 0
#endif

#include <dlfcn.h>

#include "fe-swxa-adapter.h"
#include "fe-hsm-adapter.h"

/*
 * SWXA SDF API function table.
 *
 * libswsds.so is loaded at runtime via dlopen in swxa_load_sdf_api()
 * to avoid symbol conflicts with libsdf.client.so (Tianzheng HSM),
 * which exports overlapping SDF symbols.
 */
typedef struct SwxaSdfApi
{
	void *handle;	/* dlopen handle */

	int (*SDF_OpenDeviceWithPathAndName)(char *pcCfgfile, void **phDeviceHandle);
	int (*SDF_OpenSession)(void *hDeviceHandle, void **phSessionHandle);
	int (*SDF_CloseDevice)(void *hDeviceHandle);
	int (*SDF_CloseSession)(void *hSessionHandle);
	int (*SDF_GetSymmKeyHandle)(void *hSessionHandle, unsigned int uiKeyIndex, void **phKeyHandle);
	int (*SDF_DestroyKey)(void *hSessionHandle, void *hKeyHandle);
	int (*SDF_GenerateRandom)(void *hSessionHandle, unsigned int uiLength, unsigned char *pucRandom);
	int (*SDF_Encrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
					   unsigned char *pucIV, unsigned char *pucData,
					   unsigned int uiDataLength, unsigned char *pucEncData,
					   unsigned int *puiEncDataLength);
	int (*SDF_Decrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
					   unsigned char *pucIV, unsigned char *pucEncData,
					   unsigned int uiEncDataLength, unsigned char *pucData,
					   unsigned int *puiDataLength);
	int (*SDF_ImportKey)(void *hSessionHandle, unsigned char *pucKey,
						 unsigned int uiKeyLength, void **phKeyHandle);
} SwxaSdfApi;

/* Static SDF API table - initialized once by swxa_load_sdf_api() */
static SwxaSdfApi swxa_sdf = {NULL};

/* SDF error/algorithm constants (from swsds.h - kept local to avoid header dependency) */
#define SDR_OK					0x0
#define SDR_BASE				0x01000000
#define SDR_NOTSUPPORT			(SDR_BASE + 0x00000002)
#define SDR_COMMFAIL			(SDR_BASE + 0x00000003)
#define SDR_HARDFAIL			(SDR_BASE + 0x00000004)
#define SDR_OPENDEVICE			(SDR_BASE + 0x00000005)
#define SDR_OPENSESSION			(SDR_BASE + 0x00000006)
#define SDR_PARDENY				(SDR_BASE + 0x00000007)
#define SDR_KEYNOTEXIST			(SDR_BASE + 0x00000008)
#define SDR_ALGNOTSUPPORT		(SDR_BASE + 0x00000009)
#define SDR_INARGERR			(SDR_BASE + 0x0000001D)

#define SGD_SMS4_CBC			0x00000402

/*
 * SGD_SMS4 - Base SM4 symmetric key type for GetKeyStatus API.
 * Defined as the common base 0x00000400.
 */
#define SGD_SMS4 0x00000400

/* Forward declarations */
static HsmErrorCode sdf_error_to_hsm_error(int sdf_error);
static HsmErrorCode swxa_load_sdf_api(void);
static HsmErrorCode swxa_init(HsmDevice **device, const char *config_file);
static HsmErrorCode swxa_connect(HsmDevice *device);
static HsmErrorCode swxa_disconnect(HsmDevice *device);
static void swxa_cleanup(HsmDevice *device);
static HsmErrorCode swxa_check_key_exists(HsmDevice *device, unsigned int key_index);
static HsmErrorCode swxa_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info);
static HsmErrorCode swxa_export_public_key(HsmDevice *device, unsigned int key_index,
										   void **key_data, size_t *key_len);
static HsmErrorCode swxa_encrypt(HsmDevice *device, unsigned int key_index,
								 const void *plaintext, size_t plaintext_len,
								 void **ciphertext, size_t *ciphertext_len);
static HsmErrorCode swxa_decrypt(HsmDevice *device, unsigned int key_index,
								 const void *ciphertext, size_t ciphertext_len,
								 void **plaintext, size_t *plaintext_len);
static HsmErrorCode swxa_sign(HsmDevice *device, unsigned int key_index,
							  const void *data, size_t data_len,
							  void **signature, size_t *signature_len);
static HsmErrorCode swxa_verify(HsmDevice *device, unsigned int key_index,
								const void *data, size_t data_len,
								const void *signature, size_t signature_len);
static HsmErrorCode swxa_sm4_encrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *plaintext, size_t plain_len,
									 unsigned char *ciphertext, size_t *cipher_len);
static HsmErrorCode swxa_sm4_decrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *ciphertext, size_t cipher_len,
									 unsigned char *plaintext, size_t *plain_len);
static const char *swxa_error_string(HsmErrorCode error_code);
static bool swxa_is_connected(HsmDevice *device);

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

/* Static adapter instance */
static HsmAdapterInterface swxa_adapter = {0};

/*
 * get_swxa_adapter - Get SWXA adapter instance
 */
HsmAdapterInterface *
get_swxa_adapter(void)
{
	if (swxa_adapter.init == NULL)
	{
		swxa_adapter.init = swxa_init;
		swxa_adapter.connect = swxa_connect;
		swxa_adapter.disconnect = swxa_disconnect;
		swxa_adapter.cleanup = swxa_cleanup;
		swxa_adapter.check_key_exists = swxa_check_key_exists;
		swxa_adapter.get_key_info = swxa_get_key_info;
		swxa_adapter.export_public_key = swxa_export_public_key;
		swxa_adapter.encrypt = swxa_encrypt;
		swxa_adapter.decrypt = swxa_decrypt;
		swxa_adapter.error_string = swxa_error_string;
		swxa_adapter.is_connected = swxa_is_connected;
		/* Stage 3: sign/verify/sm4_encrypt/sm4_decrypt */
		swxa_adapter.sign = swxa_sign;
		swxa_adapter.verify = swxa_verify;
		swxa_adapter.sm4_encrypt = swxa_sm4_encrypt;
		swxa_adapter.sm4_decrypt = swxa_sm4_decrypt;
	}
	return &swxa_adapter;
}

/*
 * swxa_load_sdf_api - Load SWXA SDF library via dlopen
 *
 * Loads libswsds.so at runtime using dlopen with RTLD_LOCAL to avoid
 * symbol conflicts with libsdf.client.so (Tianzheng HSM adapter).
 * After loading, resolves all SDF function pointers via dlsym.
 */
static HsmErrorCode
swxa_load_sdf_api(void)
{
	const char *dl_error;

	if (swxa_sdf.handle != NULL)
		return HSM_SUCCESS;

		/* Try full paths first (common install locations), then bare name for
		 * LD_LIBRARY_PATH / RPATH discovery.
		 * Use RTLD_DEEPBIND so the SWXA library's own symbols take priority over
		 * libsdf.client.so (Tianzheng) which is already in the global namespace.
		 * Without RTLD_DEEPBIND, libswsds.so's internal SDF calls would resolve to
		 * libsdf.client.so's implementations, causing protocol errors. */
	swxa_sdf.handle = dlopen("/usr/local/lib/libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if (!swxa_sdf.handle)
		swxa_sdf.handle = dlopen("/usr/lib64/libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if (!swxa_sdf.handle)
		swxa_sdf.handle = dlopen("libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if (!swxa_sdf.handle)
	{
		fprintf(stderr, "ERROR: Failed to load libswsds.so: %s\n", dlerror());
		return HSM_ERROR_INIT_FAILED;
	}

	/* Resolve all SDF functions via dlsym */
#define LOAD_SDF_FUNC(name) \
	do { \
		swxa_sdf.name = dlsym(swxa_sdf.handle, #name); \
		if (!swxa_sdf.name) { \
			fprintf(stderr, "ERROR: Failed to resolve %s in libswsds.so: %s\n", \
					#name, dlerror()); \
			dlclose(swxa_sdf.handle); \
			swxa_sdf.handle = NULL; \
			return HSM_ERROR_INIT_FAILED; \
		} \
	} while(0)

	LOAD_SDF_FUNC(SDF_OpenDeviceWithPathAndName);
	LOAD_SDF_FUNC(SDF_OpenSession);
	LOAD_SDF_FUNC(SDF_CloseDevice);
	LOAD_SDF_FUNC(SDF_CloseSession);
	LOAD_SDF_FUNC(SDF_GetSymmKeyHandle);
	LOAD_SDF_FUNC(SDF_DestroyKey);
	LOAD_SDF_FUNC(SDF_GenerateRandom);
	LOAD_SDF_FUNC(SDF_Encrypt);
	LOAD_SDF_FUNC(SDF_Decrypt);
	LOAD_SDF_FUNC(SDF_ImportKey);

#undef LOAD_SDF_FUNC

	dl_error = dlerror();
	if (dl_error)
	{
		fprintf(stderr, "ERROR: Unexpected dlopen/dlsym error: %s\n", dl_error);
		dlclose(swxa_sdf.handle);
		swxa_sdf.handle = NULL;
		return HSM_ERROR_INIT_FAILED;
	}

	return HSM_SUCCESS;
}

/*
 * swxa_init - Initialize SWXA device
 *
 * Loads libswsds.so at runtime via dlopen, then checks swsds.ini from
 * $PGDATA/ directory. Validates that the config file exists before returning.
 * The config_file parameter (KEY_PATH like "hsm:id=1") is not used
 * for ini path resolution - ini path is always $PGDATA/swsds.ini.
 *
 * Note: HSM_ERROR_INIT_FAILED from this function may also indicate that
 * libswsds.so was not found or could not be loaded.
 */
static HsmErrorCode
swxa_init(HsmDevice **device, const char *config_file)
{
	HsmDevice *dev;
	char ini_path[1024];
	struct stat st;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	/* Load libswsds.so at runtime via dlopen */
	{
		HsmErrorCode load_result = swxa_load_sdf_api();
		if (load_result != HSM_SUCCESS)
			return load_result;
	}

	(void)config_file;  /* KEY_PATH (e.g. "hsm:id=1"), not ini path */

	/* Try multiple paths in order */
	{
		const char *pgdata = getenv("PGDATA");
		const char *search_dirs[4];
		int ndirs = 0;
		int i;
		bool found = false;

		if (pgdata)
			search_dirs[ndirs++] = pgdata;
		search_dirs[ndirs++] = ".";
		search_dirs[ndirs++] = "/etc";

		for (i = 0; i < ndirs; i++)
		{
			snprintf(ini_path, sizeof(ini_path), "%s/swsds.ini", search_dirs[i]);
			if (stat(ini_path, &st) == 0 && S_ISREG(st.st_mode))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			fprintf(stderr, "ERROR: swsds.ini not found\n");
			fprintf(stderr, "HINT: Place swsds.ini in PGDATA/, current directory, or /etc/\n");
			return HSM_ERROR_INIT_FAILED;
		}
	}

	/* Allocate device structure */
	dev = (HsmDevice *)calloc(1, sizeof(HsmDevice));
	if (!dev)
		return HSM_ERROR_INTERNAL;

	dev->config_file = strdup(ini_path);
	if (!dev->config_file)
	{
		free(dev);
		return HSM_ERROR_INTERNAL;
	}

	dev->device_handle = NULL;
	dev->session_handle = NULL;
	dev->connected = false;

	*device = dev;
	return HSM_SUCCESS;
}

/*
 * swxa_connect - Open device and session
 */
static HsmErrorCode
swxa_connect(HsmDevice *device)
{
	int ret;
	void *hDevice = NULL;
	void *hSession = NULL;

	if (!device || !device->config_file)
		return HSM_ERROR_INVALID_PARAM;

	if (device->connected)
		return HSM_SUCCESS;

	/* Open device via config file */
	ret = swxa_sdf.SDF_OpenDeviceWithPathAndName(device->config_file, &hDevice);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_OpenDeviceWithPathAndName failed: 0x%08x\n", ret);
		fprintf(stderr, "HINT: Check network connectivity and swsds.ini configuration\n");
		return sdf_error_to_hsm_error(ret);
	}

	/* Open session */
	ret = swxa_sdf.SDF_OpenSession(hDevice, &hSession);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_OpenSession failed: 0x%08x\n", ret);
		swxa_sdf.SDF_CloseDevice(hDevice);
		return sdf_error_to_hsm_error(ret);
	}

	device->device_handle = hDevice;
	device->session_handle = hSession;
	device->connected = true;

	return HSM_SUCCESS;
}

/*
 * swxa_disconnect - Close session only
 *
 * Only closes the session handle. Does NOT call SDF_CloseDevice because
 * libswsds.so's device-level cleanup (including internal connection pool
 * and semaphore teardown) can leave the library in an inconsistent state
 * for subsequent OpenDevice calls from the same process. The device handle
 * is closed once at process exit via swxa_cleanup's atexit or final cleanup.
 */
static HsmErrorCode
swxa_disconnect(HsmDevice *device)
{
	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	if (!device->connected)
		return HSM_SUCCESS;

	if (device->session_handle)
	{
		swxa_sdf.SDF_CloseSession(device->session_handle);
		device->session_handle = NULL;
	}

	/* Skip SDF_CloseDevice to preserve SWXA library internal state
	 * (connection pool, semaphores) across device re-connects. */
	device->connected = false;
	return HSM_SUCCESS;
}

/*
 * swxa_cleanup - Free device resources
 */
static void
swxa_cleanup(HsmDevice *device)
{
	if (!device)
		return;

	if (device->connected)
		swxa_disconnect(device);

	if (device->config_file)
		free(device->config_file);

	free(device);
}

/*
 * swxa_check_key_exists - Check if key exists at given index
 *
 * Uses SDF_GetSymmKeyHandle to attempt to open the key.
 * If the key handle is obtained successfully, the key exists.
 */
static HsmErrorCode
swxa_check_key_exists(HsmDevice *device, unsigned int key_index)
{
	int ret;
	void *hKeyHandle = NULL;

	if (!device || !device->connected)
		return HSM_ERROR_CONNECT_FAILED;

	/* Try to get key handle - succeeds if key exists */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		if (ret == SDR_KEYNOTEXIST)
			return HSM_ERROR_KEY_NOT_FOUND;
		/* SDR_INARGERR (invalid index) also means key doesn't exist */
		if (ret == SDR_INARGERR)
			return HSM_ERROR_KEY_NOT_FOUND;
		fprintf(stderr, "WARNING: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Key exists, release handle */
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
	return HSM_SUCCESS;
}

/*
 * swxa_error_string - Convert error code to string
 */
static const char *
swxa_error_string(HsmErrorCode error_code)
{
	switch (error_code)
	{
		case HSM_SUCCESS:
			return "Success";
		case HSM_ERROR_INIT_FAILED:
			return "SWXA device initialization failed (check swsds.ini)";
		case HSM_ERROR_CONNECT_FAILED:
			return "SWXA device connection failed (check network/config)";
		case HSM_ERROR_KEY_NOT_FOUND:
			return "Key not found in SWXA password machine";
		case HSM_ERROR_KEY_ACCESS_DENIED:
			return "Key access denied";
		case HSM_ERROR_INVALID_PARAM:
			return "Invalid parameter";
		case HSM_ERROR_OPERATION_FAILED:
			return "SWXA operation failed";
		case HSM_ERROR_NOT_SUPPORTED:
			return "Operation not supported";
		case HSM_ERROR_INTERNAL:
			return "Internal error";
		default:
			return "Unknown error";
	}
}

/*
 * swxa_is_connected - Check if device is connected
 */
static bool
swxa_is_connected(HsmDevice *device)
{
	return device && device->connected;
}

/*
 * swxa_get_key_info - Get key information from SWXA HSM
 *
 * Returns the key type (SGD_SMS4) and key bits (128) for SM4 symmetric keys.
 * Uses SDF_GetSymmKeyHandle to verify the key exists before returning info.
 */
static HsmErrorCode
swxa_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info)
{
	int ret;
	void *hKeyHandle = NULL;
	HsmKeyInfo *info;

	if (!device || !device->connected || !key_info)
		return HSM_ERROR_INVALID_PARAM;

	/* Verify key exists by trying to get a handle */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		if (ret == SDR_INARGERR || ret == SDR_KEYNOTEXIST)
			return HSM_ERROR_KEY_NOT_FOUND;
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return HSM_ERROR_KEY_NOT_FOUND;
	}
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	/* Allocate key info */
	info = (HsmKeyInfo *)calloc(1, sizeof(HsmKeyInfo));
	if (!info)
		return HSM_ERROR_INTERNAL;

	info->key_index = key_index;
	info->key_bits = 128;		/* SM4 is 128-bit */
	info->key_type = SGD_SMS4;	/* SM4 symmetric key */
	info->key_name = NULL;
	info->key_data = NULL;
	info->key_len = 0;

	*key_info = info;
	return HSM_SUCCESS;
}

/*
 * swxa_export_public_key - Export public key from SWXA HSM
 *
 * Not supported for SWXA because KEK is an SM4 symmetric key,
 * which has no associated public key.
 */
static HsmErrorCode
swxa_export_public_key(HsmDevice *device, unsigned int key_index,
					   void **key_data, size_t *key_len)
{
	(void)device;
	(void)key_index;
	(void)key_data;
	(void)key_len;

	fprintf(stderr, "ERROR: export_public_key not supported for SWXA (SM4 symmetric key)\n");
	return HSM_ERROR_NOT_SUPPORTED;
}

/*
 * swxa_encrypt - Encrypt data using key at given index in SWXA HSM
 *
 * 1. Gets key handle via SDF_GetSymmKeyHandle
 * 2. Generates random IV via SDF_GenerateRandom
 * 3. Applies PKCS#7 padding to plaintext
 * 4. Encrypts via SDF_Encrypt (SGD_SMS4_CBC)
 * 5. Destroys key handle
 * 6. Returns ciphertext = IV (16 bytes) + encrypted data
 */
static HsmErrorCode
swxa_encrypt(HsmDevice *device, unsigned int key_index,
			 const void *plaintext, size_t plaintext_len,
			 void **ciphertext, size_t *ciphertext_len)
{
	int ret;
	void *hKeyHandle = NULL;
	unsigned char iv[16];
	size_t padded_len;
	unsigned char *padded_plaintext = NULL;
	unsigned char *enc_data = NULL;
	unsigned int enc_len;
	unsigned char pad_value;
	unsigned char *result_buf = NULL;
	unsigned char iv_save[16];

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!plaintext || plaintext_len == 0 || !ciphertext || !ciphertext_len)
		return HSM_ERROR_INVALID_PARAM;

	/* Step 1: Get key handle for the KEK at key_index */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 2: Generate random IV */
	ret = swxa_sdf.SDF_GenerateRandom(device->session_handle, sizeof(iv), iv);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GenerateRandom failed: 0x%08x\n", ret);
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return sdf_error_to_hsm_error(ret);
	}

	/*
	 * Backup IV before SDF_Encrypt potentially modifies the buffer.
	 * Some SDF implementations modify the IV in place.
	 */
	memcpy(iv_save, iv, sizeof(iv));

	/* Step 3: PKCS#7 padding to 16-byte block boundary */
	if (plaintext_len == 0)
	{
		padded_len = 16;
		pad_value = 16;
	}
	else
	{
		size_t remainder = plaintext_len % 16;
		if (remainder == 0)
		{
			padded_len = plaintext_len + 16;
			pad_value = 16;
		}
		else
		{
			padded_len = plaintext_len + (16 - remainder);
			pad_value = (unsigned char)(16 - remainder);
		}
	}

	padded_plaintext = (unsigned char *)malloc(padded_len);
	if (!padded_plaintext)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(padded_plaintext, plaintext, plaintext_len);
	memset(padded_plaintext + plaintext_len, pad_value, padded_len - plaintext_len);

	/* Step 4: Encrypt via HSM */
	enc_data = (unsigned char *)malloc(padded_len);
	if (!enc_data)
	{
		free(padded_plaintext);
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}

	enc_len = (unsigned int)padded_len;
	ret = swxa_sdf.SDF_Encrypt(device->session_handle, hKeyHandle, SGD_SMS4_CBC,
							  iv, padded_plaintext, (unsigned int)padded_len,
							  enc_data, &enc_len);

	free(padded_plaintext);
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	if (ret != SDR_OK)
	{
		free(enc_data);
		fprintf(stderr, "ERROR: SDF_Encrypt failed for key index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 5: Build result = IV (from backup) + encrypted data */
	result_buf = (unsigned char *)malloc(sizeof(iv_save) + enc_len);
	if (!result_buf)
	{
		free(enc_data);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(result_buf, iv_save, sizeof(iv_save));
	memcpy(result_buf + sizeof(iv_save), enc_data, enc_len);
	free(enc_data);

	*ciphertext = result_buf;
	*ciphertext_len = sizeof(iv_save) + enc_len;

	return HSM_SUCCESS;
}

/*
 * swxa_decrypt - Decrypt data using key at given index in SWXA HSM
 *
 * 1. Extracts IV from ciphertext (first 16 bytes)
 * 2. Gets key handle via SDF_GetSymmKeyHandle
 * 3. Decrypts via SDF_Decrypt (SGD_SMS4_CBC)
 * 4. Destroys key handle
 * 5. Removes PKCS#7 padding
 * 6. Returns plaintext
 */
static HsmErrorCode
swxa_decrypt(HsmDevice *device, unsigned int key_index,
			 const void *ciphertext, size_t ciphertext_len,
			 void **plaintext, size_t *plaintext_len)
{
	int ret;
	void *hKeyHandle = NULL;
	unsigned char iv[16];
	unsigned char *dec_data = NULL;
	unsigned int dec_len;
	unsigned char *result_buf = NULL;
	unsigned char pad_value;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!ciphertext || ciphertext_len == 0 || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	if (ciphertext_len < sizeof(iv))
		return HSM_ERROR_INVALID_PARAM;

	/* Step 1: Extract IV from ciphertext (first 16 bytes) */
	memcpy(iv, ciphertext, sizeof(iv));

	/* Step 2: Get key handle for the KEK at key_index */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 3: Decrypt (skip IV at beginning of ciphertext) */
	dec_data = (unsigned char *)malloc(ciphertext_len - sizeof(iv));
	if (!dec_data)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}

	ret = swxa_sdf.SDF_Decrypt(device->session_handle, hKeyHandle, SGD_SMS4_CBC,
							  iv,
							  (unsigned char *)ciphertext + sizeof(iv),
							  (unsigned int)(ciphertext_len - sizeof(iv)),
							  dec_data, &dec_len);

	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	if (ret != SDR_OK)
	{
		free(dec_data);
		fprintf(stderr, "ERROR: SDF_Decrypt failed for key index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 4: Remove PKCS#7 padding */
	if (dec_len == 0)
	{
		free(dec_data);
		return HSM_ERROR_INVALID_PARAM;
	}

	pad_value = dec_data[dec_len - 1];
	if (pad_value > 0 && pad_value <= 16 && dec_len >= pad_value)
	{
		size_t unpadded_len = dec_len - pad_value;
		result_buf = (unsigned char *)malloc(unpadded_len);
		if (!result_buf)
		{
			free(dec_data);
			return HSM_ERROR_INTERNAL;
		}
		memcpy(result_buf, dec_data, unpadded_len);
		free(dec_data);
		*plaintext = result_buf;
		*plaintext_len = unpadded_len;
		return HSM_SUCCESS;
	}

		/* Invalid padding or data is not padded, return as-is */
		fprintf(stderr, "WARNING: swxa_decrypt: invalid PKCS#7 padding (pad_value=%u), "\
				"returning raw decrypted data\n", pad_value);
		*plaintext = dec_data;
		*plaintext_len = dec_len;
		return HSM_SUCCESS;
}


/*
 * swxa_sign - Sign data using key at given index in SWXA HSM
 *
 * Not supported: SWXA KEK is an SM4 symmetric key.
 * Sign/verify require asymmetric keys (SM2/RSA/ECDSA).
 */
static HsmErrorCode
swxa_sign(HsmDevice *device, unsigned int key_index,
		  const void *data, size_t data_len,
		  void **signature, size_t *signature_len)
{
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	return HSM_ERROR_NOT_SUPPORTED;
}

/*
 * swxa_verify - Verify signature using key at given index in SWXA HSM
 *
 * Not supported: SWXA KEK is an SM4 symmetric key.
 */
static HsmErrorCode
swxa_verify(HsmDevice *device, unsigned int key_index,
			const void *data, size_t data_len,
			const void *signature, size_t signature_len)
{
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	return HSM_ERROR_NOT_SUPPORTED;
}

/*
 * swxa_sm4_encrypt - Encrypt data using SM4-CBC with raw key via HSM
 *
 * Imports the raw SM4 key as a session key via SDF_ImportKey,
 * encrypts with PKCS#7 padding via SDF_Encrypt (SGD_SMS4_CBC),
 * then destroys the session key.
 */
static HsmErrorCode
swxa_sm4_encrypt(HsmDevice *device,
				 const unsigned char *key, size_t key_len,
				 const unsigned char *iv,
				 const unsigned char *plaintext, size_t plain_len,
				 unsigned char *ciphertext, size_t *cipher_len)
{
	int ret;
	void *hKey;
	size_t padded_len;
	unsigned char *padded = NULL;
	unsigned char pad_val;
	unsigned int out_len;
	unsigned char local_key[32];
	unsigned char local_iv[16];

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!key || key_len == 0 || key_len > sizeof(local_key) || !iv || !plaintext || plain_len == 0 ||
		!ciphertext || !cipher_len || *cipher_len == 0)
		return HSM_ERROR_INVALID_PARAM;

	/* Copy key and IV to local buffers to protect against SDF modification */
	memcpy(local_key, key, key_len);
	memcpy(local_iv, iv, sizeof(local_iv));

	/* Import raw SM4 key as session key handle */
	ret = swxa_sdf.SDF_ImportKey(device->session_handle,
								local_key, (unsigned int)key_len, &hKey);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_encrypt: SDF_ImportKey failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* PKCS#7 padding: pad to 16-byte block boundary */
	padded_len = ((plain_len / 16) + 1) * 16;
	pad_val = (unsigned char)(padded_len - plain_len);
	padded = (unsigned char *)malloc(padded_len);
	if (!padded)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKey);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(padded, plaintext, plain_len);
	memset(padded + plain_len, pad_val, padded_len - plain_len);

	/* SM4-CBC encrypt via HSM */
	out_len = (unsigned int)padded_len;
	ret = swxa_sdf.SDF_Encrypt(device->session_handle, hKey, SGD_SMS4_CBC,
							  local_iv,
							  padded, (unsigned int)padded_len,
							  ciphertext, &out_len);

	free(padded);
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKey);
	memset(local_key, 0, sizeof(local_key));

	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_encrypt: SDF_Encrypt failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	*cipher_len = out_len;
	return HSM_SUCCESS;
}

/*
 * swxa_sm4_decrypt - Decrypt data using SM4-CBC with raw key via HSM
 *
 * Imports the raw SM4 key as a session key via SDF_ImportKey,
 * decrypts via SDF_Decrypt (SGD_SMS4_CBC), removes PKCS#7 padding,
 * then destroys the session key.
 */
static HsmErrorCode
swxa_sm4_decrypt(HsmDevice *device,
				 const unsigned char *key, size_t key_len,
				 const unsigned char *iv,
				 const unsigned char *ciphertext, size_t cipher_len,
				 unsigned char *plaintext, size_t *plain_len)
{
	int ret;
	void *hKey;
	unsigned int out_len;
	unsigned char *decrypted = NULL;
	unsigned char pad_val;
	unsigned char local_key[32];
	unsigned char local_iv[16];

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!key || key_len == 0 || key_len > sizeof(local_key) || !iv || !ciphertext || cipher_len == 0 ||
		!plaintext || !plain_len)
		return HSM_ERROR_INVALID_PARAM;

	/* Copy key and IV to local buffers to protect against SDF modification */
	memcpy(local_key, key, key_len);
	memcpy(local_iv, iv, sizeof(local_iv));

	/* Import raw SM4 key as session key handle */
	ret = swxa_sdf.SDF_ImportKey(device->session_handle,
								local_key, (unsigned int)key_len, &hKey);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_decrypt: SDF_ImportKey failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Allocate output buffer */
	decrypted = (unsigned char *)malloc(cipher_len);
	if (!decrypted)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKey);
		return HSM_ERROR_INTERNAL;
	}

	/* SM4-CBC decrypt via HSM */
	out_len = (unsigned int)cipher_len;
	ret = swxa_sdf.SDF_Decrypt(device->session_handle, hKey, SGD_SMS4_CBC,
							  local_iv,
							  (unsigned char *)ciphertext, (unsigned int)cipher_len,
							  decrypted, &out_len);

	swxa_sdf.SDF_DestroyKey(device->session_handle, hKey);
	memset(local_key, 0, sizeof(local_key));

	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_decrypt: SDF_Decrypt failed: 0x%08x\n", ret);
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
		fprintf(stderr, "WARNING: swxa_sm4_decrypt: invalid PKCS#7 padding (pad_value=%u), "\
				"returning raw decrypted data\n", pad_val);
		memcpy(plaintext, decrypted, out_len);
		*plain_len = out_len;
	}
	free(decrypted);
	return HSM_SUCCESS;
}

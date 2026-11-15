/*-------------------------------------------------------------------------
 *
 * test_hsm_adapter.c
 *	  Test program for HSM adapter
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fe-hsm-adapter.h"

/* ========================================================================
 * Mock HSM adapter implementation for testing
 * ======================================================================== */

static HsmErrorCode mock_hsm_init(HsmDevice **device, const char *config_file);
static HsmErrorCode mock_hsm_connect(HsmDevice *device);
static HsmErrorCode mock_hsm_disconnect(HsmDevice *device);
static void mock_hsm_cleanup(HsmDevice *device);
static HsmErrorCode mock_hsm_check_key_exists(HsmDevice *device, unsigned int key_index);
static HsmErrorCode mock_hsm_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info);
static HsmErrorCode mock_hsm_export_public_key(HsmDevice *device, unsigned int key_index,
											   void **key_data, size_t *key_len);
static HsmErrorCode mock_hsm_encrypt(HsmDevice *device, unsigned int key_index,
									 const void *plaintext, size_t plaintext_len,
									 void **ciphertext, size_t *ciphertext_len);
static HsmErrorCode mock_hsm_decrypt(HsmDevice *device, unsigned int key_index,
									 const void *ciphertext, size_t ciphertext_len,
									 void **plaintext, size_t *plaintext_len);
static HsmErrorCode mock_hsm_sign(HsmDevice *device, unsigned int key_index,
								  const void *data, size_t data_len,
								  void **signature, size_t *signature_len);
static HsmErrorCode mock_hsm_verify(HsmDevice *device, unsigned int key_index,
									const void *data, size_t data_len,
									const void *signature, size_t signature_len);
static HsmErrorCode mock_hsm_sm4_encrypt(HsmDevice *device,
										 const unsigned char *key, size_t key_len,
										 const unsigned char *iv,
										 const unsigned char *plaintext, size_t plain_len,
										 unsigned char *ciphertext, size_t *cipher_len);
static HsmErrorCode mock_hsm_sm4_decrypt(HsmDevice *device,
										 const unsigned char *key, size_t key_len,
										 const unsigned char *iv,
										 const unsigned char *ciphertext, size_t cipher_len,
										 unsigned char *plaintext, size_t *plain_len);
static const char *mock_hsm_error_string(HsmErrorCode error_code);
static bool mock_hsm_is_connected(HsmDevice *device);

static HsmAdapterInterface mock_hsm_adapter = {0};

HsmAdapterInterface *
get_mock_hsm_adapter(void)
{
	if (mock_hsm_adapter.init == NULL)
	{
		mock_hsm_adapter.init = mock_hsm_init;
		mock_hsm_adapter.connect = mock_hsm_connect;
		mock_hsm_adapter.disconnect = mock_hsm_disconnect;
		mock_hsm_adapter.cleanup = mock_hsm_cleanup;
		mock_hsm_adapter.check_key_exists = mock_hsm_check_key_exists;
		mock_hsm_adapter.get_key_info = mock_hsm_get_key_info;
		mock_hsm_adapter.export_public_key = mock_hsm_export_public_key;
		mock_hsm_adapter.encrypt = mock_hsm_encrypt;
		mock_hsm_adapter.decrypt = mock_hsm_decrypt;
		mock_hsm_adapter.sign = mock_hsm_sign;
		mock_hsm_adapter.verify = mock_hsm_verify;
		mock_hsm_adapter.error_string = mock_hsm_error_string;
		mock_hsm_adapter.is_connected = mock_hsm_is_connected;
		mock_hsm_adapter.sm4_encrypt = mock_hsm_sm4_encrypt;
		mock_hsm_adapter.sm4_decrypt = mock_hsm_sm4_decrypt;
	}

	return &mock_hsm_adapter;
}

/* Test function prototypes */
static void test_parse_hsm_key_path(void);
static void test_mock_hsm_adapter(void);
static void test_setup_hsm_kek(void);

int main(int argc, char **argv)
{
	printf("=== Testing HSM Adapter ===\n\n");

	/* Test 1: Key path parsing */
	printf("Test 1: Parsing HSM key paths\n");
	test_parse_hsm_key_path();

	/* Test 2: Mock HSM adapter */
	printf("\nTest 2: Mock HSM adapter\n");
	test_mock_hsm_adapter();

	/* Test 3: Setup HSM KEK */
	printf("\nTest 3: Setup HSM KEK\n");
	test_setup_hsm_kek();

	printf("\n=== All tests completed ===\n");
	return 0;
}

/*
 * test_parse_hsm_key_path - Test HSM key path parsing
 */
static void
test_parse_hsm_key_path(void)
{
	struct {
		const char *path;
		bool should_succeed;
		unsigned int expected_index;
	} test_cases[] = {
		{"hsm:id=1", true, 1},
		{"hsm:id=123", true, 123},
		{"hsm:id=65535", true, 65535},
		{"hsm:id=0", false, 0}, /* Invalid: index must be >= 1 */
		{"hsm:id=65536", false, 0}, /* Invalid: index too large */
		{"hsm:id=abc", false, 0}, /* Invalid: not a number */
		{"hsm:id=", false, 0}, /* Invalid: missing index */
		{"hsm:idx=1", false, 0}, /* Invalid: wrong format */
		{"localkms:/path/to/key", false, 0}, /* Wrong prefix */
		{NULL, false, 0}
	};

	int i;
	unsigned int key_index;
	bool result;

	for (i = 0; test_cases[i].path != NULL; i++)
	{

		if (result == test_cases[i].should_succeed)
		{
			if (result)
			{
				if (key_index == test_cases[i].expected_index)
					printf("  PASS: %s -> index %u\n",
				else
					printf("  FAIL: %s -> expected index %u, got %u\n",
						   test_cases[i].path,
			}
			else
			{
				printf("  PASS: %s correctly rejected\n", test_cases[i].path);
			}
		}
		else
		{
			printf("  FAIL: %s -> expected %s, got %s\n",
				   test_cases[i].path,
				   test_cases[i].should_succeed ? "success" : "failure",
				   result ? "success" : "failure");
		}
	}
}

/*
 * test_mock_hsm_adapter - Test mock HSM adapter
 */
static void
test_mock_hsm_adapter(void)
{
	HsmAdapterInterface *adapter;
	HsmDevice *device = NULL;
	HsmErrorCode error;
	HsmKeyInfo *key_info = NULL;
	void *key_data = NULL;
	size_t key_len = 0;
	int i;

	/* Get mock adapter */
	adapter = get_mock_hsm_adapter();
	if (!adapter)
	{
		printf("  FAIL: Cannot get mock adapter\n");
		return;
	}

	/* Test initialization */
	if (error != HSM_SUCCESS)
	{
		printf("  FAIL: Initialization failed: %s\n",
			   adapter->error_string(error));
		return;
	}
	printf("  PASS: Initialization successful\n");

	/* Test connection */
	error = adapter->connect(device);
	if (error != HSM_SUCCESS)
	{
		printf("  FAIL: Connection failed: %s\n",
			   adapter->error_string(error));
		goto cleanup;
	}
	printf("  PASS: Connection successful\n");

	/* Test key existence checks */
	printf("  Testing key existence checks:\n");
	for (i = 1; i <= 10; i++)
	{
		if (i <= 5)
		{
			if (error == HSM_SUCCESS)
				printf("    Key %d: exists (expected)\n", i);
			else
				printf("    Key %d: ERROR - should exist but got: %s\n",
		}
		else
		{
			if (error == HSM_ERROR_KEY_NOT_FOUND)
				printf("    Key %d: not found (expected)\n", i);
			else
				printf("    Key %d: ERROR - should not exist but got: %s\n",
		}
	}

	/* Test get key info */
	if (error == HSM_SUCCESS && key_info)
	{
		printf("  PASS: Got key info for index %u\n", key_info->key_index);
		printf("        Key bits: %u, Key type: %u\n",
	}
	else
	{
		printf("  FAIL: Cannot get key info: %s\n",
			   adapter->error_string(error));
	}

	/* Test export public key */
	if (error == HSM_SUCCESS && key_data && key_len > 0)
	{
		printf("  PASS: Exported public key, length: %zu bytes\n", key_len);
	}
	else
	{
		printf("  FAIL: Cannot export public key: %s\n",
			   adapter->error_string(error));
	}

	/* Test encryption/decryption */
	{
		const char *plaintext = "Hello, HSM!";
		size_t plaintext_len = strlen(plaintext) + 1;
		void *ciphertext = NULL;
		size_t ciphertext_len = 0;
		void *decrypted = NULL;
		size_t decrypted_len = 0;

		error = adapter->encrypt(device, 1, plaintext, plaintext_len,
								&ciphertext, &ciphertext_len);
		if (error == HSM_SUCCESS && ciphertext)
		{
			printf("  PASS: Encryption successful, ciphertext length: %zu\n",
				   ciphertext_len);

			error = adapter->decrypt(device, 1, ciphertext, ciphertext_len,
									&decrypted, &decrypted_len);
			if (error == HSM_SUCCESS && decrypted &&
				memcmp(plaintext, decrypted, plaintext_len) == 0)
			{
				printf("  PASS: Decryption successful, matches original\n");
			}
			else
			{
				printf("  FAIL: Decryption failed: %s\n",
					   adapter->error_string(error));
			}

			if (ciphertext) free(ciphertext);
			if (decrypted) free(decrypted);
		}
		else
		{
			printf("  FAIL: Encryption failed: %s\n",
				   adapter->error_string(error));
		}
	}

cleanup:
	/* Cleanup */
	if (key_info)
	{
		if (key_info->key_name) free(key_info->key_name);
		if (key_info->key_data) free(key_info->key_data);
		free(key_info);
	}
	if (key_data) free(key_data);

	if (device)
	{
		if (adapter->is_connected(device))
			adapter->disconnect(device);
		adapter->cleanup(device);
	}
}

/*
 * test_setup_hsm_kek - Test HSM KEK setup
 */
static void
test_setup_hsm_kek(void)
{
	struct {
		const char *kek_name;
		const char *key_path;
		int expected_result;
	} test_cases[] = {
		{"my_kek1", "hsm:id=1", 0}, /* Should succeed */
		{"my_kek2", "hsm:id=3", 0}, /* Should succeed */
		{"my_kek3", "hsm:id=6", -1}, /* Should fail (key doesn't exist) */
		{"my_kek4", "hsm:id=0", -1}, /* Invalid key path */
		{"my_kek5", "localkms:/path", -1}, /* Wrong format */
		{NULL, NULL, 0}
	};

	int i, result;

	for (i = 0; test_cases[i].kek_name != NULL; i++)
	{
		printf("  Testing: %s with path %s\n",

		result = setup_hsm_kek(test_cases[i].kek_name,
							  test_cases[i].key_path);

		if ((result == 0 && test_cases[i].expected_result == 0) ||
			(result != 0 && test_cases[i].expected_result != 0))
		{
			printf("    PASS: Got expected result (%d)\n", result);
		}
		else
		{
			printf("    FAIL: Expected %d, got %d\n",
		}
	}
}
/* ========================================================================
 * Mock HSM function implementations
 * ======================================================================== */

static HsmErrorCode
mock_hsm_init(HsmDevice **device, const char *config_file)
{
	HsmDevice *dev;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	dev = (HsmDevice *) malloc(sizeof(HsmDevice));
	if (!dev)
		return HSM_ERROR_INTERNAL;

	memset(dev, 0, sizeof(HsmDevice));

	if (config_file)
		dev->config_file = strdup(config_file);

	dev->connected = false;
	*device = dev;

	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_connect(HsmDevice *device)
{
	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	device->connected = true;
	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_disconnect(HsmDevice *device)
{
	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	device->connected = false;
	return HSM_SUCCESS;
}

static void
mock_hsm_cleanup(HsmDevice *device)
{
	if (!device)
		return;

	if (device->config_file)
		free(device->config_file);

	free(device);
}

static HsmErrorCode
mock_hsm_check_key_exists(HsmDevice *device, unsigned int key_index)
{
	if (!device || !device->connected)
		return HSM_ERROR_CONNECT_FAILED;

	if (key_index >= 1 && key_index <= 5)
		return HSM_SUCCESS;
	else
		return HSM_ERROR_KEY_NOT_FOUND;
}

static HsmErrorCode
mock_hsm_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info)
{
	HsmKeyInfo *info;

	if (!device || !device->connected || !key_info)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	info = (HsmKeyInfo *) malloc(sizeof(HsmKeyInfo));
	if (!info)
		return HSM_ERROR_INTERNAL;

	memset(info, 0, sizeof(HsmKeyInfo));
	info->key_index = key_index;
	info->key_bits = 2048;
	info->key_type = 1;

	*key_info = info;
	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_export_public_key(HsmDevice *device, unsigned int key_index,
						   void **key_data, size_t *key_len)
{
	static unsigned char dummy_key[] = {
		0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
		0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
		0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00
	};

	if (!device || !device->connected || !key_data || !key_len)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	*key_data = malloc(sizeof(dummy_key));
	if (!*key_data)
		return HSM_ERROR_INTERNAL;

	memcpy(*key_data, dummy_key, sizeof(dummy_key));
	*key_len = sizeof(dummy_key);

	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_encrypt(HsmDevice *device, unsigned int key_index,
				 const void *plaintext, size_t plaintext_len,
				 void **ciphertext, size_t *ciphertext_len)
{
	unsigned char iv[16];
	size_t total_len;
	int i;

	if (!device || !device->connected || !plaintext || !ciphertext || !ciphertext_len)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	for (i = 0; i < sizeof(iv); i++)
		iv[i] = rand() % 256;

	total_len = sizeof(iv) + plaintext_len;
	*ciphertext = malloc(total_len);
	if (!*ciphertext)
		return HSM_ERROR_INTERNAL;

	memcpy(*ciphertext, iv, sizeof(iv));
	memcpy((unsigned char *)*ciphertext + sizeof(iv), plaintext, plaintext_len);

	*ciphertext_len = total_len;

	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_decrypt(HsmDevice *device, unsigned int key_index,
				 const void *ciphertext, size_t ciphertext_len,
				 void **plaintext, size_t *plaintext_len)
{
	unsigned char iv[16];
	size_t plain_len;

	if (!device || !device->connected || !ciphertext || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	if (ciphertext_len < 16)
		return HSM_ERROR_INVALID_PARAM;

	memcpy(iv, ciphertext, sizeof(iv));

	plain_len = ciphertext_len - sizeof(iv);

	*plaintext = malloc(plain_len);
	if (!*plaintext)
		return HSM_ERROR_INTERNAL;

	memcpy(*plaintext, (const unsigned char *)ciphertext + sizeof(iv), plain_len);
	*plaintext_len = plain_len;

	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_sign(HsmDevice *device, unsigned int key_index,
			  const void *data, size_t data_len,
			  void **signature, size_t *signature_len)
{
	static unsigned char dummy_signature[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
	};

	if (!device || !device->connected || !data || !signature || !signature_len)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	*signature = malloc(sizeof(dummy_signature));
	if (!*signature)
		return HSM_ERROR_INTERNAL;

	memcpy(*signature, dummy_signature, sizeof(dummy_signature));
	*signature_len = sizeof(dummy_signature);

	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_verify(HsmDevice *device, unsigned int key_index,
				const void *data, size_t data_len,
				const void *signature, size_t signature_len)
{
	if (!device || !device->connected || !data || !signature)
		return HSM_ERROR_INVALID_PARAM;

	if (mock_hsm_check_key_exists(device, key_index) != HSM_SUCCESS)
		return HSM_ERROR_KEY_NOT_FOUND;

	return HSM_SUCCESS;
}

static const char *
mock_hsm_error_string(HsmErrorCode error_code)
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
mock_hsm_is_connected(HsmDevice *device)
{
	return device && device->connected;
}

static HsmErrorCode
mock_hsm_sm4_encrypt(HsmDevice *device,
					 const unsigned char *key, size_t key_len,
					 const unsigned char *iv,
					 const unsigned char *plaintext, size_t plain_len,
					 unsigned char *ciphertext, size_t *cipher_len)
{
	(void)device;
	(void)key;
	(void)key_len;
	(void)iv;

	if (!plaintext || plain_len == 0 || !ciphertext || !cipher_len)
		return HSM_ERROR_INVALID_PARAM;

	memcpy(ciphertext, plaintext, plain_len);
	*cipher_len = plain_len;
	return HSM_SUCCESS;
}

static HsmErrorCode
mock_hsm_sm4_decrypt(HsmDevice *device,
					 const unsigned char *key, size_t key_len,
					 const unsigned char *iv,
					 const unsigned char *ciphertext, size_t cipher_len,
					 unsigned char *plaintext, size_t *plain_len)
{
	(void)device;
	(void)key;
	(void)key_len;
	(void)iv;

	if (!ciphertext || cipher_len == 0 || !plaintext || !plain_len)
		return HSM_ERROR_INVALID_PARAM;

	memcpy(plaintext, ciphertext, cipher_len);
	*plain_len = cipher_len;
	return HSM_SUCCESS;
}

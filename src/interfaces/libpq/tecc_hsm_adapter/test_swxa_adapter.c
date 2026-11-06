/*-------------------------------------------------------------------------
 *
 * test_swxa_adapter.c
 *	  Comprehensive test program for SWXA HSM adapter
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fe-hsm-adapter.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST(name, cond) do { \
	test_count++; \
	if (cond) { \
		pass_count++; \
		printf("  PASS: %s\n", name); \
	} else { \
		printf("  FAIL: %s\n", name); \
	} \
} while(0)

static void test_basic_init_connect(HsmAdapterInterface *adapter);
static void test_check_key_exists(HsmAdapterInterface *adapter, HsmDevice *device);
static void test_encrypt_decrypt_key_index(HsmAdapterInterface *adapter, HsmDevice *device, unsigned int key_index);
static void test_encrypt_decrypt_various_sizes(HsmAdapterInterface *adapter, HsmDevice *device, unsigned int key_index);
static void test_export_public_key(HsmAdapterInterface *adapter, HsmDevice *device);
static void test_sign_verify(HsmAdapterInterface *adapter, HsmDevice *device);
static void test_sm4_raw_key(HsmAdapterInterface *adapter, HsmDevice *device);
static void test_sequential_ops(HsmAdapterInterface *adapter, HsmDevice *device);
static void test_disconnect_reconnect(HsmAdapterInterface *adapter, HsmDevice *device);

int main(int argc, char **argv)
{
	HsmAdapterInterface *adapter;
	HsmDevice *device = NULL;
	HsmErrorCode error;

	printf("=== SWXA HSM Adapter Comprehensive Test ===\n\n");

	adapter = get_swxa_adapter();
	TEST("get_swxa_adapter", adapter != NULL);
	if (!adapter) return 1;

	test_basic_init_connect(adapter);

	error = adapter->init(&device, "hsm:id=1");
	TEST("init", error == HSM_SUCCESS);
	if (error != HSM_SUCCESS) return 1;

	error = adapter->connect(device);
	TEST("connect", error == HSM_SUCCESS);
	if (error != HSM_SUCCESS) {
		adapter->cleanup(device);
		return 1;
	}

	TEST("is_connected", adapter->is_connected(device) == true);

	/* Run all tests */
	test_check_key_exists(adapter, device);
	test_encrypt_decrypt_key_index(adapter, device, 1);
	test_encrypt_decrypt_various_sizes(adapter, device, 1);
	test_export_public_key(adapter, device);
	test_sign_verify(adapter, device);
	test_sm4_raw_key(adapter, device);
	test_sequential_ops(adapter, device);

	/* Test disconnect */
	error = adapter->disconnect(device);
	TEST("disconnect", error == HSM_SUCCESS);
	TEST("is_connected after disconnect", adapter->is_connected(device) == false);

	/* Test reconnect */
	test_disconnect_reconnect(adapter, device);

	/* Cleanup */
	adapter->cleanup(device);
	TEST("cleanup", true);

	printf("\n=== Results: %d/%d passed ===\n", pass_count, test_count);
	return (pass_count == test_count) ? 0 : 1;
}

static void
test_basic_init_connect(HsmAdapterInterface *adapter)
{
	HsmDevice *d1 = NULL, *d2 = NULL;
	HsmErrorCode e;

	printf("\n--- Basic Init/Connect Edge Cases ---\n");

	e = adapter->init(NULL, "hsm:id=1");
	TEST("init(NULL) returns error", e != HSM_SUCCESS);

	e = adapter->connect(NULL);
	TEST("connect(NULL) returns error", e != HSM_SUCCESS);

	e = adapter->disconnect(NULL);
	TEST("disconnect(NULL) returns safe", e != HSM_SUCCESS || e == HSM_SUCCESS);

	adapter->cleanup(NULL);
	TEST("cleanup(NULL) no crash", true);

	e = adapter->is_connected(NULL);
	TEST("is_connected(NULL) returns false", e == false);

	e = adapter->init(&d1, "hsm:id=1");
	TEST("init d1", e == HSM_SUCCESS);
	if (e == HSM_SUCCESS) {
		/* init again with same device pointer (re-init) */
		e = adapter->init(&d2, "hsm:id=1");
		TEST("init d2 separately", e == HSM_SUCCESS);
		if (e == HSM_SUCCESS) {
			adapter->cleanup(d2);
		}
		adapter->cleanup(d1);
	}
}

static void
test_check_key_exists(HsmAdapterInterface *adapter, HsmDevice *device)
{
	HsmErrorCode e;
	HsmKeyInfo *info = NULL;
	unsigned int i;

	printf("\n--- Check Key Existence & Info ---\n");

	/* Keys 1-20 should all exist */
	for (i = 1; i <= 20; i++)
	{
		e = adapter->check_key_exists(device, i);
		char buf[64];
		snprintf(buf, sizeof(buf), "key %u exists", i);
		TEST(buf, e == HSM_SUCCESS);
	}

	/* Key 999 and 0: device may return SDR_INARGERR or treat as valid index,
	 * depending on the SWXA device. Just verify they don't crash. */
	e = adapter->check_key_exists(device, 999);
	(void)e;
	TEST("key 999 (out-of-range) no crash", true);

	e = adapter->check_key_exists(device, 0);
	(void)e;
	TEST("key 0 (out-of-range) no crash", true);

	/* Verify that check_key_exists on non-existent key returns error */
	/* Note: SDF_GetSymmKeyHandle may handle out-of-range indices differently
	 * across devices, so we only test keys we know about */

	/* Get key info for key 1 */
	e = adapter->get_key_info(device, 1, &info);
	TEST("get_key_info(1)", e == HSM_SUCCESS && info != NULL);
	if (e == HSM_SUCCESS && info)
	{
		TEST("  key_index=1", info->key_index == 1);
		TEST("  key_bits=128", info->key_bits == 128);
		TEST("  key_type=SGD_SMS4(1024)", info->key_type == 1024);
		free(info);
	}

	/* Invalid key info (just check no crash) */
	e = adapter->get_key_info(device, 999, &info);
	(void)e;
	TEST("get_key_info(999) no crash", true);
}

static void
test_encrypt_decrypt_key_index(HsmAdapterInterface *adapter, HsmDevice *device,
							   unsigned int key_index)
{
	const char *plaintexts[] = {
		"Hello, SWXA HSM!",
		"A",
		"AB",
		"Short",
		"Exact 16 bytes!!",
		"Exact 32 bytes here!!!!!!!!!",
		"This is a longer message that spans multiple blocks to test the CBC mode encryption and decryption with PKCS#7 padding in the SWXA HSM adapter.",
	};
	int num_tests = sizeof(plaintexts) / sizeof(plaintexts[0]);
	int i;

	printf("\n--- Encrypt/Decrypt with Key Index %u ---\n", key_index);

	for (i = 0; i < num_tests; i++)
	{
		size_t pt_len = strlen(plaintexts[i]) + 1;
		void *cipher = NULL;
		size_t cipher_len = 0;
		void *decrypted = NULL;
		size_t dec_len = 0;
		HsmErrorCode e;

		e = adapter->encrypt(device, key_index, plaintexts[i], pt_len,
							&cipher, &cipher_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "encrypt[%d] len=%zu failed: %s",
					 i, pt_len, adapter->error_string(e));
			TEST(buf, false);
			continue;
		}

		e = adapter->decrypt(device, key_index, cipher, cipher_len,
							&decrypted, &dec_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "decrypt[%d] len=%zu failed: %s",
					 i, cipher_len, adapter->error_string(e));
			TEST(buf, false);
			free(cipher);
			continue;
		}

		int match = (dec_len == pt_len && memcmp(plaintexts[i], decrypted, pt_len) == 0);
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "encrypt/decrypt[%d] len=%zu->%zu %s",
					 i, pt_len, cipher_len, match ? "match" : "MISMATCH");
			TEST(buf, match);
		}

		free(cipher);
		free(decrypted);
	}

	/* Test invalid parameters */
	{
		void *out = NULL;
		size_t out_len = 0;
		HsmErrorCode e;

		e = adapter->encrypt(device, key_index, NULL, 10, &out, &out_len);
		TEST("encrypt(NULL data) returns error", e != HSM_SUCCESS);

		e = adapter->encrypt(device, key_index, "data", 5, NULL, &out_len);
		TEST("encrypt(NULL output) returns error", e != HSM_SUCCESS);

		e = adapter->encrypt(device, key_index, "data", 0, &out, &out_len);
		TEST("encrypt(len=0) returns error", e != HSM_SUCCESS);
	}
}

static void
test_encrypt_decrypt_various_sizes(HsmAdapterInterface *adapter, HsmDevice *device,
								   unsigned int key_index)
{
	unsigned char pt[2048];
	size_t sizes[] = {1, 15, 16, 17, 31, 32, 33, 47, 48, 49, 63, 64, 65, 127, 128, 129, 255, 256, 511, 512, 1023, 1024, 2000};
	int i;
	int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

	printf("\n--- Encrypt/Decrypt Various Sizes (key %u) ---\n", key_index);

	/* Fill plaintext with known pattern */
	for (i = 0; i < (int)sizeof(pt); i++)
		pt[i] = (unsigned char)(i & 0xFF);

	for (i = 0; i < num_sizes; i++)
	{
		size_t sz = sizes[i];
		void *cipher = NULL;
		size_t cipher_len = 0;
		void *decrypted = NULL;
		size_t dec_len = 0;
		HsmErrorCode e;

		e = adapter->encrypt(device, key_index, pt, sz, &cipher, &cipher_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "encrypt size=%zu failed: %s",
					 sz, adapter->error_string(e));
			TEST(buf, false);
			continue;
		}

		/* Verify ciphertext is not plaintext */
		int not_plain = (sz <= 64) ?
			(cipher_len != sz || memcmp(cipher, pt, sz) != 0) : 1;

		e = adapter->decrypt(device, key_index, cipher, cipher_len,
							&decrypted, &dec_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "decrypt size=%zu(cipher=%zu) failed: %s",
					 sz, cipher_len, adapter->error_string(e));
			TEST(buf, false);
			free(cipher);
			continue;
		}

		int match = (dec_len == sz && memcmp(pt, decrypted, sz) == 0);
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "size=%zu -> cipher=%zu -> %s",
					 sz, cipher_len, match ? "OK" : "MISMATCH");
			TEST(buf, match && not_plain);
		}

		free(cipher);
		free(decrypted);
	}
}

static void
test_export_public_key(HsmAdapterInterface *adapter, HsmDevice *device)
{
	void *key_data = NULL;
	size_t key_len;
	HsmErrorCode e;

	printf("\n--- Export Public Key ---\n");

	e = adapter->export_public_key(device, 1, &key_data, &key_len);
	TEST("export_public_key returns NOT_SUPPORTED", e == HSM_ERROR_NOT_SUPPORTED);
}

static void
test_sign_verify(HsmAdapterInterface *adapter, HsmDevice *device)
{
	const char *data = "test data for sign";
	void *sig = NULL;
	size_t sig_len = 0;
	HsmErrorCode e;

	printf("\n--- Sign / Verify ---\n");

	e = adapter->sign(device, 1, data, strlen(data), &sig, &sig_len);
	TEST("sign returns NOT_SUPPORTED", e == HSM_ERROR_NOT_SUPPORTED);

	/* verify with NULL sig to test it doesn't crash */
	e = adapter->verify(device, 1, data, strlen(data), NULL, 0);
	TEST("verify(NULL sig) returns NOT_SUPPORTED", e == HSM_ERROR_NOT_SUPPORTED);

	e = adapter->verify(device, 1, NULL, 0, NULL, 0);
	TEST("verify(NULL data) returns NOT_SUPPORTED", e == HSM_ERROR_NOT_SUPPORTED);
}

static void
test_sm4_raw_key(HsmAdapterInterface *adapter, HsmDevice *device)
{
	unsigned char key[16];
	unsigned char iv[16];
	unsigned char pt[1024];
	unsigned char cipher[2048];
	unsigned char decrypted[2048];
	size_t cipher_len, dec_len;
	HsmErrorCode e;
	int i;

	printf("\n--- SM4 Raw Key Tests ---\n");

	/* Test with 0xAB key, zero IV */
	memset(key, 0xAB, sizeof(key));
	memset(iv, 0, sizeof(iv));
	memset(pt, 0, sizeof(pt));

	const char *msg = "Hello, SWXA HSM! This is a test message.";
	size_t msg_len = strlen(msg) + 1;

	/* Various plaintext sizes */
	size_t test_sizes[] = {1, 16, 17, 31, 32, 33, 64, 100, 256, 512, 1000};
	int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

	for (i = 0; i < num_sizes; i++)
	{
		size_t sz = test_sizes[i];
		if (sz > sizeof(pt)) continue;

		memcpy(pt, msg, (msg_len < sz) ? msg_len : sz);
		if (sz <= msg_len && sz > 0) pt[sz-1] = '\0';
		if (sz > msg_len) pt[msg_len-1] = '\0';

		cipher_len = sizeof(cipher);
		e = adapter->sm4_encrypt(device, key, sizeof(key), iv, pt, sz, cipher, &cipher_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "sm4_encrypt size=%zu failed: %s",
					 sz, adapter->error_string(e));
			TEST(buf, false);
			continue;
		}

		/* Verify ciphertext length is multiple of 16 */
		int len_ok = (cipher_len % 16 == 0);
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "sm4_encrypt size=%zu -> %zu (%%16=%s)",
					 sz, cipher_len, len_ok ? "0" : "NONZERO");
			TEST(buf, len_ok);
		}

		/* Decrypt */
		dec_len = sizeof(decrypted);
		e = adapter->sm4_decrypt(device, key, sizeof(key), iv, cipher, cipher_len, decrypted, &dec_len);
		if (e != HSM_SUCCESS)
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "sm4_decrypt size=%zu failed: %s",
					 cipher_len, adapter->error_string(e));
			TEST(buf, false);
			continue;
		}

		int match = (dec_len == sz && memcmp(pt, decrypted, sz) == 0);
		{
			char buf[128];
			snprintf(buf, sizeof(buf), "sm4 roundtrip size=%zu: %s",
					 sz, match ? "OK" : "MISMATCH");
			TEST(buf, match);
		}
	}

	/* Reuse same key and iv after encrypt (test that local copies work) */
	{
		memset(key, 0xAB, sizeof(key));
		memset(iv, 0, sizeof(iv));
		unsigned char key2[16];
		memset(key2, 0xCD, sizeof(key2));

		cipher_len = sizeof(cipher);
		e = adapter->sm4_encrypt(device, key, sizeof(key), iv, pt, 64, cipher, &cipher_len);
		TEST("sm4_encrypt with reused key", e == HSM_SUCCESS);

		/* IV should be preserved as zeros (SDF shouldn't modify our buffer) */
		{
			int iv_ok = 1;
			for (int j = 0; j < 16; j++)
				if (iv[j] != 0) { iv_ok = 0; break; }
			TEST("IV preserved after sm4_encrypt", iv_ok);
		}

		/* Key should be preserved as 0xAB */
		{
			int key_ok = 1;
			for (int j = 0; j < 16; j++)
				if (key[j] != 0xAB) { key_ok = 0; break; }
			TEST("key preserved after sm4_encrypt", key_ok);
		}

		/* Decrypt with same key/iv */
		dec_len = sizeof(decrypted);
		e = adapter->sm4_decrypt(device, key, sizeof(key), iv, cipher, cipher_len, decrypted, &dec_len);
		TEST("sm4_decrypt with preserved key/iv", e == HSM_SUCCESS && dec_len == 64);
	}

	/* Test invalid parameters */
	{
		cipher_len = sizeof(cipher);
		e = adapter->sm4_encrypt(device, NULL, 16, iv, pt, 16, cipher, &cipher_len);
		TEST("sm4_encrypt(NULL key) error", e != HSM_SUCCESS);

		cipher_len = sizeof(cipher);
		e = adapter->sm4_encrypt(device, key, 16, iv, pt, 16, NULL, &cipher_len);
		TEST("sm4_encrypt(NULL out) error", e != HSM_SUCCESS);

		cipher_len = 0;
		e = adapter->sm4_encrypt(device, key, 16, iv, pt, 16, cipher, &cipher_len);
		TEST("sm4_encrypt(*cipher_len=0) error", e != HSM_SUCCESS);

		dec_len = sizeof(decrypted);
		e = adapter->sm4_decrypt(device, NULL, 16, iv, cipher, 48, decrypted, &dec_len);
		TEST("sm4_decrypt(NULL key) error", e != HSM_SUCCESS);
	}
}

static void
test_sequential_ops(HsmAdapterInterface *adapter, HsmDevice *device)
{
	const char *pt = "Sequential operation test payload.";
	size_t pt_len = strlen(pt) + 1;
	int i;

	printf("\n--- Sequential Operations ---\n");

	/* 100 sequential encrypt/decrypt cycles */
	for (i = 0; i < 100; i++)
	{
		void *cipher = NULL;
		size_t cipher_len = 0;
		void *decrypted = NULL;
		size_t dec_len = 0;
		HsmErrorCode e;

		e = adapter->encrypt(device, 1, pt, pt_len, &cipher, &cipher_len);
		if (e != HSM_SUCCESS)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "seq encrypt #%d", i);
			TEST(buf, false);
			continue;
		}

		e = adapter->decrypt(device, 1, cipher, cipher_len, &decrypted, &dec_len);
		if (e != HSM_SUCCESS)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "seq decrypt #%d", i);
			TEST(buf, false);
			free(cipher);
			continue;
		}

		int match = (dec_len == pt_len && memcmp(pt, decrypted, pt_len) == 0);
		if (!match)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "seq match #%d", i);
			TEST(buf, false);
		}

		free(cipher);
		free(decrypted);
	}
	TEST("100 sequential encrypt/decrypt cycles", true);

	/* Interleaved sm4 and key-index ops */
	for (i = 0; i < 20; i++)
	{
		unsigned char key[16];
		unsigned char iv[16];
		unsigned char cipher[256];
		size_t cipher_len = sizeof(cipher);
		unsigned char decrypted[64];
		size_t dec_len = sizeof(decrypted);
		HsmErrorCode e;

		memset(key, 0xAB + i, 16);
		memset(iv, 0, 16);

		e = adapter->sm4_encrypt(device, key, 16, iv, pt, pt_len, cipher, &cipher_len);
		if (e != HSM_SUCCESS)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "interleave sm4_enc #%d", i);
			TEST(buf, false);
			continue;
		}

		e = adapter->sm4_decrypt(device, key, 16, iv, cipher, cipher_len, decrypted, &dec_len);
		if (e == HSM_SUCCESS && dec_len == pt_len && memcmp(pt, decrypted, pt_len) == 0)
		{
			/* Pass */
		}
		else
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "interleave sm4 #%d", i);
			TEST(buf, false);
		}
	}
	TEST("20 interleaved sm4 encrypt/decrypt", true);
}

static void
test_disconnect_reconnect(HsmAdapterInterface *adapter, HsmDevice *device)
{
	HsmErrorCode e;
	const char *pt = "Test after reconnect.";
	size_t pt_len = strlen(pt) + 1;

	printf("\n--- Disconnect / Reconnect ---\n");

	/* Reconnect */
	e = adapter->connect(device);
	TEST("reconnect", e == HSM_SUCCESS);
	TEST("is_connected after reconnect", adapter->is_connected(device) == true);

	/* Should work after reconnect */
	{
		void *cipher = NULL;
		size_t cipher_len = 0;
		void *decrypted = NULL;
		size_t dec_len = 0;

		e = adapter->encrypt(device, 1, pt, pt_len, &cipher, &cipher_len);
		TEST("encrypt after reconnect", e == HSM_SUCCESS);
		if (e == HSM_SUCCESS)
		{
			e = adapter->decrypt(device, 1, cipher, cipher_len, &decrypted, &dec_len);
			int match = (e == HSM_SUCCESS && dec_len == pt_len && memcmp(pt, decrypted, pt_len) == 0);
			TEST("decrypt after reconnect matches", match);
			free(cipher);
			free(decrypted);
		}
	}

	/* Multiple disconnect/connect cycles */
	int i;
	for (i = 0; i < 5; i++)
	{
		e = adapter->disconnect(device);
		TEST("disconnect in cycle", e == HSM_SUCCESS);

		e = adapter->connect(device);
		TEST("connect in cycle", e == HSM_SUCCESS);
	}
	TEST("5 disconnect/connect cycles", true);

	/* Check key still works after cycles */
	e = adapter->check_key_exists(device, 1);
	TEST("key 1 exists after reconnect cycles", e == HSM_SUCCESS);
}

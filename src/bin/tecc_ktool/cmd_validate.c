/*
 * cmd_validate.c — tecc_ktool validate command
 *
 * Performs format + HMAC validation on KEK files.
 *
 * src/bin/tecc_ktool/cmd_validate.c
 */

#include "tecc_ktool_common.h"
#include "kek_file.h"

int
cmd_validate(int argc, char *argv[])
{
	const char *key_path = NULL;
	unsigned char mek[MEK_BIN_LEN];
	FILE	   *fp;
	struct stat st;
	int			hmac_ok = 0;

	if (argc < 2 || strcmp(argv[1], "--key-path") != 0 || argc < 3)
	{
		fprintf(stderr, "Usage: tecc_ktool validate --key-path <path>\n");
		return CMD_USAGE_ERROR;
	}
	key_path = argv[2];

	/* Basic format check */
	if (stat(key_path, &st) != 0)
	{
		fprintf(stderr, "FAIL: cannot access %s: %s\n", key_path, strerror(errno));
		return CMD_ERROR;
	}

	/*
	 * A .pub file is a plain SM2 public key (no TECCKEK envelope / HMAC).
	 * Validate it separately: fixed 65-byte uncompressed point + on-curve.
	 */
	{
		const char *ext = strrchr(key_path, '.');

		if (ext != NULL && strcmp(ext, ".pub") == 0)
		{
			char		reason[256];
			int			ok = tecc_sm2_pub_check(key_path, reason, sizeof(reason));

			if (ok != 0)
			{
				fprintf(stderr, "FAIL: %s is not a valid SM2 public key: %s\n",
						key_path, reason[0] ? reason : "unknown error");
				return CMD_ERROR;
			}
			printf("OK:   %s\n", key_path);
			printf("  Format: SM2 public key (plain, no HMAC)\n");
			printf("  Size:   %zu bytes\n", (size_t) st.st_size);
			printf("  Curve:  sm2p256v1 on-curve ✅\n");
			return CMD_OK;
		}
	}

	if (st.st_size < KEK_FILE_MIN_SIZE)
	{
		fprintf(stderr, "FAIL: %s too small (%zu < %d)\n",
				key_path, (size_t)st.st_size, KEK_FILE_MIN_SIZE);
		return CMD_ERROR;
	}

	/* Verify magic */
	{
		unsigned char magic[8] = {0};
		fp = fopen(key_path, "rb");
		if (!fp || fread(magic, 1, KEK_FILE_MAGIC_LEN, fp) != KEK_FILE_MAGIC_LEN)
		{
			if (fp) fclose(fp);
			fprintf(stderr, "FAIL: cannot read %s\n", key_path);
			return CMD_ERROR;
		}
		fclose(fp);
		magic[KEK_FILE_MAGIC_LEN] = '\0';
		if (strcmp((char *)magic, KEK_FILE_MAGIC) != 0)
		{
			fprintf(stderr, "FAIL: bad magic (got '%s')\n", (char *)magic);
			return CMD_ERROR;
		}
	}

	/* Verify HMAC with the fixed MEK */
	get_mek(mek);
	hmac_ok = (tecc_kek_verify_hmac(mek, MEK_BIN_LEN, key_path) == 0);

	printf("OK:   %s\n", key_path);
	printf("  Format: TECCKEK v1\n");
	printf("  Size:   %zu bytes\n", (size_t)st.st_size);
	printf("  HMAC:   %s\n", hmac_ok ? "verified ✅" : "unverified (HMAC mismatch — key file corrupted or tampered)");

	return CMD_OK;
}

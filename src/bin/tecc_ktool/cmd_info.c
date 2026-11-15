/*
 * cmd_info.c — tecc_ktool info command
 *
 * Shows KEK file metadata per requirement (key_algo / 算法 / 版本 / 创建时间),
 * output labels are English: key_algo, Algorithm, Created, Format.
 *   key_algo   — sm4/sm2, inferred from file extension (.key/.priv)
 *   Algorithm  — file protection algorithm, decoded from header algo_id
 *                 (encrypt_algo=SM4-CBC + kdf_algo=PBKDF2-HMAC-SHA256)
 *   Created    — filesystem mtime (创建时间)
 *   Format     — header version byte (版本, TECCKEK v1)
 * File layout (see kek_file.c):
 *   magic(7) + ver(1) + reserved(8) + algo_id(4) + salt(16) + iv(16)
 *   + SM4-CBC ciphertext + HMAC-SHA256(32)
 *
 * Integrity (HMAC) is checked by the dedicated `validate` command, not here.
 *
 * src/bin/tecc_ktool/cmd_info.c
 */

#include "tecc_ktool_common.h"

/* algo_id size — same layout as kek_file.c (not exported from its header) */
#define KEK_FILE_ALGO_ID_SIZE 4

int
cmd_info(int argc, char *argv[])
{
	const char *key_path = NULL;
	const char *ext;
	struct stat st;
	FILE	   *fp;
	unsigned char header[KEK_FILE_HEADER_SIZE + KEK_FILE_ALGO_ID_SIZE];
	unsigned char magic[8] = {0};
	unsigned int encrypt_algo;
	unsigned int kdf_algo;
	char		algo_str[96];
	char		timebuf[32];
	struct tm	tm_res;

	/* Parse --key-path */
	if (argc < 2 || strcmp(argv[1], "--key-path") != 0 || argc < 3)
	{
		fprintf(stderr, "Usage: tecc_ktool info --key-path <path>\n");
		return CMD_USAGE_ERROR;
	}
	key_path = argv[2];

	/* stat the file */
	if (stat(key_path, &st) != 0)
	{
		fprintf(stderr, "ERROR: cannot access %s: %s\n",
				key_path, strerror(errno));
		return CMD_ERROR;
	}

	/*
	 * A .pub file is a plain SM2 public key, not a TECCKEK envelope — there
	 * is no header/algo_id/HMAC to decode.  Show its metadata and return.
	 */
	{
		const char *ext = strrchr(key_path, '.');

		if (ext != NULL && strcmp(ext, ".pub") == 0)
		{
			char		timebuf[32];
			struct tm	tm_res;

			strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
					 localtime_r(&st.st_mtime, &tm_res));
			printf("key_algo:  sm2 (public)\n");
			printf("Algorithm: n/a — public key, plain text (no HMAC)\n");
			printf("Created:   %s\n", timebuf);
			printf("Format:    SM2 public key (uncompressed point)\n");
			printf("Size:      %lld bytes\n", (long long) st.st_size);
			return CMD_OK;
		}
	}

	/* Check file size */
	if (st.st_size < (off_t)KEK_FILE_MIN_SIZE)
	{
		fprintf(stderr, "ERROR: %s is too small (%zu bytes, expected >= %d)\n",
				key_path, (size_t)st.st_size, KEK_FILE_MIN_SIZE);
		return CMD_ERROR;
	}

	/* Read header (16 bytes) + algo_id (4 bytes) */
	fp = fopen(key_path, "rb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s: %s\n",
				key_path, strerror(errno));
		return CMD_ERROR;
	}

	if (fread(header, 1, sizeof(header), fp) != sizeof(header))
	{
		fprintf(stderr, "ERROR: cannot read header from %s\n", key_path);
		fclose(fp);
		return CMD_ERROR;
	}
	fclose(fp);

	/* Verify magic */
	memcpy(magic, header, KEK_FILE_MAGIC_LEN);
	magic[KEK_FILE_MAGIC_LEN] = '\0';

	if (strcmp((char *)magic, KEK_FILE_MAGIC) != 0)
	{
		fprintf(stderr, "ERROR: %s is not a valid KEK file (bad magic: %s)\n",
				key_path, (char *)magic);
		return CMD_ERROR;
	}

	/* Decode algo_id: encrypt_algo(2) + kdf_algo(2), big-endian */
	encrypt_algo = ((unsigned int) header[KEK_FILE_HEADER_SIZE] << 8) |
		header[KEK_FILE_HEADER_SIZE + 1];
	kdf_algo = ((unsigned int) header[KEK_FILE_HEADER_SIZE + 2] << 8) |
		header[KEK_FILE_HEADER_SIZE + 3];

	/* 算法 = file protection algorithm, from algo_id */
	if (encrypt_algo == 0x0001 && kdf_algo == 0x0001)
		snprintf(algo_str, sizeof(algo_str),
				 "SM4-CBC + PBKDF2-HMAC-SHA256");
	else
		snprintf(algo_str, sizeof(algo_str),
				 "unknown (encrypt=0x%04x, kdf=0x%04x)",
				 encrypt_algo, kdf_algo);

	/* Determine key type from extension */
	ext = strrchr(key_path, '.');
	if (ext && strcmp(ext, ".key") == 0)
	{
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
				 localtime_r(&st.st_mtime, &tm_res));
		printf("key_algo:  sm4\n");
		printf("Algorithm: %s\n", algo_str);
		printf("Created:   %s\n", timebuf);
		printf("Format:    %s\n", (header[7] == 1) ? "TECCKEK v1" : "Unknown");
	}
	else if (ext && strcmp(ext, ".priv") == 0)
	{
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
				 localtime_r(&st.st_mtime, &tm_res));
		printf("key_algo:  sm2\n");
		printf("Algorithm: %s\n", algo_str);
		printf("Created:   %s\n", timebuf);
		printf("Format:    %s\n", (header[7] == 1) ? "TECCKEK v1" : "Unknown");
	}
	else
	{
		fprintf(stderr, "WARNING: %s has unknown extension (expected .key or .priv)\n", key_path);
	}

	return CMD_OK;
}

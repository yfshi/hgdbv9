/*
 * cmd_delete.c — tecc_ktool delete command
 *
 * Destroys the KEK file at the given path: overwrites the file data with
 * 4 passes (zeros → ones → random → zeros), fsync-ing after each pass so
 * the overwrite reaches disk, then unlinks the file.  This prevents the key
 * material from being recovered by ordinary file-recovery tools.
 *
 * Only the file named by --key-path is removed.  An SM2 key pair is split
 * across .priv/.pub — deleting one does NOT touch the other, so the caller
 * keeps explicit control over which half is destroyed.
 *
 * src/bin/tecc_ktool/cmd_delete.c
 */

#include "tecc_ktool_common.h"

#ifdef _WIN32
#include <io.h>
#endif

/* security_generate_random_bytes from src/common/tecc_key/security.c */
extern int security_generate_random_bytes(unsigned char *buffer, size_t length);

/*
 * Overwrite the file with 4 passes (0x00, 0xFF, random, 0x00) and fsync
 * after each pass, then unlink it.  Returns 0 on success, -1 on error.
 */
static int
destroy_file(const char *key_path)
{
	struct stat st;
	FILE	   *fp;
	unsigned char *buf;
	size_t		size;
	int			pass;
	int			ret = -1;

	if (stat(key_path, &st) != 0 || st.st_size <= 0)
		return -1;
	size = (size_t) st.st_size;

	buf = (unsigned char *) pg_malloc(size);
	if (buf == NULL)
		return -1;

	fp = fopen(key_path, "r+b");
	if (fp == NULL)
	{
		pg_free(buf);
		return -1;
	}

	/* Pass 1: zeros, Pass 2: ones, Pass 3: random, Pass 4: zeros */
	for (pass = 0; pass < 4; pass++)
	{
		if (pass == 0 || pass == 3)
			memset(buf, 0x00, size);
		else if (pass == 1)
			memset(buf, 0xFF, size);
		else
		{
			/* Random pass; fall back to zeros if the RNG fails */
			if (security_generate_random_bytes(buf, size) != 0)
				memset(buf, 0x00, size);
		}

		if (fseek(fp, 0, SEEK_SET) != 0 ||
			fwrite(buf, 1, size, fp) != size ||
			fflush(fp) != 0)
			goto done;
#ifdef _WIN32
		if (_commit(_fileno(fp)) != 0)
			goto done;
#else
		if (fsync(fileno(fp)) != 0)
			goto done;
#endif
	}

	/* Overwrite done; remove the directory entry */
	if (unlink(key_path) == 0)
		ret = 0;

done:
	fclose(fp);
	pg_free(buf);
	return ret;
}

int
cmd_delete(int argc, char *argv[])
{
	const char *key_path = NULL;

	if (argc < 2 || strcmp(argv[1], "--key-path") != 0 || argc < 3)
	{
		fprintf(stderr, "Usage: tecc_ktool delete --key-path <path>\n");
		return CMD_USAGE_ERROR;
	}
	key_path = argv[2];

	if (destroy_file(key_path) != 0)
	{
		fprintf(stderr, "ERROR: cannot destroy %s: %s\n",
				key_path, strerror(errno));
		return CMD_ERROR;
	}
	printf("Destroyed: %s (overwritten with 4 passes before removal)\n", key_path);

	return CMD_OK;
}

/* ---------------------------------------------------------------------
 *
 * File: pg_pgpass.c
 *
 * Abstract:
 * 		A tool for encrypting and decrypting pgpass files.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 		src/bin/pg_pgpass/pg_pgpass.c
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#define FRONTEND 1

#include "postgres_fe.h"

#include "common/controldata_utils.h"
#include "common/logging.h"
#include "getopt_long.h"
#include "pg_getopt.h"
#include "common/hkdf.h"
#ifdef USE_GMSSL
#include <gmssl/sm4.h>
#endif
#include "common/pass_crypt.h"
#include "utils/elog.h"
#include "storage/fd.h"
#include "common/string.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#ifndef WIN32
#define PGPASSFILE ".pgpass"
#else
#define PGPASSFILE "pgpass.conf"
#endif

static char *
pwdfMatchesAny(char *buf)
{
	char *tbuf;
	bool bslash = false;

	if (buf == NULL)
		return NULL;
	tbuf = buf;

	if (tbuf[0] == '*' && tbuf[1] == ':')
		return tbuf + 2;
	while (*tbuf != 0)
	{
		if (*tbuf == '\\' && !bslash)
		{
			tbuf++;
			bslash = true;
		}
		if (*tbuf == ':' && !bslash)
			return tbuf + 1;
		bslash = false;

		tbuf++;
	}
	return NULL;
}

/*
 * pgpass_crypt
 *
 * Traverse the pgpass file and encrypt and decrypt
 * each password field. If both plaintext password
 * and ciphertext password exist in the same file,
 * encryption and decryption will be rejected.
 */
static bool
pgpass_crypt(bool isencrypt, const unsigned char *key,
			const unsigned char *hmac_key, const char *pgpassfile,
			const char *tmpfile)
{
	FILE *fp;
	FILE *tfp;
	struct stat stat_buf;
	char data[512];
	size_t len = sizeof(data);
	char item[256];
	char out_item[256];
	bool ret = false;

	/* If password file cannot be opened, ignore it. */
	if (stat(pgpassfile, &stat_buf) != 0)
		return false;

#ifndef WIN32
	if (!S_ISREG(stat_buf.st_mode))
	{
		fprintf(stderr,
				"WARNING: password file \"%s\" is not a plain file\n",
				pgpassfile);
		return false;
	}

	/* If password file is insecure, alert the user and ignore it. */
	if (stat_buf.st_mode & (S_IRWXG | S_IRWXO))
	{
		fprintf(stderr,
				"WARNING: password file \"%s\" has group or world access; permissions should be u=rw (0600) or less\n",
				pgpassfile);
		return false;
	}
#else

	/*
	 * On Win32, the directory is protected, so we don't have to check the
	 * file.
	 */
#endif

	fp = fopen(pgpassfile, "r+");
	if (fp == NULL)
	{
		perror("Open file failed");
		return false;
	}

	tfp = fopen(tmpfile, "w+");
	if (fp == NULL)
	{
		perror("Open file failed");
		return false;
	}

	while (!feof(fp) && !ferror(fp))
	{
		memset(data, 0, sizeof(data));
		len = sizeof(data);
		/* Read some data, appending it to what we already have */
		if (fgets(data, len, fp) == NULL)
			break;

		memset(item, 0, sizeof(item));
		memset(out_item, 0, sizeof(out_item));
		strcpy(item, data);
		strcpy(out_item, data);

		/* ignore comments */
		if (data[0] != '#')
		{
			char *t = data;
			int len;

			/* strip trailing newline and carriage return */
			len = pg_strip_crlf(t);

			if (len > 0 &&
				(t = pwdfMatchesAny(t)) != NULL && // hostname
				(t = pwdfMatchesAny(t)) != NULL && // port
				(t = pwdfMatchesAny(t)) != NULL && // dbname
				(t = pwdfMatchesAny(t)) != NULL)   // username
			{
				/* Found a match. */
				char *ret;
				char output[BUFFER_MAX_SIZE];

				ret = strdup(t);

				if (!ret)
				{
					/* Out of memory. XXX: an error message would be nice. */
					return false;
				}

				memset(output, 0, sizeof(output));
				if (isencrypt)
				{
					if (pass_is_encrypted(ret))
					{
						fprintf(stderr, "An encrypted password exists.\n");
						goto err;
					}
					if (!pass_encrypt(ret, key, hmac_key, output))
						goto err;
				}
				else
				{
					if (!pass_is_encrypted(ret))
					{
						fprintf(stderr, "An unencrypted password exists.\n");
						goto err;
					}
					if (!pass_decrypt(ret, key, hmac_key, output))
						goto err;
				}

				memset(out_item, 0, sizeof(out_item));
				snprintf(out_item, sizeof(out_item), "%s", item);
				snprintf(out_item + (t - data), sizeof(out_item) - (t - data), "%s\n", output);
			}
		}
		fputs(out_item, tfp);
		fputs(out_item, stdout);
	}
	fputs("\n", stdout);
	ret = true;
err:
	fclose(fp);
	fclose(tfp);
	return ret;
}

static void
usage(const char *progname)
{
	printf(_("%s Encrypt or decrypt the .pgpass.\n\n"), progname);
	printf(_("Usage:\n"));
	printf(_("  %s [OPTION] [DATADIR]\n"), progname);
	printf(_("\nOptions:\n"));
	printf(_(" [-d,                        Decrypt(default:Encrypt)\n"));
	printf(_(" [-P, --pgpass=]PGPASSFILE   .pgpass directory\n"));
	printf(_(" [-D, --pgdata=]             Database directory(if use identifier)\n"));
	printf(_(" [-M, --interface=]          Gets the MAC address of the specified NIC\n"));
	printf(_(" [-C, --customkey=]          Custom Key\n"));

	printf(_("  -V, --version              output version information, then exit\n"));
	printf(_("  -?, --help                 show this help, then exit\n\n"));

	printf(_("Report bugs to <%s>.\n"), PACKAGE_BUGREPORT);
	printf(_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
}

/*
 * pg_pgpass
 *
 * The pg_pgpass tool is used to encrypt and decrypt pgpass files.
 * It generates encryption and decryption passwords using a fixed
 * password as well as optional database ids, MAC addresses,
 * and custom passwords as seeds.
 */
int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"pgpass", required_argument, NULL, 'P'},
		{"pgdata", required_argument, NULL, 'D'},
		{"customkey", required_argument, NULL, 'C'},
		{"interface", required_argument, NULL, 'M'},
		{NULL, 0, NULL, 0}};

	const char *progname = NULL;
	int c;
	bool isEncrypt = true;
	char *Interface = NULL;
	char *DataDir = NULL;
	char *PgpassFile = NULL;
	char *CustomKey = NULL;
	bool ret = false;

	unsigned char key[32]; // sm4_key and hmac_key

	pg_logging_init(argv[0]);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_pgpass"));
	progname = get_progname(argv[0]);

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage(progname);
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_pgpass (PostgreSQL) " PG_VERSION);
			exit(0);
		}
	}

	while ((c = getopt_long(argc, argv, "P:D:dM:C:", long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'P':
			PgpassFile = optarg;
			break;
		case 'D':
			DataDir = optarg;
			break;
		case 'd':
			isEncrypt = false;
			break;
		case 'M':
			Interface = optarg;
			break;
		case 'C':
			CustomKey = optarg;
			break;

		default:
			fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
			exit(1);
		}
	}

	if (PgpassFile == NULL)
	{
		if (optind < argc)
			PgpassFile = argv[optind++];
		else
			PgpassFile = getenv("PGPASSFILE");
	}

	/* Complain if any arguments remain */
	if (optind < argc)
	{
		pg_log_error("too many command-line arguments (first is \"%s\")",
					 argv[optind]);
		fprintf(stderr, _("Try \"%s --help\" for more information.\n"),
				progname);
		exit(1);
	}

	if (PgpassFile == NULL)
	{
		pg_log_error("no .pgpass directory specified");
		fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
		exit(1);
	}

	ret = pass_gen_key(DataDir, Interface, CustomKey, key);
	if (!ret)
	{
		pg_log_error("Failed to generate key");
		return 0;
	}

	ret = pgpass_crypt(isEncrypt, key, key + 16, PgpassFile, PGPASSFILE ".tmp");
	if (ret)
	{
		if (durable_rename(PGPASSFILE ".tmp", PgpassFile, LOG) != 0)
		{
			pg_log_error("Failed to rename file");
			return 0;
		}

		// 600
		if (chmod(PgpassFile, S_IRUSR | S_IWUSR) != 0)
		{
			pg_log_error("Failed to set file permissions");
			return 0;
		}
	}
	else
	{
		pg_log_error("Failed to %s", isEncrypt?"encrypt":"decrypt");
	}

	return 0;
}

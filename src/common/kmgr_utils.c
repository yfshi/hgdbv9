/*-------------------------------------------------------------------------
 *
 * kmgr_utils.c
 *	  Shared frontend/backend for cluster file encryption
 *
 * These functions handle reading the wrapped DEK files from the
 * file system, and wrapping and unwrapping them.  It also handles
 * running the cluster_key_command.
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/common/kmgr_utils.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef FRONTEND
#include "common/logging.h"
#endif
#include "common/cryptohash.h"
#include "common/file_perm.h"
#include "common/string.h"
#include "crypto/kmgr.h"
#include "lib/stringinfo.h"
#include "storage/fd.h"
#include "common/controldata_utils.h"
#include "common/hex.h"

#ifndef FRONTEND
#include "pgstat.h"
#include "storage/fd.h"
#endif

#ifndef FRONTEND
#include "access/xlog.h"
#else
#include "crypto/walenc.h"
#endif

#ifdef USE_SWSDS
#include <swsds.h>
#endif

#ifdef FRONTEND
static FILE *open_pipe_stream(const char *command);
static int	close_pipe_stream(FILE *file);
#endif

static void read_wrapped_data_key(const char *cryptoKeyDir, uint32 id, unsigned char **key_p, int *key_len);

/*
 * We need this array in frontend and backend code, so define it here.
 * You can only add to the end of this array since the array index is
 * stored in pg_control.
 */
encryption_method encryption_methods[NUM_ENCRYPTION_METHODS] = {
	{"", 0, PG_CIPHER_INVALID},
	{"AES", 128, PG_CIPHER_AES_CTR},
	{"AES192", 192, PG_CIPHER_AES_CTR},
	{"AES256", 256, PG_CIPHER_AES_CTR},
	{"SM4", 128, PG_CIPHER_SM4_CTR}
};

encryption_target_mode_type encryption_target_modes[NUM_ENCRYPTION_MODES] = {
	{"", ENCRYPTION_MODE_NOTHING},
	{"ALL", ENCRYPTION_MODE_ALL},
	{"WAL", ENCRYPTION_MODE_WAL},
	{"REL", ENCRYPTION_MODE_REL}
};

/* This maps wrapped key filesnames to their array slots */
char	   *wkey_filenames[KMGR_NUM_DATA_KEYS] = {
	"rel",
	"wal",
	"kwp"
};

static unsigned int	HSM_KEK_ID = INVALID_HSM_KEK_ID;
#ifdef USE_SWSDS
static SGD_HANDLE hDeviceHandle = NULL;

#define SHA256_DIGEST_LENGTH	32
static const unsigned char SHA256_ASN1_OID[] = {
		0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86,
		0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
		0x00, 0x04, 0x20
	};

hsm_encryption_algorithm_type hsm_encryption_algorithms[NUM_HSM_ENCRYPTION_ALGORITHMS] = {
	{"", 0},
	{"sm1", SGD_SM1_ECB},
	{"sm1-ecb", SGD_SM1_ECB},
	{"sm1-cbc", SGD_SM1_CBC},
	{"sm1-cfb", SGD_SM1_CFB},
	{"sm1-ofb", SGD_SM1_OFB},
	{"sm1-ctr", SGD_SM1_CTR},
	{"sm4", SGD_SMS4_ECB},
	{"sm4-ecb", SGD_SMS4_ECB},
	{"sm4-cbc", SGD_SMS4_CBC},
	{"sm4-cfb", SGD_SMS4_CFB},
	{"sm4-ofb", SGD_SMS4_OFB},
	{"sm4-ctr", SGD_SMS4_CTR},
	{"sm7", SGD_SM7_ECB},
	{"sm7-ecb", SGD_SM7_ECB},
	{"sm7-cbc", SGD_SM7_CBC},
	{"sm7-cfb", SGD_SM7_CFB},
	{"sm7-ofb", SGD_SM7_OFB},
	{"sm7-ctr", SGD_SM7_CTR},
	{"aes", SGD_AES_ECB},
	{"aes-ecb", SGD_AES_ECB},
	{"aes-cbc", SGD_AES_CBC},
	{"aes-cfb", SGD_AES_CFB},
	{"aes-ofb", SGD_AES_OFB},
	{"aes-ctr", SGD_AES_CTR},
	{"3des", SGD_3DES_ECB},
	{"3des-ecb", SGD_3DES_ECB},
	{"3des-cbc", SGD_3DES_CBC},
	{"3des-cfb", SGD_3DES_CFB},
	{"3des-ofb", SGD_3DES_OFB},
	{"3des-ctr", SGD_3DES_CTR},
	/* HSM_ENCRYPTION_ALGORITHM_HASH_BEGIN */
	{"sm3", SGD_SM3},
	{"sha1", SGD_SHA1},
	{"sha224", SGD_SHA224},
	{"sha256", SGD_SHA256},
	{"sha384", SGD_SHA384},
	{"sha512", SGD_SHA512},
	{"md5", SGD_MD5},
	/* HSM_ENCRYPTION_ALGORITHM_PUB_BEGIN */
	{"rsa", SGD_RSA},
	{"sm2", SGD_SM2},
	{"sm2-sig", SGD_SM2_1},
	{"sm2-enc", SGD_SM2_3}
	/* NUM_HSM_ENCRYPTION_ALGORITHMS */
};

static int PKCS1v15_Signature_Pad(const unsigned char *hash, unsigned int hash_len,
					   			  unsigned char *em, unsigned int em_len);
#endif

/*
 * Wrap the given CryptoKey.
 *
 * Returns true and writes encrypted/wrapped/padded data to 'out', and the length
 * of the result to outlen, if successful.
 *
 * Otherwise returns false. The caller must allocate sufficient space
 * for cipher data calculated by using KmgrSizeOfCipherText(). Please note that
 * this function modifies 'out' data even on failure.
 */
bool
kmgr_wrap_data_key(PgCipherCtx *ctx, CryptoKey *in, unsigned char *out, int *outlen)
{
	Assert(ctx && ctx->ctx && in && out);

	if (!pg_cipher_keywrap(ctx, (unsigned char *) in, sizeof(CryptoKey), out, outlen))
		return false;

	return true;
}

/*
 * Decrypt the given data. Return true and set plain text data to `out` if
 * successful.  Otherwise return false. The caller must allocate sufficient
 * space for cipher data calculated by using KmgrSizeOfPlainText(). Please
 * note that this function modifies 'out' data even on failure.
 */
bool
kmgr_unwrap_data_key(PgCipherCtx *ctx, unsigned char *in, int inlen, CryptoKey *out)
{
	int			outlen;

	Assert(ctx && ctx->ctx && in && out);

	if (!pg_cipher_keyunwrap(ctx, in, inlen, (unsigned char *) out, &outlen))
		return false;

	Assert(outlen == sizeof(CryptoKey));

	return true;
}

/*
 * Verify the correctness of the given cluster key by unwrapping the given keys.
 * If the given cluster key is correct we set unwrapped keys to out_keys and return
 * true.  Otherwise return false.  Please note that this function changes the
 * contents of out_keys even on failure.  Both in_keys and out_keys must be the
 * same length.
 */
bool
kmgr_verify_cluster_key(unsigned char *cluster_key,
						unsigned char **in_keys, int *key_lens, CryptoKey *out_keys)
{
	PgCipherCtx *ctx;

	/* Create decryption context with the KEK. */
	ctx = pg_cipher_ctx_create(PG_CIPHER_AES_KWP, cluster_key,
							   KMGR_CLUSTER_KEY_LEN, false);

	/* unwrap each DEK */
	for (int i = 0; i < KMGR_NUM_DATA_KEYS; i++)
	{
		if (!kmgr_unwrap_data_key(ctx, in_keys[i], key_lens[i], &(out_keys[i])))
		{
			/* The cluster key is not correct */
			pg_cipher_ctx_free(ctx);
			return false;
		}
		explicit_bzero(in_keys[i], key_lens[i]);
	}

	/* The cluster key is correct, free the cipher context */
	pg_cipher_ctx_free(ctx);

	return true;
}

/*
 * Run cluster key command.
 *
 * Substitute %d for directory, %p for prompt, %R for file descriptor.
 *
 * The result will be put in buffer buf, which is of size "size".
 * The return value is the length of the actual result.
 */
int
kmgr_run_cluster_key_command(char *cluster_key_command, char *buf,
							 int size, char *live_path, int terminal_fd)
{
	StringInfoData command;
	const char *sp;
	FILE	   *fh;
	int			pclose_rc;
	size_t		len = 0;

	buf[0] = '\0';

	Assert(size > 0);

	/*
	 * Build the command to be executed.
	 */
	initStringInfo(&command);

	for (sp = cluster_key_command; *sp; sp++)
	{
		if (*sp == '%')
		{
			switch (sp[1])
			{
					/* file descriptor number */
				case 'R':
					{
						char		fd_str[20];

						if (terminal_fd == -1)
						{
#ifdef FRONTEND
							pg_log_fatal("cluster key command referenced %%R, but --authprompt not specified");
							exit(EXIT_FAILURE);
#else
							ereport(ERROR,
									(errcode(ERRCODE_INTERNAL_ERROR),
									 errmsg("cluster key command referenced %%R, but --authprompt not specified")));
#endif
						}

						sp++;
						snprintf(fd_str, sizeof(fd_str), "%d", terminal_fd);
						appendStringInfoString(&command, fd_str);
						break;
					}
					/* live path */
				case 'd':
					{
						char	   *nativePath;

						sp++;

						/*
						 * This needs to use a placeholder to not modify the
						 * input with the conversion done via
						 * make_native_path().
						 */
						nativePath = pstrdup(live_path);
						make_native_path(nativePath);
						appendStringInfoString(&command, nativePath);
						pfree(nativePath);
						break;
					}
					/* literal "%" */
				case '%':
					/* convert %% to a single % */
					sp++;
					appendStringInfoChar(&command, *sp);
					break;
				default:
					/* otherwise treat the % as not special */
					appendStringInfoChar(&command, *sp);
					break;
			}
		}
		else
		{
			appendStringInfoChar(&command, *sp);
		}
	}

#ifdef FRONTEND
	fh = open_pipe_stream(command.data);
	if (fh == NULL)
	{
		pg_log_fatal("could not execute command \"%s\": %m",
					 command.data);
		exit(EXIT_FAILURE);
	}
#else
	fh = OpenPipeStream(command.data, "r");
	if (fh == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not execute command \"%s\": %m",
						command.data)));
#endif

	if (!fgets(buf, size, fh))
	{
		if (ferror(fh))
		{
#ifdef FRONTEND
			pg_log_fatal("could not read from command \"%s\": %m",
						 command.data);
			exit(EXIT_FAILURE);
#else
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read from command \"%s\": %m",
							command.data)));
#endif
		}
	}

#ifdef FRONTEND
	pclose_rc = close_pipe_stream(fh);
#else
	pclose_rc = ClosePipeStream(fh);
#endif

	if (pclose_rc == -1)
	{
#ifdef FRONTEND
		pg_log_fatal("could not close pipe to external command: %m");
		exit(EXIT_FAILURE);
#else
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close pipe to external command: %m")));
#endif
	}
	else if (pclose_rc != 0)
	{
#ifdef FRONTEND
		pg_log_fatal("command \"%s\" failed", command.data);
		exit(EXIT_FAILURE);
#else
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("command \"%s\" failed",
						command.data),
				 errdetail_internal("%s", wait_result_to_str(pclose_rc))));
#endif
	}

	/* strip trailing newline and carriage returns */
	len = pg_strip_crlf(buf);

	pfree(command.data);

	if (CHECK_KEY_HSM_FLAG(buf, len))
	{
#ifdef USE_SWSDS
		const char *p_key_id = buf + KMGR_HSM_FLAG_LEN;
		HSM_KEK_ID = atoi(p_key_id);
#else
		swsds_failure();
#endif
	}

	return len;
}

#ifdef FRONTEND
static FILE *
open_pipe_stream(const char *command)
{
	FILE	   *res;

#ifdef WIN32
	size_t		cmdlen = strlen(command);
	char	   *buf;
	int			save_errno;

	buf = malloc(cmdlen + 2 + 1);
	if (buf == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}
	buf[0] = '"';
	memcpy(&buf[1], command, cmdlen);
	buf[cmdlen + 1] = '"';
	buf[cmdlen + 2] = '\0';

	res = _popen(buf, "r");

	save_errno = errno;
	free(buf);
	errno = save_errno;
#else
	res = popen(command, "r");
#endif							/* WIN32 */
	return res;
}

static int
close_pipe_stream(FILE *file)
{
#ifdef WIN32
	return _pclose(file);
#else
	return pclose(file);
#endif							/* WIN32 */
}
#endif							/* FRONTEND */

/*
 * Reads the keys at path.
 *
 * This routine simply reads in the raw encrypted/wrapped keys;
 * it does not handle any decryption, see kmgr_key_unwrap().
 *
 * For each key returned, the key and key length are returned
 * in the keys and key_lens arrays respectfully.
 *
 * Note that keys and key_lens must be allocated before calling
 * this function as arrays of at least KMGR_NUM_DATA_KEYS length.
 */
void
kmgr_read_wrapped_data_keys(const char *path, unsigned char **keys, int *key_lens)
{
	StaticAssertStmt(lengthof(wkey_filenames) == KMGR_NUM_DATA_KEYS,
					 "wkey_filenames[] must match KMGR_NUM_DATA_KEYS");

	for (int id = 0; id < KMGR_NUM_DATA_KEYS; id++)
		read_wrapped_data_key(path, id, &(keys[id]), &(key_lens[id]));

	return;
}

/* Read a wrapped DEK file */
static void
read_wrapped_data_key(const char *cryptoKeyDir, uint32 id, unsigned char **key_p, int *key_len)
{
	char		path[MAXPGPATH];
	int			fd;
	int			r;
	struct stat st;

	CryptoKeyFilePath(path, cryptoKeyDir, id);

#ifndef FRONTEND
	if ((fd = OpenTransientFile(path, O_RDONLY | PG_BINARY)) == -1)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\" for reading: %m",
						path)));
	else if (fstat(fd, &st))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not stat file \"%s\": %m",
						path)));
#else
	if ((fd = open(path, O_RDONLY | PG_BINARY, 0)) == -1)
		pg_log_fatal("could not open file \"%s\" for reading: %m",
					 path);
	else if (fstat(fd, &st))
		pg_log_fatal("could not stat file \"%s\": %m",
					 path);
#endif

	*key_len = st.st_size;

#ifndef FRONTEND
	pgstat_report_wait_start(WAIT_EVENT_KEY_FILE_READ);
#endif

	*key_p = (unsigned char *) palloc0(*key_len);

	/* Get key bytes */
	r = read(fd, *key_p, *key_len);
	if (r != *key_len)
	{
		if (r < 0)
		{
#ifndef FRONTEND
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read file \"%s\": %m", path)));
#else
			pg_log_fatal("could not read file \"%s\": %m", path);
#endif
		}
		else
		{
#ifndef FRONTEND
			ereport(ERROR,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("could not read file \"%s\": read %d of %u",
							path, r, *key_len)));
#else
			pg_log_fatal("could not read file \"%s\": read %d of %u",
						 path, r, *key_len);
#endif
		}
	}

#ifndef FRONTEND
	pgstat_report_wait_end();
#endif

#ifndef FRONTEND
	if (CloseTransientFile(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m",
						path)));
#else
	if (close(fd) != 0)
		pg_log_fatal("could not close file \"%s\": %m", path);
#endif
}

/* Retrieve the cluster key */
bool
kmgr_retrieve_cluster_key(char *cluster_key_cmd, unsigned char* cluster_key,
                     char *live_path, int terminal_fd)
{
	int			cluster_key_len;
	char		cluster_key_hex[ALLOC_KMGR_CLUSTER_KEY_LEN];

	/* Get the key encryption key from the cluster key command */
	cluster_key_len = kmgr_run_cluster_key_command(cluster_key_cmd,
												   cluster_key_hex,
												   ALLOC_KMGR_CLUSTER_KEY_LEN,
												   live_path, terminal_fd);

	if (!KMGR_USE_HSM_MODE && pg_hex_decode(cluster_key_hex, cluster_key_len,
					(char *) cluster_key, KMGR_CLUSTER_KEY_LEN) !=
					KMGR_CLUSTER_KEY_LEN){
#ifdef FRONTEND
		pg_log_error("cluster key must be at %d hex bytes", KMGR_CLUSTER_KEY_LEN * 2);
#else
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("cluster key must be at %d hex bytes", KMGR_CLUSTER_KEY_LEN * 2)));
#endif
		return false;
	}

    return true;
}

/* Decrypt the data key */
bool
kmgr_retrieve_data_key(unsigned char *cluster_key, const char* key_path, CryptoKey *data_key)
{
	PgCipherCtx *ctx;
	char	in_key[MAX_WRAPPED_KEY_LENGTH];
	size_t	in_klen;
	int		fd;
	int		len;
	struct stat st;

	if (!KMGR_USE_HSM_MODE)
	{
		ctx = pg_cipher_ctx_create(PG_CIPHER_AES_KWP,
								(unsigned char *) cluster_key,
								KMGR_CLUSTER_KEY_LEN, false);
		if (!ctx || !ctx->ctx)
		{
#ifdef FRONTEND
			pg_log_error("could not initialize encryption context");
#else
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("could not initialize encryption context")));
#endif
		}
	}

	if ((fd = open(key_path, O_RDONLY | PG_BINARY, 0)) < 0)
	{
#ifdef FRONTEND
		pg_log_error("could not open file \"%s\": %m", key_path);
#else
		ereport(ERROR,
				(errcode_for_file_access(),
					errmsg("could not open file \"%s\": %m", key_path)));
#endif
		return false;
	}

	if (fstat(fd, &st))
	{
#ifdef FRONTEND
		pg_log_error("could not stat file \"%s\": %m", key_path);
#else
		ereport(ERROR,
				(errcode_for_file_access(),
					errmsg("could not stat file \"%s\": %m", key_path)));
#endif
		return false;
	}

	in_klen = st.st_size;

	if (in_klen > MAX_WRAPPED_KEY_LENGTH)
	{
#ifdef FRONTEND
		pg_log_error("invalid wrapped key length (%d)", (int)in_klen);
#else
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("invalid wrapped key length (%d)", (int)in_klen)));
#endif
		return false;
	}

	/* read the data key */
	len = read(fd, in_key, in_klen);
	if (len != in_klen)
	{
		if (len < 0)
		{
#ifdef FRONTEND
			pg_log_error("could not read file \"%s\": %m", key_path);
#else
			ereport(ERROR,
					(errcode_for_file_access(),
						errmsg("could not read file \"%s\": %m", key_path)));
#endif
		}
		else
		{
#ifdef FRONTEND
			pg_log_error("could not read file \"%s\": read %d of %d", 
						 key_path, len, (int)in_klen);
#else
			ereport(ERROR,
					(errcode_for_file_access(),
						errmsg("could not read file \"%s\": read %d of %d", 
						 key_path, len, (int)in_klen)));
#endif
		}
		return false;
	}

	if (!KMGR_USE_HSM_MODE)
	{
		/* decrypt with the key */
		if (!kmgr_unwrap_data_key(ctx, (unsigned char *)in_key, in_klen, data_key))
		{
#ifdef FRONTEND
			pg_log_error("incorrect key specified");
#else
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("incorrect key specified")));
#endif
			return false;
		}

		/* The cluster key is correct, free the cipher context */
		pg_cipher_ctx_free(ctx);
	}
	else
	{
		memcpy(data_key, in_key, sizeof(CryptoKey));
	}

    return true;
}

static char*
getKeyCmdFromConfigFile(const char *dataDir, const char *path)
{
#define MAX_LINE 1024
#define ParamName "cluster_key_command "
#define IncludeParam "include "
	FILE	   *infile;
	char		line[MAX_LINE];
	char		value[MAX_LINE];
	int 		i = 0,
				j = 0;
	char	   *res = NULL;

	if ((infile = fopen(path, "r")) == NULL)
		return NULL;

	while (fgets(line, sizeof(line), infile))
	{
		char	   *p = line;

		/* skip start space */
		while (isspace(*p))
			p++;
		/* skip comment line */
		if (*p == '#' || *p == '\0')
			continue;

		/* find the include line */
		if (strncmp(p, IncludeParam, strlen(IncludeParam)) == 0)
		{
			char   	include_path[MAXPGPATH] = {0};

			p += strlen(IncludeParam);
			while (isspace(*p) || *p == '=')
				p++;
			
			/* muse in quote */
			if (*p != '\'')
				continue;
			p++;

			/* fetch include file name */
			for (i = 0, j = 0; p[i] != '\0'; i++)
			{
				/* convert '' to ' */
				if (p[i] == '\'' && p[i + 1] == '\'')
				{
					value[j++] = '\'';
					i++;
				}
				else if (p[i] == '\'')
				{
					break;
				}
				else
					value[j++] = p[i];
			}
			value[j] = '\0';

			sprintf(include_path, "%s/%s", dataDir, value);
			canonicalize_path(include_path);

			res = getKeyCmdFromConfigFile(dataDir, include_path);
		}
		else 
		{
			/* find the paramter line */
			if (strncmp(p, ParamName, strlen(ParamName)) != 0)
				continue;

			p += strlen(ParamName);
			while (isspace(*p) || *p == '=')
				p++;

			/* muse in quote */
			if (*p != '\'')
				continue;
			p++;
			/* fetch value */
			for (i = 0, j = 0; p[i] != '\0'; i++)
			{
				/* convert '' to ' */
				if (p[i] == '\'' && p[i + 1] == '\'')
				{
					value[j++] = '\'';
					i++;
				}
				else if (p[i] == '\'')
				{
					break;
				}
				else
					value[j++] = p[i];
			}
			value[j] = '\0';

			if (res != NULL)
				pfree(res);
			res = pstrdup(value);
		}
	}

	fclose(infile);
	return res;
}

static char*
tryGetKmgrKeyCmd(char *dataDir)
{
	char   *res = NULL;
	char   	path[MAXPGPATH] = {0};
	char   	path_auto[MAXPGPATH] = {0};

	sprintf(path, "%s/%s", dataDir, "postgresql.conf");
	sprintf(path_auto, "%s/%s", dataDir, "postgresql.auto.conf");

	canonicalize_path(path);
	canonicalize_path(path_auto);

	/* try find in auto.conf */
	res = getKeyCmdFromConfigFile(dataDir, path_auto);
	if (res == NULL)
	{
		/* find in postgresql.conf */
		res = getKeyCmdFromConfigFile(dataDir, path);
	}

	return res;
}

/* Only support one format: cluster_key_command = '%s' */
bool
kmgr_get_keycmd_conf(char *dataDir, char **cluster_key_command, 
					 int *file_encryption_method, int *encryption_target_mode)
{
	if (dataDir == NULL)
		return false;

	/* Get cluster key cmd */
	if (cluster_key_command != NULL)
	{
		*cluster_key_command = tryGetKmgrKeyCmd(dataDir);
		if (*cluster_key_command == NULL)
			return false;
	}
	
	/* Get encryption method */
	if (encryption_target_mode != NULL && file_encryption_method != NULL)
	{
		/* Read the control file and check compatibility */
		bool	crc_ok = false;
		ControlFileData *ControlFile = get_controlfile(dataDir, &crc_ok);
		if (!crc_ok)
		{
			*file_encryption_method = DISABLED_ENCRYPTION_METHOD;
			*encryption_target_mode = ENCRYPTION_MODE_NOTHING;
			return false;
		}

		/* Set encryption method/mode only it is not be setted. */
		if (*file_encryption_method == DISABLED_ENCRYPTION_METHOD)
			*file_encryption_method = encryption_methods[ControlFile->file_encryption_method].cipher_encrypt_id;
		if (*encryption_target_mode == ENCRYPTION_MODE_NOTHING)
			*encryption_target_mode = ControlFile->encryption_target_mode;
	}

	return true;
}

inline unsigned int
hsm_get_kek_id(void)
{
	return HSM_KEK_ID;
}

#ifndef USE_SWSDS
void
swsds_failure(void)
{
#ifndef FRONTEND
	ereport(ERROR,
			(errcode(ERRCODE_CONFIG_FILE_ERROR),
			 (errmsg("HSM is not supported because swsds is not supported by this build"),
			  errhint("Compile with --with-swsds to use this feature."))));
#else
	fprintf(stderr, _("HSM is not supported because swsds is not supported by this build"));
	exit(1);
#endif
}
#endif

#ifdef USE_SWSDS
/*
 * Get the Algorithm ID from <swsds> library.
 */
unsigned int
HSM_GetAlgID(int pg_cipher_id)
{
	SGD_UINT32 retAlgID = -1;

	switch (pg_cipher_id)
	{
		
		case PG_CIPHER_AES_CTR:
			retAlgID = SGD_AES_CTR;
			break;
		case PG_CIPHER_SM4_CTR:
			retAlgID = SGD_SMS4_CTR;
			break;
		case PG_CIPHER_AES_KWP:
		default:
#ifndef FRONTEND
			elog(ERROR, "HSM do not support algorithm id %d", pg_cipher_id);
#else
			fprintf(stderr, _("HSM do not support algorithm id %d"), pg_cipher_id);
			exit(1);
#endif
		break;
	}

	return retAlgID;
}

/*
 * Get the HSM device handle.
 * If it does not exist one, try to open a new handle.
 */
void*
HSM_GetDeviceHandle(bool force_new)
{
	SGD_RV rv = 0;

	if (!force_new && hDeviceHandle)
		return hDeviceHandle;

	/* OpenDevice */
	rv = SDF_OpenDevice(&hDeviceHandle);
	if (rv != SDR_OK)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM open device failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM open device failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	return hDeviceHandle;
}

/*
 * Get a HSM session handle.
 * If it does not exist one, try to open a new handle.
 * If open a new session failed, retry using a new device handle.
 */
void*
HSM_GetSessionHandle(void* *phSessionHandle, bool force_new)
{
	unsigned int rv;
	bool	retry = false;

	if (!force_new && phSessionHandle && *phSessionHandle)
		return *phSessionHandle;

retry_hsm_open_session:
	if (force_new && phSessionHandle && *phSessionHandle)
		SDF_CloseSession(*phSessionHandle);

	/* create session handle */
	rv = SDF_OpenSession(HSM_GetDeviceHandle(retry), phSessionHandle);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_open_session;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM open session failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM open session failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	return *phSessionHandle;
}

/*
 * Generate a session key wrapped with KEK from HSM.
 * If generate a new key failed, retry using a new session handle.
 */
void*
HSM_GenerateSessionKey(void* *phSessionHandle, unsigned int uiKeyBits,
					   unsigned int uiAlgID, unsigned int uiKEKIndex,
					   unsigned char *pucKey, unsigned int *puiKeyLength,
					   void* *phKeyHandle)
{
	unsigned int rv;
	bool	retry = false;

retry_hsm_generate_key:
	rv = SDF_GenerateKeyWithKEK(HSM_GetSessionHandle(phSessionHandle, retry),
								uiKeyBits, uiAlgID, uiKEKIndex,  
								pucKey, puiKeyLength, phKeyHandle);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_generate_key;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM generate key failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM generate key failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	return *phKeyHandle;
}

/*
 * Get a HSM session key handle.
 * If it does not exist one, try to import wrapped session key with kek.
 * If import session key failed, retry using a new session handle.
 */
void*
HSM_GetKeyHandle(void* *phSessionHandle, void* *phKeyHandle,
				 const CryptoKey *key, unsigned int kekid, bool force_new)
{
	unsigned int rv;
	bool	retry = false;

	if (!force_new && phKeyHandle && *phKeyHandle)
		return *phKeyHandle;

retry_hsm_get_key:
	rv = SDF_ImportKeyWithKEK(HSM_GetSessionHandle(phSessionHandle, retry),
							  key->kAlgId, kekid, (SGD_UCHAR *)key->key, key->klen, phKeyHandle);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_get_key;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM import key failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM import key failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	return *phKeyHandle;
}

/*
 * Destroy a HSM session key handle.
 * If destroy session key failed, retry using a new session handle.
 */
void
HSM_DestroyKey(void* *phSessionHandle, void* hKeyHandle)
{
	unsigned int rv;
	bool	retry = false;

retry_hsm_destroy_key:
	rv = SDF_DestroyKey(HSM_GetSessionHandle(phSessionHandle, retry), hKeyHandle);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_destroy_key;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM destroy key failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM destroy key failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Use HSM to encrypt data.
 * If encrypt data failed, retry using a new session key handle.
 */
bool
HSM_Encrypt(void* *phSessionHandle, void* *phKeyHandle,
			const CryptoKey *key, unsigned int kekid,
			unsigned int uiAlgID, unsigned char *iv, 
			unsigned char *plaintext, unsigned int inlen,
			unsigned char *ciphertext, unsigned int *outlen)
{
	unsigned int 	rv;
	bool			retry_key = false;
	unsigned char  *restext = ciphertext;

	if (ciphertext == plaintext)
		restext = malloc(inlen);

retry_hsm_encrypt:
	rv = SDF_Encrypt(HSM_GetSessionHandle(phSessionHandle, retry_key), 
					 HSM_GetKeyHandle(phSessionHandle, phKeyHandle, key, hsm_get_kek_id(), retry_key), 
					 uiAlgID, iv, plaintext, inlen, restext, outlen);
	if (rv != SDR_OK)
	{
		if (!retry_key)
		{
			retry_key = true;
			goto retry_hsm_encrypt;
		}

		if (restext != ciphertext)
			free(restext);

		return false;
	}

	if (restext != ciphertext)
	{
		memcpy(ciphertext, restext, *outlen);
		free(restext);
	}

	return true;
}

/*
 * Use HSM to decrypt data.
 * If decrypt data failed, retry using a new session key handle.
 */
bool
HSM_Decrypt(void* *phSessionHandle, void* *phKeyHandle,
			const CryptoKey *key, unsigned int kekid,
			unsigned int uiAlgID, unsigned char *iv, 
			unsigned char *ciphertext, unsigned int inlen,
			unsigned char *plaintext, unsigned int *outlen)
{
	unsigned int 	rv;
	bool			retry_key = false;
	unsigned char  *restext = plaintext;

	if (ciphertext == plaintext)
		restext = malloc(inlen);

retry_hsm_decrypt:
	rv = SDF_Decrypt(HSM_GetSessionHandle(phSessionHandle, retry_key), 
					 HSM_GetKeyHandle(phSessionHandle, phKeyHandle, key, hsm_get_kek_id(), retry_key), 
					 uiAlgID, iv, ciphertext, inlen, restext, outlen);
	if (rv != SDR_OK)
	{
		if (!retry_key)
		{
			retry_key = true;
			goto retry_hsm_decrypt;
		}

		if (restext != plaintext)
			free(restext);

		return false;
	}

	if (restext != plaintext)
	{
		memcpy(plaintext, restext, *outlen);
		free(restext);
	}

	return true;
}

/*
 * Use HSM to generate random string.
 * If generate random string failed, retry using a new session handle.
 */
void
HSM_Generate_Random(void* *phSessionHandle, unsigned int len, unsigned char *pucRandom)
{
	unsigned int rv;
	bool	retry = false;

retry_hsm_random:
	rv = SDF_GenerateRandom(HSM_GetSessionHandle(phSessionHandle, retry), len, pucRandom);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_random;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM generate random failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM generate random failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Use HSM to do hash.
 * If hash failed, retry using a new session handle.
 */
void
HSM_Hash(void* *phSessionHandle, unsigned int AlgId,
		 unsigned char *indata, unsigned int inlen,
		 unsigned char *outdata, unsigned int *outlen)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned int rv;
	bool	retry = false;

retry_hsm_hash:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);
	rv = SDF_HashInit(hSessionHandle, AlgId, NULL, NULL, 0);
	if (rv != SDR_OK)
	{
		if (!retry)
		{
			retry = true;
			goto retry_hsm_hash;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM hash init failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM hash init failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	rv = SDF_HashUpdate(hSessionHandle, indata, inlen);
	if (rv != SDR_OK)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM hash update failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM hash update failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	rv = SDF_HashFinal(hSessionHandle, outdata, outlen);
	if (rv != SDR_OK)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM hash final failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM hash final failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}


/*
 * Generate a pair of keys from HSM.
 * If generate keys failed, retry using a new session handle.
 */
void
HSM_Generate_Key_Pair(void* *phSessionHandle, unsigned int uiKeyBits,
					  unsigned int uiAlgID, void *pubKey, void *priKey)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned int rv;
	bool	retry = false;

retry_hsm_generate_key_pair:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);

	switch (uiAlgID)
	{
		case SGD_RSA:
			rv = SDF_GenerateKeyPair_RSA(hSessionHandle, uiKeyBits, pubKey, priKey);
			break;
		case SGD_SM2:
		case SGD_SM2_1:
		case SGD_SM2_3:
			rv = SDF_GenerateKeyPair_ECC(hSessionHandle, uiAlgID, uiKeyBits, pubKey, priKey);
			break;
		default:
			rv = SDR_ALGNOTSUPPORT;
			break;
	}

	if (rv != SDR_OK)
	{
		if (!retry && rv != SDR_ALGNOTSUPPORT)
		{
			retry = true;
			goto retry_hsm_generate_key_pair;
		}

#ifndef FRONTEND
		elog(ERROR, "HSM generate key failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM generate key failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Use HSM to encrypt with public key.
 * If encrypt failed, retry using a new session handle.
 */
void
HSM_Pub_Encrypt(void* *phSessionHandle, unsigned int AlgId, 
				unsigned int keyIndex, void *key, 
				unsigned char *indata, unsigned int inlen,
				unsigned char *outdata, unsigned int *outlen)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned int rv;
	bool	retry = false;

retry_hsm_pub_encrypt:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);
	if (keyIndex > 0)
	{
		if (AlgId == SGD_RSA)
			rv = SDF_InternalPublicKeyOperation_RSA_Ex(hSessionHandle, keyIndex, SGD_RSA_ENC,
													   indata, inlen, outdata, outlen);
		else if (AlgId == SGD_SM2_3)
			rv = SDF_InternalEncrypt_ECC(hSessionHandle, keyIndex, SGD_SM2_3, 
										 indata, inlen, (ECCCipher *)outdata);
		else
			rv = SDR_ALGNOTSUPPORT;
	}
	else
	{
		if (AlgId == SGD_RSA)
			rv = SDF_ExternalPublicKeyOperation_RSA(hSessionHandle, (RSArefPublicKey *)key, 
													indata, inlen, outdata, outlen);
		else if (AlgId == SGD_SM2_3)
			rv = SDF_ExternalEncrypt_ECC(hSessionHandle, SGD_SM2_3, (ECCrefPublicKey *)key, 
										 indata, inlen, (ECCCipher *)outdata);
		else
			rv = SDR_ALGNOTSUPPORT;		
	}

	if (rv != SDR_OK)
	{
		if (!retry && rv != SDR_ALGNOTSUPPORT)
		{
			retry = true;
			goto retry_hsm_pub_encrypt;
		}
#ifndef FRONTEND
		elog(ERROR, "HSM encrypt failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM encrypt failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Use HSM to decrypt with private key.
 * If decrypt failed, retry using a new session handle.
 */
void
HSM_Pub_Decrypt(void* *phSessionHandle, unsigned int AlgId, 
				unsigned int keyIndex, void *key, 
				unsigned char *indata, unsigned int inlen,
				unsigned char *outdata, unsigned int *outlen,
				unsigned char *passwd)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned int rv;
	bool	retry = false;

retry_hsm_pub_decrypt:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);
	if (keyIndex > 0)
	{
		SDF_GetPrivateKeyAccessRight(hSessionHandle, keyIndex, passwd, (SGD_UINT32)strlen((const char*)passwd));
		if (AlgId == SGD_RSA)
			rv = SDF_InternalPrivateKeyOperation_RSA_Ex(hSessionHandle, keyIndex, SGD_RSA_ENC,
														indata, inlen, outdata, outlen);
		else if (AlgId == SGD_SM2_3)
			rv = SDF_InternalDecrypt_ECC(hSessionHandle, keyIndex, SGD_SM2_3, 
										 (ECCCipher *)indata, outdata, outlen);
		else
			rv = SDR_ALGNOTSUPPORT;
		SDF_ReleasePrivateKeyAccessRight(hSessionHandle, keyIndex);
	}
	else
	{
		if (AlgId == SGD_RSA)
			rv = SDF_ExternalPrivateKeyOperation_RSA(hSessionHandle, (RSArefPrivateKey *)key, 
													 indata, inlen, outdata, outlen);
		else if (AlgId == SGD_SM2_3)
			rv = SDF_ExternalDecrypt_ECC(hSessionHandle, SGD_SM2_3, (ECCrefPrivateKey *)key, 
										 (ECCCipher *)indata, outdata, outlen);
		else
			rv = SDR_ALGNOTSUPPORT;		
	}

	if (rv != SDR_OK)
	{
		if (!retry && rv != SDR_ALGNOTSUPPORT)
		{
			retry = true;
			goto retry_hsm_pub_decrypt;
		}
#ifndef FRONTEND
		elog(ERROR, "HSM decrypt failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM decrypt failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Padding hash value by PKCS#1 v1.5 signature.
 */
static int
PKCS1v15_Signature_Pad(const unsigned char *hash, unsigned int hash_len,
					   unsigned char *em, unsigned int em_len)
{
	const unsigned int T_LEN = sizeof(SHA256_ASN1_OID) + SHA256_DIGEST_LENGTH;
	const unsigned int PS_LEN = em_len - T_LEN - 3;

	if (em_len < T_LEN + 3)
		return -1;
	
	/* T = DER(ASN.1(OID)) || HASH */
	memcpy(em + em_len - T_LEN, SHA256_ASN1_OID, sizeof(SHA256_ASN1_OID));
	memcpy(em + em_len - SHA256_DIGEST_LENGTH, hash, SHA256_DIGEST_LENGTH);

	/* EM = 0x00 || 0x01 || PS || 0x00 || T */
	em[0] = 0x00;
	em[1] = 0x01;

	memset(em + 2, 0xFF, PS_LEN);
	em[2 + PS_LEN] = 0x00;

	return 0;
}

/*
 * Use HSM to sign with private key.
 * If sign failed, retry using a new session handle.
 */
void
HSM_Sign(void* *phSessionHandle, unsigned int AlgId, 
				unsigned int keyIndex, void *key, unsigned int keylen,
				unsigned char *indata, unsigned int inlen,
				unsigned char *signature, unsigned int *siglen,
				unsigned char *passwd)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned int rv;
	bool	retry = false;

retry_hsm_sign:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);

	/* Compute hash value */
	if (AlgId == SGD_RSA)
	{
		unsigned char hashval[64];
		unsigned int hashlen;
		unsigned char *padded_hash;
		unsigned int padded_len;
		
		if (keyIndex > 0)
			padded_len = keylen;
		else
			padded_len = ((RSArefPrivateKey *)key)->bits / 8;

		HSM_Hash(&hSessionHandle, SGD_SHA256, indata, inlen, (unsigned char *)&hashval, &hashlen);
		padded_hash = palloc(padded_len);
		PKCS1v15_Signature_Pad(hashval, hashlen, padded_hash, padded_len);
		indata = padded_hash;
		inlen = padded_len;
	}

	/* Sign */
	if (keyIndex > 0)
	{
		SDF_GetPrivateKeyAccessRight(hSessionHandle, keyIndex, passwd, (SGD_UINT32)strlen((const char*)passwd));
		if (AlgId == SGD_RSA)
			rv = SDF_InternalPrivateKeyOperation_RSA_Ex(hSessionHandle, keyIndex, SGD_RSA_SIGN,
														indata, inlen, signature, siglen);
		else
			rv = SDR_ALGNOTSUPPORT;
		SDF_ReleasePrivateKeyAccessRight(hSessionHandle, keyIndex);
	}
	else
	{
		if (AlgId == SGD_RSA)
			rv = SDF_ExternalPrivateKeyOperation_RSA(hSessionHandle, (RSArefPrivateKey *)key, 
													 indata, inlen, signature, siglen);
		else
			rv = SDR_ALGNOTSUPPORT;		
	}

	if (rv != SDR_OK)
	{
		if (!retry && rv != SDR_ALGNOTSUPPORT)
		{
			retry = true;
			goto retry_hsm_sign;
		}
#ifndef FRONTEND
		elog(ERROR, "HSM sign failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM sign failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}
}

/*
 * Use HSM to verify with public key.
 * If verify failed, retry using a new session handle.
 */
void
HSM_Verify(void* *phSessionHandle, unsigned int AlgId, 
				unsigned int keyIndex, void *key, unsigned int keylen,
				unsigned char *indata, unsigned int inlen,
				unsigned char *signature, unsigned int siglen)
{
	SGD_HANDLE hSessionHandle = NULL;
	unsigned char *plaintext = NULL;
	unsigned int  outlen = 0;
	unsigned int rv;
	bool	retry = false;

	if (AlgId == SGD_RSA)
	{
		plaintext = palloc(keylen);
		outlen = keylen;
	}

retry_hsm_verify:
	hSessionHandle = HSM_GetSessionHandle(phSessionHandle, retry);

	/* Verify or decrypt signature */
	if (keyIndex > 0)
	{
		if (AlgId == SGD_RSA)
			rv = SDF_InternalPublicKeyOperation_RSA_Ex(hSessionHandle, keyIndex, SGD_RSA_SIGN,
													   signature, siglen, plaintext, &outlen);
		else
			rv = SDR_ALGNOTSUPPORT;
	}
	else
	{
		if (AlgId == SGD_RSA)
			rv = SDF_ExternalPublicKeyOperation_RSA(hSessionHandle, (RSArefPublicKey *)key, 
													signature, siglen, plaintext, &outlen);
		else
			rv = SDR_ALGNOTSUPPORT;		
	}

	if (rv != SDR_OK)
	{
		if (!retry && rv != SDR_ALGNOTSUPPORT)
		{
			retry = true;
			goto retry_hsm_verify;
		}
#ifndef FRONTEND
		elog(ERROR, "HSM verify failed, errcode[0x%08x]", rv);
#else
		fprintf(stderr, _("HSM verify failed, errcode[0x%08x]"), rv);
		exit(1);
#endif
	}

	/* Verify hash value */
	if (AlgId == SGD_RSA)
	{
		unsigned char hashval[64];
		unsigned int hashlen;
		int	i;

		if (outlen < 11 || plaintext[0] != 0x00 || plaintext[1] != 0x01)
			goto hsm_verify_failed;
		
		for (i = 2; i < outlen; i++)
		{
			if (plaintext[i] == 0x00)
				break;
		}
		if (i >= outlen - SHA256_DIGEST_LENGTH - sizeof(SHA256_ASN1_OID))
			goto hsm_verify_failed;

		HSM_Hash(&hSessionHandle, SGD_SHA256, indata, inlen, (unsigned char *)&hashval, &hashlen);
		
		if (memcmp(plaintext + outlen - SHA256_DIGEST_LENGTH, hashval, SHA256_DIGEST_LENGTH) != 0)
		{
hsm_verify_failed:
#ifndef FRONTEND
			elog(ERROR, "HSM verify failed");
#else
			fprintf(stderr, _("HSM verify failed"));
			exit(1);
#endif
		}
	}
}

#endif

/*
 * Genereate a DEK wrap with KEK by HSM.
 */
void
hsm_generate_crypto_key(unsigned int kek_id, int len_bit, CryptoKey *key)
{
#ifdef USE_SWSDS
	SGD_HANDLE hSessionHandle = NULL;
	SGD_HANDLE hKeyHandle = NULL;

	HSM_GetSessionHandle(&hSessionHandle, true);
	HSM_GenerateSessionKey(&hSessionHandle, (SGD_UINT32)len_bit, key->kAlgId, 
						   kek_id, (SGD_UCHAR *)key->key, &key->klen, &hKeyHandle);
	HSM_DestroyKey(&hSessionHandle, hKeyHandle);
	SDF_CloseSession(hSessionHandle);
#else
	swsds_failure();
#endif
}

/*
 * Verify the DEKs wrap with KEK by HSM.
 */
void
hsm_verify_cluster_key(unsigned char **in_keys, int *key_lens, CryptoKey *out_keys)
{
#ifdef USE_SWSDS
	SGD_HANDLE hSessionHandle = NULL;
	SGD_HANDLE hKeyHandle = NULL;

	HSM_GetSessionHandle(&hSessionHandle, true);
	for (int i = 0; i < KMGR_NUM_DATA_KEYS; i++)
	{
		CryptoKey *key = &(out_keys[i]);
		memcpy(key, in_keys[i], key_lens[i]);
		HSM_GetKeyHandle(&hSessionHandle, &hKeyHandle, key, hsm_get_kek_id(), true);
		HSM_DestroyKey(&hSessionHandle, hKeyHandle);
		explicit_bzero(in_keys[i], key_lens[i]);
	}

	SDF_CloseSession(hSessionHandle);
#else
	swsds_failure();
#endif
}

#ifdef USE_SWSDS
/*
 * Get algorithm id for HSM by name.
 */
unsigned int
HSM_AlgName2Id(const char *algName)
{
	unsigned int AlgID = 0;
	int 		 i;

	for (i = 0; i < NUM_HSM_ENCRYPTION_ALGORITHMS; i++)
	{
		if (pg_strcasecmp(hsm_encryption_algorithms[i].algName, algName) == 0)
		{
			AlgID = hsm_encryption_algorithms[i].algId;
			break;
		}
	}

	if (AlgID == 0)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM unsupport algorithm: %s", algName);
#else
		fprintf(stderr, _("HSM unsupport algorithm: %s"), algName);
		exit(1);
#endif
	}

	return AlgID;
}

/*
 * Get algorithm name for HSM by id.
 */
const char* 
HSM_AlgId2Name(unsigned int algId)
{
    const char *AlgName = NULL;
	int 		i;

	for (i = 0; i < NUM_HSM_ENCRYPTION_ALGORITHMS; i++)
	{
		if (hsm_encryption_algorithms[i].algId == algId)
		{
			AlgName = hsm_encryption_algorithms[i].algName;
			break;
		}
	}

    if (AlgName == NULL)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM unsupport algorithm: 0x%08x", algId);
#else
		fprintf(stderr, _("HSM unsupport algorithm: 0x%08x"), algId);
		exit(1);
#endif
	}

	return AlgName;
}

inline int
HSM_GetIndexByAlgId(unsigned int algId)
{
	int 	index = 0;

	for (index = 0; index < NUM_HSM_ENCRYPTION_ALGORITHMS; index++)
	{
		if (hsm_encryption_algorithms[index].algId == algId)
		{
			break;
		}
	}

    if (index == 0)
	{
#ifndef FRONTEND
		elog(ERROR, "HSM unsupport algorithm: 0x%08x", algId);
#else
		fprintf(stderr, _("HSM unsupport algorithm: 0x%08x"), algId);
		exit(1);
#endif
	}

	return index;
}

#endif
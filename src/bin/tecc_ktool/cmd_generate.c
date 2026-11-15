/*
 * cmd_generate.c — tecc_ktool generate command
 *
 * Generates a KEK file:
 *   sm4 → 32 random bytes → SM4-CBC encrypted → <key-name>.key
 *   sm2 → SM2 keypair → <key-name>.priv (SM4-CBC encrypted) + <key-name>.pub (plain DER)
 *
 * Files are named <key-name>.key / <key-name>.priv / <key-name>.pub and
 * created under the --key-path directory. An existing target file is never
 * silently overwritten: generate refuses with an error unless --force is given.
 *
 * src/bin/tecc_ktool/cmd_generate.c
 */

#include "tecc_ktool_common.h"
#include "kek_file.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/err.h>

#ifdef _WIN32
#include <direct.h>			/* _mkdir */
/* Windows has no mode bits; _mkdir takes a single argument */
#define MKDIR_WITH_MODE(path)  _mkdir(path)
/* chmod maps only the read-only bit on Windows; skip it (pub key has no
 * meaningful permission semantics there) */
#define CHMOD_NOOP(path, mode) ((void) 0)
#else
#define MKDIR_WITH_MODE(path)  mkdir(path, 0700)
#define CHMOD_NOOP(path, mode)  chmod(path, mode)
#endif

/* From libpq tecc_key module */
extern int security_generate_random_bytes(unsigned char *buffer, size_t length);

/*
 * Generate an SM2 key pair using OpenSSL.
 * Returns 0 on success, -1 on error.
 * priv_der / pub_der are allocated with malloc; caller must free.
 */
/* ReqID:TECC4.0 - sm2p256v1 explicit curve params (so SM2 key generation
 * works on OpenSSL 1.0.x where NID_sm2 does not exist).  Verified against
 * `openssl ecparam -name SM2 -param_enc explicit -text`. */
#define TECC_SM2_P   \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"
#define TECC_SM2_A   \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"
#define TECC_SM2_B   \
	"28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"
#define TECC_SM2_N   \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123"
#define TECC_SM2_GX  \
	"32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7"
#define TECC_SM2_GY  \
	"BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0"

/*
 * Build an sm2p256v1 EC_GROUP with explicit parameters (no NID_sm2 needed,
 * so this works on OpenSSL 1.0.2, 1.1.x and 3.x alike).  Caller frees it.
 */
__attribute__((unused)) static EC_GROUP *
build_sm2_group(void)
{
	BN_CTX    *ctx = NULL;
	BIGNUM    *p = NULL;
	BIGNUM    *a = NULL;
	BIGNUM    *b = NULL;
	BIGNUM    *n = NULL;
	BIGNUM    *gx = NULL;
	BIGNUM    *gy = NULL;
	BIGNUM    *cofactor = NULL;
	EC_GROUP  *group = NULL;
	EC_POINT  *g = NULL;
	int			ok = 0;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		return NULL;
	BN_CTX_start(ctx);
	p = BN_CTX_get(ctx);
	a = BN_CTX_get(ctx);
	b = BN_CTX_get(ctx);
	n = BN_CTX_get(ctx);
	gx = BN_CTX_get(ctx);
	gy = BN_CTX_get(ctx);
	cofactor = BN_CTX_get(ctx);

	if (cofactor == NULL ||
		!BN_hex2bn(&p, TECC_SM2_P) ||
		!BN_hex2bn(&a, TECC_SM2_A) ||
		!BN_hex2bn(&b, TECC_SM2_B) ||
		!BN_hex2bn(&n, TECC_SM2_N) ||
		!BN_hex2bn(&gx, TECC_SM2_GX) ||
		!BN_hex2bn(&gy, TECC_SM2_GY) ||
		!BN_set_word(cofactor, 1))
		goto done;

	group = EC_GROUP_new_curve_GFp(p, a, b, ctx);
	if (group == NULL)
		goto done;
	g = EC_POINT_new(group);
	if (g == NULL)
		goto done;
	if (!EC_POINT_set_affine_coordinates_GFp(group, g, gx, gy, ctx))
		goto done;
	if (!EC_GROUP_set_generator(group, g, n, cofactor))
		goto done;
	ok = 1;

done:
	EC_POINT_free(g);
	if (!ok && group != NULL)
	{
		EC_GROUP_free(group);
		group = NULL;
	}
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	return group;
}

static int
generate_sm2_keypair(unsigned char **priv_der, size_t *priv_len,
                     unsigned char **pub_der, size_t *pub_len)
{
#if defined(USE_OPENSSL)
	EVP_PKEY_CTX *pctx = NULL;
	EVP_PKEY    *pkey = NULL;
	EVP_PKEY    *params = NULL;
	EC_GROUP    *group = NULL;
	EC_KEY      *ec = NULL;

#ifdef NID_sm2
	/* On OpenSSL 1.1.1+ NID_sm2 exists; use the standard EVP paramgen path,
	 * which yields a PKCS#8 key carrying the SM2 OID. */
	pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
	if (!pctx)
		goto fail;
	if (EVP_PKEY_paramgen_init(pctx) <= 0 ||
		EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_sm2) <= 0 ||
		EVP_PKEY_paramgen(pctx, &params) <= 0)
		goto fail;
	EVP_PKEY_CTX_free(pctx);
	pctx = NULL;

	pctx = EVP_PKEY_CTX_new(params, NULL);
	if (!pctx)
		goto fail;
	EVP_PKEY_keygen_init(pctx);
	EVP_PKEY_keygen(pctx, &pkey);
	EVP_PKEY_CTX_free(pctx);
	pctx = NULL;
	EVP_PKEY_free(params);
	params = NULL;

	if (!pkey)
		goto fail;

	/* Export private key as PKCS#8 DER, public key as SubjectPublicKeyInfo DER */
	*priv_len = i2d_PrivateKey(pkey, priv_der);
	*pub_len = i2d_PublicKey(pkey, pub_der);
	EVP_PKEY_free(pkey);
	pkey = NULL;

	if (*priv_len <= 0 || *pub_len <= 0)
		goto fail;

	return 0;
#else							/* NID_sm2 undefined (OpenSSL 1.0.2) */
	/* Low-level EC_KEY path with an explicitly-built sm2p256v1 group.  The
	 * resulting PKCS#8 uses id-ecPublicKey with explicit params instead of
	 * the SM2 OID, but it parses fine via d2i_PrivateKey(EVP_PKEY_EC). */
	group = build_sm2_group();
	if (group == NULL)
		goto fail;
	ec = EC_KEY_new();
	if (ec == NULL)
		goto fail;
	if (EC_KEY_set_group(ec, group) != 1)
		goto fail;
	if (EC_KEY_generate_key(ec) != 1)
		goto fail;

	pkey = EVP_PKEY_new();
	if (pkey == NULL)
		goto fail;
	if (EVP_PKEY_set1_EC_KEY(pkey, ec) != 1)
		goto fail;

	*priv_len = i2d_PrivateKey(pkey, priv_der);
	*pub_len = i2d_PublicKey(pkey, pub_der);

	EVP_PKEY_free(pkey);
	pkey = NULL;
	EC_KEY_free(ec);
	ec = NULL;
	EC_GROUP_free(group);
	group = NULL;

	if (*priv_len <= 0 || *pub_len <= 0)
		goto fail;

	return 0;
#endif							/* NID_sm2 */

fail:
	if (pctx) EVP_PKEY_CTX_free(pctx);
	if (params) EVP_PKEY_free(params);
	if (pkey) EVP_PKEY_free(pkey);
	if (group) EC_GROUP_free(group);
	if (ec) EC_KEY_free(ec);
	fprintf(stderr, "ERROR: SM2 key generation failed (OpenSSL may not support SM2)\n");
	return -1;
#else
	(void)priv_der; (void)priv_len; (void)pub_der; (void)pub_len;
	fprintf(stderr, "ERROR: SM2 key generation requires OpenSSL\n");
	return -1;
#endif
}

/*
 * Validate that key-name can be used as a safe file base name.
 * Returns 0 on success, -1 on error.
 */
static int
validate_key_name(const char *key_name)
{
	if (key_name[0] == '\0' ||
		strchr(key_name, '/') != NULL ||
		strchr(key_name, '\\') != NULL ||
		strcmp(key_name, ".") == 0 ||
		strcmp(key_name, "..") == 0 ||
		key_name[0] == '.')
	{
		fprintf(stderr,
				"ERROR: --key-name must be a plain file name "
				"(no path separators, no leading '.')\n");
		return -1;
	}
	return 0;
}

int
cmd_generate(int argc, char *argv[])
{
	const char *key_name = NULL;
	const char *key_algo = NULL;	/* sm4 / sm2 */
	const char *key_path = NULL;	/* directory to create the files in */
	int			force = 0;
	char		out_priv[1024];
	char		out_pub[1024];
	unsigned char mek[MEK_BIN_LEN];
	unsigned char *key_data = NULL;
	size_t		key_len = 0;
	int			is_asym = 0;
	int			ch;
	int			ret;

	static struct option long_opts[] = {
		{"key-name",   required_argument, NULL, 'n'},
		{"key-algo",   required_argument, NULL, 't'},
		{"key-path",   required_argument, NULL, 'p'},
		{"force",      no_argument,       NULL, 'f'},
		{NULL, 0, NULL, 0}
	};

	while ((ch = getopt_long(argc, argv, "n:t:p:f", long_opts, NULL)) != -1)
	{
		switch (ch)
		{
			case 'n': key_name = optarg; break;
			case 't': key_algo = optarg; break;
			case 'p': key_path = optarg; break;
			case 'f': force = 1; break;
			default:
				fprintf(stderr, "Usage: ... generate --key-name <name> "
						"--key-algo <sm4|sm2> --key-path <dir>\n");
				return CMD_USAGE_ERROR;
		}
	}

	if (!key_name || !key_algo || !key_path)
	{
		fprintf(stderr, "ERROR: --key-name, --key-algo, and --key-path are required\n");
		return CMD_USAGE_ERROR;
	}

	if (strcmp(key_algo, "sm2") == 0)
		is_asym = 1;
	else if (strcmp(key_algo, "sm4") == 0)
		is_asym = 0;
	else
	{
		fprintf(stderr, "ERROR: --key-algo must be 'sm4' or 'sm2'\n");
		return CMD_USAGE_ERROR;
	}

	/* ReqID:TECC4.0 - key-name must be usable as a file base name */
	if (validate_key_name(key_name) != 0)
		return CMD_USAGE_ERROR;

	/* Build output file names: <key-name>.key (sym) or <key-name>.priv/.pub (asym) */
	snprintf(out_priv, sizeof(out_priv), "%s/%s.%s",
			 key_path, key_name, is_asym ? "priv" : "key");
	if (is_asym)
		snprintf(out_pub, sizeof(out_pub), "%s/%s.pub", key_path, key_name);

	/* Refuse to overwrite existing key files unless --force is given.
	 * Silently replacing a .key/.priv would leave DEKs encrypted with the
	 * old KEK undecryptable, so the default is to fail. */
	if (!force)
	{
		struct stat st;

		if (stat(out_priv, &st) == 0)
		{
			fprintf(stderr, "ERROR: %s already exists (use --force to overwrite)\n",
					out_priv);
			return CMD_ERROR;
		}
		if (is_asym && stat(out_pub, &st) == 0)
		{
			fprintf(stderr, "ERROR: %s already exists (use --force to overwrite)\n",
					out_pub);
			return CMD_ERROR;
		}
	}

	/* Ensure the target directory exists */
	if (MKDIR_WITH_MODE(key_path) != 0 && errno != EEXIST)
	{
		fprintf(stderr, "ERROR: cannot create directory %s: %s\n",
				key_path, strerror(errno));
		return CMD_ERROR;
	}

	/* Get MEK */
	if (get_mek(mek) != MEK_BIN_LEN)
	{
		fprintf(stderr, "ERROR: failed to obtain master key\n");
		return CMD_ERROR;
	}

	if (is_asym)
	{
		unsigned char *pub_der = NULL;
		size_t		pub_len = 0;
		FILE	   *pub_fp;

		/* Generate SM2 key pair */
		if (generate_sm2_keypair(&key_data, &key_len, &pub_der, &pub_len) != 0)
			return CMD_ERROR;

		/* Write .pub (DER public key, plain) */
		pub_fp = fopen(out_pub, "wb");
		if (!pub_fp)
		{
			fprintf(stderr, "ERROR: cannot write %s: %s\n", out_pub, strerror(errno));
			free(key_data);
			free(pub_der);
			return CMD_ERROR;
		}
		fwrite(pub_der, 1, pub_len, pub_fp);
		fclose(pub_fp);
		CHMOD_NOOP(out_pub, 0644);
		free(pub_der);

		printf("Generated: %s (public key)\n", out_pub);
	}
	else
	{
		/* sym: generate 32 random bytes */
		key_data = (unsigned char *)malloc(32);
		if (!key_data)
		{
			fprintf(stderr, "ERROR: out of memory\n");
			return CMD_ERROR;
		}
		if (security_generate_random_bytes(key_data, 32) != 0)
		{
			fprintf(stderr, "ERROR: failed to generate random key\n");
			free(key_data);
			return CMD_ERROR;
		}
		key_len = 32;
	}

	/* Encrypt and write KEK file */
	ret = tecc_kek_encrypt_file(mek, MEK_BIN_LEN, key_data, key_len, out_priv);
	free(key_data);

	if (ret != 0)
	{
		fprintf(stderr, "ERROR: failed to write KEK file %s\n", out_priv);
		return CMD_ERROR;
	}

	printf("Generated: %s (key_algo=%s, size=%zuB)\n", out_priv, key_algo, key_len);
	if (is_asym)
		printf("Public:   %s\n", out_pub);
	printf("\nTo register this key in the database:\n");
	printf("  CREATE KEK %s WITH (\n"
		   "      KEY_STORE = 'localkms',\n"
		   "      KEY_ALGO = '%s',\n"
		   "      KEY_PATH = '%s'\n"
		   "  );\n", key_name, is_asym ? "sm2" : "sm4", out_priv);

	return CMD_OK;
}

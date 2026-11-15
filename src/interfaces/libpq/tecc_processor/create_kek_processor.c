/*-------------------------------------------------------------------------
 *
 * create_kek_processor.c
 *	  CREATE KEK statement processor with HSM support
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/tecc_processor/create_kek_processor.c
 *
 *-------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _MSC_VER
/* sys/stat.h is provided by the PG port layer on Windows */
#else
#include <strings.h>
#include <sys/stat.h>
#endif

#include "create_kek_processor.h"
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"
#include "../tecc_common/tecc_logging.h"
#include "../../../common/tecc_key/kms_adapter.h"
#include "../../../common/tecc_key/security.h"
#include "../frontend_parser/tecc_nodes.h"

/* OpenSSL for SM2 key pair generation */
#ifdef USE_OPENSSL
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#endif

/* PostgreSQL OID constants */
#ifndef CSTRINGOID
#define CSTRINGOID 227
#endif

/* Forward declarations */
static int validate_create_kek_options(const CreateKekStmt *create_kek_stmt,
									  char **key_store, char **key_path,
									  char **key_algo);
static int process_hsm_kek(const char *kek_name, const char *key_path, const char *key_algo);
static int process_swxa_kek(const char *kek_name, const char *key_path, const char *key_algo);
static int process_local_kek(const char *kek_name, const char *key_path,
							 const char *key_algo);
static void free_options(char *key_store, char *key_path, char *key_algo);

/*
 * tecc_create_kek_processor_run - Process CREATE KEK statement
 */
TeccCreateKekProcessorResult
tecc_create_kek_processor_run(const CreateKekStmt *create_kek_stmt,
							 TeccStatementData *statement_data)
{
	const char *kek_name;
	char *key_store = NULL;
	char *key_path = NULL;
	char *key_algo = NULL;
	int result;

	if (create_kek_stmt == NULL || statement_data == NULL)
		return TECC_CREATE_KEK_PROCESSOR_INVALID_INPUT;

	/* Extract simple KEK name from qualified name list */
	kek_name = (create_kek_stmt->names != NIL) ?
		strVal(llast(create_kek_stmt->names)) : NULL;

	/* Validate and extract options */
	result = validate_create_kek_options(create_kek_stmt, &key_store, &key_path, &key_algo);
	if (result != 0)
	{
		fprintf(stderr, "ERROR: Failed to validate CREATE KEK options\n");
		free_options(key_store, key_path, key_algo);
		return TECC_CREATE_KEK_PROCESSOR_VALIDATION_ERROR;
	}


	/* Process based on KEY_STORE type */
	if (strcmp(key_store, "tzwykms") == 0 ||
	    strcmp(key_store, "hsmkms") == 0)
	{
		/* Process HSM KEK.
		 * tzwykms is the V4.0.1+ name for the TZWY HSM; hsmkms is kept
		 * as a backwards-compatible alias for V4.0. */
		result = process_hsm_kek(kek_name, key_path, key_algo);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup HSM KEK\n");
			free_options(key_store, key_path, key_algo);
			return TECC_CREATE_KEK_PROCESSOR_HSM_ERROR;
		}
	}
	else if (strcmp(key_store, "swxakms") == 0)
	{
		/* Process SWXA HSM KEK */
		result = process_swxa_kek(kek_name, key_path, key_algo);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup SWXA HSM KEK\n");
			free_options(key_store, key_path, key_algo);
			return TECC_CREATE_KEK_PROCESSOR_HSM_ERROR;
		}
	}
	else if (strcmp(key_store, "localkms") == 0)
	{
		/* Process local KEK */
		result = process_local_kek(kek_name, key_path, key_algo);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup local KEK\n");
			free_options(key_store, key_path, key_algo);
			return TECC_CREATE_KEK_PROCESSOR_INTERNAL_ERROR;
		}
	}
	else
	{
		fprintf(stderr, "ERROR: Unsupported KEY_STORE type: %s\n", key_store);
		fprintf(stderr, "Supported types: 'localkms', 'tzwykms', 'swxakms' ('hsmkms' accepted for backward compatibility)\n");
		free_options(key_store, key_path, key_algo);
		return TECC_CREATE_KEK_PROCESSOR_UNSUPPORTED_KMS;
	}

	free_options(key_store, key_path, key_algo);
	return TECC_CREATE_KEK_PROCESSOR_SUCCESS;
}

/*
 * validate_create_kek_options - Validate and extract CREATE KEK options
 */
static int
validate_create_kek_options(const CreateKekStmt *create_kek_stmt,
						   char **key_store, char **key_path,
						   char **key_algo)
{
	const char *kek_name;
	ListCell *cell;
	DefElem *def;
	const char *name;
	Node *value;
	char *str_value;

	if (!create_kek_stmt || !key_store || !key_path)
		return -1;

	/* Initialize outputs */
	*key_store = NULL;
	*key_path = NULL;
	*key_algo = NULL;	/* ReqID:TECC4.0 - key_algo init */

	/* Extract simple KEK name from qualified name list */
	kek_name = (create_kek_stmt->names != NIL) ?
		strVal(llast(create_kek_stmt->names)) : NULL;

	/* Check if KEK name is provided */
	if (!kek_name || strlen(kek_name) == 0)
	{
		fprintf(stderr, "ERROR: KEK name is required\n");
		return -1;
	}

	/* Check if options list exists */
	if (!create_kek_stmt->options)
	{
		fprintf(stderr, "ERROR: WITH options are required for CREATE KEK\n");
		return -1;
	}


	/* Parse options */
	foreach(cell, create_kek_stmt->options)
	{
		def = (DefElem *) lfirst(cell);
		if (!def || (def->type != 13 && def->type != 378))
		{
			continue;
		}

		name = def->defname;
		value = (Node *) def->arg;

		if (!name || !value)
		{
			continue;
		}


		/* Get string value from node */

		/* Check if it's a Value node (type FE_T_String) */
		if (value->type == FE_T_String)
		{
			Value *val = (Value *) value;
			if (val->type == FE_T_String)
				str_value = val->val.str;
			else
			{
				fprintf(stderr, "ERROR: Option value must be a string for %s\n", name);
				return -1;
			}
		}
		/* Check if it's a Const node (type FE_T_Const) */
		else if (value->type == FE_T_Const)
		{
			Const *c = (Const *) value;
			/* TEXTOID = 25, CSTRINGOID = 227 */
			if (c->consttype == 25 || c->consttype == 227)
			{
				/* Const节点存储字符串值 */
				str_value = c->constvalue.sval;
			}
			else
			{
				fprintf(stderr, "ERROR: Option value must be a string for %s\n", name);
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "ERROR: Invalid option value type for %s (type=%d)\n", name, value->type);
			return -1;
		}

		/* Process each option */
		if (strcasecmp(name, "key_store") == 0)
		{
			/* Validate KEY_STORE type. tzwykms is the V4.0.1+ name for
			 * the TZWY HSM; hsmkms is kept as a backwards-compatible
			 * alias for V4.0. */
			if (strcmp(str_value, "localkms") != 0 &&
			    strcmp(str_value, "tzwykms") != 0 &&
			    strcmp(str_value, "hsmkms") != 0 &&
			    strcmp(str_value, "swxakms") != 0)
			{
				fprintf(stderr, "ERROR: Invalid KEY_STORE type: %s\n", str_value);
				fprintf(stderr, "Supported types: 'localkms', 'tzwykms', 'swxakms' ('hsmkms' accepted for backward compatibility)\n");
				return -1;
			}
			*key_store = strdup(str_value);
		}
		else if (strcasecmp(name, "key_algo") == 0)
		{
			/* Validate KEY_ALGO. Canonical values: sm4 / sm2. */
			if (strcmp(str_value, "sm4") != 0 && strcmp(str_value, "sm2") != 0)
			{
				fprintf(stderr, "ERROR: Invalid KEY_ALGO: %s (must be sm4 or sm2)\n", str_value);
				return -1;
			}
			*key_algo = strdup(str_value);
		}
		else if (strcasecmp(name, "key_path") == 0)
		{
			/* Store key path - validation will be done later based on key_store type */
			*key_path = strdup(str_value);
		}
			else if (strcasecmp(name, "_tecc_processed") == 0)
			{
				/* Silently ignore - backend-only marker injected by TECC frontend */
			}
			else if (strcasecmp(name, "_kek_file_checksum") == 0)
			{
				/* Silently ignore - SM3 checksum injected by TECC frontend */
			}
			else if (strcasecmp(name, "algorithm") == 0)
			{
				/* Silently ignore - backend-only KEK option */
			}
			else if (strcasecmp(name, "key_length") == 0)
			{
				/* Silently ignore - backend-only KEK option */
			}
			else if (strcasecmp(name, "public_key") == 0)
			{
				/* Silently ignore - DB-stored SM2 public key.  Present on
				 * pg_dump replay (design 6.2.1 exports it inline); the live
				 * frontend obtains it from the .pub file instead. */
			}
			else
			{
				fprintf(stderr, "WARNING: Unknown option: %s\n", name);
			}
		}
	/* Validate required options */
	if (!*key_store)
	{
		fprintf(stderr, "ERROR: key_store option is required\n");
		return -1;
	}

	if (!*key_path)
	{
		fprintf(stderr, "ERROR: key_path option is required\n");
		return -1;
	}

	/* Validate absolute path for localkms
	 * On Windows, relative paths are allowed if TECC_KEK_PATH is set.
	 * Linux always requires an absolute path (/tmp/...). */
	if (strcmp(*key_store, "localkms") == 0)
	{
#ifdef _WIN32
		{
			const char *kek_path_env = getenv("TECC_KEK_PATH");
			if (kek_path_env == NULL || kek_path_env[0] == '\0')
			{
				/* Windows absolute: drive letter + colon + backslash */
				if (!((*key_path)[0] != '\0' && (*key_path)[1] == ':' && (*key_path)[2] == '\\'))
				{
					fprintf(stderr, "ERROR: KEY_PATH must be an absolute path for localkms\n");
					return -1;
				}
			}
		}
#else
		if ((*key_path)[0] != '/')
		{
			fprintf(stderr, "ERROR: KEY_PATH must be an absolute path for localkms\n");
			return -1;
		}
#endif
	}

	return 0;
}

/*
 * process_hsm_kek - Process HSM KEK setup
 */
static int
process_hsm_kek(const char *kek_name, const char *key_path, const char *key_algo)
{
	unsigned int key_index;
	HsmAdapterInterface *adapter;
	HsmDevice *device = NULL;
	HsmErrorCode error;
	int result = -1;

	if (!kek_name || !key_path)
		return -1;

	/* ReqID:TECC4.0 - 非对称（key_algo='sm2'，HE KEK）：SM2 密钥索引无对称
	 * 密钥句柄，对称 check_key_exists（SDF_ImportKeyWithKEK/SM4）必失败，
	 * 改用 export_public_key 导出公钥验证（镜像 process_swxa_kek）；
	 * 对称（key_algo='sm4'/默认）：走 setup_hsm_kek。 */
	if (key_algo == NULL || strcmp(key_algo, "sm2") != 0)
		return setup_hsm_kek(kek_name, key_path);

	/* Parse key path (format: "hsm:id=<index>") */
	if (!parse_hsm_key_path(key_path, &key_index))
	{
		fprintf(stderr, "ERROR: Invalid KEY_PATH format: %s\n", key_path);
		fprintf(stderr, "Expected format: hsm:id=<index>\n");
		return -1;
	}

	/* ReqID:TECC4.0 - 原先用 printf 打 stdout，会混进使用方（psql/应用）捕获的结果流，
	 * 且无级别不可控；改走既有 TECC_LOG（级别见 TECC_LOG_LEVEL，默认 ERROR 即静默）。 */
	TECC_LOG(TECC_LOG_DEBUG1, "Setting up TZWY HSM KEK: %s (key index: %u)",
			 kek_name, key_index);

	/* Get TZWY HSM adapter */
	adapter = get_tzhsm_adapter();
	if (!adapter)
	{
		fprintf(stderr, "ERROR: Failed to get TZWY HSM adapter\n");
		return -1;
	}

	/* Initialize HSM device */
	error = adapter->init(&device, "/usr/lib64/cfg/client_config.xml");
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to initialize TZWY HSM device: %s\n",
				adapter->error_string(error));
		return -1;
	}

	/* Connect to HSM */
	error = adapter->connect(device);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to connect to TZWY HSM device: %s\n",
				adapter->error_string(error));
		goto cleanup;
	}

	/* 导出公钥验证 SM2 密钥存在 */
	{
		void *pub_key = NULL;
		size_t pub_len = 0;

		error = adapter->export_public_key(device, key_index, &pub_key, &pub_len);
		if (error != HSM_SUCCESS || pub_key == NULL || pub_len == 0)
		{
			fprintf(stderr, "ERROR: SM2 key not found in TZWY HSM at index %u: %s\n",
					key_index, error != HSM_SUCCESS ? adapter->error_string(error) : "empty public key");
			fprintf(stderr, "HINT: Please generate/inject an SM2 key into the password machine first\n");
			if (pub_key)
				free(pub_key);
			goto cleanup;
		}
		free(pub_key);
		TECC_LOG(TECC_LOG_INFO, "SUCCESS: SM2 key found in TZWY HSM at index %u", key_index);
	}
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

/*
 * process_swxa_kek - Process SWXA HSM KEK setup (三未信安)
 */
static int
process_swxa_kek(const char *kek_name, const char *key_path, const char *key_algo)
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

	/* Parse key path (format: "hsm:id=<index>") */
	if (!parse_hsm_key_path(key_path, &key_index))
	{
		fprintf(stderr, "ERROR: Invalid KEY_PATH format: %s\n", key_path);
		fprintf(stderr, "Expected format: hsm:id=<index>\n");
		return -1;
	}

	TECC_LOG(TECC_LOG_DEBUG1, "Setting up SWXA KEK: %s (key index: %u)",
			 kek_name, key_index);

	/* Get SWXA adapter */
	adapter = get_swxa_adapter();
	if (!adapter)
	{
		fprintf(stderr, "ERROR: Failed to get SWXA adapter\n");
		return -1;
	}

	/* Initialize SWXA device */
	error = adapter->init(&device, key_path);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to initialize SWXA device: %s\n",
				adapter->error_string(error));
		goto cleanup;
	}

	/* Connect to password machine */
	error = adapter->connect(device);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Failed to connect to SWXA password machine: %s\n",
				adapter->error_string(error));
		goto cleanup;
	}

	/* ReqID:TECC4.0 - 检查密钥存在性：
	 * - 非对称（key_algo='sm2'，HE KEK）：用 export_public_key 导出公钥验证
	 *   （SM2 密钥索引无对称密钥句柄，SDF_GetSymmKeyHandle 会失败）；
	 * - 对称（key_algo='sm4'/默认）：用 check_key_exists（SDF_GetSymmKeyHandle）。 */
	if (key_algo != NULL && strcmp(key_algo, "sm2") == 0)
	{
		void *pub_key = NULL;
		size_t pub_len = 0;

		error = adapter->export_public_key(device, key_index, &pub_key, &pub_len);
		if (error != HSM_SUCCESS || pub_key == NULL || pub_len == 0)
		{
			fprintf(stderr, "ERROR: SM2 key not found in SWXA password machine at index %u: %s\n",
					key_index, error != HSM_SUCCESS ? adapter->error_string(error) : "empty public key");
			fprintf(stderr, "HINT: Please generate/inject an SM2 key into the password machine first\n");
			if (pub_key)
				free(pub_key);
			goto cleanup;
		}
		free(pub_key);
		TECC_LOG(TECC_LOG_INFO, "SUCCESS: SM2 key found in SWXA password machine at index %u",
				 key_index);
	}
	else
	{
		error = adapter->check_key_exists(device, key_index);
		if (error != HSM_SUCCESS)
		{
			fprintf(stderr, "ERROR: Key not found in SWXA password machine at index %u: %s\n",
					key_index, adapter->error_string(error));
			fprintf(stderr, "HINT: Please inject the key into the password machine first\n");
			goto cleanup;
		}
		TECC_LOG(TECC_LOG_INFO, "SUCCESS: Key found in SWXA password machine at index %u",
				 key_index);
	}
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

/*
 * process_local_kek - Process local KEK setup
 */
static int
process_local_kek(const char *kek_name, const char *key_path,
				   const char *key_algo)
{

	KmsConfig *config = NULL;
	KmsAdapter *adapter = NULL;
	KmsOperationResult *result = NULL;
	unsigned char *key_data = NULL;
	size_t key_len = 0;
	int ret = -1;
	char *last_slash;
	size_t parent_len;
	char parent_path[1024];
	/* ReqID:TECC4.0 - TECC_KEK_PATH-resolved path: the location the key
	 * loader (high_level_api.c) will actually read.  CREATE KEK must verify
	 * the file exists THERE, not at the raw key_path option. */
	char effective_path[1024];
	const char *check_path;
	struct stat st;
	/* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
	struct stat key_file_stat;

	if (!kek_name || !key_path)
		return -1;

	/* Validate key path - should be a valid file path */
	if (strlen(key_path) == 0)
	{
		fprintf(stderr, "ERROR: Key path is empty\n");
		return -1;
	}

	/*
	 * Resolve the effective key file path for existence checks.
	 * The key loader (high_level_api.c / kms_adapter.c) overrides the
	 * directory portion with $TECC_KEK_PATH when set (cross-platform /
	 * regression helper), so we must verify the file at the SAME location
	 * the loader will read -- otherwise a valid tecc_ktool-generated file
	 * would be rejected (or a non-existent one accepted) under TECC_KEK_PATH.
	 */
	{
		const char *env_path = getenv("TECC_KEK_PATH");
		const char *filename = key_path;
		const char *last_sep;

		check_path = key_path;	/* default: no override */
		if (env_path != NULL && env_path[0] != '\0')
		{
			last_sep = strrchr(key_path, '/');
#ifdef _WIN32
			{
				const char *bwd = strrchr(key_path, '\\');
				if (bwd != NULL && (last_sep == NULL || bwd > last_sep))
					last_sep = bwd;
			}
#endif
			if (last_sep != NULL)
				filename = last_sep + 1;
			if (strlen(env_path) + 1 + strlen(filename) >= sizeof(effective_path))
			{
				fprintf(stderr, "ERROR: TECC_KEK_PATH + key file name too long\n");
				return -1;
			}
			snprintf(effective_path, sizeof(effective_path), "%s/%s", env_path, filename);
			check_path = effective_path;
		}
	}

	/* Check if parent directory exists */
	#ifdef _WIN32
	{
		/* On Windows, if TECC_KEK_PATH is set, check that directory
		 * instead of extracting from key_path (cross-platform path). */
		const char *env_path = getenv("TECC_KEK_PATH");
		if (env_path != NULL && env_path[0] != '\0')
		{
			if (strlen(env_path) >= sizeof(parent_path))
			{
				fprintf(stderr, "ERROR: TECC_KEK_PATH too long\n");
				return -1;
			}
			strcpy(parent_path, env_path);
		}
		else
		{
			{
				const char *last_fwd = strrchr(key_path, '/');
				const char *last_bwd = strrchr(key_path, '\\');
				last_slash = (last_fwd > last_bwd) ? (char *)last_fwd : (char *)last_bwd;
			}
			if (last_slash != NULL)
			{
				parent_len = last_slash - key_path;
				strncpy(parent_path, key_path, parent_len);
				parent_path[parent_len] = '\0';
			}
			else
			{
				parent_path[0] = '\0';
			}
		}
	}
#else
	{
		last_slash = strrchr(key_path, '/');
		if (last_slash != NULL)
		{
			parent_len = last_slash - key_path;
			if (parent_len >= sizeof(parent_path))
			{
				fprintf(stderr, "ERROR: Parent path too long\n");
				return -1;
			}
			strncpy(parent_path, key_path, parent_len);
			parent_path[parent_len] = '\0';
		}
		else
		{
			parent_path[0] = '\0';
		}
	}
#endif
	if (parent_path[0] != '\0')
	{
		if (stat(parent_path, &st) != 0)
		{
			fprintf(stderr, "ERROR: Parent directory doesn't exist: %s\n", parent_path);
			return -1;
		}
		if (!S_ISDIR(st.st_mode))
		{
			fprintf(stderr, "ERROR: Parent path is not a directory: %s\n", parent_path);
			return -1;
		}
	}

	/* Create KMS configuration */
	fflush(stderr);
	config = kms_config_create(KMS_TYPE_LOCAL, key_path);
	if (!config)
	{
		fprintf(stderr, "ERROR: Failed to create KMS configuration\n");
		return -1;
	}

	/* Create KMS adapter */
	adapter = kms_adapter_create(KMS_TYPE_LOCAL, config);
	if (!adapter)
	{
		fprintf(stderr, "ERROR: Failed to create KMS adapter\n");
		kms_config_destroy(config);
		return -1;
	}

	/* Initialize adapter */
	ret = adapter->init(adapter, config);
	if (ret != 0)
	{
		fprintf(stderr, "ERROR: Failed to initialize KMS adapter\n");
		goto cleanup;
	}

	/* Connect to KMS */
	ret = adapter->connect(adapter);
	if (ret != 0)
	{
		fprintf(stderr, "ERROR: Failed to connect to KMS\n");
		goto cleanup;
	}

	/* Check if key file already exists */
	/* ReqID:TECC4.0 - key_file_stat declared at function scope beginning */
	if (stat(check_path, &key_file_stat) == 0)
	{
		/* File exists */
		if (key_file_stat.st_size == 0)
		{
			/* File exists but is empty - this is an error */
			const char *key_bn = strrchr(check_path, '/'); key_bn = key_bn ? key_bn + 1 : check_path;
			fprintf(stderr, "ERROR: Key file %s exists but is empty\n", key_bn);
			ret = -1;
			goto cleanup;
		}
		else
		{
			/* File exists and is non-empty - will be registered as-is */
		}
	}
	else if (errno != ENOENT)
	{
		/* stat failed for some reason other than file not existing */
		const char *key_bn = strrchr(check_path, '/'); key_bn = key_bn ? key_bn + 1 : check_path;
		fprintf(stderr, "ERROR: Failed to check key file %s: %s\n", key_bn, strerror(errno));
		ret = -1;
		goto cleanup;
	}

	/*
	 * Verify key file existence based on key_algo.
	 *   sm4 / NULL (default) -> symmetric key file (.key) generated by tecc_ktool
	 *   sm2                 -> SM2 keypair (.priv + .pub) generated by tecc_ktool
	 *
	 * 需求：CREATE KEK 不生成密钥文件，密钥文件由 tecc_ktool generate 生成。
	 * 对称分支对齐非对称分支：仅校验文件已存在并登记元数据，不调用 KMS 写盘。
	 */
	{
		int is_asym = (key_algo != NULL && strcmp(key_algo, "sm2") == 0);

		if (is_asym)
		{
			struct stat asym_stat;
			const char *ext;

			/* Validate .priv extension */
			ext = strrchr(check_path, '.');
			if (!ext || strcmp(ext, ".priv") != 0)
			{
				fprintf(stderr, "ERROR: .priv extension required for SM2 keys\n");
				ret = -1;
				goto cleanup;
			}

			/* Verify .priv file exists (generated by tecc_ktool) */
			if (stat(check_path, &asym_stat) != 0)
			{
				const char *key_bn = strrchr(check_path, '/'); key_bn = key_bn ? key_bn + 1 : check_path;
				fprintf(stderr, "ERROR: key file not found: %s\n", key_bn);
				fprintf(stderr, "HINT: Use tecc_ktool generate --key-algo sm2 to create it first\n");
				ret = -1;
				goto cleanup;
			}

			/* Verify .pub file exists alongside .priv
			 * 设计 6.2.3：非对称 KEK 还原/创建须在 .priv 同目录存在同名 .pub
			 * 公钥文件（CREATE KEK 从该目录加载 .pub 入库）；缺失即报错，
			 * 否则 KEK 无可用公钥（QA TOOL_DUMP_012：仅 .priv 无 .pub 应失败）。 */
			{
				char pub_path[1024];
				const char *pub_bn;
				snprintf(pub_path, sizeof(pub_path), "%.*s.pub",
						 (int)(strlen(check_path) - 5), check_path);
				pub_bn = strrchr(pub_path, '/'); pub_bn = pub_bn ? pub_bn + 1 : pub_path;
				if (stat(pub_path, &asym_stat) != 0)
				{
					fprintf(stderr, "ERROR: public key file not found: %s\n", pub_bn);
					fprintf(stderr, "HINT: tecc_ktool generate generates both .priv and .pub\n");
					fprintf(stderr, "HINT: Restore requires .priv and .pub in the same directory\n");
					ret = -1;
					goto cleanup;
				}
			}

			/* asym KEK: file already exists (created by tecc_ktool).
			 * Skip KMS adapter create_key -- the file is already on disk
			 * in the correct format.  Just register metadata in the database.
			 * key_data stays NULL, so the adapter loop below is skipped. */
			key_data = NULL;
			key_len = 0;
		}
		else
		{
			/* 对称 KEK（sm4/默认）：密钥文件必须已由 tecc_ktool generate 生成。
			 * 需求要求 CREATE KEK 不生成文件、只登记元数据（密钥文件由 tecc_ktool
			 * 生成），对齐 sm2 分支：不生成密钥、不调用 create_key 写盘，
			 * 仅校验文件已存在。 */
			struct stat sym_stat;
			const char *ext;

			/* ReqID:TECC4.0 - 对称 KEK（显式 sm4 或省略默认 sm4）登记的 key_path
			 * 必须是 tecc_ktool generate 生成的 .key 对称密钥文件（V4.0.1 §7.1/§7.7：
			 * sm4 ↔ .key，sm2 ↔ .priv+.pub）。指向 .priv/.pub 等非对称材料属
			 * KEY_ALGO 与文件类型不匹配（QA 报修 + 产品定调：默认同样强制 .key）。
			 * 省略 KEY_ALGO 却指向 sm2 材料时给自解释报错、引导补 KEY_ALGO='sm2'，
			 * 从源头消除"未标注 sm2 的 KEK 被当对称"（tecc_kek_is_asym 对无
			 * key_algo 行按对称处理）的歧义。 */
			ext = strrchr(check_path, '.');
			if (!ext || strcmp(ext, ".key") != 0)
			{
				if (key_algo == NULL &&
					ext && (strcmp(ext, ".priv") == 0 || strcmp(ext, ".pub") == 0))
				{
					fprintf(stderr,
							"ERROR: KEY_ALGO omitted (defaults to sm4, which uses .key files);\n"
							"       SM2 key file (%s) requires explicit KEY_ALGO='sm2'\n",
							ext);
				}
				else
				{
					fprintf(stderr, "ERROR: .key extension required for SM4 keys\n");
				}
				ret = -1;
				goto cleanup;
			}

			/* Verify key file exists (generated by tecc_ktool) */
			if (stat(check_path, &sym_stat) != 0)
			{
				const char *key_bn = strrchr(check_path, '/'); key_bn = key_bn ? key_bn + 1 : check_path;
				fprintf(stderr, "ERROR: key file not found: %s\n", key_bn);
				fprintf(stderr, "HINT: Use tecc_ktool generate --key-algo sm4 to create it first\n");
				ret = -1;
				goto cleanup;
			}

			/* sym KEK: file already exists on disk (created by tecc_ktool).
			 * Skip KMS adapter create_key -- the file is already on disk
			 * in the correct format.  Just register metadata in the database.
			 * key_data stays NULL, so the adapter loop below is skipped. */
			key_data = NULL;
			key_len = 0;
		}	/* end else (sym branch) */

		/* KEK 文件已由 tecc_ktool 生成，跳过 KMS adapter -- 只登记元数据。 */
		if (key_data != NULL)
		{
			/* Create operation result */
			result = kms_operation_result_create(KMS_OP_CREATE_KEY);
			if (!result)
			{
				fprintf(stderr, "ERROR: Failed to create operation result\n");
				ret = -1;
				goto cleanup;
			}

			/* Create key in local KMS */
			ret = adapter->create_key(adapter, kek_name, key_data, key_len, result);
			if (ret != 0)
			{
				fprintf(stderr, "ERROR: Failed to create key in local KMS: %s\n",
					   result->error_message ? result->error_message : "Unknown error");
				goto cleanup;
			}
		}

		ret = 0;
	}	/* end key-material block */

cleanup:
	/* Securely erase key data from memory */
	if (key_data)
	{
		memset(key_data, 0, key_len);
		free(key_data);
	}

	if (result)
		kms_operation_result_destroy(result);

	if (adapter)
	{
		if (adapter->cleanup)
			adapter->cleanup(adapter);
		/* Note: kms_adapter_destroy should free adapter */
	}

	if (config)
		kms_config_destroy(config);

	return ret;
}

/*
 * free_options - Free option strings
 */
static void
free_options(char *key_store, char *key_path, char *key_algo)
{
	if (key_store)
		free(key_store);
	if (key_path)
		free(key_path);
	if (key_algo)
		free(key_algo);
}

/*
 * tecc_restore_validate_kek_key_files - Restore-time key-file presence check
 * for CREATE KEK replay.
 *
 * pg_dump output carries _tecc_processed + public_key inline, so the frontend
 * normally short-circuits replay SQL straight to the server.  But design
 * 6.2.3/6.2.4 still require the target key files to be present at the
 * (TECC_KEK_PATH-resolved) path before the KEK metadata is re-created:
 *   localkms sm4 -> .key
 *   localkms sm2 -> .priv AND same-name .pub in the same directory
 * A missing file means the restored KEK has no usable key material, so the
 * replay must fail and roll the restore transaction back.
 *
 * Only localkms keeps key material on disk; HSM/SWXA/TZWY stores keep it on
 * the device and have no file to verify -> return OK.
 *
 * Returns 0 on success (files present / no file required), -1 on missing or
 * otherwise invalid key files.
 */
int
tecc_restore_validate_kek_key_files(const CreateKekStmt *create_kek_stmt)
{
	const char *kek_name;
	char *key_store = NULL;
	char *key_path = NULL;
	char *key_algo = NULL;
	int ret = -1;

	if (create_kek_stmt == NULL)
		return -1;

	/* Extract simple KEK name from qualified name list */
	kek_name = (create_kek_stmt->names != NIL) ?
		strVal(llast(create_kek_stmt->names)) : NULL;

	if (validate_create_kek_options(create_kek_stmt, &key_store, &key_path, &key_algo) != 0)
		goto cleanup;

	if (key_store != NULL && strcmp(key_store, "localkms") == 0)
	{
		/* Same presence rules as a live CREATE KEK.  When the file(s) exist,
		 * process_local_kek skips create_key (key_data stays NULL) and returns
		 * 0 -- side-effect free.  When a required file is missing it prints the
		 * specific ERROR and returns -1. */
		ret = process_local_kek(kek_name ? kek_name : "", key_path, key_algo);
	}
	else
	{
		/* HSM/SWXA/TZWY KEKs: key material on the device, nothing to check. */
		ret = 0;
	}

cleanup:
	free_options(key_store, key_path, key_algo);
	return ret;
}

/*
 * extract_create_kek_option_text - Pull a single-quoted TECC option value out
 * of a CREATE KEK statement in its canonical pg_dump text form:
 *     <name> = '<value>'
 * pg_dump emits the schema-qualified parens spelling without the "WITH"
 * keyword, which the TECC frontend grammar cannot parse, so no parse tree is
 * ever produced for it.  Restore-time validation therefore has to read the
 * options straight from the text.  Returns a malloc'd value or NULL.
 */
static char *
extract_create_kek_option_text(const char *query, const char *optname)
{
	const char *p = query;
	size_t nlen = strlen(optname);

	while (*p != '\0')
	{
		if (pg_strncasecmp(p, optname, nlen) == 0)
		{
			const char *q = p + nlen;

			/* skip whitespace between name and '=' */
			while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
				q++;
			if (*q == '=')
			{
				const char *val_start;

				q++;
				while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
					q++;
				if (*q == '\'')
				{
					const char *val_end;

					val_start = q + 1;
					val_end = strchr(val_start, '\'');
					if (val_end != NULL)
					{
						size_t vlen = (size_t) (val_end - val_start);
						char *out = (char *) malloc(vlen + 1);

						if (out != NULL)
						{
							memcpy(out, val_start, vlen);
							out[vlen] = '\0';
						}
						return out;
					}
				}
			}
			/* '=' or quote not found right after this occurrence - the name
			 * could be a prefix of another token; keep scanning forward. */
		}
		p++;
	}
	return NULL;
}

/*
 * tecc_restore_validate_create_kek_text - Restore-time key-file presence check
 * for the canonical pg_dump CREATE KEK text form (see the comment on
 * extract_create_kek_option_text: this spelling never yields a parse tree).
 *
 * Locates key_store/key_algo/key_path in the text and, for localkms, reuses
 * process_local_kek's TECC_KEK_PATH resolution + presence rules:
 *   sm4 -> .key, sm2 -> .priv AND same-name .pub.
 * Missing files -> nonzero, so the replay is aborted and the restore
 * transaction rolls back (design 6.2.3/6.2.4, QA TOOL_DUMP_012).
 *
 * Returns 0 when the key files are present, the store is not localkms, or the
 * text cannot be parsed (be permissive -- never break a legitimate replay).
 */
int
tecc_restore_validate_create_kek_text(const char *query)
{
	char	   *key_store = NULL;
	char	   *key_path = NULL;
	char	   *key_algo = NULL;
	int			ret = 0;

	if (query == NULL)
		return 0;

	key_store = extract_create_kek_option_text(query, "key_store");
	key_path = extract_create_kek_option_text(query, "key_path");
	key_algo = extract_create_kek_option_text(query, "key_algo");

	if (key_store != NULL && strcmp(key_store, "localkms") == 0 &&
		key_path != NULL && key_path[0] != '\0')
	{
		/* Same presence rules as a live CREATE KEK.  When the file(s) exist,
		 * process_local_kek skips create_key and returns 0 (side-effect free);
		 * a missing .key / .priv / .pub makes it print the ERROR and return -1. */
		ret = process_local_kek("", key_path, key_algo);
	}

	if (key_store)
		free(key_store);
	if (key_path)
		free(key_path);
	if (key_algo)
		free(key_algo);
	return ret;
}

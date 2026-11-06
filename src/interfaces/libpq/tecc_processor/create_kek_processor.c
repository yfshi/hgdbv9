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
#include "../../../common/tecc_key/kms_adapter.h"
#include "../../../common/tecc_key/security.h"
#include "../frontend_parser/tecc_nodes.h"

/* PostgreSQL OID constants */
#ifndef CSTRINGOID
#define CSTRINGOID 227
#endif

/* Forward declarations */
static int validate_create_kek_options(const CreateKekStmt *create_kek_stmt,
									  char **key_store, char **key_path, char **kms_args);
static int process_hsm_kek(const char *kek_name, const char *key_path, const char *kms_args);
static int process_swxa_kek(const char *kek_name, const char *key_path, const char *kms_args);
static void cleanup_swxa_device(void);
static int process_local_kek(const char *kek_name, const char *key_path, const char *kms_args);
static void free_options(char *key_store, char *key_path, char *kms_args);

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
	char *kms_args = NULL;
	int result;

	if (create_kek_stmt == NULL || statement_data == NULL)
		return TECC_CREATE_KEK_PROCESSOR_INVALID_INPUT;

	/* Extract simple KEK name from qualified name list */
	kek_name = (create_kek_stmt->names != NIL) ?
		strVal(llast(create_kek_stmt->names)) : NULL;

	/* Validate and extract options */
	result = validate_create_kek_options(create_kek_stmt, &key_store, &key_path, &kms_args);
	if (result != 0)
	{
		fprintf(stderr, "ERROR: Failed to validate CREATE KEK options\n");
		free_options(key_store, key_path, kms_args);
		return TECC_CREATE_KEK_PROCESSOR_VALIDATION_ERROR;
	}


	/* Process based on KEY_STORE type */
	if (strcmp(key_store, "hsmkms") == 0)
	{
		/* Process HSM KEK */
		result = process_hsm_kek(kek_name, key_path, kms_args);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup HSM KEK\n");
			free_options(key_store, key_path, kms_args);
			return TECC_CREATE_KEK_PROCESSOR_HSM_ERROR;
		}
	}
	else if (strcmp(key_store, "swxakms") == 0)
	{
		/* Process SWXA HSM KEK */
		result = process_swxa_kek(kek_name, key_path, kms_args);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup SWXA HSM KEK\n");
			free_options(key_store, key_path, kms_args);
			return TECC_CREATE_KEK_PROCESSOR_HSM_ERROR;
		}
	}
	else if (strcmp(key_store, "localkms") == 0)
	{
		/* Process local KEK */
		result = process_local_kek(kek_name, key_path, kms_args);
		if (result != 0)
		{
			fprintf(stderr, "ERROR: Failed to setup local KEK\n");
			free_options(key_store, key_path, kms_args);
			return TECC_CREATE_KEK_PROCESSOR_INTERNAL_ERROR;
		}
	}
	else
	{
		fprintf(stderr, "ERROR: Unsupported KEY_STORE type: %s\n", key_store);
		fprintf(stderr, "Supported types: 'localkms', 'hsmkms', 'swxakms'\n");
		free_options(key_store, key_path, kms_args);
		return TECC_CREATE_KEK_PROCESSOR_UNSUPPORTED_KMS;
	}

	free_options(key_store, key_path, kms_args);
	return TECC_CREATE_KEK_PROCESSOR_SUCCESS;
}

/*
 * validate_create_kek_options - Validate and extract CREATE KEK options
 */
static int
validate_create_kek_options(const CreateKekStmt *create_kek_stmt,
						   char **key_store, char **key_path, char **kms_args)
{
	const char *kek_name;
	ListCell *cell;
	DefElem *def;
	const char *name;
	Node *value;
	char *str_value;

	if (!create_kek_stmt || !key_store || !key_path || !kms_args)
		return -1;

	/* Initialize outputs */
	*key_store = NULL;
	*key_path = NULL;
	*kms_args = NULL;

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
			/* Validate KEY_STORE type */
			if (strcmp(str_value, "localkms") != 0 &&
			    strcmp(str_value, "hsmkms") != 0 &&
			    strcmp(str_value, "swxakms") != 0)
			{
				fprintf(stderr, "ERROR: Invalid KEY_STORE type: %s\n", str_value);
				fprintf(stderr, "Supported types: 'localkms', 'hsmkms', 'swxakms'\n");
				return -1;
			}
			*key_store = strdup(str_value);
		}
		else if (strcasecmp(name, "key_path") == 0)
		{
			/* Store key path - validation will be done later based on key_store type */
			*key_path = strdup(str_value);
		}
		else if (strcasecmp(name, "kms_args") == 0)
		{
			/* Store KMS args */
			*kms_args = strdup(str_value);
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
process_hsm_kek(const char *kek_name, const char *key_path, const char *kms_args)
{
	if (!kek_name || !key_path)
		return -1;

	if (kms_args)
	{
		/* TODO: Parse kms_args if needed */
	}

	/* Call HSM adapter to setup KEK */
	return setup_hsm_kek(kek_name, key_path, kms_args);
}

/*
 * process_swxa_kek - Process SWXA HSM KEK setup (三未信安)
 */
static int
process_swxa_kek(const char *kek_name, const char *key_path, const char *kms_args)
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

	if (kms_args)
	{
		/* TODO: Parse kms_args if needed */
	}

	/* Parse key path (format: "hsm:id=<index>") */
	if (!parse_hsm_key_path(key_path, &key_index))
	{
		fprintf(stderr, "ERROR: Invalid KEY_PATH format: %s\n", key_path);
		fprintf(stderr, "Expected format: hsm:id=<index>\n");
		return -1;
	}

	printf("Setting up SWXA KEK: %s (key index: %u)\n", kek_name, key_index);

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

	/* Check if key exists at specified index */
	error = adapter->check_key_exists(device, key_index);
	if (error != HSM_SUCCESS)
	{
		fprintf(stderr, "ERROR: Key not found in SWXA password machine at index %u: %s\n",
				key_index, adapter->error_string(error));
		fprintf(stderr, "HINT: Please inject the key into the password machine first\n");
		goto cleanup;
	}

	printf("SUCCESS: Key found in SWXA password machine at index %u\n", key_index);
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
process_local_kek(const char *kek_name, const char *key_path, const char *kms_args)
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
	config = kms_config_create(KMS_TYPE_LOCAL, key_path, kms_args);
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
	if (stat(key_path, &key_file_stat) == 0)
	{
		/* File exists */
		if (key_file_stat.st_size == 0)
		{
			/* File exists but is empty - this is an error */
			fprintf(stderr, "ERROR: Key file %s exists but is empty\n", key_path);
			ret = -1;
			goto cleanup;
		}
		else
		{
			/* File exists and is non-empty - will be validated by local_kms_save_key */
		}
	}
	else if (errno != ENOENT)
	{
		/* stat failed for some reason other than file not existing */
		fprintf(stderr, "ERROR: Failed to check key file %s: %s\n", key_path, strerror(errno));
		ret = -1;
		goto cleanup;
	}

	/* Always generate key data - local_kms_save_key will validate existing file */
	{
	key_len = 32;  /* 256-bit key */
	key_data = (unsigned char *)malloc(key_len);
	if (!key_data)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for key data\n");
		ret = -1;
		goto cleanup;
	}

	/* Generate cryptographically secure random key */
	if (security_generate_random_bytes(key_data, key_len) != 0)  /* SECURITY_SUCCESS = 0 */
	{
		fprintf(stderr, "ERROR: Failed to generate random key\n");
		ret = -1;
		goto cleanup;
	}

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

	ret = 0;
	}

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
free_options(char *key_store, char *key_path, char *kms_args)
{
	if (key_store)
		free(key_store);
	if (key_path)
		free(key_path);
	if (kms_args)
		free(kms_args);
}

/*-------------------------------------------------------------------------
 *
 * config.c
 *    Configuration management implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/config.c
 *
 * ReqID:TECC4.0 - Key management configuration
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include "config.h"
#include "tecc_common.h"
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/* Default configuration values */
#define DEFAULT_SERVER_HOST "localhost"
#define DEFAULT_SERVER_PORT 5432
#define DEFAULT_DATABASE_NAME "postgres"
#define DEFAULT_USERNAME "postgres"
#define DEFAULT_CACHE_ENABLED true
#define DEFAULT_MAX_CACHE_SIZE 100
#define DEFAULT_CACHE_TTL 3600  /* 1 hour */
#define DEFAULT_PREFETCH_ENABLED false
#define DEFAULT_KMS_TYPE "localkms"
#define DEFAULT_KMS_PATH "/var/lib/tecc/keys"
#define DEFAULT_KMS_ENCRYPTION_ENABLED true
#define DEFAULT_SECURE_ERASE_ENABLED true
#define DEFAULT_SECURE_ERASE_PASSES 3
#define DEFAULT_MEMORY_LOCK_ENABLED false
#define DEFAULT_CONNECTION_POOL_SIZE 10
#define DEFAULT_QUERY_TIMEOUT 30
#define DEFAULT_BATCH_OPERATIONS false
#define DEFAULT_LOG_LEVEL "INFO"
#define DEFAULT_LOG_FILE "/var/log/tecc/tecc.log"
#define DEFAULT_LOG_TO_STDOUT false

/* Thread-local error buffer */
static pthread_key_t config_error_key;
static pthread_once_t config_error_key_once = PTHREAD_ONCE_INIT;

/*
 * create_config_error_key
 *    Create thread-specific error key for config module
 */
static void
create_config_error_key(void)
{
    pthread_key_create(&config_error_key, free);
}

/*
 * get_config_error_buffer
 *    Get thread-specific error buffer for config module
 */
static char *
get_config_error_buffer(void)
{
    char *buffer;

    pthread_once(&config_error_key_once, create_config_error_key);

    buffer = pthread_getspecific(config_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(config_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_config_error
 *    Set config error message
 */
static void TECC_PRINTF_ATTR(1, 2)
set_config_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_config_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}

/*
 * config_get_last_error
 *    Get the last config error message
 */
const char *
config_get_last_error(void)
{
    char *buffer = get_config_error_buffer();
    if (buffer == NULL)
        return "";
    return buffer;
}

/*
 * config_clear_last_error
 *    Clear the last config error message
 */
void
config_clear_last_error(void)
{
    char *buffer = get_config_error_buffer();
    if (buffer != NULL)
        memset(buffer, 0, 512);
}

/*
 * config_context_create
 *    Create a new configuration context
 */
ConfigContext *
config_context_create(const char *config_file)
{
    ConfigContext *context = NULL;
    int ret;

    context = (ConfigContext *)calloc(1, sizeof(ConfigContext));
    if (context == NULL)
    {
        set_config_error("Failed to allocate memory for config context");
        return NULL;
    }

    if (config_file != NULL)
        context->config_file = strdup(config_file);
    else
        context->config_file = NULL;

    context->num_sections = 0;
    context->sections = NULL;
    context->load_time = 0;
    context->auto_reload = false;

    /* Initialize mutex */
    ret = pthread_mutex_init(&context->lock, NULL);
    if (ret != 0)
    {
        set_config_error("Failed to initialize config mutex: %d", ret);
        if (context->config_file != NULL)
            free(context->config_file);
        free(context);
        return NULL;
    }

    return context;
}

/*
 * config_context_destroy
 *    Destroy a configuration context
 */
void
config_context_destroy(ConfigContext *context)
{
    int i, j;

    if (context == NULL)
        return;

    pthread_mutex_lock(&context->lock);

    /* Destroy all sections and entries */
    for (i = 0; i < context->num_sections; i++)
    {
        ConfigSection *section = context->sections[i];
        if (section != NULL)
        {
            /* Destroy all entries in section */
            for (j = 0; j < section->num_entries; j++)
            {
                ConfigEntry *entry = section->entries[j];
                if (entry != NULL)
                {
                    if (entry->name != NULL)
                        free(entry->name);
                    if (entry->value != NULL)
                    {
                        /* Free value based on type */
                        switch (entry->type)
                        {
                            case CONFIG_TYPE_STRING:
                            case CONFIG_TYPE_PATH:
                            case CONFIG_TYPE_ENUM:
                                free(entry->value);
                                break;
                            case CONFIG_TYPE_LIST:
                                {
                                    char **list = (char **)entry->value;
                                    int k;
                                    for (k = 0; list[k] != NULL; k++)
                                        free(list[k]);
                                    free(list);
                                }
                                break;
                            default:
                                /* BOOL and INT are just values, not pointers */
                                break;
                        }
                    }
                    if (entry->default_value != NULL)
                    {
                        /* Free default value based on type */
                        switch (entry->type)
                        {
                            case CONFIG_TYPE_STRING:
                            case CONFIG_TYPE_PATH:
                            case CONFIG_TYPE_ENUM:
                                free(entry->default_value);
                                break;
                            case CONFIG_TYPE_LIST:
                                {
                                    char **list = (char **)entry->default_value;
                                    int k;
                                    for (k = 0; list[k] != NULL; k++)
                                        free(list[k]);
                                    free(list);
                                }
                                break;
                            default:
                                /* BOOL and INT are just values, not pointers */
                                break;
                        }
                    }
                    if (entry->description != NULL)
                        free(entry->description);
                    free(entry);
                }
            }

            /* Free entries array and section */
            if (section->entries != NULL)
                free(section->entries);
            if (section->name != NULL)
                free(section->name);
            free(section);
        }
    }

    /* Free sections array */
    if (context->sections != NULL)
        free(context->sections);

    pthread_mutex_unlock(&context->lock);

    /* Destroy mutex and context */
    pthread_mutex_destroy(&context->lock);
    if (context->config_file != NULL)
        free(context->config_file);
    free(context);
}

/*
 * config_context_load
 *    Load configuration from file
 */
int
config_context_load(ConfigContext *context)
{
    /* TODO: Implement actual configuration file parsing */
    /* For now, create default configuration */
    return 0;
}

/*
 * config_context_reload
 *    Reload configuration from file
 */
int
config_context_reload(ConfigContext *context)
{
    if (context == NULL)
    {
        set_config_error("Config context is NULL");
        return -1;
    }

    /* Check if file has been modified since last load */
    if (config_context_needs_reload(context))
    {
        /* TODO: Implement actual reload logic */
        context->load_time = time(NULL);
        return 0;
    }

    return 0;  /* No reload needed */
}

/*
 * config_context_save
 *    Save configuration to file
 */
int
config_context_save(ConfigContext *context)
{
    /* TODO: Implement actual configuration file writing */
    return 0;
}

/*
 * config_context_needs_reload
 *    Check if configuration needs reloading
 */
bool
config_context_needs_reload(ConfigContext *context)
{
    struct stat file_stat;

    if (context == NULL || context->config_file == NULL)
        return false;

    if (stat(context->config_file, &file_stat) != 0)
        return false;  /* File doesn't exist or can't be accessed */

    /* Check if file has been modified since last load */
    return file_stat.st_mtime > context->load_time;
}

/*
 * config_section_create
 *    Create a new configuration section
 */
ConfigSection *
config_section_create(const char *name)
{
    ConfigSection *section = NULL;

    if (name == NULL || strlen(name) == 0)
    {
        set_config_error("Invalid section name");
        return NULL;
    }

    section = (ConfigSection *)calloc(1, sizeof(ConfigSection));
    if (section == NULL)
    {
        set_config_error("Failed to allocate memory for config section");
        return NULL;
    }

    section->name = strdup(name);
    if (section->name == NULL)
    {
        free(section);
        set_config_error("Failed to allocate memory for section name");
        return NULL;
    }

    section->num_entries = 0;
    section->entries = NULL;
    section->enabled = true;

    return section;
}

/*
 * config_section_destroy
 *    Destroy a configuration section
 */
void
config_section_destroy(ConfigSection *section)
{
    int i;

    if (section == NULL)
        return;

    /* Destroy all entries */
    for (i = 0; i < section->num_entries; i++)
    {
        ConfigEntry *entry = section->entries[i];
        if (entry != NULL)
            config_entry_destroy(entry);
    }

    /* Free entries array */
    if (section->entries != NULL)
        free(section->entries);

    /* Free section name */
    if (section->name != NULL)
        free(section->name);

    /* Free section */
    free(section);
}

/*
 * config_section_add_entry
 *    Add an entry to a configuration section
 */
int
config_section_add_entry(ConfigSection *section, ConfigEntry *entry)
{
    ConfigEntry **new_entries = NULL;

    if (section == NULL || entry == NULL)
    {
        set_config_error("Invalid parameters for adding config entry");
        return -1;
    }

    /* Resize entries array */
    new_entries = (ConfigEntry **)realloc(section->entries,
                                          (section->num_entries + 1) * sizeof(ConfigEntry *));
    if (new_entries == NULL)
    {
        set_config_error("Failed to resize config entries array");
        return -1;
    }

    section->entries = new_entries;
    section->entries[section->num_entries] = entry;
    section->num_entries++;

    return 0;
}

/*
 * config_section_get_entry
 *    Get an entry from a configuration section by name
 */
ConfigEntry *
config_section_get_entry(ConfigSection *section, const char *name)
{
    int i;

    if (section == NULL || name == NULL)
        return NULL;

    for (i = 0; i < section->num_entries; i++)
    {
        ConfigEntry *entry = section->entries[i];
        if (entry != NULL && entry->name != NULL && strcmp(entry->name, name) == 0)
            return entry;
    }

    return NULL;
}

/*
 * config_section_remove_entry
 *    Remove an entry from a configuration section
 */
int
config_section_remove_entry(ConfigSection *section, const char *name)
{
    int i, j;
    bool found = false;

    if (section == NULL || name == NULL)
    {
        set_config_error("Invalid parameters for removing config entry");
        return -1;
    }

    /* Find the entry */
    for (i = 0; i < section->num_entries; i++)
    {
        ConfigEntry *entry = section->entries[i];
        if (entry != NULL && entry->name != NULL && strcmp(entry->name, name) == 0)
        {
            /* Destroy the entry */
            config_entry_destroy(entry);
            found = true;

            /* Shift remaining entries */
            for (j = i; j < section->num_entries - 1; j++)
                section->entries[j] = section->entries[j + 1];

            section->num_entries--;
            break;
        }
    }

    if (!found)
    {
        set_config_error("Config entry '%s' not found in section", name);
        return -1;
    }

    /* Resize entries array if needed */
    if (section->num_entries > 0)
    {
        ConfigEntry **new_entries = (ConfigEntry **)realloc(section->entries,
                                                            section->num_entries * sizeof(ConfigEntry *));
        if (new_entries != NULL)
            section->entries = new_entries;
    }
    else
    {
        free(section->entries);
        section->entries = NULL;
    }

    return 0;
}

/*
 * config_entry_destroy
 *    Destroy a configuration entry
 */
void
config_entry_destroy(ConfigEntry *entry)
{
    if (entry == NULL)
        return;

    if (entry->name != NULL)
        free(entry->name);

    if (entry->value != NULL)
    {
        /* Free value based on type */
        switch (entry->type)
        {
            case CONFIG_TYPE_STRING:
            case CONFIG_TYPE_PATH:
            case CONFIG_TYPE_ENUM:
                free(entry->value);
                break;
            case CONFIG_TYPE_LIST:
                {
                    char **list = (char **)entry->value;
                    int k;
                    for (k = 0; list[k] != NULL; k++)
                        free(list[k]);
                    free(list);
                }
                break;
            default:
                /* BOOL and INT are just values, not pointers */
                break;
        }
    }

    if (entry->default_value != NULL)
    {
        /* Free default value based on type */
        switch (entry->type)
        {
            case CONFIG_TYPE_STRING:
            case CONFIG_TYPE_PATH:
            case CONFIG_TYPE_ENUM:
                free(entry->default_value);
                break;
            case CONFIG_TYPE_LIST:
                {
                    char **list = (char **)entry->default_value;
                    int k;
                    for (k = 0; list[k] != NULL; k++)
                        free(list[k]);
                    free(list);
                }
                break;
            default:
                /* BOOL and INT are just values, not pointers */
                break;
        }
    }

    if (entry->description != NULL)
        free(entry->description);

    free(entry);
}

/*
 * client_config_create
 *    Create a new client configuration
 */
ClientConfig *
client_config_create(void)
{
    ClientConfig *config = NULL;

    config = (ClientConfig *)calloc(1, sizeof(ClientConfig));
    if (config == NULL)
    {
        set_config_error("Failed to allocate memory for client config");
        return NULL;
    }

    /* Set default values */
    config->server_host = strdup(DEFAULT_SERVER_HOST);
    config->server_port = DEFAULT_SERVER_PORT;
    config->database_name = strdup(DEFAULT_DATABASE_NAME);
    config->username = strdup(DEFAULT_USERNAME);
    config->password = NULL;  /* No default password */

    config->cache_enabled = DEFAULT_CACHE_ENABLED;
    config->max_cache_size = DEFAULT_MAX_CACHE_SIZE;
    config->cache_ttl = DEFAULT_CACHE_TTL;
    config->prefetch_enabled = DEFAULT_PREFETCH_ENABLED;

    config->default_kms_type = strdup(DEFAULT_KMS_TYPE);
    config->default_kms_path = strdup(DEFAULT_KMS_PATH);
    config->kms_encryption_enabled = DEFAULT_KMS_ENCRYPTION_ENABLED;

    config->secure_erase_enabled = DEFAULT_SECURE_ERASE_ENABLED;
    config->secure_erase_passes = DEFAULT_SECURE_ERASE_PASSES;
    config->memory_lock_enabled = DEFAULT_MEMORY_LOCK_ENABLED;

    config->connection_pool_size = DEFAULT_CONNECTION_POOL_SIZE;
    config->query_timeout = DEFAULT_QUERY_TIMEOUT;
    config->batch_operations = DEFAULT_BATCH_OPERATIONS;

    config->log_level = strdup(DEFAULT_LOG_LEVEL);
    config->log_file = strdup(DEFAULT_LOG_FILE);
    config->log_to_stdout = DEFAULT_LOG_TO_STDOUT;

    return config;
}

/*
 * client_config_destroy
 *    Destroy a client configuration
 */
void
client_config_destroy(ClientConfig *config)
{
    if (config == NULL)
        return;

    if (config->server_host != NULL)
        free(config->server_host);
    if (config->database_name != NULL)
        free(config->database_name);
    if (config->username != NULL)
        free(config->username);
    if (config->password != NULL)
    {
        /* Securely erase password */
        memset(config->password, 0, strlen(config->password));
        free(config->password);
    }

    if (config->default_kms_type != NULL)
        free(config->default_kms_type);
    if (config->default_kms_path != NULL)
        free(config->default_kms_path);

    if (config->log_level != NULL)
        free(config->log_level);
    if (config->log_file != NULL)
        free(config->log_file);

    free(config);
}

/*
 * client_config_get_connection_string
 *    Get connection string from client configuration
 */
char *
client_config_get_connection_string(ClientConfig *config)
{
    char *conn_str = NULL;
    ClientConfig *temp_config_ptr = NULL;

    /* Use provided config or create default one */
    if (config == NULL)
    {
        temp_config_ptr = client_config_create();
        if (temp_config_ptr == NULL)
            return NULL;
        config = temp_config_ptr;
    }

    /* Build connection string */
    if (config->password != NULL && strlen(config->password) > 0)
    {
        /* Connection string with password */
        {
            size_t len = snprintf(NULL, 0, "host=%s port=%d dbname=%s user=%s password=%s",
                                 config->server_host, config->server_port,
                                 config->database_name, config->username,
                                 config->password);
            conn_str = (char *)malloc(len + 1);
            snprintf(conn_str, len + 1, "host=%s port=%d dbname=%s user=%s password=%s",
                    config->server_host, config->server_port,
                    config->database_name, config->username,
                    config->password);
        }
    }
    else
    {
        /* Connection string without password */
        {
            size_t len = snprintf(NULL, 0, "host=%s port=%d dbname=%s user=%s",
                                 config->server_host, config->server_port,
                                 config->database_name, config->username);
            conn_str = (char *)malloc(len + 1);
            snprintf(conn_str, len + 1, "host=%s port=%d dbname=%s user=%s",
                    config->server_host, config->server_port,
                    config->database_name, config->username);
        }
    }

    /* Cleanup temporary config if created */
    if (temp_config_ptr != NULL)
        client_config_destroy(temp_config_ptr);

    return conn_str;
}
/*-------------------------------------------------------------------------
 *
 * config.h
 *    Configuration management for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/config.h
 *
 * ReqID:TECC4.0 - Key management configuration
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_CONFIG_H
#define TECC_KEY_CONFIG_H

#include "tecc_common.h"

/* Configuration file paths */
#define TECC_CLIENT_CONFIG_FILE "tecc_client.conf"
#define TECC_SERVER_CONFIG_FILE "postgresql.conf"
#define TECC_KEY_CONFIG_SECTION "tecc_key"

/* Configuration value types */
typedef enum ConfigValueType
{
    CONFIG_TYPE_BOOL,      /* Boolean value */
    CONFIG_TYPE_INT,       /* Integer value */
    CONFIG_TYPE_STRING,    /* String value */
    CONFIG_TYPE_PATH,      /* File path */
    CONFIG_TYPE_ENUM,      /* Enumeration value */
    CONFIG_TYPE_LIST       /* List of values */
} ConfigValueType;

/* Configuration entry */
typedef struct ConfigEntry
{
    char *name;                 /* Configuration name */
    ConfigValueType type;       /* Value type */
    void *value;                /* Current value */
    void *default_value;        /* Default value */
    char *description;          /* Description */
    bool required;              /* Whether required */
    bool reloadable;            /* Whether can be reloaded */
    time_t last_modified;       /* Last modified time */
} ConfigEntry;

/* Configuration section */
typedef struct ConfigSection
{
    char *name;                 /* Section name */
    ConfigEntry **entries;      /* Array of entries */
    int num_entries;            /* Number of entries */
    bool enabled;               /* Whether section is enabled */
} ConfigSection;

/* Configuration context */
typedef struct ConfigContext
{
    char *config_file;          /* Configuration file path */
    ConfigSection **sections;   /* Array of sections */
    int num_sections;           /* Number of sections */
    time_t load_time;           /* Last load time */
    bool auto_reload;           /* Whether to auto-reload */
    pthread_mutex_t lock;       /* Configuration lock */
} ConfigContext;

/* Client configuration structure */
typedef struct ClientConfig
{
    /* Connection settings */
    char *server_host;          /* Server host */
    int server_port;            /* Server port */
    char *database_name;        /* Database name */
    char *username;            /* Username */
    char *password;            /* Password (encrypted) */

    /* Key management settings */
    bool cache_enabled;         /* Whether caching is enabled */
    int max_cache_size;         /* Maximum cache size */
    time_t cache_ttl;           /* Cache TTL in seconds */
    bool prefetch_enabled;      /* Whether prefetch is enabled */

    /* KMS settings */
    char *default_kms_type;     /* Default KMS type */
    char *default_kms_path;     /* Default KMS path */
    bool kms_encryption_enabled; /* Whether KMS encryption is enabled */

    /* Security settings */
    bool secure_erase_enabled;  /* Whether secure erase is enabled */
    int secure_erase_passes;    /* Number of secure erase passes */
    bool memory_lock_enabled;   /* Whether memory locking is enabled */

    /* Performance settings */
    int connection_pool_size;   /* Connection pool size */
    int query_timeout;          /* Query timeout in seconds */
    bool batch_operations;      /* Whether batch operations are enabled */

    /* Logging settings */
    char *log_level;            /* Log level */
    char *log_file;             /* Log file path */
    bool log_to_stdout;         /* Whether to log to stdout */
} ClientConfig;

/* Server configuration structure */
typedef struct ServerConfig
{
    /* Key system table settings */
    bool kek_table_enabled;     /* Whether KEK table is enabled */
    bool dek_table_enabled;     /* Whether DEK table is enabled */
    char *key_namespace;        /* Key namespace */

    /* Permission settings */
    bool acl_enabled;           /* Whether ACL is enabled */
    bool superuser_bypass;      /* Whether superuser bypasses permissions */

    /* KMS integration */
    bool local_kms_enabled;     /* Whether local KMS is enabled */
    char *local_kms_path;       /* Local KMS path */
    bool hsm_kms_enabled;       /* Whether HSM KMS is enabled */
    char *hsm_kms_library;      /* HSM library path */

    /* Security settings */
    bool key_encryption_enabled; /* Whether key encryption is enabled */
    char *default_algorithm;    /* Default encryption algorithm */
    int default_key_length;     /* Default key length */

    /* Performance settings */
    int max_connections;        /* Maximum connections */
    int query_cache_size;       /* Query cache size */
    bool parallel_queries;      /* Whether parallel queries are enabled */

    /* Maintenance settings */
    time_t key_rotation_interval; /* Key rotation interval */
    bool auto_key_rotation;     /* Whether auto key rotation is enabled */
    time_t audit_log_retention; /* Audit log retention period */
} ServerConfig;

/* Runtime configuration */
typedef struct RuntimeConfig
{
    /* Dynamic settings */
    bool cache_enabled;         /* Current cache state */
    int current_cache_size;     /* Current cache size */
    bool connection_pool_enabled; /* Connection pool state */

    /* Performance counters */
    int cache_hits;             /* Cache hit count */
    int cache_misses;           /* Cache miss count */
    int key_operations;         /* Key operation count */

    /* Resource usage */
    size_t memory_usage;        /* Memory usage in bytes */
    int connection_count;       /* Active connection count */
    time_t uptime;              /* Uptime in seconds */
} RuntimeConfig;

/* Function prototypes */

/* Configuration context management */
extern ConfigContext *config_context_create(const char *config_file);
extern void config_context_destroy(ConfigContext *context);
extern int config_context_load(ConfigContext *context);
extern int config_context_reload(ConfigContext *context);
extern int config_context_save(ConfigContext *context);
extern bool config_context_needs_reload(ConfigContext *context);

/* Configuration section management */
extern ConfigSection *config_section_create(const char *name);
extern void config_section_destroy(ConfigSection *section);
extern int config_section_add_entry(ConfigSection *section, ConfigEntry *entry);
extern ConfigEntry *config_section_get_entry(ConfigSection *section, const char *name);
extern int config_section_remove_entry(ConfigSection *section, const char *name);

/* Configuration entry management */
extern ConfigEntry *config_entry_create(const char *name, ConfigValueType type,
                                       void *default_value, bool required);
extern void config_entry_destroy(ConfigEntry *entry);
extern int config_entry_set_value(ConfigEntry *entry, void *value);
extern void *config_entry_get_value(ConfigEntry *entry);
extern int config_entry_validate(ConfigEntry *entry);

/* Client configuration */
extern ClientConfig *client_config_create(void);
extern void client_config_destroy(ClientConfig *config);
extern int client_config_load(ClientConfig *config, const char *config_file);
extern int client_config_save(ClientConfig *config, const char *config_file);
extern int client_config_validate(ClientConfig *config);
extern char *client_config_get_connection_string(ClientConfig *config);

/* Server configuration */
extern ServerConfig *server_config_create(void);
extern void server_config_destroy(ServerConfig *config);
extern int server_config_load(ServerConfig *config);
extern int server_config_apply(ServerConfig *config);
extern int server_config_validate(ServerConfig *config);
extern bool server_config_is_feature_enabled(ServerConfig *config, const char *feature);

/* Runtime configuration */
extern RuntimeConfig *runtime_config_create(void);
extern void runtime_config_destroy(RuntimeConfig *config);
extern int runtime_config_update(RuntimeConfig *config);
extern int runtime_config_get_stats(RuntimeConfig *config, char **stats_json);
extern int runtime_config_reset_counters(RuntimeConfig *config);

/* Configuration validation */
extern bool config_validate_bool(const char *value);
extern bool config_validate_int(const char *value, int min, int max);
extern bool config_validate_string(const char *value, int max_len);
extern bool config_validate_path(const char *path);
extern bool config_validate_enum(const char *value, const char **valid_values);

/* Configuration utilities */
extern char *config_get_string(ConfigContext *context, const char *section,
                              const char *key, const char *default_value);
extern int config_get_int(ConfigContext *context, const char *section,
                         const char *key, int default_value);
extern bool config_get_bool(ConfigContext *context, const char *section,
                           const char *key, bool default_value);
extern int config_set_string(ConfigContext *context, const char *section,
                            const char *key, const char *value);
extern int config_set_int(ConfigContext *context, const char *section,
                         const char *key, int value);
extern int config_set_bool(ConfigContext *context, const char *section,
                          const char *key, bool value);

/* Hot configuration update */
extern int config_hot_update(ConfigContext *context, const char *section,
                            const char *key, void *new_value);
extern int config_notify_change(ConfigContext *context, const char *section,
                               const char *key);

/* Error handling */
extern const char *config_get_last_error(void);
extern void config_clear_last_error(void);

#endif /* TECC_KEY_CONFIG_H */
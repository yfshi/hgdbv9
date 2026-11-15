/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_metadata_cache.h
 * Abstract:
 *   TECC metadata cache interface for frontend parser integration.
 *   Provides encrypted column metadata access for parser and processor.
 *
 * Authored by <wangxp@highgo.com>, 20260313.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/tecc_metadata_cache.h
 */

#ifndef TECC_METADATA_CACHE_H
#define TECC_METADATA_CACHE_H

#include "postgres_fe.h"
#include "../libpq-fe.h"
#include "../frontend_parser/fe_adapter.h"
#include "fe_hash_table.h"  /* 前端哈希表支持 */

/* Standard C headers for frontend code */
#include <time.h>  /* For time_t */
#include <stdint.h>  /* For int16_t */

/* 前向声明TeccState结构，避免头文件循环依赖 */
typedef struct TeccState TeccState;

/* 线程局部存储宏定义 */
#if defined(_WIN32) && defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#else
#define TECC_THREAD_LOCAL
#pragma message("Warning: Using non-thread-safe storage...")
#endif

/* Oid type definition for frontend code */
#ifndef Oid
typedef unsigned int Oid;
#define InvalidOid ((Oid) 0)
#endif

/* TECC fastpath function OIDs */
#define TECC_GET_CACHE_MARKS_FNID    50250  /* tecc_get_cache_marks_wrapper function OID */
#define TECC_CLEAR_CACHE_MARKS_FNID  50251  /* tecc_clear_cache_marks_wrapper function OID */

/* Cache mark bitmask definitions */
#define TECC_MARK_KEK    0x01
#define TECC_MARK_DEK    0x02
#define TECC_MARK_COLUMN 0x04

/* 哈希键结构定义 */
/* OID查找键 */
typedef struct {
    Oid oid;                    /* 对象标识符 */
} KekHashKey;

typedef struct {
    Oid oid;                    /* 对象标识符 */
} DekHashKey;

typedef struct {
    Oid table_oid;              /* 表对象标识符 */
    int column_attnum;          /* 列号 */
} ColumnHashKey;

/* 名称查找键 */
typedef struct {
    char key_name[NAMEDATALEN];     /* KEK名称 */
    char key_namespace[NAMEDATALEN]; /* 命名空间 */
} KekNameHashKey;

typedef struct {
    char key_name[NAMEDATALEN];     /* DEK名称 */
    char key_namespace[NAMEDATALEN]; /* 命名空间 */
} DekNameHashKey;

typedef struct {
    char database_name[NAMEDATALEN];  /* 数据库名称 */
    char schema_name[NAMEDATALEN];    /* 模式名称 */
    char table_name[NAMEDATALEN];     /* 表名称 */
    char column_name[NAMEDATALEN];    /* 列名称 */
} ColumnNameHashKey;

/* ReqID:TECC4.0 - 未限定表名 → 真实 schema 映射缓存键（表级） */
typedef struct {
    char database_name[NAMEDATALEN];  /* 数据库名称 */
    char table_name[NAMEDATALEN];     /* 表名称（未限定） */
} TableSchemaHashKey;

/* int16 type for frontend code */
#ifndef int16
typedef int16_t int16;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ReqID:TECC4.0 - Metadata types (compatible with fe-tecc-metadata.h) */
#define NAMEDATALEN 64

/* Global key (KEK) metadata */
typedef struct TeccGlobalKeyMetadata {
    char        key_name[NAMEDATALEN];    /* KEK name */
    Oid         key_namespace;            /* Namespace OID */
    Oid         key_owner;                /* Owner OID */
    char        kms_type[32];             /* KMS type: localkms/hsmkms/cloudkms */
    char        kms_path[256];            /* KMS path or identifier */
    int32       key_length;               /* Key length in bits */
    char        algorithm[32];            /* Algorithm: AES_256_CBC etc */
    time_t      created_time;             /* Creation time */
    time_t      modified_time;            /* Modification time */
    char        status[16];               /* Status: active/inactive/revoked */
} TeccGlobalKeyMetadata;

/* Column key (DEK) metadata */
typedef struct TeccColumnKeyMetadata {
    char        key_name[NAMEDATALEN];    /* DEK name */
    Oid         kek_id;                   /* Associated KEK OID */
    Oid         key_namespace;            /* Namespace OID */
    Oid         key_owner;                /* Owner OID */
    char        encrypted_key[1024];      /* Encrypted DEK data */
    bool        supports_det;             /* Supports DET algorithm */
    bool        supports_ope;             /* Supports OPE algorithm */
    bool        supports_search;          /* Supports SEARCH algorithm */
    int32       key_length;               /* Key length in bits */
    time_t      created_time;             /* Creation time */
    time_t      modified_time;            /* Modification time */
    char        status[16];               /* Status: active/inactive/revoked */
} TeccColumnKeyMetadata;

/* Encrypted column metadata */
typedef struct TeccEncryptedColumnMetadata {
    Oid         table_oid;                /* Table OID */
    char       *table_name;               /* Table name (动态分配) */
    int16       column_attnum;            /* Column attribute number */
    char       *column_name;              /* Column name (动态分配) */
    Oid         data_type_original_oid;   /* Base data type OID */
    int         data_type_original_mod;   /* Base data type modifier */
    char       *dek_name;                 /* DEK name (动态分配) */
    char       *kek_name;                 /* KEK name (动态分配) - 新增字段 */
    /* ReqID:TECC4.0 - 一列多 DEK：HE 加法/乘法 DEK 名（纯 HE 列 dek_name 为 NULL） */
    char       *he_add_dek;               /* HE 加法 DEK 名 (动态分配) */
    char       *he_mul_dek;               /* HE 乘法 DEK 名 (动态分配) */
    int         encryption_type;          /* Encryption type bitmask */
    Oid         collation_oid;            /* Collation OID */
    char       *collation_locale;         /* Collation locale (动态分配), e.g. "en_US.UTF-8" */
    time_t      create_date;              /* Creation time */
    Oid         creator;                  /* Creator OID */
    bool        has_dek_permission;       /* 当前用户是否有DEK USAGE权限（来自has_dek_privilege_name） */
    /* 兼容字段 */
    Oid         column_key_id;            /* DEK OID used (兼容字段) */
    bool        enable_det;               /* Enable DET encryption (兼容字段) */
    bool        enable_ope;               /* Enable OPE encryption (兼容字段) */
    bool        enable_search;            /* Enable SEARCH encryption (兼容字段) */
    bool        enable_he_add;            /* Enable EC-ElGamal additive homomorphic (bit 0x08) */
    bool        enable_he_mul;            /* Enable finite field ElGamal multiplicative (bit 0x10) */
} TeccEncryptedColumnMetadata;

/* ReqID:TECC4.0 - Metadata cache interface */


/* ==================== 基于简化设计的新数据结构 ==================== */

/* 刷新类型枚举 */
typedef enum {
    TECC_CACHE_REFRESH_ALL = 0,     /* 刷新所有缓存 */
    TECC_CACHE_REFRESH_KEK = 1,     /* 只刷新KEK缓存 */
    TECC_CACHE_REFRESH_DEK = 2,     /* 只刷新DEK缓存 */
    TECC_CACHE_REFRESH_COLUMN = 3,  /* 只刷新加密列缓存 */
    TECC_CACHE_REFRESH_GUC = 4      /* 只刷新GUC参数缓存（预留） */
} TeccCacheRefreshType;

/*
 * TeccCacheItemStatus - 缓存项状态
 *
 * 标记缓存项的类型，决定其在查找和插入时的行为。
 */
typedef enum TeccCacheItemStatus {
    TECC_CACHE_STATUS_ENCRYPTED = 0,       /* 正常加密列缓存项 */
    TECC_CACHE_STATUS_NOT_ENCRYPTED = 1,   /* 负缓存：确认无加密列 */
    TECC_CACHE_STATUS_PENDING = 2,         /* 待刷新：表刚创建，元数据待从DB加载 */
    TECC_CACHE_STATUS_NOT_FOUND = 3,       /* 负缓存：对象不存在 */
    TECC_CACHE_STATUS_PERMISSION_DENIED = 4, /* 负缓存：无权限 */
    TECC_CACHE_STATUS_TABLE_LOADED = 5      /* 表级哨兵：表已加载且有加密列 */
} TeccCacheItemStatus;

/* KEK缓存项结构（简化版） */
typedef struct TeccKekCacheItem {
    Oid kek_oid;                    /* KEK对象标识符 */
    char *key_name;                 /* KEK名称 */
    char *key_namespace;            /* 命名空间 */
    Oid key_owner;                  /* 所有者ID */
    int kms_type;                   /* KMS类型（1=localkms, 2=hsmkms, 3=cloudkms, 4=swxakms） */
    char *kms_path;                 /* KMS路径 */
    int key_length;                 /* 密钥长度（位） */
    int algorithm;                  /* 加密算法（1=AES_256_CBC, 2=SM4_CBC等） */
    time_t create_date;             /* 创建时间 */
    time_t modify_date;             /* 修改时间 */
    int status;                     /* 状态（1=active, 2=inactive, 3=revoked） */
    int hit_times;                  /* 命中次数（用于LRU替换） */

    /* 缓存状态（含负缓存标记） */
    TeccCacheItemStatus cache_status;   /* ENCRYPTED/NOT_FOUND/PERMISSION_DENIED */

    /* 缓存完整性验证 */
    unsigned char hmac_signature[32];  /* HMAC-SHA256签名（256位） */
    time_t signature_time;             /* 签名生成时间 */

    /* ReqID:TECC4.0 — KEK明文缓存，首次从KMS加载后填充，避免重复文件I/O+PBKDF2 */
    unsigned char *plain_kek;          /* 缓存的明文KEK数据 */
    size_t         plain_kek_length;   /* 明文KEK长度（字节） */

    /* 注意：移除了冗余字段 key_store, key_path, kek_algorithm, plain_key_value */
} TeccKekCacheItem;

/*
 * tecc_cache_check_column_dek_permission - 检查并缓存列DEK权限
 *
 * 检查当前用户是否拥有指定加密列的DEK USAGE权限。
 * 结果缓存在列缓存项中，避免重复查询。
 *
 * 参数:
 *   state - TECC状态结构
 *   table_name - 表名称
 *   column_name - 列名称
 *
 * 返回:
 *   true表示有权限，false表示无权限或检查失败
 */
extern bool tecc_cache_check_column_dek_permission(TeccState *state,
                                                   const char *schema_name,
                                                   const char *table_name,
                                                   const char *column_name);

/* DEK缓存项结构（简化版） */
typedef struct TeccDekCacheItem {
    Oid dek_oid;                    /* DEK对象标识符 */
    char *key_name;                 /* DEK名称 */
    Oid column_key_distributed_id;  /* 跨实例一致性DEK身份（hash of schema.dek_name） */
    Oid kek_oid;                    /* 关联KEK OID */
    char *kek_name;                 /* 关联KEK名称（动态分配） */
    char *key_namespace;            /* 命名空间 */
    Oid key_owner;                  /* 所有者ID */
    unsigned char *encrypted_key;   /* 加密的密钥数据（二进制格式） */
    size_t key_length;              /* 密钥长度（字节） */
    time_t create_date;             /* 创建时间 */
    time_t modify_date;             /* 修改时间 */
    int status;                     /* 状态（1=active, 2=inactive, 3=revoked） */
    int hit_times;                  /* 命中次数（用于LRU替换） */

    /* 缓存状态（含负缓存标记） */
    TeccCacheItemStatus cache_status;   /* ENCRYPTED/NOT_FOUND/PERMISSION_DENIED */

    /* 缓存完整性验证 */
    unsigned char hmac_signature[32];  /* HMAC-SHA256签名（256位） */
    time_t signature_time;             /* 签名生成时间 */

    /* ReqID:TECC4.0 — DEK明文缓存，首次解密后填充，避免重复SM4-CBC解密 */
    unsigned char *plain_dek;          /* 缓存的解密后DEK数据 */
    size_t         plain_dek_length;   /* 解密后DEK长度（字节） */

    /* 注意：移除了冗余字段 encrypted_value, support_algo, plain_key_value */
} TeccDekCacheItem;

/* 加密列缓存项结构（基于简化设计） */
typedef struct TeccColumnCacheItem {
    /* 表信息 */
    Oid table_oid;                  /* 表对象标识符 */
    char *table_name;               /* 表名称 */
    Oid schema_oid;                 /* 模式对象标识符 */
    char *schema_name;              /* 模式名称 */
    Oid database_oid;               /* 数据库对象标识符 */
    char *database_name;            /* 数据库名称 */

    /* 列信息 */
    int column_attnum;              /* 列号（从1开始） */
    char *column_name;              /* 列名称 */
    Oid original_type_oid;          /* 原始数据类型OID */
    int original_type_mod;          /* 原始数据类型修饰符 */

    /* 加密信息 */
    Oid dek_id;                     /* 关联DEK ID */
    char *dek_name;                 /* DEK名称 */
    /* ReqID:TECC4.0 - 一列多 DEK：HE 加法/乘法 DEK 名（纯 HE 列 dek_name 为 NULL） */
    char *he_add_dek_name;          /* HE 加法 DEK 名称（HE_ADD_DEK） */
    char *he_mul_dek_name;          /* HE 乘法 DEK 名称（HE_MUL_DEK） */
    int encryption_type;            /* 加密类型位掩码 */

    /* 排序规则信息 */
    Oid collation_oid;              /* 排序规则OID */
    char *collation_locale;         /* Collation locale (动态分配), e.g. "en_US.UTF-8" */

    /* DEK权限缓存 */
    bool has_dek_permission;        /* 当前用户是否有该列DEK的USAGE权限 */
    bool dek_permission_checked;    /* 是否已经检查过DEK权限 */
    TeccCacheItemStatus cache_status;   /* 缓存项状态：ENCRYPTED/NOT_ENCRYPTED/PENDING */

    /* 元数据 */
    time_t create_date;             /* 创建时间 */
    int hit_times;                  /* 命中次数（用于LRU替换） */

    /* 缓存完整性验证 */
    unsigned char hmac_signature[32];  /* HMAC-SHA256签名（256位） */
    time_t signature_time;             /* 签名生成时间 */
} TeccColumnCacheItem;

/* 可恢复错误类型位掩码 */
typedef enum {
    TECC_CACHE_RECOVERABLE_NONE = 0,            /* 无错误可恢复 */
    TECC_CACHE_RECOVERABLE_CONNECTION = 1 << 0, /* 连接错误可恢复 */
    TECC_CACHE_RECOVERABLE_QUERY = 1 << 1,      /* 查询错误可恢复 */
    TECC_CACHE_RECOVERABLE_TIMEOUT = 1 << 2,    /* 超时错误可恢复 */
    TECC_CACHE_RECOVERABLE_PERMISSION = 1 << 3, /* 权限错误可恢复 */
    TECC_CACHE_RECOVERABLE_ALL = 0x0F           /* 所有可恢复错误 */
} TeccCacheRecoverableErrors;

/* 缓存管理器结构 */
typedef struct TeccCacheManager {
    PGconn *conn;                   /* 数据库连接上下文 */

    /* OID查找哈希表 */
    FeHashTable *kek_cache;         /* KEK缓存哈希表（OID查找） */
    FeHashTable *dek_cache;         /* DEK缓存哈希表（OID查找） */
    FeHashTable *column_cache;      /* 加密列缓存哈希表（OID查找） */

    /* 名称查找哈希表 */
    FeHashTable *kek_name_cache;    /* KEK缓存哈希表（名称查找） */
    FeHashTable *dek_name_cache;    /* DEK缓存哈希表（名称查找） */
    FeHashTable *column_name_cache; /* 加密列缓存哈希表（名称查找） */
    FeHashTable *table_schema_cache; /* 未限定表名→schema 映射缓存（search_path 变更时重建） */

    int max_cache_size;             /* 最大缓存大小 */
    int current_size;               /* 当前缓存大小 */
    time_t last_sync_time;          /* 最后同步时间 */
    int load_count;                 /* 加载次数 */
    void *lock;                     /* 互斥锁（兼容性保留） */
    pthread_rwlock_t global_rwlock; /* 全局读写锁（保护统计信息等） */

    /* 缓存统计信息 */
    unsigned long cache_hits;       /* OID查找缓存命中次数 */
    unsigned long cache_misses;     /* OID查找缓存未命中次数 */
    unsigned long name_cache_hits;  /* 名称查找缓存命中次数 */
    unsigned long name_cache_misses; /* 名称查找缓存未命中次数 */

    /* GUC参数缓存 */
    struct {
        char *search_path;                 /* search_path参数（原始字符串） */
        char *sql_compatibility;           /* sql_compatibility参数 */
        char *current_schema;              /* current_schema参数 */
        bool standard_conforming_strings;  /* standard_conforming_strings参数 */
        char backslash_quote;              /* backslash_quote参数 */
        bool escape_string_warning;        /* escape_string_warning参数 */
        char *role;                        /* role参数 */
        /* 解析后的search_path，用于按PG原生规则匹配schema */
        char **search_path_schemas;        /* 解析后的schema名称列表 */
        int search_path_schema_count;      /* schema列表长度 */
    } guc_params;

    /* 状态标志 */
    bool initialized;               /* 缓存管理器是否已初始化 */
    bool guc_loaded;                /* GUC参数是否已加载 */

    /* 缓存完整性验证 */
    unsigned char hmac_key[32];            /* HMAC密钥（256位） */
    bool integrity_enabled;                /* 是否启用完整性验证 */

    /* 错误恢复状态（连接特定，避免全局变量线程安全问题） */
    TeccCacheRecoverableErrors recoverable_errors_mask;  /* 可恢复错误掩码 */
    unsigned int random_seed;                            /* 随机数种子 */

    /* 缓存刷新状态标记 */
    bool kek_cache_needs_refresh;        /* KEK缓存需要刷新标记 */
    bool dek_cache_needs_refresh;        /* DEK缓存需要刷新标记 */
    bool column_cache_needs_refresh;     /* 加密列缓存需要刷新标记 */

    /* 定时刷新配置 */
    time_t last_timed_refresh_time;      /* 上次定时刷新时间 */
    int timed_refresh_interval_sec;      /* 定时刷新间隔（秒），0表示禁用 */
    bool timed_refresh_enabled;          /* 定时刷新是否启用 */

    /* 元数据标记检查配置 */
    time_t last_metadata_check_time;     /* 上次元数据标记检查时间 */
    int metadata_check_interval_sec;     /* 元数据标记检查间隔（秒），0表示每次查询都检查 */
    bool metadata_check_enabled;         /* 元数据标记检查是否启用 */
    bool check_in_progress;              /* 标记检查进行中，防止递归调用 */
} TeccCacheManager;

/* ==================== 类型安全缓存接口 ==================== */

/* 缓存对象类型别名，便于未来扩展和类型安全 */
typedef TeccGlobalKeyMetadata TeccCacheGlobalKey;
typedef TeccColumnKeyMetadata TeccCacheColumnKey;
typedef TeccEncryptedColumnMetadata TeccCacheEncryptedColumn;

/* 缓存查询结果类型别名（常量指针，防止修改缓存内容） */
typedef const TeccGlobalKeyMetadata *TeccCacheGlobalKeyResult;
typedef const TeccColumnKeyMetadata *TeccCacheColumnKeyResult;
typedef const TeccEncryptedColumnMetadata *TeccCacheEncryptedColumnResult;

/* ==================== 基于简化设计的新接口 ==================== */

/* 初始化接口 */
int tecc_cache_init(TeccState *state);
void tecc_cache_cleanup(TeccState *state);

/* 查询接口 */
TeccColumnCacheItem *tecc_cache_get_encrypted_column(const TeccState *state, Oid table_oid, int column_attnum);
TeccColumnCacheItem *tecc_cache_get_encrypted_column_by_name(const TeccState *state,
                                                             const char *database_name,
                                                             const char *schema_name,
                                                             const char *table_name,
                                                             const char *column_name);
TeccColumnCacheItem **tecc_cache_get_columns_by_table(const TeccState *state, Oid table_oid, int *count);
TeccColumnCacheItem **tecc_cache_get_columns_by_table_name(TeccState *state,
                                                           const char *database_name,
                                                           const char *schema_name,
                                                           const char *table_name,
                                                           int *count);
/* ReqID:TECC4.0 - Pending refresh sentinel management for post-DDL cache loading */
int tecc_cache_insert_pending(TeccState *state, const char *database_name,
                               const char *schema_name, const char *table_name);
int tecc_cache_remove_pending(TeccState *state, const char *database_name,
                               const char *schema_name, const char *table_name);
/* ReqID:TECC4.0 - Remove any table-level name sentinel (PENDING/NOT_ENCRYPTED/TABLE_LOADED) */
int tecc_cache_remove_name_sentinel(TeccState *state, const char *database_name,
                                    const char *schema_name, const char *table_name);
/* ReqID:TECC4.0 - Public wrapper to insert a single column cache item into formal cache */
int tecc_cache_insert_column_item(TeccState *state, TeccColumnCacheItem *item);

/* KEK/DEK查询接口 */
TeccKekCacheItem *tecc_cache_get_kek_by_oid(const TeccState *state, Oid kek_oid);
TeccKekCacheItem *tecc_cache_get_kek_by_name(const TeccState *state, const char *key_name, const char *key_namespace);
TeccDekCacheItem *tecc_cache_get_dek_by_oid(const TeccState *state, Oid dek_oid);
TeccDekCacheItem *tecc_cache_get_dek_by_name(const TeccState *state, const char *key_name, const char *key_namespace);

TeccKekCacheItem *tecc_cache_get_kek_by_name_search_path(const TeccState *state, const char *key_name);
TeccDekCacheItem *tecc_cache_get_dek_by_name_search_path(const TeccState *state, const char *key_name);

/* ReqID:TECC4.0 - 通过分布式ID查找DEK名称（用于表达式结果解密） */
extern const char *tecc_cache_get_dek_name_by_distributed_id(const TeccState *state, Oid dist_id);

/* 管理接口 */
int tecc_cache_refresh(const TeccState *state, TeccCacheRefreshType refresh_type);
int tecc_refresh_cache(const TeccState *state);  /* 简化接口：刷新所有缓存 */
void tecc_cache_clear(const TeccState *state);
int tecc_cache_clear_specific(TeccCacheManager *manager, TeccCacheRefreshType cache_type);

/* 定时刷新接口 */
int tecc_cache_set_timed_refresh_interval(TeccState *state, int interval_sec);
int tecc_cache_enable_timed_refresh(TeccState *state, bool enable);
int tecc_cache_get_timed_refresh_status(TeccState *state,
                                        bool *enabled,
                                        int *interval_sec,
                                        time_t *last_refresh_time);
void perform_timed_refresh_if_needed(TeccCacheManager *manager);

/* 缓存管理器访问接口 */
TeccCacheManager *tecc_cache_get_manager(const TeccState *state);

/* 缓存刷新标记接口（延迟刷新：仅在下次查询时清空并重新加载） */
void tecc_cache_mark_kek_refresh(const TeccState *state);
void tecc_cache_mark_dek_refresh(const TeccState *state);

/* 全局缓存管理器已移除，改为连接特定管理 */

/* 获取缓存管理器函数（连接特定） */
TeccCacheManager *get_cache_manager(const TeccState *state);

/* GUC参数管理接口 */
int tecc_cache_load_guc_params(const TeccState *state);
const char *tecc_cache_get_guc_param(const TeccState *state, const char *param_name);
int tecc_cache_update_guc_param(const TeccState *state, const char *param_name, const char *param_value);

/* search_path解析接口 — 按PG原生规则匹配未指定schema的表名 */
void tecc_cache_parse_search_path(TeccCacheManager *manager, PGconn *conn);
extern const char **tecc_cache_get_search_path_schemas(const TeccState *state, int *count);
extern void tecc_cache_update_search_path(struct TeccState *state, const char *new_search_path);

/* ReqID:TECC4.0 - 未限定表名解析：返回表在 search_path 下的真实 schema。
 * 命中缓存返回缓存内指针（语句处理期间使用，search_path 变更时会重建失效）；
 * 表不在 search_path 任何 schema 或查询失败时返回 NULL（查询失败不写负缓存）。 */
extern const char *tecc_cache_get_table_schema(const TeccState *state,
                                               const char *database_name,
                                               const char *table_name);

/* 统计接口 */
typedef struct TeccCacheStats {
    unsigned long cache_hits;           /* OID查找缓存命中次数 */
    unsigned long cache_misses;         /* OID查找缓存未命中次数 */
    unsigned long name_cache_hits;      /* 名称查找缓存命中次数 */
    unsigned long name_cache_misses;    /* 名称查找缓存未命中次数 */
    unsigned long load_count;           /* 加载次数 */
    unsigned long current_size;         /* 当前缓存大小 */
    unsigned long max_size;             /* 最大缓存大小 */
} TeccCacheStats;

TeccCacheStats tecc_cache_get_stats(PGconn *conn);

/* 公开的缓存项插入/删除接口 */
int tecc_cache_insert_kek_item(TeccState *state, TeccKekCacheItem *item);
int tecc_cache_insert_dek_item(TeccState *state, TeccDekCacheItem *item);
void tecc_cache_remove_pending_kek(TeccState *state, const char *key_name, const char *key_namespace);
void tecc_cache_remove_pending_dek(TeccState *state, const char *key_name, const char *key_namespace);

/* ==================== 缓存项内存管理函数 ==================== */

/* 释放KEK缓存项内存 */
void tecc_cache_free_kek_item(TeccKekCacheItem *kek_item);

/* 释放DEK缓存项内存 */
void tecc_cache_free_dek_item(TeccDekCacheItem *dek_item);

/* 释放加密列缓存项内存 */
void tecc_cache_free_column_item(TeccColumnCacheItem *column_item);

/* 批量释放加密列缓存项数组内存 */
void tecc_cache_free_column_items(TeccColumnCacheItem **column_items, int count);

/* ==================== 缓存模块锁操作宏 ==================== */

/* 锁操作宏（简化使用） */
#define CACHE_LOCK(manager) \
    do { \
        if ((manager) != NULL && (manager)->lock != NULL) { \
            pthread_mutex_lock((pthread_mutex_t *)(manager)->lock); \
        } \
    } while(0)

#define CACHE_UNLOCK(manager) \
    do { \
        if ((manager) != NULL && (manager)->lock != NULL) { \
            pthread_mutex_unlock((pthread_mutex_t *)(manager)->lock); \
        } \
    } while(0)

#define CACHE_TRY_LOCK(manager) \
    ((manager) != NULL && (manager)->lock != NULL ? \
     pthread_mutex_trylock((pthread_mutex_t *)(manager)->lock) == 0 : false)

/* ==================== 缓存管理器读写锁操作宏 ==================== */

/*
 * TECC frontend is single-threaded (psql main loop), so rwlocks are not
 * needed for mutual exclusion.  The global_rwlock is kept in the struct
 * for initialization/destruction symmetry but all lock/unlock macros
 * are no-ops to avoid recursive-write and other self-deadlock scenarios.
 *
 * If multi-threaded frontend support is added later, re-enable these
 * with a proper trylock + upgrade scheme (no write-lock-while-read-lock).
 */
#define CACHE_RLOCK(manager)        ((void)0)
#define CACHE_WLOCK(manager)        ((void)0)
#define CACHE_RWUNLOCK(manager)     ((void)0)


/* 便捷日志宏 */
#define TECC_CACHE_ERROR(format, ...) \
    TECC_LOG(TECC_LOG_ERROR, "[CACHE ERROR] %s:%d: " format, PG_FUNCNAME_MACRO, __LINE__, ##__VA_ARGS__)

/* 包含通用日志系统 */
#ifndef TECC_LOGGING_H
#include "../tecc_common/tecc_logging.h"
#endif

#define TECC_CACHE_WARN(format, ...) \
    TECC_LOG(TECC_LOG_WARNING, "[CACHE WARN] %s:%d: " format, PG_FUNCNAME_MACRO, __LINE__, ##__VA_ARGS__)

#define TECC_CACHE_INFO(format, ...) \
    TECC_LOG(TECC_LOG_INFO, "[CACHE INFO] %s:%d: " format, PG_FUNCNAME_MACRO, __LINE__, ##__VA_ARGS__)

#define TECC_CACHE_DEBUG(format, ...) \
    TECC_LOG(TECC_LOG_DEBUG1, "[CACHE DEBUG] %s:%d: " format, PG_FUNCNAME_MACRO, __LINE__, ##__VA_ARGS__)

/* 带错误码的日志宏 */
#define TECC_CACHE_LOG_ERROR_CODE(error_code, format, ...) \
    do { \
        const char *error_msg = tecc_cache_error_message(error_code); \
        TECC_LOG(TECC_LOG_ERROR, "[CACHE ERROR] %s:%d: " format " (错误码: %d, 描述: %s)", \
                      PG_FUNCNAME_MACRO, __LINE__, ##__VA_ARGS__, error_code, error_msg); \
    } while(0)

/* ==================== 错误恢复和重试机制接口 ==================== */

/* 重试配置结构 */
typedef struct {
    int max_retries;                    /* 最大重试次数 */
    int base_delay_ms;                  /* 基础延迟时间（毫秒） */
    int max_delay_ms;                   /* 最大延迟时间（毫秒） */
    bool exponential_backoff;           /* 是否使用指数退避 */
    bool jitter_enabled;                /* 是否启用抖动 */
} TeccCacheRetryConfig;

/* 默认重试配置 - C90兼容版本 */
#define TECC_CACHE_DEFAULT_RETRY_CONFIG_INIT(max_r, base_d, max_d, exp_b, jitter_e) \
    { max_r, base_d, max_d, exp_b, jitter_e }

/* 默认重试配置值 */
#define TECC_CACHE_DEFAULT_MAX_RETRIES 3
#define TECC_CACHE_DEFAULT_BASE_DELAY_MS 100
#define TECC_CACHE_DEFAULT_MAX_DELAY_MS 5000
#define TECC_CACHE_DEFAULT_EXPONENTIAL_BACKOFF true
#define TECC_CACHE_DEFAULT_JITTER_ENABLED true

/* 错误恢复和重试函数声明 */
bool tecc_cache_is_recoverable_error(const TeccState *state, int error_code);
int tecc_cache_retry_with_recovery(const TeccState *state,
                                   int (*operation)(const TeccState *state, void *arg),
                                   void *operation_arg,
                                   const TeccCacheRetryConfig *config);
int tecc_cache_recover_connection(const TeccState *state);
int tecc_cache_warmup_after_recovery(const TeccState *state);
void tecc_cache_set_recoverable_errors_mask(const TeccState *state, TeccCacheRecoverableErrors mask);
TeccCacheRecoverableErrors tecc_cache_get_recoverable_errors_mask(const TeccState *state);

/* 异常状态自动恢复函数声明 */
int tecc_cache_auto_recover(const TeccState *state);

/* ==================== 缓存项转换函数 ==================== */

/*
 * tecc_convert_column_cache_item_to_metadata - 将列缓存项转换为加密列元数据
 *
 * 参数:
 *   state - TECC状态结构
 *   cache_item - 列缓存项指针
 *
 * 返回:
 *   转换后的加密列元数据指针，NULL表示转换失败
 *
 * 注意:
 *   调用者负责释放返回的TeccEncryptedColumnMetadata结构
 *   使用tecc_free_encrypted_column_metadata函数释放
 */
TeccEncryptedColumnMetadata *tecc_convert_column_cache_item_to_metadata(
    const TeccState *state,
    const TeccColumnCacheItem *cache_item);

/*
 * tecc_free_encrypted_column_metadata - 释放加密列元数据
 * 注意：此函数在数据获取器模块中定义，缓存模块使用该实现
 *
 * 参数:
 *   metadata - 要释放的加密列元数据指针
 */
/* void tecc_free_encrypted_column_metadata(TeccEncryptedColumnMetadata *metadata); */

/* ==================== 安全内存清理函数 ==================== */

/*
 * tecc_cache_secure_clear_memory - 安全清理内存中的敏感数据
 *
 * 参数:
 *   ptr - 要清理的内存指针
 *   size - 要清理的内存大小
 *
 * 功能:
 *   - 使用volatile指针确保编译器不会优化清零操作
 *   - 多次清零确保数据被彻底清除
 *   - 防止敏感数据残留在内存中
 */
void tecc_cache_secure_clear_memory(void *ptr, size_t size);

/*
 * tecc_cache_secure_clear_dek_key - 安全清理DEK加密密钥数据
 *
 * 参数:
 *   dek_item - DEK缓存项指针
 *
 * 功能:
 *   - 安全清理DEK中的加密密钥数据
 *   - 清理所有敏感字段
 */
void tecc_cache_secure_clear_dek_key(TeccDekCacheItem *dek_item);

/*
 * tecc_cache_secure_clear_kek_metadata - 安全清理KEK元数据
 *
 * 参数:
 *   kek_item - KEK缓存项指针
 *
 * 功能:
 *   - 安全清理KEK中的敏感元数据
 *   - 清理KMS路径等敏感信息
 */
void tecc_cache_secure_clear_kek_metadata(TeccKekCacheItem *kek_item);

/* ==================== 缓存完整性验证函数 ==================== */

/*
 * tecc_cache_generate_hmac_key - 生成HMAC密钥
 *
 * 参数:
 *   state - TECC状态
 *   manager - 缓存管理器
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int tecc_cache_generate_hmac_key(const TeccState *state, TeccCacheManager *manager);

/*
 * tecc_cache_sign_item - 为缓存项生成HMAC签名
 *
 * 参数:
 *   manager - 缓存管理器
 *   item - 缓存项指针
 *   item_type - 缓存项类型（1=KEK, 2=DEK, 3=Column）
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int tecc_cache_sign_item(TeccCacheManager *manager, void *item, int item_type);

/*
 * tecc_cache_verify_item - 验证缓存项的HMAC签名
 *
 * 参数:
 *   manager - 缓存管理器
 *   item - 缓存项指针
 *   item_type - 缓存项类型（1=KEK, 2=DEK, 3=Column）
 *
 * 返回:
 *   0表示签名有效，非0表示签名无效或错误
 */
int tecc_cache_verify_item(TeccCacheManager *manager, void *item, int item_type);

/*
 * tecc_cache_enable_integrity - 启用缓存完整性验证
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int tecc_cache_enable_integrity(const TeccState *state);

/*
 * tecc_cache_disable_integrity - 禁用缓存完整性验证
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int tecc_cache_disable_integrity(const TeccState *state);

/*
 * tecc_metadata_cache_get_encrypted_column_by_name - 兼容性接口：按名称获取加密列元数据
 *
 * 参数:
 *   table_name - 表名称
 *   column_name - 列名称
 *
 * 返回:
 *   加密列元数据指针，NULL表示未找到
 */
TeccCacheEncryptedColumnResult tecc_metadata_cache_get_encrypted_column_by_name(const char *table_name, const char *column_name);

/* ==================== 元数据更新标记检查函数 ==================== */

/*
 * check_metadata_update_mark - 检查TECC元数据更新标记
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   TECC_CACHE_OK - 成功
 *   TECC_CACHE_ERROR - 失败
 */
int check_metadata_update_mark(const TeccState *state);

/*
 * should_check_metadata_mark - 判断是否需要检查元数据标记
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   true - 需要检查
 *   false - 不需要检查
 */
bool should_check_metadata_mark(const TeccCacheManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* TECC_METADATA_CACHE_H */
/* End - ReqID:TECC4.0 */
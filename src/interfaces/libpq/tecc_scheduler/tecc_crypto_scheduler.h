/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_crypto_scheduler.h
 *
 * Abstract:
 *   Unified crypto scheduler interface for TECC processor module.
 *   Provides unified encryption/decryption interfaces for all algorithms.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_crypto_scheduler.h
 */

#ifndef TECC_CRYPTO_SCHEDULER_H
#define TECC_CRYPTO_SCHEDULER_H

#include "postgres_fe.h"
#include "c.h"
#include "../tecc_common/tecc_common_types.h"
/* GUC parameter definitions are now handled through cache interface */
/* expr_guc_params.h has been removed */
#include "tecc_crypto_context.h"  /* Crypto context definitions */
#include "tecc_key_manager_adapter.h"  /* Key manager adapter definitions */
#include "../../../common/tecc_types.h"  /* For TeccCryptoErrorCode definitions */

/* Forward declaration for PGconn */
struct pg_conn;
typedef struct pg_conn PGconn;

/* Forward declaration for crypto buffer */
typedef struct TeccCryptoBuffer TeccCryptoBuffer;

/* Unified encryption/decryption result structures */
typedef struct TeccUnifiedEncryptResult
{
    TeccCryptoBuffer *cipherdata_buffer;  /* Buffer containing CipherDataType */
    char *hex_string;                     /* Hex-encoded cipherdata */
    size_t hex_len;                       /* Length of hex string */
    int error_code;                       /* Error code (TECC_CRYPTO_SUCCESS on success) */
    char error_msg[256];                  /* Error message */
} TeccUnifiedEncryptResult;

typedef struct TeccUnifiedDecryptResult
{
    char *plaintext;                      /* Decrypted plaintext */
    size_t plain_len;                     /* Length of plaintext */
    bool skipped_decrypt;                 /* true when no DEK permission, plaintext is hex */
    int error_code;                       /* Error code (TECC_CRYPTO_SUCCESS on success) */
    char error_msg[256];                  /* Error message */
} TeccUnifiedDecryptResult;

/*
 * Unified encryption interface
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_string(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccCryptoContext *external_context
);

/* Note: tecc_unified_encrypt_string now includes connection parameter for thread safety */

/*
 * Schema-aware unified encryption — for DML literal encryption on
 * schema-qualified tables (e.g. sch1.tb1). Pass the target table's real
 * schema so the scheduler's column-metadata lookup doesn't fall back to
 * search_path (which misses tables outside it). NULL schema behaves like
 * tecc_unified_encrypt_string (search_path resolution).
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_schema(
    PGconn *conn,
    const char *schema_name,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccCryptoContext *external_context
);

/*
 * ReqID:TECC4.0 - Unified encryption with explicit algorithm options.
 * For scenarios where column metadata is not in the cache (e.g. CREATE TABLE
 * DEFAULT encryption before the table exists). The caller specifies the
 * base data type OID, type modifier, and which algorithms to use directly.
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_options(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    bool supports_he_add,
    bool supports_he_mul,
    TeccCryptoContext *external_context
);

/*
 * ReqID:TECC4.0 - 同 tecc_unified_encrypt_string_with_options，但由调用方显式
 * 给出该列将来生效的 collation locale（如 "zh_CN.utf8"）。
 *
 * 元数据缓存里查不到列时（典型场景就是 CREATE TABLE ... DEFAULT：表还没建），
 * 上面的函数拿不到 collation_locale，DET/OPE 归一化会跳过 ICU sort key，
 * 于是同一个明文在建表路径与 INSERT/WHERE 路径生成互不相等的密文 ——
 * DEFAULT 插入的行被等值查询静默漏掉。建表路径用本变体把后端即将写入
 * hg_encrypted_columns_info.collation_locale 的那个字符串显式传进来。
 *
 * collation_locale 为 NULL 或空串时行为与不支持 collation 时完全一致。
 *
 * ReqID:TECC4.0 - 同理，he_add_dek_name/he_mul_dek_name 是调用方从同一条
 * ENCRYPTED WITH 里解析出的同态 DEK 名（HE_ADD_DEK/HE_MUL_DEK）。这类写入路径
 * 恰好也拿不到 hg_encrypted_column_args，同态 DEK 没有别的来源；不传的话
 * det+he-add 这类混合列会拿对称 DEK 去找 EC 公钥而失败，报
 * "per-DEK EC public key not loaded"。传 NULL 表示该列没有对应同态 DEK。
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_options_and_collation(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    bool supports_he_add,
    bool supports_he_mul,
    const char *collation_locale,
    const char *he_add_dek_name,
    const char *he_mul_dek_name,
    TeccCryptoContext *external_context
);

/*
 * Unified decryption interface
 * Note: This function now includes connection parameter for thread safety
 */
extern TeccUnifiedDecryptResult *tecc_unified_decrypt_hex(
    PGconn *conn,
    const char *hex_string,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context
);

/* ReqID:TECC4.0 - Decrypt raw algorithm segment hex for tecc_decrypt_raw(). */
extern TeccUnifiedDecryptResult *tecc_decrypt_raw_segment_hex(
	PGconn *conn,
	const char *hex_string,
	const char *algorithm,
	const char *schema_name,
	const char *table_name,
	const char *column_name,
	TeccCryptoContext *external_context
);

/* Begin - ReqID:TECC4.0 */
/*
 * Batch hex decrypt — decrypts multiple hex strings for the same column
 * at once. Shares SM4 context and DEK lookup across all items.
 * Returns array of count result pointers; each must be freed via
 * tecc_unified_free_decrypt_result(). Returns NULL on fatal error.
 */
extern TeccUnifiedDecryptResult **tecc_unified_batch_decrypt_hex(
    PGconn *conn,
    const char **hex_strings,
    size_t count,
    const char *schema_name,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context
);
/* End - ReqID:TECC4.0 */


/*
 * Internal function declarations (defined in tecc_crypto_scheduler.c)
 * Note: These are static functions in tecc_crypto_scheduler.c, not exported
 */

/* static TeccAlgorithmSupport tecc_unified_get_algorithm_support_internal(
    const char *table_name,
    const char *column_name); */

/*
 * Get algorithm support for column
 */
extern TeccAlgorithmSupport tecc_unified_get_algorithm_support(
    PGconn *conn,
    const char *table_name,
    const char *column_name
);

/* Note: tecc_unified_get_algorithm_support now includes connection parameter for thread safety */


/*
 * Extended encryption with collation support and connection parameter (thread-safe version)
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_collation_and_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    TeccCryptoContext *external_context
);

/*
 * Unified encryption for WHERE clause comparisons.
 * Encrypts plaintext using the algorithm appropriate for the operator type:
 *   - = / <> : DET (for equality comparison)
 *   - < / > / <= / >= : OPE (for range comparison)
 *   - LIKE / ILIKE : SEARCH (for pattern matching)
 * Only encrypts the required algorithm, not RND (RND is for storage, not comparison).
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_for_where(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *schema_name,
    const char *kek_name,
    const char *dek_name,
    TeccOperatorType operator_type,
    char like_escape_char,
    TeccCryptoContext *external_context
);

/*
 * ReqID:TECC4.0 - Homomorphic constant encryption for scalar operations.
 * Encrypts an integer constant with the column's HE public key and returns a
 * minimal CipherDataType (hex) carrying ONLY the EG_ADD slot (use_he_add=true,
 * EC-ElGamal) or the EG_MUL slot (use_he_add=false, finite field ElGamal),
 * including the per-segment DEK distributed ID prefix.
 *
 * The caller rewrites `enc + k` / `enc * k` into `enc + 'hex'::intN_enc`, so
 * the server resolves the generic ciphertext×ciphertext HE operators
 * (he_add_generic / he_sub_generic / he_mul_generic) and the plaintext
 * constant never leaves the client.  Returns NULL on failure (e.g. column is
 * not an HE column, HE DEK unresolvable, or HE algorithm missing).
 */
extern TeccUnifiedEncryptResult *tecc_unified_encrypt_he_constant(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *schema_name,
    bool use_he_add
);

/*
 * Crypto scheduler management
 */
typedef struct TeccMetadataCache TeccMetadataCache;
typedef struct TeccKeyManager TeccKeyManager;

/* Forward declaration for metadata types (actual definitions in tecc_metadata_cache.h) */
typedef struct TeccEncryptedColumnMetadata TeccEncryptedColumnMetadata;

typedef struct TeccCryptoScheduler
{
    /* Module references */
    TeccMetadataCache *metadata_cache;  /* Metadata cache reference */
    void *key_manager;                  /* Key manager reference */

    /* Configuration */
    bool debug_mode;                    /* Debug mode */
    int max_retry_count;                /* Maximum retry count */

    /* GUC parameter integration */
    const TeccGucParams *guc_params; /* Current GUC parameters */
    bool guc_enabled;                   /* Whether GUC processing is enabled */

    /* Statistics */
    int encryption_count;               /* Encryption operation count */
    int decryption_count;               /* Decryption operation count */
    int error_count;                    /* Error count */

    /* Batch processing statistics */
    int batch_encryption_count;         /* Batch encryption operation count */
    int batch_decryption_count;         /* Batch decryption operation count */
    int batch_error_count;              /* Batch error count */
    size_t total_batch_items_processed; /* Total items processed in batches */
    size_t successful_batch_items;      /* Successful items in batches */

    /* Key manager adapter */
    TeccKeyManagerAdapter *key_adapter; /* Key manager adapter */
} TeccCryptoScheduler;

/*
 * Crypto scheduler management
 */
extern TeccCryptoScheduler *tecc_crypto_scheduler_init(TeccMetadataCache *cache, TeccKeyManagerAdapter *key_mgr);
extern void tecc_crypto_scheduler_cleanup(TeccCryptoScheduler *scheduler);
extern void tecc_crypto_scheduler_set_debug(TeccCryptoScheduler *scheduler, bool enable);
extern void tecc_crypto_scheduler_get_stats(TeccCryptoScheduler *scheduler,
                                           int *enc_count, int *dec_count, int *err_count);

/* Get batch processing statistics */
extern void tecc_crypto_scheduler_get_batch_stats(TeccCryptoScheduler *scheduler,
                                                int *batch_enc_count,
                                                int *batch_dec_count,
                                                int *batch_error_count,
                                                size_t *total_batch_items,
                                                size_t *successful_batch_items);

/*
 * ReqID:TECC4.0 - 连接级 HSM 初始化锁（挂在 TeccState.hsm_init_lock 上）
 *
 * 只用于串行化 tecc_setup_hsm_context 的首次初始化段（见该函数内说明），
 * 不影响软加密路径。create 返回 0 成功；失败（内存/互斥量不可用）返回 -1，
 * 调用方应降级继续（锁为 NULL 时不加锁）。destroy 幂等。
 */
extern int tecc_hsm_init_lock_create(void *tecc_state);
extern void tecc_hsm_init_lock_destroy(void *tecc_state);

/*
 * Lazy initialization - ensure scheduler is initialized
 */
extern bool tecc_crypto_scheduler_ensure_initialized(void);

/*
 * Cleanup global scheduler instance
 */
extern void tecc_crypto_scheduler_cleanup_global(void);

/*
 * Memory management for result structures
 */
extern void tecc_unified_free_encrypt_result(TeccUnifiedEncryptResult *result);
extern void tecc_unified_free_decrypt_result(TeccUnifiedDecryptResult *result);

/*
 * Data validation interface
 */
extern int tecc_crypto_scheduler_validate_input_data(
    const unsigned char *data,
    size_t data_len,
    Oid data_type,
    TeccAlgorithmType algo_type,
    char *error_msg,
    size_t error_msg_size
);

/*
 * GUC parameter integration for scheduler module
 */

/* Set GUC parameters for scheduler */
extern void tecc_crypto_scheduler_set_guc_params(TeccCryptoScheduler *scheduler,
                                                const TeccGucParams *guc_params);

/* Get current GUC parameters */
extern const TeccGucParams *tecc_crypto_scheduler_get_guc_params(
    const TeccCryptoScheduler *scheduler);

/* Enable/disable GUC processing */
extern void tecc_crypto_scheduler_set_guc_enabled(TeccCryptoScheduler *scheduler,
                                                 bool enabled);

/* Check if GUC processing is enabled */
extern bool tecc_crypto_scheduler_is_guc_enabled(const TeccCryptoScheduler *scheduler);


/* Apply GUC parameters to data before encryption */
extern int tecc_crypto_scheduler_apply_guc_to_data(TeccCryptoScheduler *scheduler,
                                                  TeccCryptoContext *context);

/*
 * Self test
 */
extern int tecc_crypto_scheduler_self_test(void);

/* ==================== Batch Processing Interfaces ==================== */
/* Begin - ReqID:TECC4.0 - Batch processing interfaces for import/export module */

/* Batch encryption result structure */
typedef struct TeccBatchEncryptResult {
    TeccCryptoBuffer **cipherdata_buffers;  /* 密文数据缓冲区数组 */
    char **hex_strings;                     /* 十六进制字符串数组 */
    size_t count;                           /* 处理的数据项数量 */
    int *error_codes;                       /* 每个数据项的加密错误码 */
    int overall_error_code;                 /* 整体错误码 */
    char overall_error_msg[256];            /* 整体错误消息 */
} TeccBatchEncryptResult;

/* Batch decryption result structure */
typedef struct TeccBatchDecryptResult {
    char **plaintexts;                      /* 解密后的明文数组 */
    size_t *plaintext_lengths;              /* 明文长度数组 */
    size_t count;                           /* 处理的数据项数量 */
    int *error_codes;                       /* 每个数据项的解密错误码 */
    int overall_error_code;                 /* 整体错误码 */
    char overall_error_msg[256];            /* 整体错误消息 */
} TeccBatchDecryptResult;

/* Memory pool structure for stream processing */
typedef struct TeccStreamMemoryPool {
    char **buffers;                         /* 缓冲区数组 */
    size_t *buffer_sizes;                   /* 缓冲区大小数组 */
    int buffer_count;                       /* 缓冲区数量 */
    int max_buffers;                        /* 最大缓冲区数 */
    size_t total_allocated;                 /* 总分配内存 */

    /* 性能统计字段 */
    size_t total_allocations;               /* 总分配次数 */
    size_t total_allocation_size;           /* 总分配大小 */
    size_t peak_allocated;                  /* 峰值分配内存 */
    size_t cache_hits;                      /* 缓存命中次数 */
    size_t cache_misses;                    /* 缓存未命中次数 */
    struct timeval creation_time;           /* 创建时间 */
} TeccStreamMemoryPool;

/* Stream encryption context structure */
typedef struct TeccStreamCryptoContext {
    TeccCryptoContextExtended *base_context;  /* 基础加密上下文 */
    TeccStreamMemoryPool *stream_pool;        /* 流式处理专用内存池 */
    size_t processed_count;                   /* 已处理数据项计数 */
    bool initialized;                         /* 上下文是否已初始化 */
    int algorithm_type;                       /* 算法类型 */
    char *key_id;                             /* 密钥ID */
    Oid collation_oid;                        /* 排序规则OID */
    char collation_locale[NAMEDATALEN];       /* 排序规则locale字符串 */
    TeccOperatorType operator_type;           /* 操作符类型（SEARCH算法） */
    TeccCryptoScheduler *scheduler;           /* 加密调度器实例（连接级，避免全局变量） */

    /* 性能统计字段 */
    struct timeval start_time;                /* 开始处理时间 */
    struct timeval last_operation_time;       /* 最后操作时间 */
    size_t total_encrypted_bytes;             /* 总加密字节数 */
    size_t total_decrypted_bytes;             /* 总解密字节数 */
    size_t encryption_count;                  /* 加密操作次数 */
    size_t decryption_count;                  /* 解密操作次数 */
    size_t batch_operations;                  /* 批量操作次数 */
    size_t single_operations;                 /* 单次操作次数 */
    double avg_encryption_time;               /* 平均加密时间（毫秒） */
    double avg_decryption_time;               /* 平均解密时间（毫秒） */
    size_t memory_saved;                      /* 内存节省量（字节） */
} TeccStreamCryptoContext;

/* Batch encryption interface */
extern TeccBatchEncryptResult *tecc_unified_batch_encrypt_strings(
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name
);

/* Batch encryption interface with connection parameter (thread-safe version) */
extern TeccBatchEncryptResult *tecc_unified_batch_encrypt_strings_with_conn(
    PGconn *conn,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name
);

/* Batch decryption interface */
extern TeccBatchDecryptResult *tecc_unified_batch_decrypt_hex_strings(
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name
);

/* Batch decryption interface with connection parameter (thread-safe version) */
extern TeccBatchDecryptResult *tecc_unified_batch_decrypt_hex_strings_with_conn(
    PGconn *conn,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name
);

/* Create stream encryption context */
extern TeccStreamCryptoContext *tecc_create_stream_crypto_context(
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name
);

/* Create stream encryption context with connection parameter (thread-safe version) */
extern TeccStreamCryptoContext *tecc_create_stream_crypto_context_with_conn(
    PGconn *conn,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name
);

/* Stream encryption of single data item */
extern TeccUnifiedEncryptResult *tecc_stream_encrypt_data(
    TeccStreamCryptoContext *context,
    const char *plaintext,
    size_t plaintext_len
);

/* Stream decryption of single data item */
extern TeccUnifiedDecryptResult *tecc_stream_decrypt_data(
    TeccStreamCryptoContext *context,
    const char *hex_string,
    size_t hex_len
);

/* Batch encryption using stream context */
extern TeccBatchEncryptResult *tecc_stream_batch_encrypt_data(
    TeccStreamCryptoContext *context,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count
);

/* Batch decryption using stream context */
extern TeccBatchDecryptResult *tecc_stream_batch_decrypt_data(
    TeccStreamCryptoContext *context,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count
);

/* Set algorithm type for stream context */
extern int tecc_stream_set_algorithm_type(
    TeccStreamCryptoContext *context,
    int algorithm_type
);

/* Set collation for stream context */
extern int tecc_stream_set_collation(
    TeccStreamCryptoContext *context,
    Oid collation_oid
);

/* Set operator type for stream context (SEARCH algorithm) */
extern int tecc_stream_set_operator_type(
    TeccStreamCryptoContext *context,
    TeccOperatorType operator_type
);

/* Reset stream context (for reuse) */
extern int tecc_stream_reset_context(
    TeccStreamCryptoContext *context
);

/* Destroy stream context */
extern void tecc_stream_destroy_context(
    TeccStreamCryptoContext *context
);

/* Free batch encryption result */
extern void tecc_unified_free_batch_encrypt_result(
    TeccBatchEncryptResult *result
);

/* Free batch decryption result */
extern void tecc_unified_free_batch_decrypt_result(
    TeccBatchDecryptResult *result
);

/* Get batch processing statistics (deprecated - use thread-safe version) */
extern int tecc_get_batch_processing_stats(
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item
);

/* Get batch processing statistics with connection parameter (thread-safe version) */
extern int tecc_get_batch_processing_stats_with_conn(
    PGconn *conn,
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item
);

/* Get detailed batch error statistics */
extern int tecc_get_batch_error_details(
    PGconn *conn,
    int *total_errors,
    int *invalid_param_errors,
    int *memory_errors,
    int *format_errors,
    int *system_errors,
    int *batch_errors
);

/* Optimize batch size based on performance metrics */
extern size_t tecc_optimize_batch_size(
    size_t data_size,
    int algorithm_type,
    size_t available_memory
);

/* Smart batch size optimization with performance history */
extern size_t tecc_smart_optimize_batch_size(
    PGconn *conn,
    size_t data_size,
    int algorithm_type,
    size_t available_memory
);

/* ==================== Stream Memory Pool Management Interfaces ==================== */
/* Begin - ReqID:TECC4.0 - Stream memory pool management interfaces for import/export module */

/* Create stream memory pool */
extern TeccStreamMemoryPool *tecc_stream_memory_pool_create(int max_buffers);

/* Allocate buffer from stream memory pool */
extern char *tecc_stream_memory_pool_alloc(TeccStreamMemoryPool *pool, size_t size);

/* Destroy stream memory pool */
extern void tecc_stream_memory_pool_destroy(TeccStreamMemoryPool *pool);

/* End - ReqID:TECC4.0 - Stream memory pool management interfaces for import/export module */

/* ==================== Stream Performance Statistics Interfaces ==================== */
/* Begin - ReqID:TECC4.0 - Stream performance statistics interfaces for import/export module */

/* Get stream context performance statistics */
extern int tecc_stream_get_performance_stats(
    TeccStreamCryptoContext *context,
    size_t *total_processed,
    size_t *encryption_count,
    size_t *decryption_count,
    size_t *batch_operations,
    size_t *single_operations,
    double *avg_encryption_time,
    double *avg_decryption_time,
    size_t *memory_saved
);

/* Get memory pool statistics */
extern int tecc_stream_get_memory_pool_stats(
    TeccStreamCryptoContext *context,
    size_t *total_allocations,
    size_t *total_allocation_size,
    size_t *peak_allocated,
    size_t *cache_hits,
    size_t *cache_misses,
    double *cache_hit_rate
);

/* Reset stream context statistics */
extern int tecc_stream_reset_statistics(
    TeccStreamCryptoContext *context
);

/* Update stream context performance statistics after operation */
extern int tecc_stream_update_performance_stats(
    TeccStreamCryptoContext *context,
    bool is_encryption,
    size_t data_size,
    double operation_time_ms,
    bool is_batch
);

/* Compare stream vs standard performance */
extern int tecc_stream_compare_performance(
    TeccStreamCryptoContext *stream_context,
    size_t standard_operation_count,
    double standard_total_time_ms,
    double *performance_improvement,
    double *memory_saving_ratio
);

/* Generate performance report */
extern char *tecc_stream_generate_performance_report(
    TeccStreamCryptoContext *context
);

/* End - ReqID:TECC4.0 - Stream performance statistics interfaces for import/export module */

#endif /* TECC_CRYPTO_SCHEDULER_H */
/* End - ReqID:TECC4.0 */

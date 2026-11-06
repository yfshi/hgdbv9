/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_common_types.h
 *
 * Abstract:
 *   Common type definitions for TECC module.
 *   Provides unified type definitions for all TECC modules to avoid conflicts.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common_types.h
 */

#ifndef TECC_COMMON_TYPES_H
#define TECC_COMMON_TYPES_H

/* PostgreSQL forward declarations */
#ifndef Oid
typedef unsigned int Oid;
#endif

/* PGconn forward declaration for libpq connection */
struct pg_conn;
typedef struct pg_conn PGconn;

#ifndef bool
typedef unsigned char bool;
#define true 1
#define false 0
#endif

#ifndef NAMEDATALEN
#define NAMEDATALEN 64
#endif

/* Standard integer types */
#include <stddef.h>  /* For size_t */
#include <stdint.h>
#include <sys/time.h>  /* For struct timeval */
#include <time.h>      /* For time_t */
#ifndef int32
typedef int32_t int32;
#endif

/* Include shared algorithm types */
#include "../../../common/tecc_types.h"

/* Include shared encrypted type OID definitions (generated from pg_type.dat) */
#include "../../../backend/catalog/tecc_enc_type_d.h"

/* Operator type enumeration for SEARCH algorithm */
/* Note: TeccOperatorType is defined in common/tecc_types.h */

/* Include frontend parser types for List definition */
#include "../frontend_parser/tecc_nodes.h"

/* GUC parameter definitions are now handled through cache interface */
/* expr_guc_params.h has been removed */

/* Begin - ReqID:TECC4.0 - 复杂参数处理和硬编码值加密常量定义 */

/* Encryption type constants for column metadata */
typedef enum TeccEncryptionType
{
    ENCRYPTION_TYPE_NONE = 0,      /* No encryption */
    ENCRYPTION_TYPE_RND = 1,       /* SM4-RND algorithm */
    ENCRYPTION_TYPE_DET = 2,       /* DET-SM4 algorithm */
    ENCRYPTION_TYPE_OPE = 3,       /* SM4-OPE algorithm */
    ENCRYPTION_TYPE_SEARCH = 4     /* SEARCH algorithm */
} TeccEncryptionType;

/* Value type enumeration for parameter and constant processing */
typedef enum TeccValueType
{
    VALUE_TYPE_UNKNOWN = 0,
    VALUE_TYPE_INTEGER,
    VALUE_TYPE_FLOAT,
    VALUE_TYPE_STRING,
    VALUE_TYPE_BOOLEAN,
    VALUE_TYPE_NULL,
    VALUE_TYPE_BINARY
} TeccValueType;

/* End - ReqID:TECC4.0 - 复杂参数处理和硬编码值加密常量定义 */

/* Begin - ReqID:TECC4.0 - TeccGucParams结构体定义（简化版本，与openGauss保持一致） */

/*
 * TeccGucParams structure
 * Simplified version based on openGauss GucParams structure
 * Only contains the 5 parameters actually used by openGauss:
 * 1. role - current role name
 * 2. standard_conforming_strings - standard conforming strings
 * 3. backslash_quote - backslash quote handling mode
 * 4. escape_string_warning - escape string warning
 * 5. searchpath - search path string
 */
typedef struct TeccGucParams
{
    /* Role parameter */
    char        role[64];                   /* Current role name */

    /* String processing parameters */
    bool        standard_conforming_strings; /* Standard conforming strings */
    int         backslash_quote;            /* Backslash quote handling mode */
    bool        escape_string_warning;      /* Escape string warning */

    /* Search path parameter */
    char        searchpath[1024];           /* Search path string */
} TeccGucParams;

/* End - ReqID:TECC4.0 - TeccGucParams结构体定义 */

/* Begin - ReqID:TECC4.0 - TeccExprGucParams结构体定义（完整定义） */

/*
 * TeccExprGucParams structure
 * Expression GUC parameters for data formatting
 * This is the actual definition (expr_guc_params.h has been removed)
 */
typedef struct TeccExprGucParams
{
    int placeholder;  /* Placeholder field */
} TeccExprGucParams;

/* End - ReqID:TECC4.0 - TeccExprGucParams结构体定义 */



/* Algorithm type is now defined in tecc_types.h */
/* TeccAlgorithmSupport is also defined in tecc_types.h */

/* Complete buffer type enumeration - Combined from scheduler and formatter */
typedef enum TeccBufferType
{
    /* Core buffer types (used by formatter) */
    TECC_BUFFER_TYPE_INPUT = 0,
    TECC_BUFFER_TYPE_OUTPUT,
    TECC_BUFFER_TYPE_WORK,

    /* Extended buffer types (used by scheduler) */
    TECC_BUFFER_NORMALIZED,         /* Normalized data */
    TECC_BUFFER_ENCRYPTED,          /* Encrypted data (RND) */
    TECC_BUFFER_ENCRYPTED_DET,      /* Encrypted data (DET) */
    TECC_BUFFER_ENCRYPTED_OPE,      /* Encrypted data (OPE) */
    TECC_BUFFER_ENCRYPTED_SEARCH,   /* Encrypted data (SEARCH) */
    TECC_BUFFER_CIPHERDATA,         /* CipherDataType structure */

    /* Buffer count definitions */
    TECC_BUFFER_TYPE_MAX = 4,       /* For formatter compatibility */
    TECC_BUFFER_MAX = 10            /* For scheduler compatibility */
} TeccBufferType;

/* Buffer descriptor - Compatible definition */
typedef struct TeccBufferDesc
{
    TeccBufferType type;            /* Buffer type */
    void *data;                     /* Data pointer */
    size_t size;                    /* Data size */
    size_t capacity;                /* Buffer capacity */
    bool in_use;                    /* Whether in use */
} TeccBufferDesc;

/* Common OID definitions for data types */
#define BOOLOID 16
#define INT2OID 21
#define INT4OID 23
#define INT8OID 20
#define FLOAT4OID 700
#define FLOAT8OID 701
#define NUMERICOID 1700
#define VARCHAROID 1043
#define BPCHAROID 1042
#define TEXTOID 25

/* Core crypto context structure - Minimal fields for formatter */
typedef struct TeccCryptoContextCore
{
    Oid data_type;                     /* Data type OID */
    int type_mod;                      /* Type modifier (for NUMERIC typmod) */
    Oid collation_oid;                 /* Collation OID (for character types) */
    char collation_locale[NAMEDATALEN];/* Collation locale (e.g. "en_US.UTF-8") */
    TeccAlgorithmType algo_type;       /* Algorithm type (for formatter functions) */
    TeccAlgorithmSupport algo_support; /* Algorithm support flags */
    int selected_algorithms;           /* Selected algorithms (bit flags) */
    void *buffer_manager;              /* Buffer manager pointer (for future use) */
    TeccBufferDesc buffers[4];         /* Basic buffer array (compatible with formatter) */
} TeccCryptoContextCore;

/* Extended crypto context structure - Full fields for scheduler */
typedef struct TeccCryptoContextExtended
{
    /* Core context (must be first field for compatibility) */
    TeccCryptoContextCore core;

    /* Extended identification fields */
    char table_name[NAMEDATALEN];      /* Table name */
    char column_name[NAMEDATALEN];     /* Column name */
    char kek_name[NAMEDATALEN];        /* KEK name */
    char dek_name[NAMEDATALEN];        /* DEK name */

    /* Input data */
    const unsigned char *plaintext;    /* Plaintext data pointer */
    size_t plaintext_len;              /* Plaintext data length */

    /* Output data */
    void *cipherdata;                  /* Ciphertext data structure */

    /* Key information */
    unsigned char *dek_key;            /* Decrypted DEK */
    size_t dek_key_len;                /* DEK length */

    /* Algorithm-specific data */
    unsigned char *rnd_ciphertext;     /* RND ciphertext */
    size_t rnd_cipher_len;             /* RND ciphertext length */
    unsigned char *det_ciphertext;     /* DET ciphertext */
    size_t det_cipher_len;             /* DET ciphertext length */
    unsigned char *ope_ciphertext;     /* OPE ciphertext */
    size_t ope_cipher_len;             /* OPE ciphertext length */
    unsigned char *search_ciphertext;  /* SEARCH ciphertext */
    size_t search_cipher_len;          /* SEARCH ciphertext length */

    /* Status information */
    int error_code;                    /* Error code */
    char error_msg[256];               /* Error message */

    /* GUC parameter integration */
    const TeccGucParams *guc_params; /* GUC parameters for data formatting */
    bool guc_processing_enabled;       /* Whether GUC processing is enabled */

    /* Operator type extension for SEARCH algorithm interface */
    TeccOperatorType operator_type;    /* Operator type for algorithm selection */
    bool is_like_pattern;              /* Whether this is a LIKE pattern */
    void *parsed_pattern;              /* Parsed pattern structure (for SEARCH algorithm) */

    /* Extended buffer management */
    TeccBufferDesc ext_buffers[6];     /* Extended buffers (beyond the basic 4) */
    unsigned char *work_buffer;        /* Work buffer (64KB pre-allocated) */
    size_t work_buffer_size;           /* Work buffer size */
    bool buffer_reuse_enabled;         /* Buffer reuse switch */

    /* HSM SM4 encryption support */
    void *hsm_adapter;                 /* HsmAdapterInterface pointer (NULL = use software SM4) */
    void *hsm_device;                  /* HsmDevice pointer for HSM session */
    int hsm_kms_type;                  /* KMS type: 0=none, 2=hsmkms, 4=swxakms */

    /* ReqID:TECC4.0 - Pre-normalized data (computed once, shared by all algorithms) */
    unsigned char *normalized_plaintext;     /* Normalized plaintext (owned by context) */
    size_t normalized_plaintext_len;         /* Normalized plaintext length */
    bool data_pre_normalized;                /* Flag: data already normalized */

    /* ReqID:TECC4.0 - Text trailing space accounting for encrypted comparison.
     * After rtrim, space_count records how many trailing spaces were removed.
     * 0 means no trailing spaces (format_byte=0x00). >0 means trailing spaces
     * existed (format_byte=0x01). Only meaningful for text types
     * (VARCHAROID/BPCHAROID/TEXTOID) with DET/OPE algorithms. */
    uint32_t text_space_count;
} TeccCryptoContextExtended;

/* Type alias for backward compatibility */
/* Formatter modules should use TeccCryptoContextCore */
/* Scheduler modules should use TeccCryptoContextExtended */

/* Memory allocation functions - use pg_malloc/pg_free from fe_memutils.h for frontend code */
/* Note: tecc_processor_palloc/pfree functions are deprecated and should not be used */
/* Frontend code must use pg_malloc()/pg_free() from fe_memutils.h */

/* Utility macros for type conversion */
#define TECC_CONTEXT_CORE(context) ((TeccCryptoContextCore *)(context))
#define TECC_CONTEXT_EXTENDED(context) ((TeccCryptoContextExtended *)(context))

/* Check if context is extended type */
#define TECC_IS_EXTENDED_CONTEXT(context) \
    ((context) != NULL && sizeof(*(context)) >= sizeof(TeccCryptoContextExtended))

/* Buffer access macros with type safety */
#define TECC_GET_BUFFER(context, buffer_type) \
    (TECC_IS_EXTENDED_CONTEXT(context) ? \
     ((buffer_type) < TECC_BUFFER_TYPE_MAX ? \
      &TECC_CONTEXT_EXTENDED(context)->core.buffers[buffer_type] : \
      (buffer_type) < TECC_BUFFER_MAX ? \
      &TECC_CONTEXT_EXTENDED(context)->ext_buffers[(buffer_type) - TECC_BUFFER_TYPE_MAX] : \
      NULL) : \
     ((buffer_type) < TECC_BUFFER_TYPE_MAX ? \
      &TECC_CONTEXT_CORE(context)->buffers[buffer_type] : \
      NULL))

/* FLEXIBLE_ARRAY_MEMBER for C90 compatibility */
#ifndef FLEXIBLE_ARRAY_MEMBER
#define FLEXIBLE_ARRAY_MEMBER  /* empty */
#endif

/* Crypto buffer management for efficient memory sharing between modules */
typedef struct TeccCryptoBuffer
{
    void *data;                     /* Buffer data */
    size_t size;                    /* Buffer size */
    size_t used;                    /* Used portion of buffer */
    bool owned_by_caller;           /* Whether caller owns the buffer */
    struct TeccCryptoBuffer *next;  /* For buffer chaining */
} TeccCryptoBuffer;

/* Include backend CipherDataType definition for consistency */
#include "utils/cipherdatatype.h"

/* Parsed cipherdata structure */
typedef struct TeccParsedCipherdata
{
    /* Version information */
    int32 version;

    /* Algorithm data pointers and lengths - corresponds to backend access macros */
    const unsigned char *rnd_data;      /* RND ciphertext data pointer */
    size_t rnd_data_len;                /* RND ciphertext length */
    const unsigned char *det_data;      /* DET ciphertext data pointer */
    size_t det_data_len;                /* DET ciphertext length */
    const unsigned char *ope_data;      /* OPE ciphertext data pointer */
    size_t ope_data_len;                /* OPE ciphertext length */
    const unsigned char *search_data;   /* SEARCH ciphertext data pointer */
    size_t search_data_len;             /* SEARCH ciphertext length */
} TeccParsedCipherdata;

/* ==================== Raw Value Data Structures ==================== */

/* Raw value types */
typedef enum TeccRawValueType
{
    TECC_RAW_VALUE_TYPE_UNKNOWN = 0,
    TECC_RAW_VALUE_TYPE_BOOLEAN,
    TECC_RAW_VALUE_TYPE_INTEGER,
    TECC_RAW_VALUE_TYPE_FLOAT,
    TECC_RAW_VALUE_TYPE_STRING,
    TECC_RAW_VALUE_TYPE_BINARY,
    TECC_RAW_VALUE_TYPE_NULL
} TeccRawValueType;

typedef enum TeccRawValueState
{
    TECC_RAW_VALUE_STATE_ORIGINAL = 0,
    TECC_RAW_VALUE_STATE_PROCESSED,
    TECC_RAW_VALUE_STATE_ENCRYPTED,
    TECC_RAW_VALUE_STATE_DECRYPTED
} TeccRawValueState;

/* Raw value structure */
typedef struct TeccRawValue
{
    TeccRawValueType type;
    TeccRawValueState state;

    /* Original value data */
    union {
        bool boolean_value;
        int64_t integer_value;
        double float_value;
        char *string_value;
        void *binary_value;
    } original;

    /* Processed value data */
    union {
        char *encrypted_value;
        char *decrypted_value;
        void *processed_value;
    } processed;

    /* Value metadata */
    size_t length;
    char *schema_name;          /* Schema name */
    char *table_name;           /* Table name */
    char *column_name;          /* Column name */
    char *column_type;          /* Column type */
    Oid type_oid;               /* Type OID */

    /* Processing flags */
    bool needs_encryption;
    bool needs_decryption;
    bool is_null;
    bool is_default;            /* ReqID:TECC4.0 - DEFAULT keyword placeholder */

    /* SQL rewrite support */
    bool is_param;              /* Whether this is a parameter */
    int param_index;            /* Parameter index for parameterized queries */
    size_t location;            /* Original position in SQL string */
    size_t data_size;           /* Size of original data in SQL */
    size_t position_offset;     /* Position offset after rewrite */
    int data_value_format;      /* 0=text, 1=binary */
    unsigned char *data_value;  /* Data without quotes/formatting */
    size_t data_value_size;     /* Size of data_value */
    bool processed_data_needs_free; /* Whether processed_data needs to be freed */
} TeccRawValue;

/* List type is defined in tecc_nodes.h */

/* Raw values list structure */
typedef struct TeccRawValuesList
{
    TeccRawValue **values;
    int num_values;
    int max_values;
    size_t total_size;
    List *values_list;  /* PostgreSQL List of TeccRawValue pointers */
    int last_error;     /* Last error code */
} TeccRawValuesList;

/* ==================== Statement Type Definitions ==================== */

/* Statement type enumeration */
typedef enum TeccStatementType
{
    TECC_STMT_TYPE_UNKNOWN = 0,
    TECC_STMT_TYPE_SELECT,
    TECC_STMT_TYPE_INSERT,
    TECC_STMT_TYPE_UPDATE,
    TECC_STMT_TYPE_DELETE,
    TECC_STMT_TYPE_CREATE_KEK,
    TECC_STMT_TYPE_CREATE_DEK,
    TECC_STMT_TYPE_CREATE_TABLE,
    TECC_STMT_TYPE_ALTER_TABLE,
    TECC_STMT_TYPE_DROP,
    TECC_STMT_TYPE_COPY,
    TECC_STMT_TYPE_PREPARE,      /* Added for prepared statement handling */
    TECC_STMT_TYPE_EXECUTE,       /* Added for EXECUTE statement handling */
    TECC_STMT_TYPE_DECLARE_CURSOR, /* Added for DECLARE CURSOR statement handling */
    TECC_STMT_TYPE_EXPLAIN         /* EXPLAIN statement */
} TeccStatementType;

/* ==================== Parameter Mapping Structure ==================== */

/* Parameter-to-column mapping entry */
typedef struct TeccParamMapEntry
{
    int param_index;                    /* Parameter index (0-based) */
    char *column_name;                  /* Column name this parameter corresponds to */
    char *table_name;                   /* Table name */
    char *schema_name;                  /* Schema name */
    bool needs_encryption;              /* Whether this parameter needs encryption */
    char *encrypted_value;              /* Encrypted parameter value (if encryption was performed) */
    size_t encrypted_value_size;        /* Size of encrypted value */

    /* Begin - ReqID:TECC4.0 - 增强复杂参数处理支持 */
    /* Expression context information for complex parameter handling */
    void *expression_context;           /* Expression context where parameter appears (opaque pointer) */
    int expression_depth;               /* Depth in expression tree */
    Oid inferred_type_oid;              /* Inferred type OID */
    bool is_in_function_call;           /* Whether parameter is in function call */
    bool is_in_subquery;                /* Whether parameter is in subquery */
    bool is_in_op_expr;                 /* Whether parameter is in operator expression */
    /* End - ReqID:TECC4.0 - 增强复杂参数处理支持 */
} TeccParamMapEntry;

/* ==================== Constant Information Structure ==================== */

/* Begin - ReqID:TECC4.0 - 硬编码值自动加密支持 */

/* Constant information structure for hardcoded value encryption */
typedef struct TeccConstantInfo
{
    /* Basic constant information */
    void *constant_node;               /* Constant AST node (opaque pointer) */
    char *original_value;              /* Original value string */
    size_t value_length;               /* Value length */
    Oid type_oid;                      /* PostgreSQL type OID */
    TeccValueType value_type;          /* Value type enumeration */

    /* Context information */
    void *parent_expression;           /* Parent expression node (opaque pointer) */
    TeccOperatorType operator_type;    /* Operator type (if applicable) */
    bool is_in_where_clause;           /* Whether constant is in WHERE clause */
    bool is_in_function_arg;           /* Whether constant is in function argument */
    bool is_in_subquery;               /* Whether constant is in subquery */

    /* Encryption information */
    bool needs_encryption;             /* Whether constant needs encryption */
    char *encrypted_value;             /* Encrypted value */
    size_t encrypted_size;             /* Encrypted value size */
    const char *algorithm;             /* Encryption algorithm used */

    /* Column reference (if applicable) */
    char *column_name;                 /* Column name (if constant corresponds to a column) */
    char *table_name;                  /* Table name */
    char *schema_name;                 /* Schema name */
} TeccConstantInfo;

/* End - ReqID:TECC4.0 - 硬编码值自动加密支持 */

/* ==================== Statement Data Structure ==================== */

/* Statement data structure for SQL processing */
typedef struct TeccStatementData {
    /* Database connection */
    PGconn *conn;                   /* PostgreSQL connection pointer */

    /* Original query string */
    const char *query;
    char *original_query;          /* Copy of original query for reference */

    /* Parse tree - output of frontend parser, input to processor */
    List *parse_tree;

    /* Encrypted columns list - processor analysis result */
    List *encrypted_columns;

    /* Processing status */
    int needs_rewrite;              /* Whether SQL rewrite is needed */
    int fallback_did_encrypt;         /* PBE fallback already encrypted params */
    int where_needs_rewrite;        /* Whether WHERE clause rewrite is needed */
    int error_code;                 /* Error code */
    char error_message[256];        /* Error message buffer */
    char *rewritten_query;          /* Rewritten query string (if needed) */

    /* Parameter information (prepared statements) */
    size_t nParams;
    const uint32_t *paramTypes;
    /* Fix const correctness - should be const char *const * like openGauss */
    /* Processor should not modify paramValues directly, use other mechanism to return modified values */
    const char *const *paramValues; /* Parameter values array (const pointer to const char pointers) */
    const int *paramLengths;
    const int *paramFormats;

    /* Parameter-to-column mapping for encryption */
    TeccParamMapEntry *param_map;      /* Parameter mapping array */
    int param_map_size;                /* Size of parameter mapping array */

    /* Result information */
    int resultFormat;

    /* Raw values list for SQL rewriting */
    TeccRawValuesList *raw_values_list;

    /* Begin - ReqID:TECC4.0 - 硬编码值自动加密支持 */
    /* Constant information for hardcoded value encryption */
    TeccConstantInfo *constant_info;      /* Constant information array */
    int constant_info_size;               /* Size of constant information array */
    /* End - ReqID:TECC4.0 - 硬编码值自动加密支持 */

    /* Other context information */
    const char *stmtName;
    size_t offset;

    /* COPY相关字段 */
    void *copy_state;           /* COPY状态信息指针 */
    bool is_copy_command;       /* 是否为COPY命令 */
    void *copy_stmt;            /* COPY语句解析树指针 */

    /* CSV数据处理 */
    char *csv_buffer;           /* CSV缓冲区 */
    size_t csv_buffer_size;     /* CSV缓冲区大小 */
    size_t csv_buffer_offset;   /* CSV缓冲区偏移 */

    /* Begin - ReqID:TECC4.0 - GUC参数处理相关字段 */

    /* GUC参数处理器上下文 */
    void *guc_processor_ctx;    /* GUC参数处理器上下文指针 */

    /* GUC参数状态 */
    bool guc_params_applied;    /* GUC参数是否已应用到查询 */
    bool guc_params_synced;     /* GUC参数是否已从服务端同步 */
    bool guc_params_modified;   /* GUC参数是否已修改 */

    /* GUC参数缓存 */
    void *guc_params_cache;     /* GUC参数缓存指针 */

    /* SET语句处理状态 */
    bool is_set_statement;      /* 是否为SET语句 */
    char *set_param_name;       /* SET语句参数名 */
    char *set_param_value;      /* SET语句参数值 */

    /* End - ReqID:TECC4.0 - GUC参数处理相关字段 */

    /* Begin - ReqID:TECC4.0 - SEARCH算法操作符类型扩展字段 */

    /* Operator type for WHERE clause processing */
    TeccOperatorType operator_type;          /* Current operator type for WHERE clause */
    TeccOperatorType current_operator_type;  /* Alias for operator_type (backward compatibility) */

    /* Scenario type for algorithm selection */
    int scenario_type;                       /* Scenario type (INSERT, UPDATE, WHERE, etc.) */

    /* End - ReqID:TECC4.0 - SEARCH算法操作符类型扩展字段 */

    /* Begin - ReqID:TECC4.0 - Statement type field for post-processing */
    TeccStatementType stmt_type;            /* Statement type for post-processing logic */
    /* End - ReqID:TECC4.0 - Statement type field for post-processing */

    /* ReqID:TECC4.0 - WHERE processor result code from last walker run.
     * The walker callbacks convert non-SUCCESS to boolean (false), losing
     * the specific error code (e.g. MISSING_DET_OPE).  This field preserves
     * it so the caller can dispatch the correct error message. */
    int where_processor_result;

    /* ReqID:TECC4.0 - Multi-table context flag for strict alias checking.
     * Set to 1 by UPDATE FROM / DELETE USING processing when multiple
     * tables are involved. When set, where_is_encrypted_column_full
     * enforces strict alias/table name matching to prevent ambiguous
     * encrypted column resolution. */
    int multi_table_context;

    /* ReqID:TECC4.0 - DROP object type & behaviour for selective cache
     * clearing.  drop_remove_type stores the ObjectType (e.g.
     * OBJECT_TABLE, OBJECT_KEK, OBJECT_DEK); drop_behavior stores
     * DropBehavior (DROP_CASCADE / DROP_RESTRICT).  Negative means N/A. */
    int drop_remove_type;
    int drop_behavior;
    Oid drop_key_oid;               /* ReqID:TECC4.0 - OID of dropped KEK/DEK for cache invalidation */
} TeccStatementData;

/* ==================== COPY Data Structures ==================== */

/* Begin - ReqID:TECC4.0 - Enhanced COPY state structures for import/export module */

/* Forward declarations for metadata types */
typedef struct TeccEncryptedColumnMetadata TeccEncryptedColumnMetadata;
typedef struct TeccColumnKeyMetadata TeccColumnKeyMetadata;
typedef struct TeccGlobalKeyMetadata TeccGlobalKeyMetadata;

/* COPY错误代码枚举 */
typedef enum TeccCopyErrorCode {
    TECC_COPY_SUCCESS = 0,         /* 操作成功 */
    TECC_COPY_FILE_OPEN_ERROR,     /* 文件打开错误 */
    TECC_COPY_FILE_READ_ERROR,     /* 文件读取错误 */
    TECC_COPY_FILE_WRITE_ERROR,    /* 文件写入错误 */
    TECC_COPY_DB_CONN_ERROR,       /* 数据库连接错误 */
    TECC_COPY_DB_QUERY_ERROR,      /* 数据库查询错误 */
    TECC_COPY_DB_COPY_ERROR,       /* 数据库COPY命令错误 */
    TECC_COPY_ENCRYPTION_ERROR,    /* 加密错误 */
    TECC_COPY_DECRYPTION_ERROR,    /* 解密错误 */
    TECC_COPY_KEY_INVALID,         /* 密钥无效 */
    TECC_COPY_DATA_CORRUPTED,      /* 数据损坏 */
    TECC_COPY_OUT_OF_MEMORY,       /* 内存不足 */
    TECC_COPY_INTERRUPTED,         /* 用户中断 */
    TECC_COPY_PARSE_ERROR,         /* 命令解析错误 */
    TECC_COPY_SYNTAX_ERROR,        /* 语法错误 */
    TECC_COPY_PERMISSION_DENIED,   /* 权限拒绝 */
    TECC_COPY_DISK_FULL,           /* 磁盘空间不足 */
    TECC_COPY_TIMEOUT,             /* 操作超时 */
    TECC_COPY_NETWORK_ERROR,       /* 网络错误 */
    TECC_COPY_INTERNAL_ERROR       /* 内部错误 */
} TeccCopyErrorCode;

/* COPY错误信息结构体 */
typedef struct TeccCopyError {
    TeccCopyErrorCode error_code;  /* 错误代码 */
    char error_msg[256];           /* 错误消息 */
    int row_number;                /* 发生错误的行号 */
    int column_index;              /* 发生错误的列索引 */
    time_t timestamp;              /* 时间戳 */
} TeccCopyError;

/* COPY进度信息结构体 */
typedef struct TeccCopyProgress {
    uint64_t total_bytes;          /* 总字节数（0表示未知） */
    uint64_t processed_bytes;      /* 已处理字节数 */
    uint64_t total_rows;           /* 总行数（如果已知） */
    uint64_t processed_rows;       /* 已处理行数 */
    struct timeval start_time;     /* 开始时间 */
    struct timeval last_update;    /* 最后更新时间 */
    bool show_progress;            /* 是否显示进度条 */
    bool interactive;              /* 是否为交互式终端 */
    int terminal_width;            /* 终端宽度 */

    /* 统计信息 */
    double bytes_per_second;       /* 平均字节/秒 */
    double rows_per_second;        /* 平均行/秒 */
    uint64_t peak_memory;          /* 峰值内存使用 */
} TeccCopyProgress;

/* 增强的COPY状态结构体 - 基于字段加密方案 */
typedef struct TeccEnhancedCopyState {
    /* 基础状态 */
    char *table_name;              /* 表名 */
    char **column_names;           /* 列名数组 */
    int column_count;              /* 列数 */

    /* 表信息 */
    Oid table_oid;                 /* 表OID */
    char *schema_name;             /* 模式名 */

    /* 加密列详细信息 */
    TeccEncryptedColumnMetadata **encrypted_column_info;  /* 加密列信息数组 */
    int encrypted_column_count;    /* 加密列数量 */

    /* 列映射信息 */
    int *column_mapping;           /* 列映射（文件列→表列） */
    int mapping_count;             /* 映射数量 */

    /* 数据处理状态 */
    bool is_from_stdin;            /* 是否为COPY FROM STDIN */
    bool is_to_stdout;             /* 是否为COPY TO STDOUT */
    int current_row;               /* 当前处理行号 */
    char *row_buffer;              /* 行缓冲区 */
    size_t buffer_size;            /* 缓冲区大小 */
    size_t buffer_capacity;        /* 缓冲区容量 */

    /* 格式信息 */
    bool csv_mode;                 /* CSV格式 */
    char delimiter;                /* 分隔符 */
    char quote;                    /* 引号字符 */
    char escape;                   /* 转义字符 */
    bool header;                   /* 包含标题行 */
    char *null_string;             /* NULL字符串表示 */
    bool binary_mode;              /* 二进制模式 */

    /* 性能优化 */
    bool batch_processing;         /* 批量处理模式 */
    int batch_size;                /* 批量大小 */
    int max_batch_size;            /* 最大批量大小 */
    bool stream_optimized;         /* 流式优化启用 */

    /* 错误处理 */
    TeccCopyError *errors;         /* 错误信息数组 */
    int error_count;               /* 错误数量 */
    int max_errors;                /* 最大错误数 */
    bool stop_on_error;            /* 遇到错误时停止 */

    /* 进度信息 */
    TeccCopyProgress progress;     /* 进度信息 */

    /* 处理器上下文 */
    void *processor_ctx;           /* 处理器上下文 */
    void *crypto_ctx;              /* 加密上下文 */
    void *formatter_ctx;           /* 格式化器上下文 */

    /* 流式处理上下文 */
    void **stream_crypto_contexts; /* 流式加密上下文数组（每个加密列一个） */
    int stream_context_count;      /* 流式上下文数量 */
    bool stream_contexts_initialized; /* 流式上下文是否已初始化 */

    /* 内存管理 */
    void *memory_pool;             /* 内存池 */
    size_t memory_used;            /* 已使用内存 */
    size_t memory_limit;           /* 内存限制 */

    /* 连接信息 */
    PGconn *conn;                  /* 数据库连接 */

    /* 状态标志 */
    bool initialized;              /* 是否已初始化 */
    bool processing;               /* 是否正在处理 */
    bool completed;                /* 是否已完成 */
    bool cancelled;                /* 是否已取消 */
} TeccEnhancedCopyState;

/* 向后兼容的COPY状态结构体别名 */
typedef TeccEnhancedCopyState TeccCopyState;

/* End - ReqID:TECC4.0 - Enhanced COPY state structures for import/export module */

/* ==================== COPY State Management Functions ==================== */

/* Begin - ReqID:TECC4.0 - COPY state management function declarations */

/* 创建增强的COPY状态 */
extern TeccEnhancedCopyState *tecc_copy_state_create(PGconn *conn, const char *table_name,
                                                     const char *schema_name, Oid table_oid);

/* 初始化COPY状态 */
extern int tecc_copy_state_init(TeccEnhancedCopyState *copy_state, bool is_from_stdin,
                                bool is_to_stdout, bool csv_mode, char delimiter);

/* 设置COPY格式选项 */
extern int tecc_copy_state_set_format_options(TeccEnhancedCopyState *copy_state,
                                              char quote, char escape, bool header,
                                              const char *null_string, bool binary_mode);

/* 设置性能选项 */
extern int tecc_copy_state_set_performance_options(TeccEnhancedCopyState *copy_state,
                                                   bool batch_processing, int batch_size,
                                                   bool stream_optimized, size_t memory_limit);

/* 设置错误处理选项 */
extern int tecc_copy_state_set_error_options(TeccEnhancedCopyState *copy_state,
                                             int max_errors, bool stop_on_error);

/* 加载加密列信息 */
extern int tecc_copy_state_load_encrypted_columns(TeccEnhancedCopyState *copy_state);

/* 设置列映射 */
extern int tecc_copy_state_set_column_mapping(TeccEnhancedCopyState *copy_state,
                                              const int *mapping, int mapping_count);

/* 添加错误信息 */
extern int tecc_copy_state_add_error(TeccEnhancedCopyState *copy_state,
                                     TeccCopyErrorCode error_code, const char *error_msg,
                                     int row_number, int column_index);

/* 更新进度信息 */
extern int tecc_copy_state_update_progress(TeccEnhancedCopyState *copy_state,
                                           uint64_t processed_bytes, uint64_t processed_rows);

/* 开始进度跟踪 */
extern int tecc_copy_state_start_progress(TeccEnhancedCopyState *copy_state,
                                          uint64_t total_bytes, uint64_t total_rows,
                                          bool show_progress, bool interactive);

/* 结束进度跟踪 */
extern int tecc_copy_state_end_progress(TeccEnhancedCopyState *copy_state);

/* 分配行缓冲区 */
extern char *tecc_copy_state_alloc_row_buffer(TeccEnhancedCopyState *copy_state,
                                              size_t required_size);

/* 释放行缓冲区 */
extern void tecc_copy_state_free_row_buffer(TeccEnhancedCopyState *copy_state);

/* 重置COPY状态（用于重用） */
extern int tecc_copy_state_reset(TeccEnhancedCopyState *copy_state);

/* 销毁COPY状态 */
extern void tecc_copy_state_destroy(TeccEnhancedCopyState *copy_state);

/* 获取错误统计信息 */
extern int tecc_copy_state_get_error_stats(const TeccEnhancedCopyState *copy_state,
                                           int *total_errors, int *recoverable_errors,
                                           int *fatal_errors);

/* 获取性能统计信息 */
extern int tecc_copy_state_get_performance_stats(const TeccEnhancedCopyState *copy_state,
                                                 double *bytes_per_second,
                                                 double *rows_per_second,
                                                 uint64_t *peak_memory);

/* 检查COPY状态是否有效 */
extern bool tecc_copy_state_is_valid(const TeccEnhancedCopyState *copy_state);

/* 检查COPY状态是否可继续处理 */
extern bool tecc_copy_state_can_continue(const TeccEnhancedCopyState *copy_state);

/* End - ReqID:TECC4.0 - COPY state management function declarations */

/* ==================== Crypto Error Codes ==================== */

/* Scheduler error codes - defined in tecc_types.h */
/* typedef enum TeccCryptoErrorCode */
/* { */
/*     TECC_CRYPTO_SUCCESS = 0,              / * Success * / */
/*     TECC_CRYPTO_INVALID_PARAM = -1,       / * Invalid parameter * / */
/*     TECC_CRYPTO_KEY_ERROR = -2,           / * Key error * / */
/*     TECC_CRYPTO_ALGO_ERROR = -3,          / * Algorithm error * / */
/*     TECC_CRYPTO_MEMORY_ERROR = -4,        / * Memory error * / */
/*     TECC_CRYPTO_DATA_ERROR = -5,          / * Data error * / */
/*     TECC_CRYPTO_SCHEDULE_ERROR = -6,      / * Schedule error * / */
/*     TECC_CRYPTO_SYSTEM_ERROR = -7,        / * System error * / */
/*     TECC_CRYPTO_COLLATION_ERROR = -8,     / * Collation error * / */
/*     TECC_CRYPTO_FORMAT_ERROR = -9,        / * Format error * / */
/*     TECC_CRYPTO_CACHE_ERROR = -10         / * Cache error * / */
/* } TeccCryptoErrorCode; */

/* Formatter error codes are defined in tecc_types.h */
/* Use the definition from there to avoid duplication */

/* Error code conversion function declaration */
extern int tecc_formatter_error_to_scheduler_error(TeccFormatterErrorCode fmt_error);

/* ==================== COPY State Error Recovery Functions ==================== */

/* Begin - ReqID:TECC4.0 - COPY state error recovery function declarations */

/* 检查错误是否可恢复 */
extern bool tecc_copy_state_is_recoverable_error(TeccCopyErrorCode error_code);

/* 重置错误状态 */
extern int tecc_copy_state_reset_errors(TeccEnhancedCopyState *copy_state);

/* 带恢复的重试操作 */
extern int tecc_copy_state_retry_operation(TeccEnhancedCopyState *copy_state,
                                          int (*operation)(TeccEnhancedCopyState *, void *),
                                          void *operation_arg,
                                          int max_retries,
                                          int base_delay_ms);

/* End - ReqID:TECC4.0 - COPY state error recovery function declarations */

#endif /* TECC_COMMON_TYPES_H */
/* End - ReqID:TECC4.0 */

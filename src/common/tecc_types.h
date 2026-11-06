/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_types.h
 *
 * Abstract:
 *   Shared type definitions for TECC module.
 *   Provides unified type definitions for both frontend and backend code.
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_types.h
 */

#ifndef TECC_TYPES_H
#define TECC_TYPES_H

#include <stddef.h>
#include <stdint.h>

/* ==================== Basic Type Definitions ==================== */

/* Boolean type for C90 compatibility */
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#endif

/* Standard integer types */
#ifndef int32
typedef int32_t int32;
#endif

#ifndef uint32
typedef uint32_t uint32;
#endif

#ifndef uint64
typedef uint64_t uint64;
#endif

/* ==================== Algorithm Types ==================== */

/* Algorithm type enumeration */
typedef enum TeccAlgorithmType
{
    TECC_ALGO_RND = 1,     /* SM4-RND algorithm */
    TECC_ALGO_DET = 2,     /* DET-SM4 algorithm */
    TECC_ALGO_OPE = 3,     /* SM4-OPE algorithm */
    TECC_ALGO_SEARCH = 4,  /* SEARCH algorithm */
    TECC_ALGO_MAX
} TeccAlgorithmType;

/* Algorithm support structure */
typedef struct TeccAlgorithmSupport
{
    bool supports_rnd;     /* RND algorithm support */
    bool supports_det;     /* DET algorithm support */
    bool supports_ope;     /* OPE algorithm support */
    bool supports_search;  /* SEARCH algorithm support */
} TeccAlgorithmSupport;

/* ==================== Algorithm Error Codes ==================== */

/* Unified algorithm error codes */
typedef enum TeccAlgorithmErrorCode
{
    TECC_ALGO_SUCCESS = 0,                    /* Success */

    /* Parameter errors (1000-1099) */
    TECC_ALGO_INVALID_PARAM = 1001,          /* Invalid parameter */
    TECC_ALGO_NULL_POINTER = 1002,           /* Null pointer */
    TECC_ALGO_INVALID_ALGORITHM = 1003,      /* Invalid algorithm type */
    TECC_ALGO_INVALID_KEY_LENGTH = 1004,     /* Invalid key length */
    TECC_ALGO_INVALID_DATA_LENGTH = 1005,    /* Invalid data length */

    /* Memory errors (1100-1199) */
    TECC_ALGO_MEMORY_ERROR = 1101,           /* Memory allocation failed */
    TECC_ALGO_BUFFER_OVERFLOW = 1102,        /* Buffer overflow */

    /* Algorithm errors (1200-1299) */
    TECC_ALGO_ENCRYPTION_FAILED = 1201,      /* Encryption failed */
    TECC_ALGO_DECRYPTION_FAILED = 1202,      /* Decryption failed */
    TECC_ALGO_AUTHENTICATION_FAILED = 1203,  /* Authentication failed */
    TECC_ALGO_NOT_IMPLEMENTED = 1204,        /* Function not implemented */
    TECC_ALGO_OPERATION_NOT_SUPPORTED = 1205,/* Operation not supported */

    /* Data errors (1300-1399) */
    TECC_ALGO_DATA_FORMAT_ERROR = 1301,      /* Data format error */
    TECC_ALGO_DATA_TOO_LARGE = 1302,         /* Data too large */
    TECC_ALGO_DATA_CORRUPTED = 1303,         /* Data corrupted */

    /* System errors (1400-1499) */
    TECC_ALGO_SYSTEM_ERROR = 1401,           /* System error */
    TECC_ALGO_CRYPTO_LIBRARY_ERROR = 1402    /* Crypto library error */
} TeccAlgorithmErrorCode;

/* ==================== Operator Type Enumeration ==================== */

/* Operator type enumeration for SEARCH algorithm interface extension */
typedef enum TeccOperatorType
{
    /* ==================== Special Values ==================== */
    TECC_OP_UNKNOWN = 0,         /* Unknown or unspecified operator */

    /* ==================== Comparison Operators ==================== */
    TECC_OP_EQUAL = 1,           /* = equality comparison */
    TECC_OP_NOT_EQUAL,           /* <> or != inequality comparison */
    TECC_OP_LESS_THAN,           /* < less than */
    TECC_OP_GREATER_THAN,        /* > greater than */
    TECC_OP_LESS_EQUAL,          /* <= less than or equal */
    TECC_OP_GREATER_EQUAL,       /* >= greater than or equal */

    /* ==================== Pattern Matching Operators ==================== */
    TECC_OP_LIKE,                /* LIKE pattern matching */
    TECC_OP_NOT_LIKE,            /* NOT LIKE reverse pattern matching */
    TECC_OP_ILIKE,               /* ILIKE case-insensitive pattern matching */
    TECC_OP_NOT_ILIKE,           /* NOT ILIKE reverse case-insensitive pattern matching */

    /* ==================== Range and Set Operators ==================== */
    TECC_OP_BETWEEN,             /* BETWEEN operator */
    TECC_OP_NOT_BETWEEN,         /* NOT BETWEEN operator */
    TECC_OP_IN,                  /* IN operator */
    TECC_OP_NOT_IN,              /* NOT IN operator */

    /* ==================== Null Checking Operators ==================== */
    TECC_OP_IS_NULL,             /* IS NULL operator */
    TECC_OP_IS_NOT_NULL,         /* IS NOT NULL operator */
    TECC_OP_IS_DISTINCT_FROM,    /* IS DISTINCT FROM */
    TECC_OP_IS_NOT_DISTINCT_FROM,/* IS NOT DISTINCT FROM */

    /* ==================== Statement Scenario Types (not operators, but need distinction) ==================== */
    TECC_SCENARIO_INSERT_VALUE = 1000,      /* Value in INSERT statement */
    TECC_SCENARIO_UPDATE_VALUE,             /* New value in UPDATE statement */
    TECC_SCENARIO_DEFAULT_VALUE,            /* Default value (CREATE TABLE or ALTER TABLE) */
    TECC_SCENARIO_WHERE_CONDITION,          /* Value in WHERE condition (non-LIKE) */
    TECC_SCENARIO_LIKE_PATTERN,             /* Pattern parameter for LIKE operator */

    /* ==================== Special Algorithm Scenarios ==================== */
    TECC_ALGO_SCENARIO_SEARCH_PATTERN = 2000,   /* SEARCH algorithm pattern segmentation scenario */
    TECC_ALGO_SCENARIO_SEARCH_DATA              /* SEARCH algorithm data encryption scenario */
} TeccOperatorType;

/* ==================== Crypto Error Codes ==================== */

/* Scheduler error codes */
typedef enum TeccCryptoErrorCode
{
    TECC_CRYPTO_SUCCESS = 0,              /* Success */
    TECC_CRYPTO_INVALID_PARAM = -1,       /* Invalid parameter */
    TECC_CRYPTO_KEY_ERROR = -2,           /* Key error */
    TECC_CRYPTO_ALGO_ERROR = -3,          /* Algorithm error */
    TECC_CRYPTO_MEMORY_ERROR = -4,        /* Memory error */
    TECC_CRYPTO_DATA_ERROR = -5,          /* Data error */
    TECC_CRYPTO_SCHEDULE_ERROR = -6,      /* Schedule error */
    TECC_CRYPTO_SYSTEM_ERROR = -7,        /* System error */
    TECC_CRYPTO_COLLATION_ERROR = -8,     /* Collation error */
    TECC_CRYPTO_FORMAT_ERROR = -9,        /* Format error */
    TECC_CRYPTO_CACHE_ERROR = -10,        /* Cache error */
    TECC_CRYPTO_BATCH_ERROR = -11         /* Batch processing error */
} TeccCryptoErrorCode;

/* ==================== Algorithm Constants ==================== */

/* Algorithm capability flags */
typedef enum TeccAlgorithmCapability
{
    TECC_ALGO_CAP_ENCRYPT = 0x01,    /* Supports encryption */
    TECC_ALGO_CAP_DECRYPT = 0x02,    /* Supports decryption */
    TECC_ALGO_CAP_COMPARE = 0x04,    /* Supports comparison */
    TECC_ALGO_CAP_AUTH = 0x08        /* Supports authentication */
} TeccAlgorithmCapability;

/* Algorithm constants */
#define TECC_MAX_KEY_SIZE        32     /* Maximum key length */
#define TECC_MAX_DATA_SIZE       (256 * 1024)  /* Maximum data length (256KB) */
#define TECC_SM4_BLOCK_SIZE      16     /* SM4 block size */
#define TECC_SM4_KEY_SIZE        16     /* SM4 key length */
#define TECC_HMAC_SM3_SIZE       32     /* HMAC-SM3 output length */

/* SM4-RND constants */
#define SM4_RND_HEADER_SIZE     (32 + 16)  /* auth_tag + iv */
#define SM4_RND_MIN_SIZE        (SM4_RND_HEADER_SIZE + 1)
#define SM4_RND_MAX_SIZE        (SM4_RND_HEADER_SIZE + TECC_MAX_DATA_SIZE)

/* DET-SM4 constants */
#define DET_SM4_HEADER_SIZE     0
#define DET_SM4_MIN_SIZE        16  /* SM4 block size */
#define DET_SM4_MAX_SIZE        TECC_MAX_DATA_SIZE

/* SEARCH constants */
#define SEARCH_HEADER_SIZE      (sizeof(uint32))  /* data length */
#define SEARCH_MIN_SIZE         (SEARCH_HEADER_SIZE + 1)
#define SEARCH_MAX_SIZE         (SEARCH_HEADER_SIZE + TECC_MAX_DATA_SIZE)

/* ==================== Performance and Memory Statistics ==================== */

/* Performance statistics structure */
typedef struct TeccAlgoPerfStats
{
    uint64_t encrypt_count;             /* Encryption count */
    uint64_t decrypt_count;             /* Decryption count */
    uint64_t total_encrypt_time_us;     /* Total encryption time (microseconds) */
    uint64_t total_decrypt_time_us;     /* Total decryption time (microseconds) */
    uint64_t min_encrypt_time_us;       /* Minimum encryption time */
    uint64_t max_encrypt_time_us;       /* Maximum encryption time */
    uint64_t min_decrypt_time_us;       /* Minimum decryption time */
    uint64_t max_decrypt_time_us;       /* Maximum decryption time */
} TeccAlgoPerfStats;

/* Memory statistics structure */
typedef struct TeccAlgoMemoryStats
{
    size_t total_allocated;             /* Total allocated memory */
    size_t peak_usage;                  /* Peak usage */
    size_t reuse_count;                 /* Reuse count */
    size_t alloc_count;                 /* Allocation count */
    size_t saved_by_reuse;              /* Memory saved by reuse */
    size_t buffer_hit_rate;             /* Buffer hit rate (percentage) */
} TeccAlgoMemoryStats;


/* ==================== Formatter Error Codes ==================== */

/* Formatter error codes - offset by -1000 for easy mapping */
typedef enum TeccFormatterErrorCode
{
    /* Success */
    TECC_FORMATTER_SUCCESS = 0,

    /* Parameter errors (maps to TECC_CRYPTO_INVALID_PARAM) */
    TECC_FORMATTER_ERROR_INVALID_PARAM = -1001,
    TECC_FORMATTER_ERROR_NULL_POINTER = -1002,
    TECC_FORMATTER_ERROR_INVALID_TYPE = -1003,
    TECC_FORMATTER_ERROR_INVALID_INPUT = -1004,
    TECC_FORMATTER_ERROR_UNSUPPORTED_TYPE = -1005,

    /* Memory errors (maps to TECC_CRYPTO_MEMORY_ERROR) */
    TECC_FORMATTER_ERROR_MEMORY_ALLOC = -1101,
    TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL = -1102,

    /* Data errors (maps to TECC_CRYPTO_DATA_ERROR) */
    TECC_FORMATTER_ERROR_DATA_TOO_LARGE = -1201,
    TECC_FORMATTER_ERROR_DATA_FORMAT = -1202,
    TECC_FORMATTER_ERROR_ENCODING_FAILED = -1203,
    TECC_FORMATTER_ERROR_DECODING_FAILED = -1204,
    TECC_FORMATTER_ERROR_CONVERSION_FAILED = -1205,
    TECC_FORMATTER_ERROR_INVALID_NUMERIC = -1206,
    TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW = -1207,

    /* Format errors (maps to TECC_CRYPTO_FORMAT_ERROR) */
    TECC_FORMATTER_ERROR_INVALID_HEADER = -1301,
    TECC_FORMATTER_ERROR_VERSION_MISMATCH = -1302,
    TECC_FORMATTER_ERROR_BUILD_FAILED = -1303,

    /* ICU collation errors (maps to TECC_CRYPTO_COLLATION_ERROR) */
    TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE = -1401,
    TECC_FORMATTER_ERROR_COLLATION_INVALID = -1402,
    TECC_FORMATTER_ERROR_SORTKEY_FAILED = -1403,

    /* System errors (maps to TECC_CRYPTO_SYSTEM_ERROR) */
    TECC_FORMATTER_ERROR_SYSTEM = -1501
} TeccFormatterErrorCode;

/* ==================== Utility Functions ==================== */

/* Error code conversion function declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* Convert algorithm error to scheduler error */
int tecc_algo_error_to_scheduler_error(int algo_error);

/* Convert algorithm error to formatter error */
int tecc_algo_error_to_formatter_error(int algo_error);

/* Get algorithm error string */
const char *tecc_algo_get_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* TECC_TYPES_H */
/* End - ReqID:TECC4.0 */
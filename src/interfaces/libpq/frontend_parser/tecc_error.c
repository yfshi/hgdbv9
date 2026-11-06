/*
 * TECC前端解析器错误处理机制
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的错误处理需求
 *
 * 设计原则：
 * 1. 错误分类明确：按照错误类型、严重级别分类
 * 2. 错误信息详细：包含错误码、消息、位置、修复建议等
 * 3. 错误恢复强大：支持错误恢复和继续解析
 * 4. 多语言支持：支持中英文错误消息
 */

#include "tecc_error.h"
#include "tecc_nodes.h"
#include "tecc_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Portability: MSVC does not support __attribute__((unused)) */
#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* 线程局部存储宏定义 - 与tecc_parser.c保持一致
 * 在支持__thread的编译器（GCC/Clang）中使用__thread
 * 在Windows/MSVC中使用__declspec(thread)
 * 其他平台使用pthread线程特定数据或回退到非线程安全存储
 */
#if defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#elif defined(_WIN32) && defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#else
/* 其他平台：尝试使用C11 _Thread_local如果支持 */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define TECC_THREAD_LOCAL _Thread_local
#else
/* 回退到非线程安全存储并发出警告 */
#define TECC_THREAD_LOCAL
#pragma message("Warning: Using non-thread-safe storage for thread-local variables. Platform lacks proper thread-local storage support.")
#endif
#endif

/* ==================== 内部数据结构 ==================== */

/* 错误处理器 */
typedef struct ErrorHandler {
    void (*handler)(int error_code, const char *error_message,
                   const LocationInfo *location, void *user_data);
    void *user_data;
    struct ErrorHandler *next;
} ErrorHandler;

/* 错误上下文 */
typedef struct ErrorContext {
    ErrorHandler *handlers;         /* 错误处理器链表 */
    int error_count;                /* 错误计数 */
    int warning_count;              /* 警告计数 */
    bool recovery_enabled;          /* 是否启用错误恢复 */
    int max_errors;                 /* 最大错误数 */
    char *language;                 /* 错误消息语言 */

    /* 错误缓冲 */
    ParserError *first_error;       /* 第一个错误 */
    ParserError *last_error;        /* 最后一个错误 */
} ErrorContext;

/* ==================== 线程局部错误上下文 ==================== */

static TECC_THREAD_LOCAL ErrorContext g_error_context = {
    .handlers = NULL,
    .error_count = 0,
    .warning_count = 0,
    .recovery_enabled = true,
    .max_errors = 100,
    .language = NULL,
    .first_error = NULL,
    .last_error = NULL
};

/* ==================== 错误消息定义 ==================== */

/* 错误消息结构 */
typedef struct ErrorMessage {
    int code;
    const char *message_en;
    const char *message_zh;
    int severity;
    const char *hint_en;
    const char *hint_zh;
} ErrorMessage;

/* 错误消息表 */
static const ErrorMessage error_messages[] = {
    /* 语法错误 (1000-1999) */
    {
        PARSER_SYNTAX_ERROR,
        "Syntax error",
        "语法错误",
        3,
        "Check SQL syntax",
        "请检查SQL语法"
    },
    {
        PARSER_KEYWORD_MISSING_ERROR,
        "Missing required keyword",
        "缺少必需的关键字",
        3,
        "Add missing keyword",
        "请添加缺少的关键字"
    },
    {
        PARSER_UNEXPECTED_TOKEN_ERROR,
        "Unexpected token",
        "意外的标记",
        3,
        "Check token sequence",
        "请检查标记顺序"
    },
    {
        PARSER_INVALID_IDENTIFIER_ERROR,
        "Invalid identifier",
        "无效的标识符",
        3,
        "Use valid identifier name",
        "请使用有效的标识符名称"
    },
    {
        PARSER_MISSING_PARENTHESIS_ERROR,
        "Missing parenthesis",
        "缺少括号",
        3,
        "Add missing parenthesis",
        "请添加缺少的括号"
    },
    {
        PARSER_INVALID_NUMBER_ERROR,
        "Invalid number format",
        "无效的数字格式",
        3,
        "Use valid number format",
        "请使用有效的数字格式"
    },
    {
        PARSER_INVALID_STRING_ERROR,
        "Invalid string format",
        "无效的字符串格式",
        3,
        "Use valid string format",
        "请使用有效的字符串格式"
    },

    /* 语义错误 (2000-2999) */
    {
        PARSER_SEMANTIC_ERROR,
        "Semantic error",
        "语义错误",
        3,
        "Check SQL semantics",
        "请检查SQL语义"
    },
    {
        PARSER_TYPE_MISMATCH_ERROR,
        "Type mismatch",
        "类型不匹配",
        3,
        "Check data types",
        "请检查数据类型"
    },
    {
        PARSER_COLLATION_CHANGE_ERROR,
        "Collation change not allowed",
        "不允许的排序规则变更",
        3,
        "Use compatible collations",
        "请使用兼容的排序规则"
    },
    {
        PARSER_UNDEFINED_TABLE_ERROR,
        "Undefined table",
        "未定义的表",
        3,
        "Define table or use existing table",
        "请定义表或使用现有的表"
    },
    {
        PARSER_UNDEFINED_COLUMN_ERROR,
        "Undefined column",
        "未定义的列",
        3,
        "Define column or use existing column",
        "请定义列或使用现有的列"
    },
    {
        PARSER_DUPLICATE_TABLE_ERROR,
        "Duplicate table name",
        "重复的表名",
        3,
        "Use unique table names",
        "请使用唯一的表名"
    },
    {
        PARSER_DUPLICATE_COLUMN_ERROR,
        "Duplicate column name",
        "重复的列名",
        3,
        "Use unique column names",
        "请使用唯一的列名"
    },
    {
        PARSER_INVALID_FUNCTION_ERROR,
        "Invalid function",
        "无效的函数",
        3,
        "Use valid function name",
        "请使用有效的函数名"
    },
    {
        PARSER_INVALID_ARGUMENT_ERROR,
        "Invalid function argument",
        "无效的函数参数",
        3,
        "Check function arguments",
        "请检查函数参数"
    },

    /* 系统错误 (3000-3999) */
    {
        PARSER_MEMORY_ERROR,
        "Memory allocation failed",
        "内存分配失败",
        4,
        "Check system memory",
        "请检查系统内存"
    },
    {
        PARSER_IO_ERROR,
        "I/O error",
        "输入输出错误",
        4,
        "Check file system",
        "请检查文件系统"
    },
    {
        PARSER_INTERNAL_ERROR,
        "Internal parser error",
        "解析器内部错误",
        4,
        "Report this error to developers",
        "请将此错误报告给开发人员"
    },
    {
        PARSER_CONFIG_ERROR,
        "Parser configuration error",
        "解析器配置错误",
        4,
        "Check parser configuration",
        "请检查解析器配置"
    },

    /* TECC专用错误 (4000-4999) */
    {
        4001,
        "Invalid KEK name",
        "无效的KEK名称",
        3,
        "KEK name must be valid identifier",
        "KEK名称必须是有效的标识符"
    },
    {
        4002,
        "Invalid DEK name",
        "无效的DEK名称",
        3,
        "DEK name must be valid identifier",
        "DEK名称必须是有效的标识符"
    },
    {
        4003,
        "KEK not found",
        "KEK不存在",
        3,
        "Create KEK first or use existing KEK",
        "请先创建KEK或使用现有的KEK"
    },
    {
        4004,
        "DEK not found",
        "DEK不存在",
        3,
        "Create DEK first or use existing DEK",
        "请先创建DEK或使用现有的DEK"
    },
    {
        4005,
        "Encryption algorithm not supported",
        "不支持的加密算法",
        3,
        "Use supported algorithm (AES256, SM4, etc.)",
        "请使用支持的加密算法（AES256、SM4等）"
    },
    {
        PARSER_INVALID_KEY_SIZE_ERROR,
        "Invalid key size",
        "无效的密钥大小",
        3,
        "Use valid key size for algorithm",
        "请使用算法有效的密钥大小"
    },
    {
        PARSER_ENCRYPTION_MODE_ERROR,
        "Invalid encryption mode",
        "无效的加密模式",
        3,
        "Use valid encryption mode",
        "请使用有效的加密模式"
    },
    {
        PARSER_INVALID_CIPHER_TEXT_ERROR,
        "Invalid cipher text format",
        "无效的密文格式",
        3,
        "Use valid cipher text format",
        "请使用有效的密文格式"
    },

    {0, NULL, NULL, 0, NULL, NULL}  /* 结束标记 */
};

/* ==================== 内部辅助函数 ==================== */

/* 获取错误消息 */
static const ErrorMessage *get_error_message(int error_code) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    int i;

    for (i = 0; error_messages[i].code != 0; i++) {
        if (error_messages[i].code == error_code) {
            return &error_messages[i];
        }
    }
    return NULL;
}

/* 获取本地化消息 */
static const char *get_localized_message(const ErrorMessage *msg, const char *language) {
    if (!msg) return "Unknown error";

    if (language && strcmp(language, "zh") == 0 && msg->message_zh) {
        return msg->message_zh;
    }
    return msg->message_en;
}

/* 获取本地化提示 */
static const char *get_localized_hint(const ErrorMessage *msg, const char *language) {
    if (!msg || !msg->hint_en) return NULL;

    if (language && strcmp(language, "zh") == 0 && msg->hint_zh) {
        return msg->hint_zh;
    }
    return msg->hint_en;
}

/* 创建错误信息 */
static ParserError *create_parser_error_internal(int error_code,
                                                const char *message,
                                                const char *sql_fragment,
                                                const LocationInfo *location,
                                                int severity,
                                                const char *hint,
                                                const char *detail) {
    ParserError *error = (ParserError *)tecc_palloc(sizeof(ParserError));
    if (!error) return NULL;

    error->type = FE_T_Node;  /* 错误节点使用无效类型，FE_T_Node=0 */
    error->error_code = error_code;
    error->error_message = message ? tecc_pstrdup(message) : NULL;
    error->sql_fragment = sql_fragment ? tecc_pstrdup(sql_fragment) : NULL;

    if (location) {
        error->location = *location;
    } else {
        memset(&error->location, 0, sizeof(LocationInfo));
    }

    error->severity = severity;
    error->hint = hint ? tecc_pstrdup(hint) : NULL;
    error->detail = detail ? tecc_pstrdup(detail) : NULL;
    error->next = NULL;

    return error;
}

/* 格式化错误详情 */
TECC_ATTR_UNUSED static char *format_error_detail(const char *format, ...) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char buffer[1024];
    va_list args;

    if (!format) return NULL;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return tecc_pstrdup(buffer);
}

/* 调用错误处理器 */
static void call_error_handlers(int error_code,
                               const char *error_message,
                               const LocationInfo *location) {
    ErrorHandler *handler = g_error_context.handlers;
    while (handler) {
        if (handler->handler) {
            handler->handler(error_code, error_message, location, handler->user_data);
        }
        handler = handler->next;
    }
}

/* 添加到错误缓冲 */
static void add_to_error_buffer(ParserError *error) {
    if (!error) return;

    if (!g_error_context.first_error) {
        g_error_context.first_error = error;
        g_error_context.last_error = error;
    } else {
        g_error_context.last_error->next = error;
        g_error_context.last_error = error;
    }

    /* severity >= 3 is error, severity < 3 is warning */
    if (error->severity >= 3) {
        g_error_context.error_count++;
    } else {
        g_error_context.warning_count++;
    }
}

/* 检查是否达到错误限制 */
static bool error_limit_reached(void) {
    return g_error_context.error_count >= g_error_context.max_errors;
}

/* 将错误追加到全局错误列表 */
void tecc_error_append(ParserError *error) {
    add_to_error_buffer(error);
}

/* ==================== 公共接口实现 ==================== */

/* 报告错误 */
void tecc_error_report(int error_code,
                      const char *sql_fragment,
                      const LocationInfo *location,
                      const char *detail_format, ...) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const ErrorMessage *msg = NULL;
    const char *message = "Unknown error";
    const char *hint = NULL;
    int severity = 3;
    char *detail = NULL;
    ParserError *error = NULL;

    if (error_limit_reached()) {
        return;
    }

    /* 获取错误消息 */
    msg = get_error_message(error_code);
    message = msg ? get_localized_message(msg, g_error_context.language) : "Unknown error";
    hint = msg ? get_localized_hint(msg, g_error_context.language) : NULL;
    severity = msg ? msg->severity : 3;

    /* 格式化详情 */
    if (detail_format) {
        char buffer[1024];
        va_list args;
        va_start(args, detail_format);
        vsnprintf(buffer, sizeof(buffer), detail_format, args);
        va_end(args);
        detail = tecc_pstrdup(buffer);
    }

    /* 创建错误信息 */
    error = create_parser_error_internal(error_code, message,
                                                     sql_fragment, location,
                                                     severity, hint, detail);
    if (detail) tecc_pfree(detail);

    if (error) {
        /* 添加到缓冲 */
        add_to_error_buffer(error);

        /* 调用处理器 */
        call_error_handlers(error_code, message, location);
    }
}

/* 报告错误（简化版本） */
void tecc_error_report_simple(int error_code, int location) {
    LocationInfo loc = {location, location, 1, 1, 1, 1};
    tecc_error_report(error_code, NULL, &loc, NULL);
}

/* 设置错误处理器 */
void tecc_error_set_handler(void (*handler)(int error_code,
                                           const char *error_message,
                                           const LocationInfo *location,
                                           void *user_data),
                           void *user_data) {
    ErrorHandler *new_handler = (ErrorHandler *)tecc_palloc(sizeof(ErrorHandler));
    if (!new_handler) return;

    new_handler->handler = handler;
    new_handler->user_data = user_data;
    new_handler->next = g_error_context.handlers;
    g_error_context.handlers = new_handler;
}

/* 移除错误处理器 */
void tecc_error_remove_handler(void (*handler)(int error_code,
                                              const char *error_message,
                                              const LocationInfo *location,
                                              void *user_data)) {
    ErrorHandler **prev = &g_error_context.handlers;
    ErrorHandler *current = g_error_context.handlers;

    while (current) {
        if (current->handler == handler) {
            *prev = current->next;
            tecc_pfree(current);
            current = *prev;
        } else {
            prev = &current->next;
            current = current->next;
        }
    }
}

/* 清除所有错误 */
void tecc_error_clear_all(void) {
    ParserError *error = g_error_context.first_error;
    while (error) {
        ParserError *next = error->next;
        if (error->error_message) tecc_pfree(error->error_message);
        if (error->sql_fragment) tecc_pfree(error->sql_fragment);
        if (error->hint) tecc_pfree(error->hint);
        if (error->detail) tecc_pfree(error->detail);
        tecc_pfree(error);
        error = next;
    }

    g_error_context.first_error = NULL;
    g_error_context.last_error = NULL;
    g_error_context.error_count = 0;
    g_error_context.warning_count = 0;
}

/* 获取错误列表 */
ParserError *tecc_error_get_list(void) {
    return g_error_context.first_error;
}

/* 获取错误数量 */
int tecc_error_get_count(void) {
    return g_error_context.error_count;
}

/* 获取警告数量 */
int tecc_error_get_warning_count(void) {
    return g_error_context.warning_count;
}

/* 设置错误恢复 */
void tecc_error_set_recovery(bool enabled) {
    g_error_context.recovery_enabled = enabled;
}

/* 设置最大错误数 */
void tecc_error_set_max_errors(int max_errors) {
    if (max_errors > 0) {
        g_error_context.max_errors = max_errors;
    }
}

/* 设置错误消息语言 */
void tecc_error_set_language(const char *language) {
    if (g_error_context.language) {
        tecc_pfree(g_error_context.language);
    }

    if (language && *language) {
        g_error_context.language = tecc_pstrdup(language);
    } else {
        g_error_context.language = NULL;
    }
}

/* 创建错误信息（外部使用） */
ParserError *tecc_error_create(int error_code,
                              const char *message,
                              const char *sql_fragment,
                              const LocationInfo *location,
                              int severity,
                              const char *hint,
                              const char *detail) {
    return create_parser_error_internal(error_code, message, sql_fragment,
                                       location, severity, hint, detail);
}

/* 复制错误信息 */
ParserError *tecc_error_copy(const ParserError *error) {
    if (!error) return NULL;

    return create_parser_error_internal(error->error_code,
                                       error->error_message,
                                       error->sql_fragment,
                                       &error->location,
                                       error->severity,
                                       error->hint,
                                       error->detail);
}

/* 释放错误信息 */
void tecc_error_free(ParserError *error) {
    if (!error) return;

    if (error->error_message) tecc_pfree(error->error_message);
    if (error->sql_fragment) tecc_pfree(error->sql_fragment);
    if (error->hint) tecc_pfree(error->hint);
    if (error->detail) tecc_pfree(error->detail);
    tecc_pfree(error);
}

/* 释放错误列表 */
void tecc_error_free_list(ParserError *errors) {
    while (errors) {
        ParserError *next = errors->next;
        tecc_error_free(errors);
        errors = next;
    }
}

/* 错误信息转字符串 */
char *tecc_error_to_string(const ParserError *error) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t size;
    char *buffer;
    char *pos;
    int remaining;
    int written;

    if (!error) return NULL;

    /* 估算缓冲区大小 */
    size = 512;
    if (error->error_message) size += strlen(error->error_message);
    if (error->sql_fragment) size += strlen(error->sql_fragment);
    if (error->hint) size += strlen(error->hint);
    if (error->detail) size += strlen(error->detail);

    buffer = (char *)tecc_palloc(size);
    if (!buffer) return NULL;

    pos = buffer;
    remaining = size;

    /* 错误码和消息 */
    written = snprintf(pos, remaining, "Error %d: %s",
                          error->error_code,
                          error->error_message ? error->error_message : "");
    if (written > 0 && written < remaining) {
        pos += written;
        remaining -= written;
    }

    /* 位置信息 */
    if (error->location.start_location >= 0) {
        written = snprintf(pos, remaining, " at position %d-%d",
                          error->location.start_location,
                          error->location.end_location);
        if (written > 0 && written < remaining) {
            pos += written;
            remaining -= written;
        }

        if (error->location.start_line > 0) {
            written = snprintf(pos, remaining, " (line %d:%d to %d:%d)",
                              error->location.start_line,
                              error->location.start_column,
                              error->location.end_line,
                              error->location.end_column);
            if (written > 0 && written < remaining) {
                pos += written;
                remaining -= written;
            }
        }
    }

    /* SQL片段 */
    if (error->sql_fragment && *error->sql_fragment) {
        written = snprintf(pos, remaining, "\nSQL: %s", error->sql_fragment);
        if (written > 0 && written < remaining) {
            pos += written;
            remaining -= written;
        }
    }

    /* 提示 */
    if (error->hint && *error->hint) {
        written = snprintf(pos, remaining, "\nHint: %s", error->hint);
        if (written > 0 && written < remaining) {
            pos += written;
            remaining -= written;
        }
    }

    /* 详情 */
    if (error->detail && *error->detail) {
        written = snprintf(pos, remaining, "\nDetail: %s", error->detail);
        if (written > 0 && written < remaining) {
            pos += written;
            remaining -= written;
        }
    }

    return buffer;
}

/* 获取错误严重级别名称 */
const char *tecc_error_get_severity_name(int severity) {
    switch (severity) {
        case 1: return "INFO";
        case 2: return "WARNING";
        case 3: return "ERROR";
        case 4: return "FATAL";
        default: return "UNKNOWN";
    }
}

/* 检查是否可恢复 */
bool tecc_error_is_recoverable(int error_code) {
    /* 语法错误通常可恢复 */
    if (error_code >= 1000 && error_code < 2000) {
        return g_error_context.recovery_enabled;
    }

    /* 语义错误可能可恢复 */
    if (error_code >= 2000 && error_code < 3000) {
        return g_error_context.recovery_enabled;
    }

    /* 系统错误通常不可恢复 */
    return false;
}

/* 重置错误上下文 */
void tecc_error_reset_context(void) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    ErrorHandler *handler;

    tecc_error_clear_all();

    /* 清理处理器 */
    handler = g_error_context.handlers;
    while (handler) {
        ErrorHandler *next = handler->next;
        tecc_pfree(handler);
        handler = next;
    }

    g_error_context.handlers = NULL;

    /* 重置其他设置 */
    if (g_error_context.language) {
        tecc_pfree(g_error_context.language);
        g_error_context.language = NULL;
    }

    g_error_context.recovery_enabled = true;
    g_error_context.max_errors = 100;
}


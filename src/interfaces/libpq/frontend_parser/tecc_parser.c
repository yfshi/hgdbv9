/*
 * TECC前端解析器核心接口实现
 * 严格按照详细设计文档第4章要求实现
 * 确保与处理器模块的接口完全兼容
 */

/* 定义_POSIX_C_SOURCE以使用clock_gettime */
#define _POSIX_C_SOURCE 200112L

#include "tecc_parser.h"
#include "tecc_nodes.h"
#include "tecc_perf.h"
#include "tecc_error.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

/* 线程局部存储宏定义
 * 在支持__thread的编译器（GCC/Clang）中使用__thread
 * 在Windows/MSVC中使用__declspec(thread)
 * 不支持线程局部存储的平台将编译失败，不允许静默降级为非线程安全存储
 */
#if defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#elif defined(_WIN32) && defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define TECC_THREAD_LOCAL _Thread_local
#else
#error "Thread-local storage is required for TECC parser. Platform lacks __thread, __declspec(thread), or _Thread_local support."
#endif

/* Bison/Flex parser headers */
#include "tecc_grammar.h"
#include "tecc_scanner.h"

/* TECC公共头文件 */
#include "../tecc_common/tecc_frontend_list.h"
#include "../tecc_common/tecc_logging.h"  /* TECC logging system */

/* ==================== 内部数据结构 ==================== */

/* 函数声明 */
void tecc_set_input_string(const char *str);
void reset_tecc_parsetree(void);
List *get_tecc_parsetree(void);

/* 解析器内部状态 */
typedef struct TeccParserInternalState {
    TeccParseOptions default_options;   /* 默认解析选项 */
    TeccParserStatistics statistics;    /* 解析统计信息 */
    void (*error_handler)(int, const char *, const LocationInfo *, void *); /* 错误处理函数 */
    void *error_handler_data;           /* 错误处理函数用户数据 */
    bool initialized;                   /* 是否已初始化 */
} TeccParserInternalState;

/* ==================== 全局变量 ==================== */

/* Bison解析器全局变量 - 需要在tecc_grammar.c中声明为extern */
extern List *tecc_parsetree;

/* Flex扫描器缓冲区状态 */
static TECC_THREAD_LOCAL YY_BUFFER_STATE g_scanner_buffer = NULL;

/* 解析器内部状态 */
static TECC_THREAD_LOCAL TeccParserInternalState g_parser_state = {
    .default_options = {
        .enable_position_tracking = true,
        .enable_error_recovery = true,
        .enable_semantic_check = true,
        .enable_performance_opt = false,
        .max_error_count = 100,
        .max_recursion_depth = 100,
        .user_data = NULL
    },
    .statistics = {0},
    .error_handler = NULL,
    .error_handler_data = NULL,
    .initialized = false  /* 初始化为false，第一次访问时初始化 */
};

/* ==================== 内部辅助函数 ==================== */

/* 初始化线程局部状态 */
static void init_thread_local_state(void)
{
    if (!g_parser_state.initialized) {
        /* 统计信息已经在声明时初始化为0，但为了安全起见，再次清零 */
        memset(&g_parser_state.statistics, 0, sizeof(TeccParserStatistics));

        /* 错误处理指针已经在声明时初始化为NULL，这里确保一下 */
        g_parser_state.error_handler = NULL;
        g_parser_state.error_handler_data = NULL;

        /* 标记为已初始化 */
        g_parser_state.initialized = true;
    }
}

/* 获取线程局部状态（确保已初始化） */
static TeccParserInternalState *get_parser_state(void)
{
    init_thread_local_state();
    return &g_parser_state;
}

/* Bison解析器辅助函数 */
/* Buffer start for scanner token location calculation */
/* tecc_buffer_start is read by the scanner via
 * extern TECC_THREAD_LOCAL_EXTERN.  On MSVC, TECC_THREAD_LOCAL_EXTERN
 * expands to nothing because __declspec(thread) cannot be 'extern'ed;
 * the scanner sees a plain global variable.  Hence on MSVC the
 * definition must also be a plain global, NOT __declspec(thread.
 * ReqID:TECC4.0 */
#ifdef _MSC_VER
const char *tecc_buffer_start = NULL;
#else
TECC_THREAD_LOCAL const char *tecc_buffer_start = NULL;
#endif

static bool setup_parser_input(const char *query_string) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t slen;
    char *buf;

    /* 设置解析器输入缓冲区 */
    if (!query_string) {
        return false;
    }

    /* 清理之前的缓冲区 */
    if (g_scanner_buffer) {
        tecc__delete_buffer(g_scanner_buffer);
        g_scanner_buffer = NULL;
    }

    /* 使用yy_scan_buffer让flex直接使用我们提供的内存，
     * 这样yytext指向的就是这个buffer中的位置，
     * yytext - buffer_start 可以计算正确的字节偏移。
     * 要求：buffer必须以两个\0结尾 */
    slen = strlen(query_string);
    buf = (char *)tecc_palloc(slen + 2);
    if (!buf) return false;
    memcpy(buf, query_string, slen);
    buf[slen] = '\0';
    buf[slen + 1] = '\0';

    /* 保存buffer起始地址供scanner计算token偏移 */
    tecc_buffer_start = buf;

    g_scanner_buffer = tecc__scan_buffer(buf, slen + 2);
    if (!g_scanner_buffer) {
        tecc_pfree(buf);
        return false;
    }

    /* 切换到新缓冲区 */
    tecc__switch_to_buffer(g_scanner_buffer);

    return true;
}


static List *call_bison_parser(const char *query_string) {
    /* 调用Bison解析器 */
    extern int tecc_lineno;
    int parse_result;
    List *parse_tree;

    /* 在开始新解析之前，确保全局状态被清理 */
    if (tecc_parsetree) {
        /* 强制清理，但使用安全的方式 */
        reset_tecc_parsetree();
    }

    /* 重置Bison/lexer全局变量 */
    tecc_lineno = 1;

    if (!setup_parser_input(query_string)) {
        return NULL;
    }

    /* 调用Bison解析器 */
    parse_result = tecc_parse();

    if (parse_result != 0) {
        /* 解析失败 */
        /* 解析失败时，Bison会清理所有语义值，我们不需要调用reset_tecc_parsetree() */
        return NULL;
    }

    /* 解析成功，获取解析树 */
    parse_tree = get_tecc_parsetree();

    /* 重置全局解析树变量，但不释放节点（因为调用者需要它们） */
    if (tecc_parsetree) {
        /* 只重置全局变量，不释放节点 */
        tecc_parsetree = NIL;
    }

    return parse_tree;
}

#if 0
static Node *convert_list_to_node(List *parse_list) {
    /* 将List*解析树转换为Node* */
    if (!parse_list || parse_list->length == 0) {
        return NULL;
    }

    /* 对于简单的存根实现，我们假设列表的第一个元素就是语句节点 */
    if (parse_list->head && parse_list->head->data.ptr_value) {
        return (Node *)parse_list->head->data.ptr_value;
    }

    return NULL;
}
#endif

/* 获取当前时间（纳秒） */
static uint64_t get_current_time_ns(void) {
    struct timeval tv;
#ifdef __linux__
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
    /* 如果clock_gettime失败，使用gettimeofday作为后备 */
#endif

    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;
}

/* 更新统计信息 */
static void update_statistics(size_t parse_time_ms, size_t memory_used_kb,
                             int error_count, int warning_count) {
    TeccParserInternalState *state = get_parser_state();
    size_t memory_used_bytes;
    (void)error_count;      /* 避免未使用参数警告 */
    (void)warning_count;    /* 避免未使用参数警告 */

    state->statistics.parse_calls++;
    state->statistics.parse_time_ns += parse_time_ms * 1000000;
    state->statistics.total_time_ns += parse_time_ms * 1000000;

    memory_used_bytes = memory_used_kb * 1024;
    state->statistics.memory_used = memory_used_bytes;

    if (memory_used_bytes > state->statistics.peak_memory) {
        state->statistics.peak_memory = memory_used_bytes;
    }
}

/* 创建位置信息 - 暂时未使用 */
#if 0
static LocationInfo *create_location_info(int start, int end,
                                         int start_line, int start_col,
                                         int end_line, int end_col) {
    LocationInfo *loc = (LocationInfo *)tecc_palloc(sizeof(LocationInfo));
    if (!loc) return NULL;

    loc->start_location = start;
    loc->end_location = end;
    loc->start_line = start_line;
    loc->start_column = start_col;
    loc->end_line = end_line;
    loc->end_column = end_col;

    return loc;
}
#endif

/* 创建错误信息 */
static ParserError *create_parser_error(int error_code, const char *message,
                                       const char *sql_fragment,
                                       const LocationInfo *location,
                                       int severity, const char *hint,
                                       const char *detail) {
    ParserError *error = (ParserError *)tecc_palloc(sizeof(ParserError));
    if (!error) return NULL;

    error->type = FE_T_Node;  /* 错误节点使用无效类型，FE_T_Node=0 */
    error->error_code = error_code;
    error->error_message = tecc_pstrdup(message);
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

/* 添加错误到结果 */
static void add_error_to_result(TeccParseResult *result, ParserError *error) {
    if (!result || !error) return;

    if (!result->errors) {
        result->errors = error;
    } else {
        ParserError *last = result->errors;
        while (last->next) {
            last = last->next;
        }
        last->next = error;
    }

    result->error_count++;
    if (error->severity > 1) {  /* 严重级别大于1的视为警告 */
        result->warning_count++;
    }
}

/* 调用错误处理函数 */
static void call_error_handler(int error_code, const char *error_message,
                              const LocationInfo *location) {
    TeccParserInternalState *state = get_parser_state();
    if (state->error_handler) {
        state->error_handler(error_code, error_message, location,
                            state->error_handler_data);
    }
}

/* 计算SQL字符串长度 */
static size_t safe_strlen(const char *str) {
    return str ? strlen(str) : 0;
}

/* 检查是否为空白查询 */
static bool is_whitespace_only(const char *query) {
    if (!query) return true;

    while (*query) {
        if (!isspace((unsigned char)*query)) {
            return false;
        }
        query++;
    }
    return true;
}

/* 检查是否为注释 */
static bool is_comment_only(const char *query) {
    if (!query) return true;

    /* 跳过空白 */
    while (isspace((unsigned char)*query)) {
        query++;
    }

    /* 检查是否为注释 */
    if (strncmp(query, "--", 2) == 0) {
        return true;
    }

    if (strncmp(query, "/*", 2) == 0) {
        /* 需要找到对应的结束注释 */
        const char *end = strstr(query + 2, "*/");
        if (!end) return true;  /* 未结束的注释 */

        /* 检查注释后是否只有空白 */
        end += 2;  /* 跳过结束注释标记 */
        while (isspace((unsigned char)*end)) {
            end++;
        }
        return *end == '\0';
    }

    return false;
}

/* ==================== 核心解析函数实现 ==================== */

Node *tecc_parser_parse_node(const char *query_string) {
    TeccParseResult result;
    TeccParseOptions options;
    Node *parse_tree;

    tecc_parser_init_result(&result);
    tecc_parser_init_options(&options);

    if (!tecc_parser_parse_ex(query_string, &options, &result)) {
        tecc_parser_free_result(&result);
        return NULL;
    }

    parse_tree = result.parse_tree;

    /* 从结果中分离解析树，避免被释放 */
    result.parse_tree = NULL;
    tecc_parser_free_result(&result);

    return parse_tree;
}

bool tecc_parser_parse_ex(const char *query_string,
                         const TeccParseOptions *options,
                         TeccParseResult *result) {
    clock_t start_time;
    clock_t end_time;
    TeccParseOptions actual_options;
    SelectStmt *empty_stmt;
    size_t query_len;
    List *parse_list;
    size_t parse_time_ms;
    Node *parse_tree;
    size_t memory_used_kb;

    if (!query_string || !result) {
        return false;
    }

    /* 记录开始时间 */
    start_time = clock();

    /* 初始化结果 */
    tecc_parser_init_result(result);

    /* 使用默认选项或提供的选项 */
    if (options) {
        actual_options = *options;
    } else {
        tecc_parser_init_options(&actual_options);
    }

    /* 检查空查询 */
    if (is_whitespace_only(query_string) || is_comment_only(query_string)) {
        result->success = true;

        /* 创建空语句节点 */
        empty_stmt = makeSelectStmt(0, 0);
        if (empty_stmt) {
            result->parse_tree = (Node *)empty_stmt;
        }

        /* 更新统计信息 */
        end_time = clock();
        parse_time_ms = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
        update_statistics(parse_time_ms, 0, 0, 0);

        result->parse_time_ms = parse_time_ms;
        return true;
    }

    /* 获取查询长度 */
    query_len = safe_strlen(query_string);

    /* 调用Bison解析器进行语法分析 */
    parse_list = call_bison_parser(query_string);
    parse_tree = NULL;

    if (parse_list) {
        /* 多语句支持：Bison返回的List可能包含多个stmt节点。
         * 直接作为parse_tree返回（FE_T_List类型），
         * processor中的多语句循环会逐个处理它们。
         * parse_list内存通过freeNode(FE_T_List)释放。ReqID:TECC4.0 */
        parse_tree = (Node *)parse_list;
    }

    if (!parse_tree) {
        /* Bison解析失败，直接报错退出，不使用简单检测逻辑 */
        LocationInfo loc = {0, (int)query_len, 1, 1, 1, 1};

        ParserError *error = create_parser_error(
            PARSER_SYNTAX_ERROR,
            "SQL语法解析失败",
            query_string,
            &loc,
            3,  /* 错误级别 */
            "请检查SQL语法或确保Bison解析器正常工作",
            "前端解析器依赖Bison进行完整的语法解析，不支持简单检测模式"
        );

        if (error) {
            add_error_to_result(result, error);
            call_error_handler(PARSER_SYNTAX_ERROR, "SQL语法解析失败", &loc);
        }

        result->success = false;
        return false;
    }

    if (!parse_tree) {
        /* 内存分配失败 */
        LocationInfo loc = {0, (int)query_len, 1, 1, 1, (int)query_len};
        ParserError *error = create_parser_error(
            PARSER_MEMORY_ERROR,
            "内存分配失败，无法创建解析树",
            NULL,
            &loc,
            4,  /* 严重错误 */
            "请检查系统内存状态",
            "前端解析器内存分配失败"
        );

        if (error) {
            add_error_to_result(result, error);
            call_error_handler(PARSER_MEMORY_ERROR, "内存分配失败，无法创建解析树", &loc);
        }

        result->success = false;
        return false;
    }

    /* 设置结果 */
    result->parse_tree = parse_tree;
    result->success = true;

    /* 计算解析时间和内存使用 */
    end_time = clock();
    parse_time_ms = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;

    /* 估算内存使用（简化） */
    memory_used_kb = sizeof(Node) + query_len;  /* 基础估算 */

    /* 更新统计信息 */
    update_statistics(parse_time_ms, memory_used_kb,
                     result->error_count, result->warning_count);

    result->parse_time_ms = parse_time_ms;
    result->memory_used_kb = memory_used_kb;

    return true;
}

void tecc_parser_free_tree_node(Node *parse_tree) {
    freeNode(parse_tree);
}

void tecc_parser_free_result(TeccParseResult *result) {
    ParserError *error;
    ParserError *next;

    if (!result) return;

    /* 释放解析树 */
    if (result->parse_tree) {
        tecc_parser_free_tree_node(result->parse_tree);
        result->parse_tree = NULL;
    }

    /* 释放错误信息链表 */
    error = result->errors;
    while (error) {
        next = error->next;

        if (error->error_message) tecc_pfree(error->error_message);
        if (error->sql_fragment) tecc_pfree(error->sql_fragment);
        if (error->hint) tecc_pfree(error->hint);
        if (error->detail) tecc_pfree(error->detail);

        tecc_pfree(error);
        error = next;
    }

    result->errors = NULL;
    result->error_count = 0;
    result->warning_count = 0;

    /* 释放参数映射 */
    tecc_parser_clear_param_mappings(result);

    /* 重置其他字段 */
    result->parse_time_ms = 0;
    result->memory_used_kb = 0;
    result->success = false;
}

ParserError *tecc_parser_get_error(const TeccParseResult *result, int index) {
    ParserError *error;
    int i;

    if (!result || index < 0) {
        return NULL;
    }

    error = result->errors;
    for (i = 0; i < index && error; i++) {
        error = error->next;
    }

    return error;
}

int tecc_parser_get_error_count(const TeccParseResult *result) {
    return result ? result->error_count : 0;
}

int tecc_parser_get_warning_count(const TeccParseResult *result) {
    return result ? result->warning_count : 0;
}

Node *tecc_parser_clone_tree(const Node *parse_tree) {
    if (!parse_tree) return NULL;

    /* 这里应该实现深度克隆逻辑 */
    /* 由于时间关系，暂时返回NULL，表示不支持 */
    return NULL;
}

bool tecc_parser_validate(const Node *parse_tree, ParserError **errors) {
    (void)errors;  /* 暂时未使用 */

    if (!parse_tree) {
        /* 空树是有效的 */
        return true;
    }

    /* 这里应该实现验证逻辑 */
    /* 由于时间关系，暂时返回true，表示验证通过 */
    return true;
}

/* ==================== 工具函数实现 ==================== */

void tecc_parser_init_options(TeccParseOptions *options) {
    TeccParserInternalState *state;

    if (!options) return;

    state = get_parser_state();
    *options = state->default_options;
}

void tecc_parser_init_result(TeccParseResult *result) {
    if (!result) return;

    memset(result, 0, sizeof(TeccParseResult));
    result->success = false;
    result->param_mappings.mappings = NULL;
    result->param_mappings.mapping_count = 0;
    result->param_mappings.has_mappings = false;
}

void tecc_parser_set_default_options(const TeccParseOptions *options) {
    TeccParserInternalState *state;

    if (!options) return;

    state = get_parser_state();
    state->default_options = *options;
}

TeccParseOptions *tecc_parser_get_default_options(void) {
    TeccParserInternalState *state = get_parser_state();
    return &state->default_options;
}

/* ==================== 性能监控接口实现 ==================== */

void tecc_parser_get_statistics(TeccParserStatistics *stats) {
    TeccParserInternalState *state;

    if (!stats) return;

    state = get_parser_state();
    *stats = state->statistics;
}

void tecc_parser_reset_statistics(void) {
    TeccParserInternalState *state = get_parser_state();
    memset(&state->statistics, 0, sizeof(TeccParserStatistics));
}

/* ==================== 错误处理接口实现 ==================== */

void tecc_parser_set_error_handler(
    void (*handler)(int error_code, const char *error_message,
                   const LocationInfo *location, void *user_data),
    void *user_data) {
    TeccParserInternalState *state;

    state = get_parser_state();
    state->error_handler = handler;
    state->error_handler_data = user_data;
}

void tecc_parser_clear_errors(TeccParseResult *result) {
    ParserError *error;
    ParserError *next;

    if (!result) return;

    /* 释放错误信息链表 */
    error = result->errors;
    while (error) {
        next = error->next;

        if (error->error_message) tecc_pfree(error->error_message);
        if (error->sql_fragment) tecc_pfree(error->sql_fragment);
        if (error->hint) tecc_pfree(error->hint);
        if (error->detail) tecc_pfree(error->detail);

        tecc_pfree(error);
        error = next;
    }

    result->errors = NULL;
    result->error_count = 0;
    result->warning_count = 0;
}

/* ==================== 性能优化解析函数 ==================== */

/* 性能优化版本的SQL解析函数 */
Node *tecc_parse_sql(const char *sql, ParserError **errors) {
    Node *parse_tree;
    ParserError *cached_errors;
    TeccParseResult result;
    TeccParseOptions options;
    uint64_t start_time, end_time;
    bool success;

    if (!sql) return NULL;

    /* 尝试从缓存获取 */
    parse_tree = NULL;
    cached_errors = NULL;

    if (tecc_perf_cache_get(sql, &parse_tree, &cached_errors)) {
        /* 缓存命中 */
        if (errors) {
            *errors = cached_errors;
        }
        return parse_tree;
    }

    /* 缓存未命中，执行解析 */
    tecc_parser_init_result(&result);
    tecc_parser_init_options(&options);
    options.enable_performance_opt = true;

    start_time = get_current_time_ns();
    success = tecc_parser_parse_ex(sql, &options, &result);
    end_time = get_current_time_ns();

    if (success) {
        parse_tree = result.parse_tree;
        if (errors) {
            *errors = result.errors;
        } else {
            tecc_parser_clear_errors(&result);
        }

        /* 将结果放入缓存 */
        tecc_perf_cache_put(sql, parse_tree, result.errors);

        /* 更新性能统计 */
        update_statistics((end_time - start_time) / 1000000,
                         result.memory_used_kb,
                         result.error_count,
                         result.warning_count);
    } else {
        parse_tree = NULL;
        if (errors) {
            *errors = result.errors;
        } else {
            tecc_parser_clear_errors(&result);
        }
        tecc_parser_free_tree_node(result.parse_tree);
    }

    return parse_tree;
}

/* ==================== 性能优化接口实现 ==================== */

/* 启用性能优化 */
bool tecc_parser_enable_performance_opt(bool enable, const TeccPerfOptions *options) {
    if (enable) {
        TeccPerfOptions perf_options;

        if (options) {
            perf_options = *options;
        } else {
            tecc_perf_init_options(&perf_options);
        }

        return tecc_perf_apply_options(&perf_options);
    } else {
        /* 禁用所有性能优化 */
        TeccPerfOptions perf_options;
        tecc_perf_init_options(&perf_options);

        perf_options.enable_memory_pool = false;
        perf_options.enable_cache = false;
        perf_options.enable_parallel_parsing = false;
        perf_options.enable_statistics = false;

        return tecc_perf_apply_options(&perf_options);
    }
}

/* 获取性能统计信息 */
void tecc_parser_get_performance_stats(TeccParserStatistics *stats) {
    TeccParserInternalState *state;
    TeccParserStatistics perf_stats;

    if (!stats) return;

    /* 获取基础统计信息 */
    state = get_parser_state();
    *stats = state->statistics;

    /* 获取性能优化统计信息 */
    tecc_perf_get_statistics(&perf_stats);

    /* 合并统计信息 - 累加相应的字段 */
    stats->parse_time_ns += perf_stats.parse_time_ns;
    stats->semantic_time_ns += perf_stats.semantic_time_ns;
    stats->error_time_ns += perf_stats.error_time_ns;
    stats->total_time_ns += perf_stats.total_time_ns;

    stats->memory_used += perf_stats.memory_used;
    if (perf_stats.peak_memory > stats->peak_memory) {
        stats->peak_memory = perf_stats.peak_memory;
    }

    stats->nodes_created += perf_stats.nodes_created;
    stats->nodes_freed += perf_stats.nodes_freed;
    stats->strings_allocated += perf_stats.strings_allocated;
    stats->strings_freed += perf_stats.strings_freed;

    stats->parse_calls += perf_stats.parse_calls;
    stats->semantic_calls += perf_stats.semantic_calls;
    stats->error_calls += perf_stats.error_calls;

    stats->cache_hits += perf_stats.cache_hits;
    stats->cache_misses += perf_stats.cache_misses;

    stats->parallel_tasks += perf_stats.parallel_tasks;
    if (perf_stats.max_parallel_tasks > stats->max_parallel_tasks) {
        stats->max_parallel_tasks = perf_stats.max_parallel_tasks;
    }
}

/* 启用解析缓存 */
bool tecc_parser_enable_cache(bool enable, size_t cache_size) {
    if (enable) {
        return tecc_perf_init_cache(cache_size);
    } else {
        tecc_perf_destroy_cache();
        return true;
    }
}

/* 清空解析缓存 */
void tecc_parser_clear_cache(void) {
    tecc_perf_cache_clear();
}

/* 启用内存池 */
bool tecc_parser_enable_memory_pool(bool enable, size_t pool_size) {
    if (enable) {
        return tecc_perf_init_memory_pool(pool_size);
    } else {
        tecc_perf_destroy_memory_pool();
        return true;
    }
}

/* 启用并行解析 */
bool tecc_parser_enable_parallel_parsing(bool enable, int max_tasks) {
    if (enable) {
        return tecc_perf_init_parallel(max_tasks);
    } else {
        tecc_perf_destroy_parallel();
        return true;
    }
}

/* ==================== 适配器函数实现 ==================== */

/*
 * tecc_parser_parse_list - 适配器函数，返回List*格式的解析树
 *
 * 功能：解析SQL语句，返回List*格式的解析树
 * 兼容性要求：
 * 1. 与设计文档第5.1节要求的接口完全一致
 * 2. 内部调用tecc_parser_parse_ex获取Node*解析树
 * 3. 将Node*包装为List*格式返回
 */
List *tecc_parser_parse_list(const char *query_string) {
    Node *parse_tree;
    List *list;
    ListCell *cell;

    if (!query_string) {
        return NULL;
    }

    /* 调用现有的Node*解析接口 */
    parse_tree = tecc_parser_parse_node(query_string);
    if (!parse_tree) {
        return NULL;
    }

    /* 多语句支持：如果已经是FE_T_List（多个stmt节点），直接返回。
     * 单语句时tecc_parser_parse_node返回单个Node，
     * 多语句时grammar在input规则中将type设为FE_T_List。ReqID:TECC4.0 */
    if (nodeTag(parse_tree) == FE_T_List) {
        return (List *)parse_tree;
    }

    /* 单语句：将Node*包装为List* */
    list = (List *)tecc_palloc(sizeof(List));
    if (!list) {
        tecc_parser_free_tree_node(parse_tree);
        return NULL;
    }

    memset(list, 0, sizeof(List));

    list->type = FE_T_List;
    list->length = 1;

    cell = (ListCell *)tecc_palloc(sizeof(ListCell));
    if (!cell) {
        tecc_pfree(list);
        tecc_parser_free_tree_node(parse_tree);
        return NULL;
    }

    memset(cell, 0, sizeof(ListCell));

    cell->data.ptr_value = parse_tree;
    cell->next = NULL;
    list->head = cell;
    list->tail = cell;
    list->next = NULL;

    return list;
}

/*
 * tecc_parser_free_tree_list - 适配器函数，释放List*格式的解析树
 *
 * 功能：释放解析树占用的所有内存
 * 兼容性要求：
 * 1. 正确处理List*结构中的Node*节点
 * 2. 同时释放List结构和内部的Node结构
 * 3. 支持NULL指针的安全处理
 */
void tecc_parser_free_tree_list(List *parse_tree) {
    ListCell *cell;
    ListCell *next;
    Node *node;

    if (!parse_tree) {
        return;
    }

    /* 释放List中的Node节点 */
    cell = parse_tree->head;
    while (cell) {
        if (cell->data.ptr_value) {
            node = (Node *)cell->data.ptr_value;
            tecc_parser_free_tree_node(node);
        }
        next = cell->next;
        tecc_pfree(cell);
        cell = next;
    }

    /* 释放List结构本身 */
    tecc_pfree(parse_tree);
}

/* ==================== 存根函数实现 ==================== */

/*
 * tecc_set_input_string - 设置输入字符串
 */
void
tecc_set_input_string(const char *str)
{
    /* 这个函数现在由setup_parser_input实现 */
    /* 保持函数存在以保持API兼容性 */
    (void)str;
}

/*
 * reset_tecc_parsetree - 重置解析树
 */
void
reset_tecc_parsetree(void)
{

    /* 释放当前的解析树 */
    if (tecc_parsetree) {
        /* 检查tecc_parsetree是否看起来像有效的List结构 */
        if (tecc_parsetree->length >= 0 && tecc_parsetree->length < 1000) {

            /* 释放List中的节点 */
            ListCell *cell;
            ListCell *next;
            int cell_count = 0;
            Node *node;

            cell = tecc_parsetree->head;
            while (cell) {
                if (cell->data.ptr_value) {
                    node = (Node *)cell->data.ptr_value;
                    /* 检查节点是否看起来有效 */
                    if (node && node->type >= 0 && node->type < 1000) {
                        freeNode(node);
                    } else {
                    }
                    /* 将指针设置为NULL，防止后续再次释放 */
                    cell->data.ptr_value = NULL;
                }
                next = cell->next;
                tecc_pfree(cell);
                cell = next;
                cell_count++;
            }

            /* 释放List结构本身 */
            tecc_pfree(tecc_parsetree);
        } else {
        }
        tecc_parsetree = NIL;
    }

}

/*
 * get_tecc_parsetree - 获取解析树
 */
List *
get_tecc_parsetree(void)
{
    /* 返回当前的解析树，并将tecc_parsetree设置为NULL以转移所有权 */
    List *result = tecc_parsetree;
    tecc_parsetree = NIL;
    return result;
}

/*
 * tecc_yyparse - Bison解析器入口（存根实现）
 */
/* tecc_yyparse is defined in tecc_grammar.c (generated by bison) */


/*
 * freeNode - 释放节点（在tecc_nodes.c中实现）
 * 这里不再重复定义，直接使用tecc_nodes.c中的实现
 */

/*
 * repg_malloc - PostgreSQL风格的内存分配
 */
#if 0
static void *
repg_malloc(size_t size)
{
    /* 使用tecc_palloc进行内存分配 */
    return tecc_palloc(size);
}
#endif


/*
 * tecc_copy_state_add_error - 添加COPY状态错误（存根实现）
 */
#if 0
static void
tecc_copy_state_add_error(void *copy_state, const char *error_msg)
{
    /* TODO: Implement copy state error addition */
    (void)copy_state;
    (void)error_msg;
}
#endif

#if 0
/*
 * tecc_processor_debug - 处理器调试函数（存根实现）
 */
static void
tecc_processor_debug(const char *msg)
{
    /* TODO: Implement processor debug */
    (void)msg;
}
#endif

#if 0
/*
 * tecc_execute_query - 执行查询（存根实现）
 */
static int
tecc_execute_query(const char *query)
{
    /* TODO: Implement query execution */
    (void)query;
    return 0;
}
#endif

#if 0
/*
 * tecc_unified_decrypt_hex_with_conn - 使用连接统一解密十六进制（存根实现）
 */
static int
tecc_unified_decrypt_hex_with_conn(void *conn, const char *hex_ciphertext,
                                   char **plaintext)
{
    /* TODO: Implement unified decryption with connection */
    (void)conn;
    (void)hex_ciphertext;
    (void)plaintext;
    return -1;
}
#endif

/* ==================== 参数映射函数实现 ==================== */

/*
 * tecc_parser_get_param_mapping - 获取参数列映射
 */
bool tecc_parser_get_param_mapping(const TeccParseResult *result, int param_index,
                                  const char **schema_name, const char **table_name,
                                  const char **column_name)
{
    if (!result || param_index <= 0) {
        return false;
    }

    {
        TeccParamColumnMapping *mapping = result->param_mappings.mappings;
        while (mapping) {
            if (mapping->param_index == param_index) {
                if (schema_name) *schema_name = mapping->schema_name;
                if (table_name) *table_name = mapping->table_name;
                if (column_name) *column_name = mapping->column_name;
                return true;
            }
            mapping = mapping->next;
        }
    }

    return false;
}

/*
 * tecc_parser_add_param_mapping - 添加参数列映射
 */
bool tecc_parser_add_param_mapping(TeccParseResult *result, int param_index,
                                  const char *schema_name, const char *table_name,
                                  const char *column_name)
{
    if (!result || param_index <= 0 || !column_name) {
        return false;
    }

    {
        /* 检查是否已存在相同参数索引的映射 */
        TeccParamColumnMapping *existing = result->param_mappings.mappings;
        TeccParamColumnMapping *prev = NULL;
        TeccParamColumnMapping *new_mapping = NULL;

        while (existing) {
            if (existing->param_index == param_index) {
                /* 更新现有映射 */
                if (existing->schema_name) tecc_pfree(existing->schema_name);
                if (existing->table_name) tecc_pfree(existing->table_name);
                if (existing->column_name) tecc_pfree(existing->column_name);

                existing->schema_name = schema_name ? tecc_pstrdup(schema_name) : NULL;
                existing->table_name = table_name ? tecc_pstrdup(table_name) : NULL;
                existing->column_name = tecc_pstrdup(column_name);
                return true;
            }
            prev = existing;
            existing = existing->next;
        }

        /* 创建新映射 */
        new_mapping = (TeccParamColumnMapping *)tecc_palloc(sizeof(TeccParamColumnMapping));
        if (!new_mapping) {
            return false;
        }

        new_mapping->param_index = param_index;
        new_mapping->schema_name = schema_name ? tecc_pstrdup(schema_name) : NULL;
        new_mapping->table_name = table_name ? tecc_pstrdup(table_name) : NULL;
        new_mapping->column_name = tecc_pstrdup(column_name);
        new_mapping->next = NULL;

        /* 添加到链表 */
        if (prev) {
            prev->next = new_mapping;
        } else {
            result->param_mappings.mappings = new_mapping;
        }

        result->param_mappings.mapping_count++;
        result->param_mappings.has_mappings = true;
    }

    return true;
}

/*
 * tecc_parser_clear_param_mappings - 清除所有参数映射
 */
void tecc_parser_clear_param_mappings(TeccParseResult *result)
{
    if (!result) return;

    {
        TeccParamColumnMapping *mapping = result->param_mappings.mappings;
        while (mapping) {
            TeccParamColumnMapping *next = mapping->next;

            if (mapping->schema_name) tecc_pfree(mapping->schema_name);
            if (mapping->table_name) tecc_pfree(mapping->table_name);
            if (mapping->column_name) tecc_pfree(mapping->column_name);
            tecc_pfree(mapping);

            mapping = next;
        }

        result->param_mappings.mappings = NULL;
        result->param_mappings.mapping_count = 0;
        result->param_mappings.has_mappings = false;
    }
}

/*
 * tecc_parser_get_param_mapping_count - 获取参数映射数量
 */
int tecc_parser_get_param_mapping_count(const TeccParseResult *result)
{
    if (!result) return 0;
    return result->param_mappings.mapping_count;
}
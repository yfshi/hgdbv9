#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "libpq-fe.h"  /* For PQdb, PQexec, PQgetvalue */
#include <string.h>  /* For strlen, strcpy, strcat */
/* Begin - ReqID:TECC4.0 */
/*
 * File: create_stmt_processor.c
 *
 * Abstract:
 *   CREATE statement processor implementation for TECC processor module.
 *   Implements processing of CREATE TABLE statements including
 *   constraint checking and encrypted column handling.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/create_stmt_processor.c
 */

#include "create_stmt_processor.h"

/* Include expression processor interface for TeccColumnRefData */
#include "column_ref_data.h"

/* Include node type definitions for IsA macro */
#include "../frontend_parser/tecc_nodes.h"

/* ReqID:TECC4.0 - Encryption and cache interfaces for DEFAULT encryption */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
/* Modify - ReqID:TECC4.0: P1-1 校验 DEK key_algo 需查询 hg_dek_args（tecc_he_load_dek_key_hex） */
#include "../tecc_scheduler/tecc_algorithm_scheduler.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"

/* ReqID:TECC4.0 - Internal libpq header for enable_tecc flag */
#include "libpq-int.h"

/* Constraint type constants are now defined in create_stmt_processor.h */

/*
 * Forward declarations
 */
static TeccCreateStmtProcessorResult create_process_columns(const CreateStmt *create_stmt,
                                                           TeccCreateStmtProcessorContext *context,
                                                           TeccStatementData *statement_data);
static void create_context_init(TeccCreateStmtProcessorContext *context);
static void create_context_cleanup(TeccCreateStmtProcessorContext *context);

/* ReqID:TECC4.0 - Forward declarations for DEFAULT encryption helpers */
Oid map_enc_type_name_to_base_oid(const char *type_name);
static const char *parse_support_algo_flags(const char *algo_value,
    bool *has_det, bool *has_ope, bool *has_search,
    bool *has_he_add, bool *has_he_mul);

/* ReqID:TECC4.0 - Forward declarations for partition boundary encryption */
static void encrypt_partition_bounds(CreateStmt *stmt,
                                     TeccCreateStmtProcessorContext *context,
                                     TeccStatementData *statement_data);

/*
 * ReqID:TECC4.0 - P1-1 查询 DEK 的 key_algo
 * 返回 0 成功（buf 填充 key_algo），非 0 失败（查询错误）。
 * 注意：传统对称 DEK 不存储 key_algo 条目，视为 'sm4'（默认）。
 */
static int
tecc_dek_get_key_algo(TeccState *state, const char *dek_name,
                      char *buf, size_t bufsz)
{
    char   *kt_raw = NULL;

    if (state == NULL || dek_name == NULL || buf == NULL || bufsz == 0)
        return -1;

    /* 只读新规范名 key_algo；传统对称 DEK 不存储 key_algo，默认为 sm4 */
    if (tecc_he_load_dek_key_hex(state, dek_name, "key_algo", &kt_raw) != 0 ||
        kt_raw == NULL || kt_raw[0] == '\0')
    {
        if (kt_raw)
            pg_free(kt_raw);
        strlcpy(buf, "sm4", bufsz);
        return 0;
    }

    strlcpy(buf, kt_raw, bufsz);
    pg_free(kt_raw);

    return 0;
}

/*
 * Encryption-supported column types
 */
static const struct {
    const char *type_name;
    const char *algorithm;
} supported_types[] = {
    {"int2_enc", "DET"},
    {"int4_enc", "DET"},
    {"int8_enc", "DET"},
    {"float4_enc", "OPE"},
    {"float8_enc", "OPE"},
    {"numeric_enc", "OPE"},
    {"text_enc", "DET"},
    {"varchar_enc", "DET"},
    {"bpchar_enc", "DET"},
    {NULL, NULL}
};

/*
 * ReqID:TECC4.0 - Bug 7972: 在 SQL 文本中定位 "关键字 值" 形态的模式。
 *
 * 关键字部分（pattern 的前 keyword_len 个字符）按大小写不敏感比较，其余部分
 * 仍按原样逐字节比较（值来自同一条 SQL 的解析结果，必须完全一致）。
 *
 * 原实现只是"先找大写 DEFAULT，找不到再找全小写 default"，混合大小写
 * （DeFaUlT/Default/dEfAuLt）两种都匹配不上 → DEFAULT 重写被静默跳过 →
 * 明文原样下发后端 → cipherdata_in 报 invalid hex string。SQL 关键字本身
 * 大小写不敏感，能写出 'DeFaUlT' 就必须能加密。
 *
 * 返回值：匹配位置（指向 haystack 内部），无匹配返回 NULL。
 * 注：调用方用 strlen(pattern) 计算匹配长度，故匹配段与 pattern 等长。
 */
static char *
tecc_find_pattern_keyword_ci(const char *haystack, const char *pattern,
                             size_t keyword_len)
{
    size_t pat_len;
    char  *p;

    if (haystack == NULL || pattern == NULL)
        return NULL;

    pat_len = strlen(pattern);
    if (pat_len < keyword_len)
        return NULL;

    p = (char *) haystack;
    for (; *p != '\0'; p++)
    {
        if (pg_strncasecmp(p, pattern, keyword_len) == 0 &&
            strncmp(p + keyword_len, pattern + keyword_len,
                    pat_len - keyword_len) == 0)
            return p;
    }

    return NULL;
}

/*
 * Main CREATE statement processing entry point
 */
TeccCreateStmtProcessorResult
tecc_create_stmt_processor_run(const CreateStmt *create_stmt,
                              TeccStatementData *statement_data)
{
    TeccCreateStmtProcessorContext context;
    TeccCreateStmtProcessorResult result;

    if (create_stmt == NULL || statement_data == NULL)
        return TECC_CREATE_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Starting CREATE statement processing");

    /* Initialize context */
    create_context_init(&context);

    /* Process table information */
    if (create_stmt->relation != NULL)
    {
        if (create_stmt->relation->schemaname != NULL)
            context.schema_name = pg_strdup(create_stmt->relation->schemaname);
        if (create_stmt->relation->relname != NULL)
            context.table_name = pg_strdup(create_stmt->relation->relname);
    }

    /* Process columns */
    result = create_process_columns(create_stmt, &context, statement_data);
    if (result != TECC_CREATE_PROCESSOR_SUCCESS)
        goto cleanup;

    /* ReqID:TECC4.0 - 临时表处理：
     * 1) TEMP + 加密列：明确拒绝（后端 DefineRelation 同样拒绝，防孤儿元数据行）；
     * 2) 普通 TEMP 表：记录到会话级临时表清单，未限定表名解析时临时表优先
     *    （与 PG 的 pg_temp 优先语义一致，避免误解析到 search_path 同名表）。 */
    if (create_stmt->is_temp)
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL && context.has_encrypted_columns)
        {
            tecc_processor_set_error(statement_data,
                TECC_PROCESSOR_ERROR,
                "TECC does not support temporary encrypted tables");
            result = TECC_CREATE_PROCESSOR_ERROR;
            goto cleanup;
        }
        if (state != NULL && create_stmt->relation != NULL)
            tecc_state_add_temp_table(state, create_stmt->relation->relname);
    }

    /* ReqID:TECC4.0 - Encrypt partition boundary values */
    encrypt_partition_bounds((CreateStmt *)create_stmt, &context, statement_data);
    if (statement_data->error_code != 0)
    {
        result = TECC_CREATE_PROCESSOR_ERROR;
        tecc_processor_debug(TECC_LOG_ERROR,
            "Partition boundary encryption failed: %s",
            statement_data->error_message);
        goto cleanup;
    }

    /* ReqID:TECC4.0 - Rewrite DEFAULT values with encrypted hex */
    if (context.default_encryptions != NIL)
    {
        const char *query = statement_data->query;
        char *current_query;
        ListCell *lc;

        if (query != NULL)
        {
            current_query = pg_strdup(query);
            if (current_query != NULL)
            {
                foreach(lc, context.default_encryptions)
                {
                    TeccDefaultEncInfo *info = (TeccDefaultEncInfo *) lfirst(lc);
                    char *search_pattern;
                    char *replace_pattern;
                    char *found_pos;
                    char *new_query;
                    size_t search_len;
                    size_t replace_len;
                    size_t prefix_len;
                    size_t new_len;
                    size_t suffix_len;
                    const char *bare_hex;
                    const char *col_type;

                    /* Build search pattern: "DEFAULT <plaintext_value>"
                     * ReqID:TECC4.0 - Bug 7972: SQL 关键字大小写不敏感，
                     * DEFAULT / default / DeFaUlT 等任意写法都要能定位。
                     *
                     * 动态分配（原先 search_pattern/replace_pattern 是定长栈数组）：
                     * 同态密文的 hex 有上千字符，定长 snprintf 会把替换文本截断，
                     * 截断后引号不闭合 —— 服务端只报 unterminated quoted string，
                     * 看不出是前端截断。 */
                    search_len = strlen(info->plaintext_value) + strlen("DEFAULT ") + 1;
                    search_pattern = (char *)pg_malloc(search_len);
                    snprintf(search_pattern, search_len,
                             "DEFAULT %s", info->plaintext_value);
                    found_pos = tecc_find_pattern_keyword_ci(current_query,
                                                             search_pattern,
                                                             strlen("DEFAULT"));
                    if (found_pos == NULL)
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                            "Cannot find DEFAULT pattern '%s' in query for column '%s'",
                            search_pattern, info->column_name);
                        pg_free(search_pattern);
                        continue;
                    }

                    /* Build replacement: "DEFAULT E'\\x<hex>'::<type>"
                     * ReqID:TECC4.0 - E-string for scs compatibility.
                     * Strip \x prefix from encrypted_hex for E'' context. */
                    bare_hex = info->encrypted_hex;
                    if (bare_hex != NULL &&
                        bare_hex[0] == '\\' && bare_hex[1] == 'x')
                        bare_hex += 2;
                    if (bare_hex == NULL)
                        bare_hex = "";
                    col_type = (info->column_type != NULL) ? info->column_type : "text";
                    /* 注意格式串里是两个反斜杠（E'' 上下文的 \\x），长度按同形计算 */
                    replace_len = strlen(bare_hex) + strlen(col_type) +
                                  strlen("DEFAULT E'\\\\x'::") + 1;
                    replace_pattern = (char *)pg_malloc(replace_len);
                    snprintf(replace_pattern, replace_len,
                             "DEFAULT E'\\\\x%s'::%s", bare_hex, col_type);

                    /* Calculate positions */
                    prefix_len = (size_t)(found_pos - current_query);
                    new_len = prefix_len + strlen(replace_pattern) +
                              strlen(found_pos + strlen(search_pattern)) + 1;

                    new_query = (char *)pg_malloc(new_len);
                    if (new_query == NULL)
                    {
                        pg_free(search_pattern);
                        pg_free(replace_pattern);
                        continue;
                    }

                    /* Build new query: prefix + replacement + suffix */
                    memcpy(new_query, current_query, prefix_len);
                    memcpy(new_query + prefix_len, replace_pattern, strlen(replace_pattern));
                    suffix_len = strlen(found_pos + strlen(search_pattern));
                    memcpy(new_query + prefix_len + strlen(replace_pattern),
                           found_pos + strlen(search_pattern), suffix_len + 1);

                    pg_free(current_query);
                    current_query = new_query;

                    pg_free(search_pattern);
                    pg_free(replace_pattern);

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Rewritten DEFAULT for column '%s': '%s' -> hex",
                        info->column_name, info->plaintext_value);
                }

                statement_data->rewritten_query = current_query;
                statement_data->needs_rewrite = 1;
            }
        }
    }

    /* ReqID:TECC4.0 - Rewrite FOR VALUES boundary values with encrypted hex */
    if (context.bound_encryptions != NIL)
    {
        const char *query = statement_data->query;
        char *current_query;

        if (query != NULL)
        {
            current_query = statement_data->rewritten_query ?
                pg_strdup(statement_data->rewritten_query) : pg_strdup(query);

            if (current_query != NULL)
            {
                ListCell *lc;
                int offset_adjust = 0;
                foreach(lc, context.bound_encryptions)
                {
                    TeccBoundEncInfo *info = (TeccBoundEncInfo *) lfirst(lc);
                    char replacement[1024];
                    int repl_len;
                    int adj_start;
                    int adj_end;
                    int old_len;        /* ReqID:TECC4.0 - actual length being replaced */
                    size_t new_len;
                    char *new_query;

                    /* Build replacement: E'\\x<hex>' — strip \x prefix from
                     * encrypted_hex to avoid double-escape in E'...' context.
                     * ReqID:TECC4.0 - Add ::type_enc cast for numeric encrypted types
                     * so the server doesn't try to cast the hex string to int2 etc. */
                    {
                        const char *hex = info->encrypted_hex;

                        if (hex[0] == '\\' && hex[1] == 'x')
                            hex += 2;
                        if (info->column_type != NULL)
                            repl_len = snprintf(replacement, sizeof(replacement),
                                                "E'\\\\x%s'::%s",
                                                hex, info->column_type);
                        else
                            repl_len = snprintf(replacement, sizeof(replacement),
                                                "E'\\\\x%s'", hex);
                    }
                    if (repl_len >= (int)sizeof(replacement))
                        continue;

                    adj_start = info->value_location + offset_adjust;
                    adj_end = info->value_location + info->value_length + offset_adjust;
                    old_len = info->value_length;

                    /* Skip any ::typename that follows the value in SQL.
                     * E.g. "(-32767::int2)" — value is "-32767", "::int2" follows.
                     * We replace both with "E'\\x...'::int2_enc". */
                    if (adj_end < (int)strlen(current_query) &&
                        current_query[adj_end] == ':' &&
                        adj_end + 1 < (int)strlen(current_query) &&
                        current_query[adj_end + 1] == ':')
                    {
                        int cast_end = adj_end + 2;
                        while (cast_end < (int)strlen(current_query) &&
                               (isalnum((unsigned char)current_query[cast_end]) ||
                                current_query[cast_end] == '_'))
                            cast_end++;
                        old_len = cast_end - adj_start;
                        adj_end = cast_end;
                    }

                    if (adj_start < 0 || adj_end > (int)strlen(current_query))
                        continue;

                    new_len = strlen(current_query) - old_len + repl_len;
                    new_query = (char *)pg_malloc(new_len + 1);

                    memcpy(new_query, current_query, adj_start);
                    memcpy(new_query + adj_start, replacement, repl_len);
                    memcpy(new_query + adj_start + repl_len,
                           current_query + adj_end,
                           strlen(current_query) - adj_end + 1);

                    pg_free(current_query);
                    current_query = new_query;
                    offset_adjust += repl_len - old_len;

                    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Rewritten FOR VALUES boundary: '%s' -> hex",
                        info->plaintext_value);
                }

                if (statement_data->rewritten_query != NULL)
                    pg_free(statement_data->rewritten_query);
                statement_data->rewritten_query = current_query;
                statement_data->needs_rewrite = 1;
            }
        }
    }

    /* Update statement data if encrypted columns found */
    if (context.has_encrypted_columns)
    {
        statement_data->encrypted_columns = context.encrypted_columns;
        statement_data->needs_rewrite = 1;
        /* ReqID:TECC4.0 - Ownership transferred: prevent cleanup from freeing
         * the list that statement_data now owns.  create_context_cleanup calls
         * tecc_processor_list_free_deep() which would free both the list and
         * its elements, leaving statement_data->encrypted_columns dangling. */
        context.encrypted_columns = NIL;
    }


cleanup:
    create_context_cleanup(&context);
    return result;
}

/*
 * Process CREATE TABLE statement
 */
TeccCreateStmtProcessorResult
tecc_create_stmt_processor_process_table(const CreateStmt *create_stmt,
                                        TeccStatementData *statement_data)
{
    return tecc_create_stmt_processor_run(create_stmt, statement_data);
}

/*
 * Check column definition
 */
TeccCreateStmtProcessorResult
tecc_create_stmt_processor_check_column(const ColumnDef *column_def,
                                       TeccCreateStmtProcessorContext *context,
                                       TeccStatementData *statement_data)
{
    bool supports_encryption;
    TeccColumnRefData *col_data = NULL;

    if (column_def == NULL || context == NULL)
        return TECC_CREATE_PROCESSOR_ERROR;

    if (column_def->colname == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "Column name is NULL");
        return TECC_CREATE_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Checking column: %s", column_def->colname);

    /* Check if column type supports encryption */
    if (column_def->typeName != NULL && column_def->typeName->names != NIL)
    {
        Node *type_node = (Node *) llast(column_def->typeName->names);
        if (IsA(type_node, String) || IsA(type_node, Const))
        {
            char *type_name = strVal((Value *)type_node);

            supports_encryption = tecc_create_stmt_processor_type_supports_encryption(type_name);
            if (supports_encryption)
            {
                context->has_encrypted_columns = true;
                /* Create TeccColumnRefData for encrypted column */
                col_data = tecc_column_ref_data_create();
                if (col_data != NULL)
                {
                    /* ReqID:TECC4.0 - Pass schema from CREATE TABLE relation so
                     * that last_table_schema is set correctly for post-DDL cache
                     * refresh. Without this, deferred DDL refresh queries the
                     * wrong schema (e.g. public instead of sch_t), causing
                     * subsequent INSERT to miss encrypted column metadata. */
                    tecc_column_ref_data_init(col_data, NULL,
                        context->schema_name,
                        context->table_name, column_def->colname);
                    /* ReqID:TECC4.0 - pending_tables: store original _enc type name */
                    if (type_name != NULL)
                    {
                        strncpy(col_data->original_type_name, type_name,
                                sizeof(col_data->original_type_name) - 1);
                        col_data->original_type_name[
                            sizeof(col_data->original_type_name) - 1] = '\0';
                    }
                    /* ReqID:TECC4.0 - Store attnum for pending_tables column_attnum mapping.
                     * Temporarily reuse column_oid (which is InvalidOid during preprocessing)
                     * to carry the attnum. This avoids adding a field to TeccColumnRefData. */
                    col_data->column_oid = context->current_attnum;
                    /* ReqID:TECC4.0 - Extract collation locale from CollateClause for
                     * pending_tables/cache. The locale (e.g. "zh_CN.utf8") is needed
                     * by the crypto scheduler to generate ICU sort keys for OPE. */
                    if (column_def->collClause != NULL)
                    {
                        CollateClause *cc = (CollateClause *)column_def->collClause;
                        if (cc->collname != NIL)
                        {
                            Node *last_node = (Node *)llast(cc->collname);
                            if (IsA(last_node, String) || IsA(last_node, Const))
                            {
                                const char *locale = strVal((Value *)last_node);
                                if (locale != NULL && locale[0] != '\0')
                                {
                                    strncpy(col_data->collation_locale, locale,
                                            sizeof(col_data->collation_locale) - 1);
                                    col_data->collation_locale[
                                        sizeof(col_data->collation_locale) - 1] = '\0';
                                }
                            }
                        }
                    }
                    context->encrypted_columns = lappend(context->encrypted_columns, col_data);
                }

                    /* Extract ENCRYPTED WITH options (dek, support_algo) */
                    if (column_def->typeName->encryptedOpts != NIL)
                    {
                        ListCell *opt_lc;
                        foreach(opt_lc, column_def->typeName->encryptedOpts)
                        {
                            DefElem *opt = (DefElem *) lfirst(opt_lc);
                            if (strcasecmp(opt->defname, "dek") == 0 && opt->arg != NULL)
                            {
                                char *dek_value = strVal((Value *)opt->arg);
                                tecc_column_ref_data_set_dek_name(col_data, dek_value);

                            }
                            /* ReqID:TECC4.0 - 一列多 DEK：he-add/he-mul 各自的同态 DEK */
                            else if (strcasecmp(opt->defname, "he_add_dek") == 0 && opt->arg != NULL)
                            {
                                char *dek_value = strVal((Value *)opt->arg);
                                tecc_column_ref_data_set_he_add_dek_name(col_data, dek_value);
                            }
                            else if (strcasecmp(opt->defname, "he_mul_dek") == 0 && opt->arg != NULL)
                            {
                                char *dek_value = strVal((Value *)opt->arg);
                                tecc_column_ref_data_set_he_mul_dek_name(col_data, dek_value);
                            }
                            else if (strcasecmp(opt->defname, "support_algo") == 0 && opt->arg != NULL)
                            {
                                char *algo_value = strVal((Value *)opt->arg);
                                /* ReqID:TECC4.0 - Check DET+OPE mutual exclusion */
                                {
                                    bool has_det = false, has_ope = false;
                                    char *algo_copy = pg_strdup(algo_value);
                                    char *saveptr = NULL;
                                    char *token = strtok_r(algo_copy, ",", &saveptr);
                                    while (token != NULL)
                                    {
                                        while (*token == ' ') token++;
                                        {
                                            char *end = token + strlen(token) - 1;
                                            while (end > token && *end == ' ') end--;
                                            *(end + 1) = '\0';
                                        }
                                        if (strcasecmp(token, "det") == 0) has_det = true;
                                        if (strcasecmp(token, "ope") == 0) has_ope = true;
                                        token = strtok_r(NULL, ",", &saveptr);
                                    }
                                    pg_free(algo_copy);
                                    if (has_det && has_ope)
                                    {
                                        tecc_processor_set_error(statement_data,
                                            TECC_PROCESSOR_INVALID_INPUT,
                                            "DET and OPE algorithms are mutually exclusive "
                                            "for column '%s'. Choose either 'det' or 'ope', not both.",
                                            column_def->colname);
                                        return TECC_CREATE_PROCESSOR_ERROR;
                                    }
                                }
                                tecc_column_ref_data_set_support_algo(col_data, algo_value);
                                
                            }
                        }
                    }
                    }

            /* ReqID:TECC4.0 - 一列多 DEK 校验：
             * 仅当列显式提供了 HE_ADD_DEK/HE_MUL_DEK（进入多 DEK 体系）时，
             * 才校验其存在且与 SUPPORT_ALGO 对应。若列只用单个 DEK 服务所有
             * 算法（未提供 HE_ADD_DEK/HE_MUL_DEK），保持 V4.0.0 兼容行为，
             * 不做强制拆分。 */
            {
                const char *col_algo = tecc_column_ref_data_get_support_algo(col_data);
                const char *he_add_dek = tecc_column_ref_data_get_he_add_dek_name(col_data);
                const char *he_mul_dek = tecc_column_ref_data_get_he_mul_dek_name(col_data);
                bool c_has_he_add = false, c_has_he_mul = false;
                bool c_dummy_det = false, c_dummy_ope = false, c_dummy_search = false;
                bool multi_dek_mode = (he_add_dek != NULL || he_mul_dek != NULL);
                TeccState *col_state;

                if (col_algo != NULL)
                    parse_support_algo_flags(col_algo, &c_dummy_det, &c_dummy_ope,
                                             &c_dummy_search, &c_has_he_add, &c_has_he_mul);

                if (!multi_dek_mode)
                {
                    /* ReqID:TECC4.0 - 单 DEK 兼容模式校验：列启用 HE 算法但
                     * 未提供 HE_ADD_DEK/HE_MUL_DEK（V4.0.0 单 DEK 风格）时，
                     * DEK 参数必须是 HE DEK（key_algo='he-add' 或 'he-mul'），
                     * 防止 sm4 DEK 用于 HE 列导致后续运算/解密加载密钥失败。
                     * 单 DEK 可同时服务 he-add/he-mul（V4.0.0 兼容，P0-2
                     * 多 DEK 落地前的过渡形态）。 */
                    char algo_buf2[16];
                    const char *dek_name2;

                    col_state = tecc_get_state(statement_data->conn);
                    if (col_state != NULL && (c_has_he_add || c_has_he_mul))
                    {
                        dek_name2 = tecc_column_ref_data_get_dek_name(col_data);

                        if (dek_name2 == NULL || dek_name2[0] == '\0' ||
                            tecc_dek_get_key_algo(col_state, dek_name2,
                                                  algo_buf2, sizeof(algo_buf2)) != 0 ||
                            (strcmp(algo_buf2, "he-add") != 0 &&
                             strcmp(algo_buf2, "he-mul") != 0))
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_INVALID_INPUT,
                                "DEK '%s' for HE column '%s' must be a HE DEK "
                                "(KEY_ALGO='he-add' or 'he-mul')",
                                dek_name2 ? dek_name2 : "(none)",
                                column_def->colname);
                            return TECC_CREATE_PROCESSOR_ERROR;
                        }
                    }
                }
                else
                {
                    col_state = tecc_get_state(statement_data->conn);

                    if (c_has_he_add && he_add_dek == NULL)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_INVALID_INPUT,
                            "Column '%s' enables he-add but no HE_ADD_DEK specified",
                            column_def->colname);
                        return TECC_CREATE_PROCESSOR_ERROR;
                    }
                    if (c_has_he_mul && he_mul_dek == NULL)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_INVALID_INPUT,
                            "Column '%s' enables he-mul but no HE_MUL_DEK specified",
                            column_def->colname);
                        return TECC_CREATE_PROCESSOR_ERROR;
                    }
                /* ReqID:TECC4.0 - HE_ADD_DEK/HE_MUL_DEK 的存在性与 key_algo 校验
                 * 统一由下方 tecc_dek_get_key_algo 完成（支持 qualified 名如
                 * 'public.dek_env_he'），不再用 search_path 查找（它不支持
                 * schema 限定名）。 */

                    /* ReqID:TECC4.0 - P1-1 key_algo 匹配校验：
                     * HE_ADD_DEK 必须指向 key_algo='he-add' 的 DEK，
                     * HE_MUL_DEK 必须指向 key_algo='he-mul' 的 DEK；
                     * 对称算法（DET/OPE/SEARCH）开启时，必须提供对称 DEK
                     * （DEK 参数）且其 key_algo='sm4'。 */
                    if (col_state != NULL)
                    {
                        char algo_buf[16];

                        if (he_add_dek != NULL &&
                            (tecc_dek_get_key_algo(col_state, he_add_dek,
                                                   algo_buf, sizeof(algo_buf)) != 0 ||
                             strcmp(algo_buf, "he-add") != 0))
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_INVALID_INPUT,
                                "HE_ADD_DEK '%s' for column '%s' must be a he-add DEK "
                                "(KEY_ALGO='he-add')", he_add_dek, column_def->colname);
                            return TECC_CREATE_PROCESSOR_ERROR;
                        }
                        if (he_mul_dek != NULL &&
                            (tecc_dek_get_key_algo(col_state, he_mul_dek,
                                                   algo_buf, sizeof(algo_buf)) != 0 ||
                             strcmp(algo_buf, "he-mul") != 0))
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_INVALID_INPUT,
                                "HE_MUL_DEK '%s' for column '%s' must be a he-mul DEK "
                                "(KEY_ALGO='he-mul')", he_mul_dek, column_def->colname);
                            return TECC_CREATE_PROCESSOR_ERROR;
                        }
                        if (c_dummy_det || c_dummy_ope || c_dummy_search)
                        {
                            const char *sym_dek = tecc_column_ref_data_get_dek_name(col_data);

                            if (sym_dek == NULL || sym_dek[0] == '\0')
                            {
                                tecc_processor_set_error(statement_data,
                                    TECC_PROCESSOR_INVALID_INPUT,
                                    "Column '%s' enables symmetric algorithms "
                                    "(DET/OPE/SEARCH) but no DEK specified", column_def->colname);
                                return TECC_CREATE_PROCESSOR_ERROR;
                            }
                            if (tecc_dek_get_key_algo(col_state, sym_dek,
                                                      algo_buf, sizeof(algo_buf)) != 0 ||
                                strcmp(algo_buf, "sm4") != 0)
                            {
                                tecc_processor_set_error(statement_data,
                                    TECC_PROCESSOR_INVALID_INPUT,
                                    "DEK '%s' for symmetric algorithms of column '%s' "
                                    "must be a sm4 DEK (KEY_ALGO='sm4')", sym_dek, column_def->colname);
                                return TECC_CREATE_PROCESSOR_ERROR;
                            }
                        }
                    }
                }
            }

            /* ReqID:TECC4.0 - Collation + OPE validation:
             * Character encrypted columns (text_enc/varchar_enc/bpchar_enc)
             * with COLLATE require support_algo to include 'ope'. Collation
             * implies ordering/comparison operations which need OPE. */
            if (column_def->collClause != NULL &&
                (strcasecmp(type_name, "text_enc") == 0 ||
                 strcasecmp(type_name, "varchar_enc") == 0 ||
                 strcasecmp(type_name, "bpchar_enc") == 0))
            {
                const char *algo_str2;
                bool h_det = false, h_ope = false, h_search = false;
                bool h_he_add = false, h_he_mul = false;
                algo_str2 = tecc_column_ref_data_get_support_algo(col_data);
                if (parse_support_algo_flags(algo_str2, &h_det, &h_ope, &h_search,
                                             &h_he_add, &h_he_mul) == NULL && !h_ope)
                {
                    tecc_processor_set_error(statement_data,
                        TECC_PROCESSOR_INVALID_INPUT,
                        "Column '%s' with COLLATE requires support_algo to include 'ope'. "
                        "Set support_algo = 'ope' for this column.",
                        column_def->colname);
                    return TECC_CREATE_PROCESSOR_ERROR;
                }
            }

            /* ReqID:TECC4.0 - Handle DEFAULT constant encryption for encrypted columns */


            /* ReqID:TECC4.0 - Handle DEFAULT constant encryption for encrypted columns */
            if (column_def->constraints != NIL)
            {
                ListCell *lc;
                const char *dek_name;
                const char *algo_str;
                const char *type_name;

                foreach(lc, column_def->constraints)
                {
                    Node *cnstr = (Node *) lfirst(lc);

                    if (!IsA(cnstr, Constraint))
                        continue;

                    {
                        Constraint *constraint = (Constraint *) cnstr;

                        if (constraint->contype != CONSTR_DEFAULT || constraint->raw_expr == NULL)
                            continue;

                        if (!IsA(constraint->raw_expr, Const))
                        {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                "Non-constant DEFAULT on encrypted column '%s' - encryption skipped",
                                column_def->colname);
                            continue;
                        }

                        {
                            Const *default_const = (Const *) constraint->raw_expr;
                            char plaintext_buf[256];
                            const char *plaintext_value;
                            TeccState *state;
                            TeccDekCacheItem *dek_item;
                            const char *kek_name;
                            Oid base_oid;
                            bool has_det, has_ope, has_search;
                            bool has_he_add = false, has_he_mul = false;
                            TeccUnifiedEncryptResult *enc_result;
                        const char *encrypt_value;
                            TeccDefaultEncInfo *info;
                            const char *algo_err;
                            char *collation_locale;
                            int32 local_typmod;
                            const char *he_add_dek_name;
                            const char *he_mul_dek_name;

                            plaintext_buf[0] = '\0';

                            /* Format plaintext value from Const.
                             * plaintext_buf: SQL representation (with quotes for strings)
                             * encrypt_value: raw value for encryption (without quotes) */
                            {

                                if (default_const->constisnull)
                                {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "NULL DEFAULT on encrypted column '%s' - encryption skipped",
                                        column_def->colname);
                                    continue;
                                }

                                /* Integer types */
                                if (default_const->consttype == 23 /* INT4OID */ ||
                                    default_const->consttype == 21 /* INT2OID */ ||
                                    default_const->consttype == 20 /* INT8OID */)
                                {
                                    snprintf(plaintext_buf, sizeof(plaintext_buf),
                                             "%ld", (long)default_const->constvalue.ival);
                                    encrypt_value = plaintext_buf;
                                }
                                /* Float types */
                                else if (default_const->consttype == 700 /* FLOAT4OID */ ||
                                         default_const->consttype == 701 /* FLOAT8OID */ ||
                                         default_const->consttype == 1700 /* NUMERICOID */)
                                {
                                    snprintf(plaintext_buf, sizeof(plaintext_buf),
                                             "%g", default_const->constvalue.dval);
                                    encrypt_value = plaintext_buf;
                                }
                                /* String/unknown type */
                                else if (default_const->constvalue.sval != NULL)
                                {
                                    encrypt_value = default_const->constvalue.sval;
                                    plaintext_buf[0] = '\'';
                                    strlcpy(plaintext_buf + 1, encrypt_value,
                                            sizeof(plaintext_buf) - 2);
                                    strlcat(plaintext_buf, "'", sizeof(plaintext_buf));
                                }
                                else
                                {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "Unsupported DEFAULT value type on encrypted column '%s' - encryption skipped",
                                        column_def->colname);
                                    continue;
                                }

                                plaintext_value = plaintext_buf;
                            }

                            /* Get DEK from cache。
                             * ReqID:TECC4.0 - 纯同态列没有对称 DEK（ENCRYPTED WITH
                             * 里只写了 HE_ADD_DEK/HE_MUL_DEK），与列路径同一处理：用
                             * 同态 DEK 名当 dek_name（见 tecc_column_cache.c 里从缓存项
                             * 构造列元数据处），否则纯 HE 列的 DEFAULT 会被静默跳过。 */
                            he_add_dek_name =
                                tecc_column_ref_data_get_he_add_dek_name(col_data);
                            he_mul_dek_name =
                                tecc_column_ref_data_get_he_mul_dek_name(col_data);
                            dek_name = tecc_column_ref_data_get_dek_name(col_data);
                            if (dek_name == NULL || dek_name[0] == '\0')
                                dek_name = (he_add_dek_name != NULL &&
                                            he_add_dek_name[0] != '\0')
                                           ? he_add_dek_name : he_mul_dek_name;
                            if (dek_name == NULL || dek_name[0] == '\0')
                            {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                    "No DEK name for encrypted column '%s' - DEFAULT encryption skipped",
                                    column_def->colname);
                                continue;
                            }

                            state = tecc_get_state(statement_data->conn);
                            if (state == NULL)
                            {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                    "No TECC state for DEFAULT encryption on column '%s'",
                                    column_def->colname);
                                continue;
                            }

                            /* ReqID:TECC4.0 - Flush pending KEK/DEK before lookup */
                            tecc_accept_pending_statements(statement_data->conn);

                            dek_item = tecc_cache_get_dek_by_name_search_path(state, dek_name);
                            if (dek_item == NULL || dek_item->kek_name == NULL)
                            {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                    "DEK '%s' not found in cache for DEFAULT encryption on column '%s'",
                                    dek_name, column_def->colname);
                                continue;
                            }
                            kek_name = dek_item->kek_name;

                            /* Map type to base OID and parse algorithm flags */
                            type_name = strVal((Value *) llast(column_def->typeName->names));
                            base_oid = map_enc_type_name_to_base_oid(type_name);
                            if (base_oid == InvalidOid)
                            {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Cannot map type '%s' for DEFAULT encryption on column '%s'",
                                    type_name, column_def->colname);
                                continue;
                            }

                            algo_str = tecc_column_ref_data_get_support_algo(col_data);
                            algo_err = parse_support_algo_flags(algo_str, &has_det, &has_ope, &has_search,
                                                               &has_he_add, &has_he_mul);
                            if (algo_err != NULL)
                            {
                                tecc_processor_debug(TECC_LOG_ERROR,
                                    "SUPPORT_ALGO error for column '%s': %s",
                                    column_def->colname, algo_err);
                                continue;
                            }

                            /* ReqID:TECC4.0 - 该列尚未入库、缓存必然 miss，必须
                             * 显式给出将来生效的 collation locale，否则 DET/OPE
                             * 归一化与查询路径不一致，DEFAULT 行等值查不到。 */
                            collation_locale = resolve_new_column_collation_locale(
                                statement_data->conn, base_oid,
                                (CollateClause *)column_def->collClause);

                            /* ReqID:TECC4.0 - DEFAULT 值同样要按列声明的长度校验：
                             * 加密列语法里 varchar_enc(N)/bpchar_enc(N) 的 N 原样存在
                             * typemod，而调度器判据要的是 atttypmod（N+VARHDRSZ）。
                             * 不换算就等于不校验 —— `varchar_enc(3) DEFAULT 'abcdefghij'`
                             * 会被静默接受，而同一张表用 INSERT 写同样长的值会报
                             * value too long for type character varying(3)。 */
                            local_typmod = -1;
                            if (column_def->typeName != NULL &&
                                column_def->typeName->typemod >= 0 &&
                                (base_oid == 1043 /* VARCHAROID */ ||
                                 base_oid == 1042 /* BPCHAROID */))
                                local_typmod = column_def->typeName->typemod + VARHDRSZ;

                            /* Encrypt the DEFAULT value */
                            enc_result = tecc_unified_encrypt_string_with_options_and_collation(
                                statement_data->conn,
                                encrypt_value,
                                context->table_name ? context->table_name : "unknown",
                                column_def->colname,
                                kek_name,
                                dek_name,
                                base_oid,
                                local_typmod,
                                has_det,
                                has_ope,
                                has_search,
                                has_he_add,
                                has_he_mul,
                                collation_locale,
                                he_add_dek_name,
                                he_mul_dek_name,
                                NULL);

                            if (collation_locale != NULL)
                                pg_free(collation_locale);

                            if (enc_result == NULL || enc_result->hex_string == NULL)
                            {
                                /* ReqID:TECC4.0 - 失败原因（列声明长度的拒收等）在调度器的
                                 * 错误记录里，必须上抛：静默跳过等于让这条 DEFAULT 留在
                                 * CREATE TABLE 里当明文，建出来的表每个 DEFAULT 行都读不回来。
                                 * 与 INSERT/cast 路径（stmt_report_encrypt_failure）一致。 */
                                const char *reason =
                                    (enc_result != NULL && enc_result->error_msg[0] != '\0')
                                    ? enc_result->error_msg
                                    : tecc_get_last_error(statement_data->conn);

                                if (enc_result != NULL)
                                    tecc_unified_free_encrypt_result(enc_result);
                                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "Encryption failed: %s",
                                    (reason != NULL && reason[0] != '\0') ?
                                    reason : "unknown error");
                                return TECC_CREATE_PROCESSOR_ERROR;
                            }

                            /* Store encryption info for later SQL rewrite */
                            info = (TeccDefaultEncInfo *)pg_malloc0(sizeof(TeccDefaultEncInfo));
                            if (info != NULL)
                            {
                                info->column_name = pg_strdup(column_def->colname);
                                info->plaintext_value = pg_strdup(plaintext_value);
                                info->encrypted_hex = pg_strdup(enc_result->hex_string);
                                info->column_type = pg_strdup(type_name);
                                context->default_encryptions = lappend(context->default_encryptions, info);

                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Encrypted DEFAULT value for column '%s': '%s' -> hex",
                                    column_def->colname, plaintext_value);
                            }
                            tecc_unified_free_encrypt_result(enc_result);
                        }
                    }
                }
            }
        }
    }

    return TECC_CREATE_PROCESSOR_SUCCESS;
}

/*
 * Check if column type supports encryption
 */
bool
tecc_create_stmt_processor_type_supports_encryption(const char *type_name)
{
    int i;

    if (type_name == NULL)
        return false;

    for (i = 0; supported_types[i].type_name != NULL; i++)
    {
        if (strcasecmp(supported_types[i].type_name, type_name) == 0)
            return true;
    }

    return false;
}

/*
 * Get encryption algorithm for column type
 */
const char *
tecc_create_stmt_processor_get_column_algorithm(const char *type_name)
{
    int i;

    if (type_name == NULL)
        return NULL;

    for (i = 0; supported_types[i].type_name != NULL; i++)
    {
        if (strcasecmp(supported_types[i].type_name, type_name) == 0)
            return supported_types[i].algorithm;
    }

    return NULL;
}

/*
 * ReqID:TECC4.0 - Map encrypted type name to base type OID.
 * E.g., "int4_enc" → INT4OID (23).
 */
Oid
map_enc_type_name_to_base_oid(const char *type_name)
{
    static const struct {
        const char *enc_name;
        Oid         base_oid;
    } map[] = {
        {"int2_enc",    21},   /* INT2OID */
        {"int4_enc",    23},   /* INT4OID */
        {"int8_enc",    20},   /* INT8OID */
        {"float4_enc",  700},  /* FLOAT4OID */
        {"float8_enc",  701},  /* FLOAT8OID */
        {"numeric_enc", 1700}, /* NUMERICOID */
        {"varchar_enc", 1043}, /* VARCHAROID */
        {"bpchar_enc",  1042}, /* BPCHAROID */
        {"text_enc",    25},   /* TEXTOID */
        {NULL, InvalidOid}
    };
    int i;

    if (type_name == NULL)
        return InvalidOid;

    for (i = 0; map[i].enc_name != NULL; i++)
    {
        if (strcasecmp(map[i].enc_name, type_name) == 0)
            return map[i].base_oid;
    }
    return InvalidOid;
}

/*
 * ReqID:TECC4.0 - 把 CollateClause 的限定名拼成 SQL 里可直接解析的标识符文本：
 * 每段加双引号、内部双引号翻倍（"sch"."MyColl"）。这样 to_regcollation() 才能
 * 按原始大小写精确解析，而不会被当作未加引号的标识符折叠成小写。
 * 返回 pg_strdup 的字符串，调用方负责 pg_free；结构异常时返回 NULL。
 */
static char *
build_qualified_collation_name(const CollateClause *coll_clause)
{
    char        buf[2 * NAMEDATALEN + 8];
    size_t      off = 0;
    ListCell   *lc;
    bool        first = true;

    if (coll_clause == NULL || coll_clause->collname == NIL)
        return NULL;

    foreach(lc, coll_clause->collname)
    {
        Node       *node = (Node *) lfirst(lc);
        const char *ident = NULL;
        const char *p;

        if (IsA(node, String) || IsA(node, Const))
            ident = strVal((Value *) node);
        if (ident == NULL || ident[0] == '\0' || strlen(ident) >= NAMEDATALEN)
            return NULL;

        if (!first)
        {
            if (off + 1 >= sizeof(buf))
                return NULL;
            buf[off++] = '.';
        }
        first = false;

        if (off + 3 >= sizeof(buf))
            return NULL;
        buf[off++] = '"';
        for (p = ident; *p != '\0'; p++)
        {
            if (off + 3 >= sizeof(buf))
                return NULL;
            if (*p == '"')
                buf[off++] = '"';
            buf[off++] = *p;
        }
        buf[off++] = '"';
    }
    buf[off] = '\0';

    return pg_strdup(buf);
}

/*
 * ReqID:TECC4.0 - 从 ENCRYPTED WITH 选项里取同态 DEK 名（HE_ADD_DEK / HE_MUL_DEK）。
 *
 * 目标列还不存在的写入路径（CTAS 的结果表列、视图/物化视图定义里的 cast、加密列
 * DEFAULT）拿不到列元数据，同态 DEK 就没有别的来源 —— 加密入口只有单个 dek_name
 * 参数，列级的 he_add_dek/he_mul_dek 要靠 hg_encrypted_column_args 解析，而这些
 * 路径恰好没有那一行。调用方把名字交给
 * tecc_unified_encrypt_string_with_options_and_collation() 即可。
 *
 * 返回的是解析树里字符串的指针（不复制、不要释放）；没有对应选项时置 NULL。
 */
void
tecc_typename_get_he_dek_names(const TypeName *type_name,
                               const char **he_add_dek_name,
                               const char **he_mul_dek_name)
{
    ListCell *lc;

    if (he_add_dek_name != NULL)
        *he_add_dek_name = NULL;
    if (he_mul_dek_name != NULL)
        *he_mul_dek_name = NULL;

    if (type_name == NULL)
        return;

    foreach(lc, type_name->encryptedOpts)
    {
        DefElem *elem = (DefElem *) lfirst(lc);

        if (elem == NULL || elem->defname == NULL || elem->arg == NULL)
            continue;
        if (nodeTag(elem->arg) != FE_T_String)
            continue;

        if (he_add_dek_name != NULL &&
            pg_strcasecmp(elem->defname, "he_add_dek") == 0)
            *he_add_dek_name = strVal((const Value *) elem->arg);
        else if (he_mul_dek_name != NULL &&
                 pg_strcasecmp(elem->defname, "he_mul_dek") == 0)
            *he_mul_dek_name = strVal((const Value *) elem->arg);
    }
}

/*
 * ReqID:TECC4.0 - 解析新建加密列将来生效的 collation locale 字符串。
 *
 * 为什么需要：DET/OPE 归一化要用 locale 生成 ICU sort key。建表时列还不存在，
 * 元数据缓存查不到该列，加密 DEFAULT 值的路径拿不到 collation_locale，归一化
 * 会退化成无 collation 的二进制拷贝；而 INSERT/WHERE 路径从
 * hg_encrypted_columns_info 拿到了 locale —— 同一明文因此产生两套互不相等的
 * （但各自确定性的）密文，DEFAULT 插入的行被等值查询静默漏掉。
 *
 * 取值必须与后端建完列之后视图给出的值一致：后端建列时
 * collOid = tecc_resolve_collation_oid(GetColumnDefCollation(...))，视图
 * hg_encrypted_columns_info 再取该 collation 的 collcollate。因此：
 *   - 列上写了 COLLATE   → 该 collation 的 collcollate；
 *   - 文本类型且无 COLLATE → 库默认排序规则的 collcollate（先把
 *     DEFAULT_COLLATION_OID 按 datcollate 解析成具体 collation，再取
 *     collcollate，规则与 tecc_resolve_collation_oid 一致）；
 *   - 非文本类型         → NULL（无 collation）。
 * 查询失败或结果为空一律返回 NULL：行为与修复前相同（拿不到 collation），
 * 不会让建表失败，但会打一条警告（否则"环境异常导致静默退化成旧行为"无从发现）。
 *
 * 返回值是 pg_strdup 的字符串，调用方负责 pg_free。
 * 目标列还不存在的其他路径（CTAS、物化视图/视图定义里的带 ENCRYPTED WITH cast）
 * 也用它：那些列同样没有 COLLATE 子句，传 coll_clause = NULL 即取库默认排序规则，
 * 结果按连接缓存在 TeccState.default_collation_locale（一个 CTAS 里每个密态字面量
 * 都调一次，不缓存就是每个字面量一次元数据查询）。
 */
char *
resolve_new_column_collation_locale(PGconn *conn, Oid base_oid,
                                    const CollateClause *coll_clause)
{
    char        query[1024];
    PGresult   *res;
    char       *result = NULL;
    bool        saved_enable_tecc;
    bool        is_text_type = (base_oid == 25    /* TEXTOID */ ||
                                base_oid == 1043  /* VARCHAROID */ ||
                                base_oid == 1042  /* BPCHAROID */);
    TeccState  *state = NULL;

    if (conn == NULL)
        return NULL;

    /* 非文本类型本来就没有 collation，免查 */
    if (coll_clause == NULL && !is_text_type)
        return NULL;

    /* 无 COLLATE 分支的结果只取决于所连库的默认排序规则，一条连接绑定一个库
     * （换库必须重连），可安全缓存；只缓存成功解析出的值。 */
    if (coll_clause == NULL)
    {
        state = tecc_get_state(conn);
        if (state != NULL && state->default_collation_locale != NULL)
            return pg_strdup(state->default_collation_locale);
    }

    if (coll_clause != NULL)
    {
        char   *qualified = build_qualified_collation_name(coll_clause);
        char   *literal;

        if (qualified == NULL)
            return NULL;
        literal = PQescapeLiteral(conn, qualified, strlen(qualified));
        pg_free(qualified);
        if (literal == NULL)
            return NULL;

        snprintf(query, sizeof(query),
                 "SELECT CASE WHEN c.oid = 100 "
                 "THEN (SELECT datcollate FROM pg_database "
                 "      WHERE datname = current_database()) "
                 "ELSE c.collcollate END "
                 "FROM pg_collation c WHERE c.oid = to_regcollation(%s)",
                 literal);
        PQfreemem(literal);
    }
    else
    {
        snprintf(query, sizeof(query),
                 "SELECT c.collcollate FROM pg_collation c "
                 "WHERE c.collencoding IN "
                 "      (pg_char_to_encoding(current_setting('server_encoding')), -1) "
                 "  AND c.collprovider IN ('d', 'c') "
                 "  AND lower(replace(c.collcollate, '-', '')) = "
                 "      lower(replace((SELECT datcollate FROM pg_database "
                 "                     WHERE datname = current_database()), '-', '')) "
                 "LIMIT 1");
    }

    /* 内部元数据查询：临时关闭 TECC 预处理，避免递归处理这条查询本身 */
    saved_enable_tecc = conn->enable_tecc;
    conn->enable_tecc = false;
    res = PQexec(conn, query);
    conn->enable_tecc = saved_enable_tecc;

    if (res != NULL)
    {
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0 &&
            !PQgetisnull(res, 0, 0))
        {
            const char *val = PQgetvalue(res, 0, 0);

            if (val != NULL && val[0] != '\0')
                result = pg_strdup(val);
        }
        PQclear(res);
    }

    if (result != NULL)
    {
        /* 缓存给同一连接后续调用（一个 CTAS 里有多个密态字面量时不必重复查） */
        if (coll_clause == NULL && state != NULL &&
            state->default_collation_locale == NULL)
            state->default_collation_locale = pg_strdup(result);
    }
    else
        /* 注意：解析不出 locale 不等于报错，加密会退回不带 collation 的归一化，
         * 密文与列自身路径不一致（等值查询静默查不到）。这里必须留下痕迹，否则
         * 环境异常（如 collation 元数据不可读）会表现为"少数语句莫名查不到"。 */
        tecc_processor_debug(TECC_LOG_WARNING,
            "Could not resolve collation locale for a new encrypted column "
            "(base type oid %u, %s); DET/OPE ciphertext may not match the "
            "column's own path",
            base_oid,
            (coll_clause != NULL) ? "explicit COLLATE clause"
                                  : "database default collation");

    return result;
}

/*
 * ReqID:TECC4.0 - Parse SUPPORT_ALGO string into boolean flags.
 * Input: "DET,SEARCH" → has_det=true, has_search=true
 */
static const char *
parse_support_algo_flags(const char *algo_value,
                          bool *has_det, bool *has_ope, bool *has_search,
                          bool *has_he_add, bool *has_he_mul)
{
    char *algo_copy;
    char *saveptr;
    char *token;

    *has_det = false;
    *has_ope = false;
    *has_search = false;
    *has_he_add = false;
    *has_he_mul = false;

    if (algo_value == NULL || algo_value[0] == '\0')
        return NULL;

    algo_copy = pg_strdup(algo_value);
    if (algo_copy == NULL)
        return NULL;

    token = strtok_r(algo_copy, ",", &saveptr);
    while (token != NULL)
    {
        while (*token == ' ') token++;
        {
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') end--;
            *(end + 1) = '\0';
        }
        if (strcasecmp(token, "det") == 0)
            *has_det = true;
        else if (strcasecmp(token, "ope") == 0)
            *has_ope = true;
        else if (strcasecmp(token, "search") == 0)
            *has_search = true;
        /* ReqID:TECC4.0 */
        else if (strcasecmp(token, "he-add") == 0)
            *has_he_add = true;
        else if (strcasecmp(token, "he-mul") == 0)
            *has_he_mul = true;
        token = strtok_r(NULL, ",", &saveptr);
    }
    pg_free(algo_copy);

    /* DET and OPE are mutually exclusive */
    if (*has_det && *has_ope)
        return "DET and OPE cannot be specified together in SUPPORT_ALGO";

    return NULL;
}

/*
 * Internal helper functions
 */
static TeccCreateStmtProcessorResult
create_process_columns(const CreateStmt *create_stmt,
                      TeccCreateStmtProcessorContext *context,
                      TeccStatementData *statement_data)
{
    ListCell *lc;
    TeccCreateStmtProcessorResult result;
    int attnum = 0;  /* ReqID:TECC4.0 - Track column attnum for pending_tables */

    if (create_stmt->tableElts == NIL)
        return TECC_CREATE_PROCESSOR_SUCCESS;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing table columns");

    foreach(lc, create_stmt->tableElts)
    {
        Node *element = (Node *) lfirst(lc);

        if (IsA(element, ColumnDef))
        {
            ColumnDef *column_def = (ColumnDef *) element;
            attnum++;
            /* ReqID:TECC4.0 - Store column attnum for pending_tables attnum mapping */
            context->current_attnum = attnum;
            result = tecc_create_stmt_processor_check_column(column_def, context, statement_data);
            if (result != TECC_CREATE_PROCESSOR_SUCCESS)
                return result;
        }
    }

    return TECC_CREATE_PROCESSOR_SUCCESS;
}

/*
 * ReqID:TECC4.0 - Query partition key column names from the parent table.
 * Returns an array of column name strings allocated with pg_malloc,
 * or NULL on failure. The caller must free the array and each string.
 */
char **
tecc_query_partkey_columns(PGconn *conn,
                      const char *schema_name,
                      const char *table_name,
                      int *ncols)
{
    char query[4096];
    PGresult *res;
    char **col_names = NULL;
    int n;
    bool saved_enable_tecc;
    char *esc_table, *esc_schema;

    if (conn == NULL || schema_name == NULL || table_name == NULL || ncols == NULL)
    {
        if (ncols) *ncols = 0;
        return NULL;
    }

    *ncols = 0;

    /* Escape identifiers as SQL string literals to prevent injection */
    esc_table = PQescapeLiteral(conn, table_name, strlen(table_name));
    esc_schema = PQescapeLiteral(conn, schema_name, strlen(schema_name));
    if (esc_table == NULL || esc_schema == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "Failed to escape identifiers for %s.%s",
            schema_name, table_name);
        if (esc_table) PQfreemem(esc_table);
        if (esc_schema) PQfreemem(esc_schema);
        return NULL;
    }

    /* Query partition key column names from pg_partitioned_table */
    snprintf(query, sizeof(query),
        "SELECT a.attname "
        "FROM pg_partitioned_table p "
        "CROSS JOIN LATERAL unnest(p.partattrs) WITH ORDINALITY AS u(attnum, pos) "
        "JOIN pg_attribute a ON a.attrelid = p.partrelid AND a.attnum = u.attnum "
        "WHERE p.partrelid = ("
        "  SELECT c.oid FROM pg_class c "
        "  JOIN pg_namespace n ON n.oid = c.relnamespace "
        "  WHERE c.relname = %s AND n.nspname = %s) "
        "ORDER BY u.pos",
        esc_table, esc_schema);

    PQfreemem(esc_table);
    PQfreemem(esc_schema);

    /* Temporarily disable TECC preprocessing for internal metadata query */
    saved_enable_tecc = conn->enable_tecc;
    conn->enable_tecc = false;
    res = PQexec(conn, query);
    conn->enable_tecc = saved_enable_tecc;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "Failed to query partition key columns for %s.%s",
            schema_name, table_name);
        if (res) PQclear(res);
        return NULL;
    }

    n = PQntuples(res);
    if (n == 0)
    {
        PQclear(res);
        return NULL;
    }

    col_names = (char **)pg_malloc0(sizeof(char *) * n);
    for (int i = 0; i < n; i++)
    {
        col_names[i] = pg_strdup(PQgetvalue(res, i, 0));
    }

    *ncols = n;
    PQclear(res);
    return col_names;
}

/*
 * ReqID:TECC4.0 - Encrypt a single partition boundary value.
 *
 * Shared helper used by both CREATE TABLE ... PARTITION OF and
 * ALTER TABLE ... ATTACH PARTITION boundary encryption paths.
 */
TeccBoundEncInfo *
tecc_encrypt_boundary_value(PGconn *conn,
                            struct TeccState *state,
                            Const *c,
                            struct TeccColumnCacheItem *col_cache,
                            const char *table_name,
                            const char *column_name,
                            bool *is_error,
                            TeccStatementData *statement_data)
{
    char plaintext_buf[256];
    const char *encrypt_value;
    TeccDekCacheItem *dek_item;
    const char *kek_name;
    TeccUnifiedEncryptResult *enc_result;
    TeccBoundEncInfo *info;

    if (is_error != NULL)
        *is_error = false;

    if (c == NULL || col_cache == NULL || state == NULL ||
        conn == NULL || column_name == NULL || statement_data == NULL)
        return NULL;

    /* Require DET or OPE encryption for partition keys */
    if (!(col_cache->encryption_type & TECC_ENC_TYPE_DET_OPE))
    {
        tecc_processor_set_error(statement_data,
            TECC_PROCESSOR_INVALID_INPUT,
            "Partition key column '%s' must use DET or OPE encryption "
            "for partition boundary. Column has encryption type 0x%02x.",
            column_name, col_cache->encryption_type);
        if (is_error != NULL)
            *is_error = true;
        return NULL;
    }

    /* Extract plaintext value from Const */
    plaintext_buf[0] = '\0';
    encrypt_value = NULL;

    if (c->constisnull)
        return NULL;

    switch (c->consttype)
    {
        case 23:  /* INT4OID */
        case 21:  /* INT2OID */
        case 20:  /* INT8OID */
        case 700: /* FLOAT4OID */
        case 701: /* FLOAT8OID */
        case 1700:/* NUMERICOID */
            {
                /*
                 * Use original SQL token text for the boundary value instead of
                 * converting through C types.  %g loses precision for FLOAT/NUMERIC
                 * (default 6 significant digits) and atoi truncates large INT8.
                 * The token text from the SQL is the most faithful representation.
                 */
                int token_len = c->data_size;
                if (token_len > (int)sizeof(plaintext_buf) - 1)
                    token_len = (int)sizeof(plaintext_buf) - 1;
                if (c->location >= 0 && statement_data != NULL &&
                    statement_data->query != NULL && token_len > 0)
                {
                    memcpy(plaintext_buf,
                           statement_data->query + c->location, token_len);
                    plaintext_buf[token_len] = '\0';
                    encrypt_value = plaintext_buf;
                }
                else
                {
                    /* Fallback: extract plaintext from C value */
                    if (c->consttype == 700 || c->consttype == 701 ||
                        c->consttype == 1700)
                        snprintf(plaintext_buf, sizeof(plaintext_buf),
                                 "%g", c->constvalue.dval);
                    else
                        snprintf(plaintext_buf, sizeof(plaintext_buf),
                                 "%ld", (long)c->constvalue.ival);
                    encrypt_value = plaintext_buf;
                }
            }
            break;
        case 25:  /* TEXTOID */
        case 1043:/* VARCHAROID */
        case 1042:/* BPCHAROID */
            if (c->constvalue.sval != NULL)
                encrypt_value = c->constvalue.sval;
            break;
        default:
            if (c->constvalue.sval != NULL)
                encrypt_value = c->constvalue.sval;
            else
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                    "Unsupported Const type %d for partition boundary "
                    "- encryption skipped", c->consttype);
                return NULL;
            }
            break;
    }

    if (encrypt_value == NULL)
        return NULL;

    /* ReqID:TECC4.0 - Skip encryption for sentinel bounds (Infinity/-Infinity/MAXVALUE).
     * These are not real data values and cannot be normalized as encrypted types.
     * MAXVALUE is already handled above (c->constisnull).
     * Handle both bare and single-quoted forms (lexer may produce either). */
    if (strcmp(encrypt_value, "Infinity") == 0 ||
        strcmp(encrypt_value, "-Infinity") == 0 ||
        strcmp(encrypt_value, "'Infinity'") == 0 ||
        strcmp(encrypt_value, "'-Infinity'") == 0)
        return NULL;

    /* ReqID:TECC4.0 - Flush pending DEK before lookup (same fix as DEFAULT encryption) */
    tecc_accept_pending_statements(conn);

    /* Get KEK name from DEK cache */
    dek_item = tecc_cache_get_dek_by_name_search_path(state, col_cache->dek_name);
    if (dek_item == NULL || dek_item->kek_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "DEK '%s' not found for partition key column '%s'",
            col_cache->dek_name ? col_cache->dek_name : "unknown",
            column_name);
        return NULL;
    }
    kek_name = dek_item->kek_name;

    /* Encrypt the boundary value */
    enc_result = tecc_unified_encrypt_string_with_options(
        conn,
        encrypt_value,
        table_name,
        column_name,
        kek_name,
        col_cache->dek_name,
        col_cache->original_type_oid,
        col_cache->original_type_mod,
        (col_cache->encryption_type & TECC_ENC_TYPE_DET) != 0,
        (col_cache->encryption_type & TECC_ENC_TYPE_OPE) != 0,
        false,  /* supports_search */
        (col_cache->encryption_type & TECC_ENC_TYPE_HE_ADD) != 0,  /* supports_he_add */
        (col_cache->encryption_type & TECC_ENC_TYPE_HE_MUL) != 0,  /* supports_he_mul */
        NULL);  /* external_context */

    if (enc_result == NULL || enc_result->hex_string == NULL)
    {
        if (enc_result != NULL)
            tecc_unified_free_encrypt_result(enc_result);
        tecc_processor_set_error(statement_data,
            TECC_PROCESSOR_ERROR,
            "Failed to encrypt partition boundary value for column '%s'",
            column_name);
        if (is_error != NULL)
            *is_error = true;
        return NULL;
    }

    /* Allocate result info */
    info = (TeccBoundEncInfo *)pg_malloc0(sizeof(TeccBoundEncInfo));
    if (info != NULL)
    {
        info->plaintext_value = pg_strdup(encrypt_value);
        info->encrypted_hex = pg_strdup(enc_result->hex_string);
        info->value_location = c->location;
        info->value_length = c->data_size;

        /* Set encrypted type name for explicit ::type_enc cast in SQL rewrite.
         * This ensures numeric encrypted types (int2_enc, numeric_enc, etc.)
         * are correctly typed when replacing FOR VALUES boundaries. */
        if (col_cache != NULL)
        {
            Oid base_oid = col_cache->original_type_oid;
            switch (base_oid)
            {
                case 21:   info->column_type = pg_strdup("int2_enc"); break;
                case 23:   info->column_type = pg_strdup("int4_enc"); break;
                case 20:   info->column_type = pg_strdup("int8_enc"); break;
                case 700:  info->column_type = pg_strdup("float4_enc"); break;
                case 701:  info->column_type = pg_strdup("float8_enc"); break;
                case 1700: info->column_type = pg_strdup("numeric_enc"); break;
                case 1043: info->column_type = pg_strdup("varchar_enc"); break;
                case 1042: info->column_type = pg_strdup("bpchar_enc"); break;
                case 25:   info->column_type = pg_strdup("text_enc"); break;
                default:   info->column_type = NULL; break;
            }
        }
    }

    tecc_unified_free_encrypt_result(enc_result);
    return info;
}

/*
 * ReqID:TECC4.0 - Free a TeccBoundEncInfo.
 */
void
tecc_free_bound_enc_info(TeccBoundEncInfo *info)
{
    if (info == NULL)
        return;
    if (info->plaintext_value != NULL)
        pg_free(info->plaintext_value);
    if (info->encrypted_hex != NULL)
        pg_free(info->encrypted_hex);
    if (info->column_type != NULL)
        pg_free(info->column_type);
    pg_free(info);
}

/*
 * ReqID:TECC4.0 - Encrypt partition boundary values for encrypted columns.
 *
 * For PARTITION OF statements, queries the parent table's partition key
 * columns and encrypted columns, then encrypts boundary values at positions
 * corresponding to encrypted columns.
 *
 * RANGE partitioning: requires OPE encryption for the partition key column
 * (DET-only will produce an error since DET doesn't preserve order).
 *
 * LIST partitioning: requires DET or OPE encryption (both support equality
 * comparison, which is all LIST needs).
 */
static void
encrypt_partition_bounds(CreateStmt *stmt,
                         TeccCreateStmtProcessorContext *context,
                         TeccStatementData *statement_data)
{
    PartitionBoundSpec *partbound;
    RangeVar *parent;
    const char *parent_schema;
    char *parent_table;
    char **partkey_cols = NULL;
    int partkey_ncols = 0;
    TeccColumnCacheItem **parent_enc_cols = NULL;
    int parent_enc_ncols = 0;
    TeccState *state = NULL;
    const char *db_name;
    ListCell *lc;
    int pos;
    List *datum_lists[2];
    int n_lists;

    if (stmt == NULL || context == NULL || statement_data == NULL)
        return;

    partbound = (PartitionBoundSpec *)stmt->partbound;
    if (partbound == NULL || partbound->is_default)
        return;

    /* Only PARTITION OF has boundary values to encrypt */
    if (stmt->inhRelations == NIL)
        return;

    parent = (RangeVar *) linitial(stmt->inhRelations);
    if (parent == NULL || parent->relname == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "Parent table not specified in PARTITION OF");
        return;
    }

    parent_table = parent->relname;
    /* ReqID:TECC4.0 - 未限定父表名按 search_path 解析真实 schema（同 stmt_load_table_metadata） */
    parent_schema = tecc_stmt_resolve_table_schema(statement_data,
                                                   parent->schemaname,
                                                   parent->relname);

    /* Step 1: Query partition key column names from parent table */
    partkey_cols = tecc_query_partkey_columns(statement_data->conn,
                                         parent_schema, parent_table,
                                         &partkey_ncols);
    if (partkey_cols == NULL || partkey_ncols == 0)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "No partition key columns found for parent '%s.%s' "
            "- partition boundary encryption skipped",
            parent_schema, parent_table);
        return;
    }

    /* Step 2: Query encrypted columns from parent table */
    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "No TECC state available - partition boundary encryption skipped");
        goto cleanup;
    }

    db_name = PQdb(statement_data->conn);
    if (db_name == NULL) db_name = "postgres";

    /* ReqID:TECC4.0 - Flush pending entries before cache lookup */
    tecc_accept_pending_statements(statement_data->conn);

    parent_enc_cols = tecc_cache_get_columns_by_table_name(
        state, db_name, parent_schema, parent_table, &parent_enc_ncols);

    /* ReqID:TECC4.0 - 缓存层在查询失败时已设置 state->last_error（TECC_ERR_DATA_FETCHER）。
     * 检测失败并上抛错误，避免加密父表被误判为普通表导致分区边界明文泄漏。
     * 不再在主连接上直接 PQexec 兜底（原 fallback 绕过 data_fetcher 分层）。 */
    if ((parent_enc_cols == NULL || parent_enc_ncols == 0) &&
        state->last_error.error_code == TECC_ERR_DATA_FETCHER)
    {
        tecc_processor_set_error(statement_data,
            TECC_PROCESSOR_ERROR,
            "failed to load encrypted column metadata for partition parent \"%s.%s\": %s",
            parent_schema, parent_table,
            state->last_error.error_msg[0] != '\0' ? state->last_error.error_msg
                                                   : "data fetcher error");
        goto cleanup;
    }

    /* Step 3: Determine which lists to iterate */
    switch (partbound->strategy)
    {
        case 'r':  /* RANGE */
            datum_lists[0] = partbound->lowerdatums;
            datum_lists[1] = partbound->upperdatums;
            n_lists = 2;
            break;
        case 'l':  /* LIST */
            datum_lists[0] = partbound->listdatums;
            n_lists = 1;
            break;
        case 'h':  /* HASH */
        default:
            goto cleanup;
    }

    /* Step 4: Encrypt values at positions matching encrypted partition key columns */

    for (int li = 0; li < n_lists; li++)
    {
        if (datum_lists[li] == NIL)
            continue;

        pos = 0;
        foreach(lc, datum_lists[li])
        {
            PartitionRangeDatum *datum = (PartitionRangeDatum *) lfirst(lc);
            Const *c;
            int col_pos;  /* position in partition key columns */
            TeccColumnCacheItem *col_cache = NULL;

            /*
             * For RANGE: each value has its own position in the partition key.
             * For LIST: all values correspond to position 0 (single partition
             * key column — multi-column LIST is not supported by PG).
             */
            col_pos = (partbound->strategy == 'l') ? 0 : pos;

            if (col_pos >= partkey_ncols)
                break;

            if (datum == NULL || datum->value == NULL)
            {
                pos++;
                continue;  /* MINVALUE/MAXVALUE */
            }

            /* Unwrap TypeCast to get the inner Const (e.g. -32767::int2 →
             * TypeCast(arg=Const(-32767))). Without this, numeric types with
             * explicit ::type casts (like int2) would be silently skipped. */
            c = NULL;
            if (IsA(datum->value, Const))
            {
                c = (Const *)datum->value;
            }
            else if (IsA(datum->value, TypeCast))
            {
                TypeCast *tc = (TypeCast *)datum->value;
                if (tc->arg != NULL && IsA(tc->arg, Const))
                    c = (Const *)tc->arg;
            }

            if (c == NULL)
            {
                pos++;
                continue;
            }

            /* Look up this column in the parent's encrypted columns */
            if (parent_enc_cols != NULL)
            {
                for (int i = 0; i < parent_enc_ncols; i++)
                {
                    if (parent_enc_cols[i] != NULL &&
                        parent_enc_cols[i]->column_name != NULL &&
                        strcmp(parent_enc_cols[i]->column_name, partkey_cols[col_pos]) == 0)
                    {
                        col_cache = parent_enc_cols[i];
                        break;
                    }
                }
            }

            if (col_cache == NULL)
            {
                pos++;
                continue;  /* Not an encrypted column */
            }

            /* Validate: RANGE partitioning requires OPE encryption */
            if (partbound->strategy == 'r' &&
                !(col_cache->encryption_type & TECC_ENC_TYPE_OPE))
            {
                tecc_processor_set_error(statement_data,
                    TECC_PROCESSOR_ERROR,
                    "RANGE partition key column '%s' must use OPE encryption "
                    "for ordering. DET-only encryption does not preserve "
                    "plaintext order. Set support_algo = 'ope' for this column.",
                    partkey_cols[col_pos]);
                goto cleanup;
            }

                /* Encrypt boundary value using shared helper */
                {
                    TeccBoundEncInfo *bound_info;
                    bool is_error = false;

                    bound_info = tecc_encrypt_boundary_value(
                        statement_data->conn, state, c, col_cache,
                        parent_table,
                        partkey_cols[col_pos],
                        &is_error, statement_data);
                    if (bound_info != NULL)
                    {
                        context->bound_encryptions = lappend(context->bound_encryptions, bound_info);
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Encrypted partition boundary for column '%s': '%s' -> hex",
                            partkey_cols[col_pos], bound_info->plaintext_value);
                    }
                    else if (is_error)
                    {
                        goto cleanup;
                    }
                }

            pos++;
        }
    }

cleanup:
    if (partkey_cols != NULL)
    {
        for (int i = 0; i < partkey_ncols; i++)
        {
            if (partkey_cols[i] != NULL)
                pg_free(partkey_cols[i]);
        }
        pg_free(partkey_cols);
    }
    /* parent_enc_cols is owned by cache, don't free */
}

static void
create_context_init(TeccCreateStmtProcessorContext *context)
{
    if (context == NULL)
        return;

    context->table_name = NULL;
    context->schema_name = NULL;
    context->columns = NIL;
    context->constraints = NIL;
    context->encrypted_columns = NIL;
    context->has_encrypted_columns = false;
    context->needs_dek_creation = false;
    context->default_encryptions = NIL;
    context->bound_encryptions = NIL;
}

static void
create_context_cleanup(TeccCreateStmtProcessorContext *context)
{
    if (context == NULL)
        return;

    if (context->table_name != NULL)
        pg_free(context->table_name);
    if (context->schema_name != NULL)
        pg_free(context->schema_name);
    if (context->encrypted_columns != NIL)
        tecc_processor_list_free_deep(context->encrypted_columns);

    /* ReqID:TECC4.0 - Free DEFAULT encryption info */
    if (context->default_encryptions != NIL)
    {
        ListCell *lc;
        foreach(lc, context->default_encryptions)
        {
            TeccDefaultEncInfo *info = (TeccDefaultEncInfo *) lfirst(lc);
            if (info != NULL)
            {
                if (info->column_name != NULL)
                    pg_free(info->column_name);
                if (info->plaintext_value != NULL)
                    pg_free(info->plaintext_value);
                if (info->encrypted_hex != NULL)
                    pg_free(info->encrypted_hex);
                if (info->column_type != NULL)
                    pg_free(info->column_type);
                pg_free(info);
            }
        }
        list_free(context->default_encryptions);
    }

    /* ReqID:TECC4.0 - Free partition bound encryption info */
    if (context->bound_encryptions != NIL)
    {
        ListCell *lc;
        foreach(lc, context->bound_encryptions)
        {
            tecc_free_bound_enc_info((TeccBoundEncInfo *) lfirst(lc));
        }
        list_free(context->bound_encryptions);
    }

    /* Note: columns and constraints are owned by the parse tree */
}
/* End - ReqID:TECC4.0 */
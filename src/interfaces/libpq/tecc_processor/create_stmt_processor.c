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
static const char *parse_support_algo_flags(const char *algo_value, bool *has_det, bool *has_ope, bool *has_search);

/* ReqID:TECC4.0 - Forward declarations for partition boundary encryption */
static void encrypt_partition_bounds(CreateStmt *stmt,
                                     TeccCreateStmtProcessorContext *context,
                                     TeccStatementData *statement_data);

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
                    char search_pattern[512];
                    char replace_pattern[1024];
                    char *found_pos;
                    char *new_query;
                    size_t prefix_len;
                    size_t new_len;
                    size_t suffix_len;

                    /* Build search pattern: "DEFAULT <plaintext_value>"
                     * ReqID:TECC4.0 - Search case-insensitively because user SQL
                     * may use lowercase 'default'. Try uppercase first, then lowercase. */
                    snprintf(search_pattern, sizeof(search_pattern),
                             "DEFAULT %s", info->plaintext_value);
                    found_pos = strstr(current_query, search_pattern);
                    if (found_pos == NULL)
                    {
                        /* Try lowercase 'default' variant */
                        size_t pat_len = strlen(search_pattern);
                        if (pat_len > 7)
                        {
                            char lower_pattern[512];
                            if (pat_len < sizeof(lower_pattern))
                            {
                                lower_pattern[0] = 'd';
                                lower_pattern[1] = 'e';
                                lower_pattern[2] = 'f';
                                lower_pattern[3] = 'a';
                                lower_pattern[4] = 'u';
                                lower_pattern[5] = 'l';
                                lower_pattern[6] = 't';
                                memcpy(lower_pattern + 7, search_pattern + 7,
                                       pat_len - 7 + 1);
                                found_pos = strstr(current_query, lower_pattern);
                            }
                        }
                    }
                    if (found_pos == NULL)
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                            "Cannot find DEFAULT pattern '%s' in query for column '%s'",
                            search_pattern, info->column_name);
                        continue;
                    }

                    /* Build replacement: "DEFAULT '<hex>'::<type>"
                     * Note: encrypted_hex already includes \x prefix from hex_encode */
                    snprintf(replace_pattern, sizeof(replace_pattern),
                             "DEFAULT '%s'::%s",
                             info->encrypted_hex,
                             info->column_type ? info->column_type : "text");

                    /* Calculate positions */
                    prefix_len = (size_t)(found_pos - current_query);
                    new_len = prefix_len + strlen(replace_pattern) +
                              strlen(found_pos + strlen(search_pattern)) + 1;

                    new_query = (char *)pg_malloc(new_len);
                    if (new_query == NULL)
                        continue;

                    /* Build new query: prefix + replacement + suffix */
                    memcpy(new_query, current_query, prefix_len);
                    memcpy(new_query + prefix_len, replace_pattern, strlen(replace_pattern));
                    suffix_len = strlen(found_pos + strlen(search_pattern));
                    memcpy(new_query + prefix_len + strlen(replace_pattern),
                           found_pos + strlen(search_pattern), suffix_len + 1);

                    pg_free(current_query);
                    current_query = new_query;

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
                    tecc_column_ref_data_init(col_data, NULL, NULL, context->table_name, column_def->colname);
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
                algo_str2 = tecc_column_ref_data_get_support_algo(col_data);
                if (parse_support_algo_flags(algo_str2, &h_det, &h_ope, &h_search) == NULL && !h_ope)
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
                            TeccUnifiedEncryptResult *enc_result;
                        const char *encrypt_value;
                            TeccDefaultEncInfo *info;
                            const char *algo_err;

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

                            /* Get DEK from cache */
                            dek_name = tecc_column_ref_data_get_dek_name(col_data);
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
                            algo_err = parse_support_algo_flags(algo_str, &has_det, &has_ope, &has_search);
                            if (algo_err != NULL)
                            {
                                tecc_processor_debug(TECC_LOG_ERROR,
                                    "SUPPORT_ALGO error for column '%s': %s",
                                    column_def->colname, algo_err);
                                continue;
                            }

                            /* Encrypt the DEFAULT value */
                            enc_result = tecc_unified_encrypt_string_with_options(
                                statement_data->conn,
                                encrypt_value,
                                context->table_name ? context->table_name : "unknown",
                                column_def->colname,
                                kek_name,
                                dek_name,
                                base_oid,
                                -1, /* default typmod */
                                has_det,
                                has_ope,
                                has_search,
                                NULL);

                            if (enc_result == NULL || enc_result->hex_string == NULL)
                            {
                                if (enc_result != NULL)
                                    tecc_unified_free_encrypt_result(enc_result);
                                continue;
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
 * ReqID:TECC4.0 - Parse SUPPORT_ALGO string into boolean flags.
 * Input: "DET,SEARCH" → has_det=true, has_search=true
 */
static const char *
parse_support_algo_flags(const char *algo_value, bool *has_det, bool *has_ope, bool *has_search)
{
    char *algo_copy;
    char *saveptr;
    char *token;

    *has_det = false;
    *has_ope = false;
    *has_search = false;

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
    char *parent_schema;
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
    parent_schema = parent->schemaname ? parent->schemaname : "public";

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

    /* ReqID:TECC4.0 - Direct main connection fallback for cache miss */
    if ((parent_enc_cols == NULL || parent_enc_ncols == 0) &&
        statement_data != NULL && statement_data->conn != NULL)
    {
        PGconn *mc = statement_data->conn;
        int saved_tecc = mc->enable_tecc;
        char qbuf[8192], *esc_t, *esc_s;
        PGresult *qr; int nr, ri;
        mc->enable_tecc = 0;
        esc_t = PQescapeLiteral(mc, parent_table, strlen(parent_table));
        esc_s = PQescapeLiteral(mc, parent_schema, strlen(parent_schema));
        if (esc_t != NULL && esc_s != NULL)
        {
            snprintf(qbuf, sizeof(qbuf),
                "SELECT column_attnum,column_name,dek_name,"
                "encryption_type,data_type_original_oid,"
                "data_type_original_mod,collation_locale,"
                "column_key_id "
                "FROM hg_encrypted_columns_info "
                "JOIN pg_class cls ON table_oid=cls.oid "
                "JOIN pg_namespace ns ON cls.relnamespace=ns.oid "
                "WHERE cls.relname=%s AND ns.nspname=%s "
                "AND current_database()='%s' ORDER BY column_attnum",
                esc_t, esc_s, db_name ? db_name : "postgres");
            PQfreemem(esc_t); PQfreemem(esc_s);
            qr = PQexec(mc, qbuf);
            mc->enable_tecc = saved_tecc;
            if (qr != NULL && PQresultStatus(qr) == PGRES_TUPLES_OK &&
                (nr = PQntuples(qr)) > 0)
            {
                parent_enc_ncols = nr;
                parent_enc_cols = (TeccColumnCacheItem **)pg_malloc(
                    nr * sizeof(TeccColumnCacheItem *));
                for (ri = 0; ri < nr; ri++)
                {
                    TeccColumnCacheItem *ci = (TeccColumnCacheItem *)
                        pg_malloc0(sizeof(TeccColumnCacheItem));
                    ci->cache_status = TECC_CACHE_STATUS_ENCRYPTED;
                    ci->database_name = pg_strdup(db_name ? db_name : "postgres");
                    ci->schema_name = pg_strdup(parent_schema);
                    ci->table_name = pg_strdup(parent_table);
                    ci->column_attnum = atoi(PQgetvalue(qr, ri, 0));
                    ci->column_name = pg_strdup(PQgetvalue(qr, ri, 1));
                    ci->dek_name = pg_strdup(PQgetvalue(qr, ri, 2));
                    ci->encryption_type = atoi(PQgetvalue(qr, ri, 3));
                    ci->original_type_oid = atoi(PQgetvalue(qr, ri, 4));
                    ci->original_type_mod = atoi(PQgetvalue(qr, ri, 5));
                    {
                        const char *loc = PQgetvalue(qr, ri, 6);
                        if (!PQgetisnull(qr, ri, 6) && loc[0] != '\0')
                            ci->collation_locale = pg_strdup(loc);
                    }
                    ci->dek_id = atoi(PQgetvalue(qr, ri, 7));
                    parent_enc_cols[ri] = ci;
                }
            }
            if (qr != NULL) PQclear(qr);
        }
        else
        {
            mc->enable_tecc = saved_tecc;
            if (esc_t != NULL) PQfreemem(esc_t);
            if (esc_s != NULL) PQfreemem(esc_s);
        }
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
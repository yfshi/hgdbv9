#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "libpq-fe.h"
#include <string.h>

/* Begin - ReqID:TECC4.0 */
/*
 * File: alter_stmt_processor.c
 *
 * Abstract:
 *   ALTER TABLE statement processor implementation for partition management.
 *   Processes ATTACH PARTITION boundary value encryption.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/alter_stmt_processor.c
 */

#include "alter_stmt_processor.h"

/* Include node type definitions */
#include "../frontend_parser/tecc_nodes.h"

/* Encryption and cache interfaces */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"

/* Shared partition key column query */
#include "create_stmt_processor.h"

/*
 * ReqID:TECC4.0 - ALTER TABLE ATTACH/DETACH PARTITION processing.
 *
 * For ATTACH PARTITION with FOR VALUES, encrypts partition boundary
 * values that correspond to deterministic-encrypted columns in the
 * parent table. Supports RANGE (FROM/TO) and LIST (IN) boundaries.
 *
 * For DETACH PARTITION, no encryption processing is needed.
 */
TeccAlterStmtProcessorResult
tecc_alter_stmt_processor_run(const AlterTableStmt *alter_stmt,
                              TeccStatementData *statement_data)
{
    ListCell *lc;
    bool has_attach = false;
    const char *parent_schema;
    char *parent_table;

    if (alter_stmt == NULL || statement_data == NULL)
        return TECC_ALTER_PROCESSOR_ERROR;

    /* Get parent table name */
    if (alter_stmt->relation == NULL || alter_stmt->relation->relname == NULL)
        return TECC_ALTER_PROCESSOR_ERROR;

    parent_table = alter_stmt->relation->relname;
    /* ReqID:TECC4.0 - 未限定表名按 search_path 解析真实 schema（同 stmt_load_table_metadata） */
    parent_schema = tecc_stmt_resolve_table_schema(statement_data,
                                                   alter_stmt->relation->schemaname,
                                                   alter_stmt->relation->relname);

    /* Scan cmds for AT_AttachPartition */
    foreach(lc, alter_stmt->cmds)
    {
        AlterTableCmd *cmd = (AlterTableCmd *) lfirst(lc);

        if (cmd == NULL)
            continue;

        if (cmd->subtype == AT_DetachPartition)
        {
            /* DETACH PARTITION — no boundary to encrypt, pass through */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                "ALTER TABLE DETACH PARTITION — no encryption needed");
            continue;
        }

        if (cmd->subtype != AT_AttachPartition)
            continue;

        has_attach = true;

        /* Process ATTACH PARTITION boundary encryption */
        {
            PartitionBoundSpec *partbound;
            char **partkey_cols = NULL;
            int partkey_ncols = 0;
            TeccColumnCacheItem **parent_enc_cols = NULL;
            int parent_enc_ncols = 0;
            TeccState *state = NULL;
            const char *db_name;
            List *datum_lists[2];
            int n_lists;
            int pos;
            ListCell *dlc;

            partbound = (PartitionBoundSpec *)cmd->def;
            if (partbound == NULL || partbound->is_default)
                continue;

            /* Query partition key column names from parent table */
            partkey_cols = tecc_query_partkey_columns(statement_data->conn,
                                                      parent_schema,
                                                      parent_table,
                                                      &partkey_ncols);
            if (partkey_cols == NULL || partkey_ncols == 0)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                    "No partition key columns found for '%s.%s'",
                    parent_schema, parent_table);
                continue;
            }

            /* Query encrypted columns from parent table */
            state = tecc_get_state(statement_data->conn);
            if (state == NULL)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                    "No TECC state — ATTACH PARTITION encryption skipped");
                goto attach_cleanup;
            }

            db_name = PQdb(statement_data->conn);
            if (db_name == NULL) db_name = "postgres";

            parent_enc_cols = tecc_cache_get_columns_by_table_name(
                state, db_name, parent_schema, parent_table, &parent_enc_ncols);

            /* Determine which datum lists to iterate */
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
                    goto attach_cleanup;
            }

            /* Encrypt boundary values at positions matching encrypted columns */
            {
                int offset_adjust = 0;
                for (int li = 0; li < n_lists; li++)
            {
                if (datum_lists[li] == NIL)
                    continue;

                pos = 0;
                foreach(dlc, datum_lists[li])
                {
                    PartitionRangeDatum *datum = (PartitionRangeDatum *) lfirst(dlc);
                    Const *c;
                    int col_pos;
                    TeccColumnCacheItem *col_cache = NULL;

                    /*
                     * For RANGE: each value has its own position.
                     * For LIST: all values correspond to position 0.
                     * NOTE: Multi-column LIST is not supported by PG,
                     * so position is always 0.
                     */
                    col_pos = (partbound->strategy == 'l') ? 0 : pos;

                    if (col_pos >= partkey_ncols)
                        break;

                    if (datum == NULL || datum->value == NULL)
                    {
                        pos++;
                        continue;  /* MINVALUE/MAXVALUE */
                    }

                    if (!IsA(datum->value, Const))
                    {
                        pos++;
                        continue;
                    }

                    c = (Const *)datum->value;

                    /* Look up this column in the parent's encrypted columns */
                    if (parent_enc_cols != NULL)
                    {
                        for (int i = 0; i < parent_enc_ncols; i++)
                        {
                            if (parent_enc_cols[i] != NULL &&
                                parent_enc_cols[i]->column_name != NULL &&
                                strcmp(parent_enc_cols[i]->column_name,
                                       partkey_cols[col_pos]) == 0)
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
                            "RANGE partition key column '%s' must use OPE "
                            "encryption for ordering. DET-only encryption does "
                            "not preserve plaintext order. Set support_algo = "
                            "'ope' for this column.",
                            partkey_cols[col_pos]);
                        goto attach_cleanup;
                    }

                    /* Encrypt boundary value using shared helper */
                    {
                        TeccBoundEncInfo *enc_info;
                        bool is_error = false;

                        enc_info = tecc_encrypt_boundary_value(
                            statement_data->conn, state, c, col_cache,
                            parent_table, partkey_cols[col_pos],
                            &is_error, statement_data);
                        if (enc_info == NULL)
                        {
                            if (is_error)
                                goto attach_cleanup;
                            pos++;
                            continue;
                        }

                        /* Inline SQL rewrite with offset tracking */
                        {
                            const char *query = statement_data->query;
                            char *current_query;
                            char replacement[1024];
                            int repl_len;
                            int adj_start, adj_end;
                            size_t new_len;
                            char *new_query;

                            current_query = statement_data->rewritten_query ?
                                pg_strdup(statement_data->rewritten_query) :
                                pg_strdup(query);

                            if (current_query == NULL)
                            {
                                tecc_free_bound_enc_info(enc_info);
                                goto attach_cleanup;
                            }

                            {
                                const char *hex = enc_info->encrypted_hex;

                                if (hex[0] == '\\' && hex[1] == 'x')
                                    hex += 2;
                                repl_len = snprintf(replacement, sizeof(replacement),
                                                    "E'\\\\x%s'", hex);
                            }
                            if (repl_len >= (int)sizeof(replacement))
                            {
                                pg_free(current_query);
                                tecc_free_bound_enc_info(enc_info);
                                pos++;
                                continue;
                            }

                            adj_start = enc_info->value_location + offset_adjust;
                            adj_end = enc_info->value_location + enc_info->value_length + offset_adjust;

                            if (adj_start < 0 ||
                                adj_end > (int)strlen(current_query))
                            {
                                pg_free(current_query);
                                tecc_free_bound_enc_info(enc_info);
                                pos++;
                                continue;
                            }

                            new_len = strlen(current_query) -
                                      enc_info->value_length + repl_len;
                            new_query = (char *)pg_malloc(new_len + 1);

                            memcpy(new_query, current_query, adj_start);
                            memcpy(new_query + adj_start, replacement, repl_len);
                            memcpy(new_query + adj_start + repl_len,
                                   current_query + adj_end,
                                   strlen(current_query) - adj_end + 1);

                            if (statement_data->rewritten_query != NULL)
                                pg_free(statement_data->rewritten_query);
                            statement_data->rewritten_query = new_query;
                            statement_data->needs_rewrite = 1;

                            pg_free(current_query);
                            offset_adjust += repl_len - enc_info->value_length;

                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ATTACH PARTITION boundary encrypted "
                                "for column '%s': '%s' -> hex",
                                partkey_cols[col_pos], enc_info->plaintext_value);
                        }

                        tecc_free_bound_enc_info(enc_info);
                    }
                    pos++;
                }
            }
            }

attach_cleanup:
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
    }

    if (!has_attach)
    {
        /* No ATTACH PARTITION command found — might be a different ALTER TABLE */
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "ALTER TABLE command — no ATTACH PARTITION found, skipping");
    }

    if (statement_data->error_code != 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
            "ALTER TABLE ATTACH PARTITION processing failed: %s",
            statement_data->error_message);
        return TECC_ALTER_PROCESSOR_ERROR;
    }

    return TECC_ALTER_PROCESSOR_SUCCESS;
}
/* End - ReqID:TECC4.0 */

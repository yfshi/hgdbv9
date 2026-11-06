/*-------------------------------------------------------------------------
 *
 * fe-tecc-copy.h
 *    TECC COPY data processing interfaces
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-tecc-copy.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef FE_TECC_COPY_H
#define FE_TECC_COPY_H

#include "libpq-fe.h"
#include "../tecc_common/tecc_common_types.h"

/* ==================== COPY Processing Interfaces ==================== */

/* COPY预处理接口 */
extern TeccCopyState *tecc_preprocess_copy(PGconn *conn, const char *command);

/* COPY数据加密接口 */
extern int tecc_process_copy_data(PGconn *conn, const char *data, int data_len,
                                  char **processed_data, int *processed_len);

/* COPY数据解密接口 */
extern int tecc_process_copy_result(PGconn *conn, const char *encrypted_data,
                                    int encrypted_len, char **decrypted_data,
                                    int *decrypted_len);

/* COPY加密上下文清理接口 */
extern void tecc_copy_state_cleanup_crypto_context(TeccEnhancedCopyState *copy_state);

/* COPY状态清理接口 */
extern void tecc_cleanup_copy_state(PGconn *conn);

/* ==================== Batch Processing Interfaces ==================== */

/* 批量处理配置接口 */
extern int tecc_copy_enable_batch_processing(PGconn *conn, int batch_size);
extern int tecc_copy_disable_batch_processing(PGconn *conn);
extern int tecc_copy_set_batch_size(PGconn *conn, int batch_size);
extern int tecc_copy_set_max_errors(PGconn *conn, int max_errors);
extern int tecc_copy_set_stop_on_error(PGconn *conn, bool stop_on_error);

/* 批量数据导入接口 */
extern int tecc_process_copy_data_batch(PGconn *conn, const char *data, int data_len,
                                        char **processed_data, int *processed_len,
                                        int *rows_processed, int *errors_found);

/* 批量数据导出接口 */
extern int tecc_process_copy_result_batch(PGconn *conn, const char *encrypted_data,
                                          int encrypted_len, char **decrypted_data,
                                          int *decrypted_len, int *rows_processed,
                                          int *errors_found);

/* 批量处理统计接口 */
extern int tecc_copy_get_batch_statistics(PGconn *conn, int *total_rows,
                                          int *successful_rows, int *failed_rows,
                                          int *total_batches, double *avg_batch_time);

/* 批量处理错误信息接口 */
extern int tecc_copy_get_batch_errors(PGconn *conn, TeccCopyError **errors,
                                      int *error_count);

/* 批量处理重置接口 */
extern int tecc_copy_reset_batch_statistics(PGconn *conn);

#endif /* FE_TECC_COPY_H */

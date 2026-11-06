/* Begin - ReqID:TECC4.0 */
/*
 * File: alter_stmt_processor.h
 *
 * Abstract:
 *   ALTER TABLE statement processor for partition management (ATTACH/DETACH).
 *   Handles encryption of partition boundary values in ATTACH PARTITION.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/alter_stmt_processor.h
 */

#ifndef ALTER_STMT_PROCESSOR_H
#define ALTER_STMT_PROCESSOR_H

#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"

/* Forward declarations */
typedef struct AlterTableStmt AlterTableStmt;

/*
 * ALTER statement processor result codes
 */
typedef enum TeccAlterStmtProcessorResult
{
    TECC_ALTER_PROCESSOR_SUCCESS = 0,
    TECC_ALTER_PROCESSOR_ERROR = -1,
    TECC_ALTER_PROCESSOR_UNSUPPORTED = -2
} TeccAlterStmtProcessorResult;

/* Main ALTER TABLE processing entry point */
extern TeccAlterStmtProcessorResult tecc_alter_stmt_processor_run(
    const AlterTableStmt *alter_stmt,
    TeccStatementData *statement_data);

#endif /* ALTER_STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */

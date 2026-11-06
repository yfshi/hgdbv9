/*-------------------------------------------------------------------------
 *
 * create_kek_processor.h
 *	  CREATE KEK statement processor with HSM support
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/tecc_processor/create_kek_processor.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CREATE_KEK_PROCESSOR_H
#define CREATE_KEK_PROCESSOR_H

#include "tecc_processor_types.h"

/* Result codes for CREATE KEK processor */
typedef enum
{
	TECC_CREATE_KEK_PROCESSOR_SUCCESS = 0,
	TECC_CREATE_KEK_PROCESSOR_INVALID_INPUT,
	TECC_CREATE_KEK_PROCESSOR_VALIDATION_ERROR,
	TECC_CREATE_KEK_PROCESSOR_HSM_ERROR,
	TECC_CREATE_KEK_PROCESSOR_UNSUPPORTED_KMS,
	TECC_CREATE_KEK_PROCESSOR_INTERNAL_ERROR
} TeccCreateKekProcessorResult;

/* Function declarations */
extern TeccCreateKekProcessorResult tecc_create_kek_processor_run(
	const CreateKekStmt *create_kek_stmt,
	TeccStatementData *statement_data);

#endif /* CREATE_KEK_PROCESSOR_H */
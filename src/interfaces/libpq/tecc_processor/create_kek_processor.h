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

/* Restore-time key-file presence check for CREATE KEK replay (pg_dump output).
 * design 6.2.3/6.2.4: the dumped SQL carries _tecc_processed + public_key and
 * passes through unchanged, but the target key files (.key / .priv+.pub) must
 * still be present at the TECC_KEK_PATH-resolved path before the metadata is
 * re-created.  Returns 0 when files are present (or store has no file to
 * verify), nonzero when a required file is missing. */
extern int tecc_restore_validate_kek_key_files(
	const CreateKekStmt *create_kek_stmt);

/* Restore-time key-file presence check for CREATE KEK replay in the canonical
 * pg_dump text form (schema-qualified parens WITHOUT the "WITH" keyword).
 * The TECC frontend grammar only parses the live "WITH (...)" spelling, so a
 * dumped CREATE KEK yields no parse tree and normally passes straight through
 * to the server; this text-based check re-applies design 6.2.3/6.2.4 so the
 * replay still fails when the required localkms key files are missing.
 * Returns 0 when files are present / store has nothing to verify / text can't
 * be parsed, nonzero when a required file is missing. */
extern int tecc_restore_validate_create_kek_text(const char *query);

#endif /* CREATE_KEK_PROCESSOR_H */
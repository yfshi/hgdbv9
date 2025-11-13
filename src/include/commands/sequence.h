/*-------------------------------------------------------------------------
 *
 * sequence.h
 *	  prototypes for sequence.c.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/sequence.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "access/xlogreader.h"
#include "catalog/objectaddress.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"
#include "storage/relfilenode.h"

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/* mys_set_insertval oid */
#define AUTO_INCREMENT_SET_VALUE_FUNC_OID        (4660)
/* zero value */
#define AUTO_INCREMENT_VALUE_ZERO        (0)
/* insert value type flags */
#define AUTO_INCREMENT_INSERT_NULL	     (1)
#define AUTO_INCREMENT_INSERT_DEFAULT	 (2)
#define AUTO_INCREMENT_INSERT_ZERO       (3)
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

typedef struct FormData_pg_sequence_data
{
	int64		last_value;
	int64		log_cnt;
	bool		is_called;
	int64		next_value;  /* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
} FormData_pg_sequence_data;

typedef FormData_pg_sequence_data *Form_pg_sequence_data;

/*
 * Columns of a sequence relation
 */

#define SEQ_COL_LASTVAL			1
#define SEQ_COL_LOG				2
#define SEQ_COL_CALLED			3
#define SEQ_COL_NEXTVAL			4 /* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */ 

#define SEQ_COL_FIRSTCOL		SEQ_COL_LASTVAL
#define SEQ_COL_LASTCOL			SEQ_COL_NEXTVAL

/* XLOG stuff */
#define XLOG_SEQ_LOG			0x00

/* Begin - ReqID:SRS-SQL-SEQ */
/* scale and session stuff */
#define SCALE_FLAG				16
#define EXTEND_FLAG				2048
#define SESSION_FLAG			64
/* End - ReqID:SRS-SQL-SEQ */

typedef struct xl_seq_rec
{
	RelFileNode node;
	/* SEQUENCE TUPLE DATA FOLLOWS AT THE END */
} xl_seq_rec;

extern int64 nextval_internal(Oid relid, bool check_permissions);
extern Datum nextval(PG_FUNCTION_ARGS);
extern List *sequence_options(Oid relid);

extern ObjectAddress DefineSequence(ParseState *pstate, CreateSeqStmt *stmt);
extern ObjectAddress AlterSequence(ParseState *pstate, AlterSeqStmt *stmt);
extern void SequenceChangePersistence(Oid relid, char newrelpersistence);
extern void DeleteSequenceTuple(Oid relid);
extern void ResetSequence(Oid seq_relid);
extern void ResetSequenceCaches(void);

extern void seq_redo(XLogReaderState *rptr);
extern void seq_desc(StringInfo buf, XLogReaderState *rptr);
extern const char *seq_identify(uint8 info);
extern void seq_mask(char *pagedata, BlockNumber blkno);
extern int64 get_sessionid(void);	/* ReqID:SRS-SQL-SEQ */

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
extern int64 nextval_autoincrement(Oid relid, bool check_permissions);

extern Datum mys_set_lastval(PG_FUNCTION_ARGS);
extern Datum mys_get_lastval(PG_FUNCTION_ARGS);
extern void set_auto_nextval(Oid relid, uint64 nextval);

extern Datum mys_set_insertval(PG_FUNCTION_ARGS);
extern uint64 get_show_autoinc(Oid tableoid);
extern Datum show_next_autoinc(PG_FUNCTION_ARGS);
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

#endif							/* SEQUENCE_H */

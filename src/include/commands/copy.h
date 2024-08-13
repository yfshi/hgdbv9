/*-------------------------------------------------------------------------
 *
 * copy.h
 *	  Definitions for using the POSTGRES copy command.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/copy.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef COPY_H
#define COPY_H

#include "nodes/execnodes.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"
#include "tcop/dest.h"

#define MAX_SKIP_COUNT  128     /* BUG:M0000368 */

/*
 * A struct to hold COPY options, in a parsed form. All of these are related
 * to formatting, except for 'freeze', which doesn't really belong here, but
 * it's expedient to parse it along with all the other options.
 */
typedef struct CopyFormatOptions
{
	/* parameters from the COPY command */
	int			file_encoding;	/* file or remote side's character encoding,
								 * -1 if not specified */
	bool		binary;			/* binary format? */
	bool		freeze;			/* freeze rows on loading? */
	bool		csv_mode;		/* Comma Separated Value format? */
	bool		header_line;	/* CSV header line? */
	char	   *null_print;		/* NULL marker string (server encoding!) */
	int			null_print_len; /* length of same */
	char	   *null_print_client;	/* same converted to file encoding */
	char	   *delim;			/* column delimiter (must be 1 byte) */
	char	   *quote;			/* CSV quote char (must be 1 byte) */
	char	   *escape;			/* CSV escape char (must be 1 byte) */
	List	   *force_quote;	/* list of column names */
	bool		force_quote_all;	/* FORCE_QUOTE *? */
	bool	   *force_quote_flags;	/* per-column CSV FQ flags */
	List	   *force_notnull;	/* list of column names */
	bool	   *force_notnull_flags;	/* per-column CSV FNN flags */
	List	   *force_null;		/* list of column names */
	bool	   *force_null_flags;	/* per-column CSV FN flags */
	bool		convert_selectively;	/* do selective binary conversion? */
	List	   *convert_select; /* list of column names (can be NIL) */

	/* Begin - ReqID:SRS-MISC-BULKLOAD */
	bool	   tolerate;            /* continue if error arise */
	char	   *badfile;            /* log error record to file*/
	char	   *badtable;           /* log error record to table */
	int	   sharding;            /* sharding number */
	int	   current_shard;       /* current shard number*/
	char	   *taskid;		/* current bulkload task id */
        char	   *filename;		/* used for error output */
	/* End - ReqID:SRS-MISC-BULKLOAD */
	/* Begin - BUG#Z1087 */
	char	   *logbad;
	bool	   log_bad;
	/* End - BUG#Z1087 */

	/* Begin - BUG:M0000368 */
	List	   *skiplist;
	//int	   *skip_list;
	//int	   skip_count;
	// skip_list[0] did not used
	bool	   skip_list[MaxHeapAttributeNumber + MAX_SKIP_COUNT + 1];
	/* End - BUG:M0000368 */

} CopyFormatOptions;

/* These are private in commands/copy[from|to].c */
typedef struct CopyFromStateData *CopyFromState;
typedef struct CopyToStateData *CopyToState;

typedef int (*copy_data_source_cb) (void *outbuf, int minread, int maxread);

extern void DoCopy(ParseState *state, const CopyStmt *stmt,
				   int stmt_location, int stmt_len,
				   uint64 *processed);

extern void ProcessCopyOptions(ParseState *pstate, CopyFormatOptions *ops_out, bool is_from, List *options);
extern CopyFromState BeginCopyFrom(ParseState *pstate, Relation rel, Node *whereClause,
								   const char *filename,
								   bool is_program, copy_data_source_cb data_source_cb, List *attnamelist, List *options);
extern void EndCopyFrom(CopyFromState cstate);
extern bool NextCopyFrom(CopyFromState cstate, ExprContext *econtext,
						 Datum *values, bool *nulls);
extern bool NextCopyFromRawFields(CopyFromState cstate,
								  char ***fields, int *nfields);
extern void CopyFromErrorCallback(void *arg);

extern uint64 CopyFrom(CopyFromState cstate);

extern DestReceiver *CreateCopyDestReceiver(void);

/*
 * internal prototypes
 */
extern CopyToState BeginCopyTo(ParseState *pstate, Relation rel, RawStmt *query,
							   Oid queryRelId, const char *filename, bool is_program,
							   List *attnamelist, List *options);
extern void EndCopyTo(CopyToState cstate);
extern uint64 DoCopyTo(CopyToState cstate);
extern List *CopyGetAttnums(TupleDesc tupDesc, Relation rel,
							List *attnamelist);

/* Begin - ReqID:SRS-MISC-BULKLOAD */
extern bool OutputBadRecord(CopyFromState cstate,uint64 lineno,StringInfoData *linebuf,char type);
/* used for handle FK checking error */
#define FK_CHECK_OID 1644
/* used for cut big record for store and output */
#define MAX_RECORD_LENGTHS 2000
/* Bad Record type */
#define BAD_RECORD_R	'r'	/* read error */
#define BAD_RECORD_I	'i'	/* index insert error */
#define BAD_RECORD_F    'f'	/* fk checking error */

/* End - ReqID:SRS-MISC-BULKLOAD */

#endif							/* COPY_H */

/*-------------------------------------------------------------------------
 *
 * psqlscan_int.h
 *	  lexical scanner internal declarations
 *
 * This file declares the PsqlScanStateData structure used by psqlscan.l
 * and shared by other lexers compatible with it, such as psqlscanslash.l.
 *
 * One difficult aspect of this code is that we need to work in multibyte
 * encodings that are not ASCII-safe.  A "safe" encoding is one in which each
 * byte of a multibyte character has the high bit set (it's >= 0x80).  Since
 * all our lexing rules treat all high-bit-set characters alike, we don't
 * really need to care whether such a byte is part of a sequence or not.
 * In an "unsafe" encoding, we still expect the first byte of a multibyte
 * sequence to be >= 0x80, but later bytes might not be.  If we scan such
 * a sequence as-is, the lexing rules could easily be fooled into matching
 * such bytes to ordinary ASCII characters.  Our solution for this is to
 * substitute 0xFF for each non-first byte within the data presented to flex.
 * The flex rules will then pass the FF's through unmolested.  The
 * psqlscan_emit() subroutine is responsible for looking back to the original
 * string and replacing FF's with the corresponding original bytes.
 *
 * Another interesting thing we do here is scan different parts of the same
 * input with physically separate flex lexers (ie, lexers written in separate
 * .l files).  We can get away with this because the only part of the
 * persistent state of a flex lexer that depends on its parsing rule tables
 * is the start state number, which is easy enough to manage --- usually,
 * in fact, we just need to set it to INITIAL when changing lexers.  But to
 * make that work at all, we must use re-entrant lexers, so that all the
 * relevant state is in the yyscan_t attached to the PsqlScanState;
 * if we were using lexers with separate static state we would soon end up
 * with dangling buffer pointers in one or the other.  Also note that this
 * is unlikely to work very nicely if the lexers aren't all built with the
 * same flex version, or if they don't use the same flex options.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/fe_utils/psqlscan_int.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PSQLSCAN_INT_H
#define PSQLSCAN_INT_H

#include "fe_utils/psqlscan.h"
#include "oracle_fe_utils/ora_psqlscan.h"	/* ReqID:SRS-PSQL-PARSER */
#include "mysql_fe_utils/mys_psqlscan.h"	/* ReqID:SRS-MISC-MYSQL-FRAME */

/*
 * These are just to allow this file to be compilable standalone for header
 * validity checking; in actual use, this file should always be included
 * from the body of a flex file, where these symbols are already defined.
 */
#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

/*
 * We use a stack of flex buffers to handle substitution of psql variables.
 * Each stacked buffer contains the as-yet-unread text from one psql variable.
 * When we pop the stack all the way, we resume reading from the outer buffer
 * identified by scanbufhandle.
 */
typedef struct StackElem
{
	YY_BUFFER_STATE buf;		/* flex input control structure */
	char	   *bufstring;		/* data actually being scanned by flex */
	char	   *origstring;		/* copy of original data, if needed */
	char	   *varname;		/* name of variable providing data, or NULL */
	struct StackElem *next;
} StackElem;

/* Begin - ReqID:SRS-CMD-PSQL */
/*
 * Oracle SQL*PLUS client cmd node types
 */
typedef enum psqlplus_cmd_type
{
	PSQLPLUS_CMD_VARIABLE,
	PSQLPLUS_CMD_EXECUTE,
	PSQLPLUS_CMD_PRINT,
	/* Begin - ReqID:SRS-SQL-SYSFUNC */
	PSQLPLUS_CMD_SET,
	PSQLPLUS_CMD_SHOW
	/* End - ReqID:SRS-SQL-SYSFUNC */
} psqlplus_cmd_type;

typedef struct BindVarType
{
	char	*name;		/* name of datatype */
	int32	oid;		/* oid of datatype */
	int32	typmod;		/* length with header */
} BindVarType;

/*
 * Generic command node
 */
typedef struct psqlplus_cmd
{
	psqlplus_cmd_type	cmd_type;
} psqlplus_cmd;

/*
 * VARIABLE command node
 */
typedef struct psqlplus_cmd_var
{
	psqlplus_cmd_type	cmd_type;
	char				*var_name;	/* bind variable name */
	BindVarType			*vartype;	/* bind variable datatype */
	bool				list_bind_var;
	bool				assign_bind_var;

	/*
	 * Starting from oracle 12c, an initial value can
	 * be specified when creating a bind variable.
	 *
	 * This is a syntax-specified variable value that
	 * may not be an acceptable valid value.Therefore,
	 * we must interact with the server and perform a
	 * legality check of the initial value.
	 *
	 * If miss_termination_quote is true, we don't need
	 * to perform an interaction with the server.
	 *
	 * init_value is used to save the initial value of the
	 * bind variable or the new value in assignment statement.
	 */
	bool				miss_termination_quote;
	bool				initial_nonnull_value;
	char				*init_value;
} psqlplus_cmd_var;

/* Data fields of the PRINT list structure */
typedef struct print_item
{
	char	*bv_name;			/* name of bind var */
	bool	valid;				/* is it a legal name */
} print_item;

/* PRINT list structure */
typedef struct print_list
{
	print_item	**items;	/* array of print_item* */
	int			length;		/* # of items */
} print_list;

/*
 * PRINT command node
 */
typedef struct psqlplus_cmd_print
{
	psqlplus_cmd_type	cmd_type;
	print_list			*print_items;	/* if NULL print all bind variables */
} psqlplus_cmd_print;

/*
 * EXECUTE command node
 */
typedef struct psqlplus_cmd_execute
{
	psqlplus_cmd_type	cmd_type;
	char				*plisqlstmts;	/* plisql statements text */
} psqlplus_cmd_execute;
/* End - ReqID:SRS-CMD-PSQL */

/* Begin - ReqID:SRS-SQL-SYSFUNC */
/*
 * SET/SHOW system variable
 */
typedef enum sys_var_type
{
	SVT_NONE = 0,
	/* SCALAR */
	SVT_BOOL,
	SVT_INTEGER,
	SVT_STRING,
	/* COMPOSITE VARIABLE */
	SVT_COMPOSITE
} sys_var_type;

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
/* subtype for SVT_COMPOSITE system variable */
typedef enum composite_sys_var_tag
{
	CSV_NONE = 0,
	CSV_SERVEROUTPUT
} composite_sys_var_tag;
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

typedef union sys_var_value
{
	int64	ival;
	char   *string;	/* ReqID:SRS-PLSQL-DBMS_OUTPUT: reuse for composite system var */
	bool	on;
} sys_var_value;

typedef struct system_variable_data
{
	char		   *name;
	sys_var_type	type;
	sys_var_value	value;
	int				min_name_len;	/* minimum matching length,
									   e.g. 3 for NUM[WIDTH] */
	void		   *extra;			/* extra information,
									   e.g. range array of datatype integer */
} system_variable_data;

typedef system_variable_data *system_variable;

typedef struct sys_var
{
	char		   *sv_name;
	sys_var_type	sv_type;
	sys_var_value	sv_value;
} sys_var;

typedef struct sys_var_list
{
	sys_var   **sysvars;
	int			length;
} sys_var_list;

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
typedef struct csv_node
{
	composite_sys_var_tag	tag;
} csv_node;

typedef struct csv_serveroutput_node
{
	composite_sys_var_tag csv_tag;
	bool	enable;
	int		size;
	//enum	format; /* TODO */
} csv_serveroutput_node;

#define SV_SERVEROUT_BUFSIZE_MIN			2000
#define SV_SERVEROUT_BUFSIZE_MAX			1000000
#define SV_SERVEROUT_BUFSIZE_UNLIMITED		-1	/* unlimit size */
#define SV_SERVEROUT_NO_SIZE_OPTION			-2	/* no SIZE option */
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

typedef struct psqlplus_cmd_sv
{
	psqlplus_cmd_type	cmd_type;
	sys_var_list	   *sv_list;
} psqlplus_cmd_sv;
/* End - ReqID:SRS-SQL-SYSFUNC */

/*
 * All working state of the lexer must be stored in PsqlScanStateData
 * between calls.  This allows us to have multiple open lexer operations,
 * which is needed for nested include files.  The lexer itself is not
 * recursive, but it must be re-entrant.
 */
typedef struct PsqlScanStateData
{
	yyscan_t	scanner;		/* Flex's state for this PsqlScanState */

	PQExpBuffer output_buf;		/* current output buffer */

	StackElem  *buffer_stack;	/* stack of variable expansion buffers */

	/*
	 * These variables always refer to the outer buffer, never to any stacked
	 * variable-expansion buffer.
	 */
	YY_BUFFER_STATE scanbufhandle;
	char	   *scanbuf;		/* start of outer-level input buffer */
	const char *scanline;		/* current input line at outer level */

	/* safe_encoding, curline, refline are used by emit() to replace FFs */
	int			encoding;		/* encoding being used now */
	bool		safe_encoding;	/* is current encoding "safe"? */
	bool		std_strings;	/* are string literals standard? */
	const char *curline;		/* actual flex input string for cur buf */
	const char *refline;		/* original data for cur buffer */

	/*
	 * All this state lives across successive input lines, until explicitly
	 * reset by psql_scan_reset.  start_state is adopted by yylex() on entry,
	 * and updated with its finishing state on exit.
	 */
	int			postion_len;	/* record the postion of the first symmetric char */	/* ReqID:SRS-SQL-ESCAPECHAR */
	int			start_state;	/* yylex's starting/finishing state */
	int			state_before_str_stop;	/* start cond. before end quote */
	int			paren_depth;	/* depth of nesting in parentheses */
	int			xcdepth;		/* depth of nesting in slash-star comments */
	char	   *dolqstart;		/* current $foo$ quote start string */

	/*
	 * State to track boundaries of BEGIN ... END blocks in function
	 * definitions, so that semicolons do not send query too early.
	 */
	int			identifier_count;	/* identifiers since start of statement */
	char		identifiers[4]; /* records the first few identifiers */
	int			begin_depth;	/* depth of begin/end pairs */

	/* Begin - ReqID:SRS-SQL-PROC */
	bool		cancel_semicolon_terminator; /* not send command when semicolon found */

	/*
	 * State to track boundaries of Oracle ANONYMOUS BLOCK.
	 * Case 1: Statements starting with << ident >> is Oracle anonymous block.
	 */
	int			token_count;			/* # of tokens, not blank or newline since start of statement */
	bool		anonymous_label_start;	/* T if the first token is "<<" */
	bool		anonymous_label_ident;	/* T if the second token is an identifier */
	bool		anonymous_label_end;	/* T if the third token is ">>" */
	bool		xcstate;				/* T If a C-style comment is being parsed */

	/*
	 * Case 2: DECLARE BEGIN ... END is Oracle anonymous block sytax.
	 * DECLARE can also be a PostgreSQL cursor declaration statement, we need to distinguish it.
	 */
	bool		maybe_anonymous_declare_start;	/* T if the first token is DECLARE */
	int			token_cursor_idx;				/* the position of keyword CURSOR in SQL statement */

	/*
	 * Case 3: DECLARE BEGIN ... END is Oracle anonymous block sytax.
	 * BEGIN can also be a PostgreSQL transaction statement.
	 */
	bool		maybe_anonymous_begin_start;	/* T if the first token is BEGIN */
	/* End - ReqID:SRS-SQL-PROC */

	/*
	 * Callback functions provided by the program making use of the lexer,
	 * plus a void* callback passthrough argument.
	 */
	const PsqlScanCallbacks *callbacks;
	const Ora_psqlScanCallbacks *oracallbacks;	/* ReqID:SRS-PSQL-PARSER */
	const Mys_psqlScanCallbacks *myscallbacks;	/* ReqID:SRS-MISC-MYSQL-FRAME */
	void	   *cb_passthrough;

	/* Begin - ReqID:SRS-CMD-PSQL */
	/*
	 * literalbuf is used to accumulate literal values when multiple rules are
	 * needed to parse a single literal.  Call startlit() to reset buffer to
	 * empty, addlit() to add text.  NOTE: the string in literalbuf is NOT
	 * necessarily null-terminated, but there always IS room to add a trailing
	 * null at offset literallen.  We store a null only when we need it.
	 */
	char	   *literalbuf;		/* palloc'd expandable buffer */
	int			literallen;		/* actual current string length */
	int			literalalloc;	/* current allocated buffer size */

	bool			is_sqlplus_cmd;	/* T if is a psqlplus command */
	psqlplus_cmd	*psqlpluscmd;
	/* End - ReqID:SRS-CMD-PSQL */
	/* Begin - BUG#M000411 */
	bool		send_space_one_comments;	/*
											 * Whether send space or one line comments to server
											 */
	/* End - BUG#M0000411 */
	/* Begin- SRS-MYSQL-SQL-USE */
    bool   is_use;
    bool   use_has_sem;
    /* End- SRS-MYSQL-SQL-USE */
} PsqlScanStateData;


/*
 * Functions exported by psqlscan.l, but only meant for use within
 * compatible lexers.
 */
extern void psqlscan_push_new_buffer(PsqlScanState state,
									 const char *newstr, const char *varname);
extern void psqlscan_pop_buffer_stack(PsqlScanState state);
extern void psqlscan_select_top_buffer(PsqlScanState state);
extern bool psqlscan_var_is_current_source(PsqlScanState state,
										   const char *varname);
extern YY_BUFFER_STATE psqlscan_prepare_buffer(PsqlScanState state,
											   const char *txt, int len,
											   char **txtcopy);
extern void psqlscan_emit(PsqlScanState state, const char *txt, int len);
extern char *psqlscan_extract_substring(PsqlScanState state,
										const char *txt, int len);
extern void psqlscan_escape_variable(PsqlScanState state,
									 const char *txt, int len,
									 PsqlScanQuoteType quote);
extern void psqlscan_test_variable(PsqlScanState state,
								   const char *txt, int len);

/* Begin - ReqID:SRS-PSQL-PARSER */
/*
 * for oracle parser
 */
extern void ora_psqlscan_push_new_buffer(PsqlScanState state, const char *newstr,
									const char *varname);
extern void ora_psqlscan_pop_buffer_stack(PsqlScanState state);
extern void ora_psqlscan_select_top_buffer(PsqlScanState state);
extern bool ora_psqlscan_var_is_current_source(PsqlScanState state, const char *varname);
extern YY_BUFFER_STATE ora_psqlscan_prepare_buffer(PsqlScanState state, const char *txt, int len,
									char **txtcopy);
extern void ora_psqlscan_emit(PsqlScanState state, const char *txt, int len);
extern char * ora_psqlscan_extract_substring(PsqlScanState state, const char *txt, int len);
extern void ora_psqlscan_escape_variable(PsqlScanState state, const char *txt, int len,
									PsqlScanQuoteType quote);
extern void ora_psqlscan_test_variable(PsqlScanState state, const char *txt, int len);
/* End - ReqID:SRS-PSQL-PARSER */

/* Begin - ReqID:SRS-SQL-SYSFUNC */
extern system_variable GetSysVariable(const char *name);
extern bool ShowSysVariable(sys_var_list *list);
extern bool SetSysVariable(sys_var_list *list, bool *serveroutput_set /* ReqID:SRS-PLSQL-DBMS_OUTPUT */);
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
/*
 * for mysql parser
 */
extern void mys_psqlscan_push_new_buffer(PsqlScanState state, const char *newstr,
									const char *varname);
extern void mys_psqlscan_pop_buffer_stack(PsqlScanState state);
extern void mys_psqlscan_select_top_buffer(PsqlScanState state);
extern bool mys_psqlscan_var_is_current_source(PsqlScanState state, const char *varname);
extern YY_BUFFER_STATE mys_psqlscan_prepare_buffer(PsqlScanState state, const char *txt, int len,
									char **txtcopy);
extern void mys_psqlscan_emit(PsqlScanState state, const char *txt, int len);
extern char * mys_psqlscan_extract_substring(PsqlScanState state, const char *txt, int len);
extern void mys_psqlscan_escape_variable(PsqlScanState state, const char *txt, int len,
									PsqlScanQuoteType quote);
extern void mys_psqlscan_test_variable(PsqlScanState state, const char *txt, int len);
/* End - ReqID:SRS-MISC-MYSQL-FRAME */

#endif							/* PSQLSCAN_INT_H */

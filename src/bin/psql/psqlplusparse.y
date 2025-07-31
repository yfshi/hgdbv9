%{
/*-------------------------------------------------------------------------
 *
 * File: psqlplusparse.y
 *
 * Abstract:
 *		bison grammar for a oracle-compatible client commands.
 *
 * Authored by zhenmingyang@highgo.com, 20221221.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/bin/psql/psqlplusparse.y
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "common/logging.h"
#include "fe_utils/psqlscan_int.h"
#include "oracle_fe_utils/ora_psqlscan.h"
#include "psqlplus.h"
#include "settings.h"

static psqlplus_cmd_var *make_variable_node(void);
static psqlplus_cmd_print *make_print_node(void);
static psqlplus_cmd_execute *make_exec_node(void);
/* Begin - ReqID:SRS-SQL-SYSFUNC */
static psqlplus_cmd_sv *make_set_node(void);
static psqlplus_cmd_sv *make_show_node(void);
static sys_var *make_system_variable(const char *name);
static sys_var_list *sv_list_make1(sys_var *sv);
static sys_var_list *sv_list_append(sys_var_list *list, sys_var *sv);
/* End - ReqID:SRS-SQL-SYSFUNC */
static BindVarType *make_bindvartype(int32 oid, int32 typmod, const char *typname);
static char *get_guc_settings(const char *gucname);
static print_item *make_print_item(char *name, bool valid, bool need_downcase/* BUG#Z351 */);
static print_list *make_print_list1(print_item *item);
static print_list *merge_print_list(print_list *list1, print_list *list2);

%}

%pure-parser
%expect 0
%name-prefix="psqlplus_yy"

%parse-param {yyscan_t yyscanner}
%lex-param   {yyscan_t yyscanner}

%union
{
	psql_YYSTYPE		psql_yysype;
	int					ival;
	char				*str;
	const char			*keyword;
	psqlplus_cmd		*psqlplus_cmd;
	BindVarType			*bindvartype;
	print_list			*print_item_list;
	/* Begin - ReqID:SRS-SQL-SYSFUNC */
	bool				boolean;
	sys_var				*system_variable;
	sys_var_list		*system_variable_list;
	/* End - ReqID:SRS-SQL-SYSFUNC */
}

%type <psqlplus_cmd> variable_stmt variable_spec print_stmt exec_stmt
%type <bindvartype> bind_vartype
%type <print_item_list> print_items_list print_items
%type <keyword> unreserved_keyword
%type <keyword> variable_keyword exec_keyword
%type <str> opt_varname bind_varname bind_varvalue bind_varvalue_sconst
%type <str> Sconst

%token <keyword>	K_VAR K_VARIABLE K_CHAR K_BYTE K_NCHAR K_VARCHAR2 K_NVARCHAR2
	K_NUMBER K_BINARY_FLOAT K_BINARY_DOUBLE K_PRINT K_EXECUTE K_EXEC K_REFCURSOR /* ReqID:SRS-PLSQL-CURSOR */
%token <str>	IDENT FCONST LITERAL DQCONST SQCONST
%token <ival>	ICONST
%token <str>	SINGLE_QUOTE_NO_END DOUBLE_QUOTE_NO_END
%token			TYPECAST

/* Begin - ReqID:SRS-SQL-SYSFUNC */
%type <boolean>					system_variable_on_off
%type <psqlplus_cmd>			system_variable_stmt
%type <system_variable>			set_system_variable
%type <system_variable_list>	set_system_variable_list show_system_variable_list
%token <keyword>				K_OFF K_ON K_SET K_SHOW
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
%type <keyword>		serveroutput_keyword
%token <keyword>	K_SERVEROUT K_SERVEROUTP K_SERVEROUTPU K_SERVEROUTPUT K_SIZE K_UNL K_UNLIMITED
%type <ival>		opt_serveroutput_size
%type <str>			show_system_variable
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

%%

psqlplus_toplevel_stmt:
		variable_stmt opt_semi
			{
				psql_yyget_extra(yyscanner)->psqlpluscmd = $1;
			}
		| print_stmt opt_semi
			{
				psql_yyget_extra(yyscanner)->psqlpluscmd = $1;
			}
		| exec_stmt opt_semi
			{
				psql_yyget_extra(yyscanner)->psqlpluscmd = $1;
			}
		/* Begin - ReqID:SRS-SQL-SYSFUNC */
		| system_variable_stmt opt_semi
			{
				psql_yyget_extra(yyscanner)->psqlpluscmd = $1;
			}
		/* End - ReqID:SRS-SQL-SYSFUNC */
	;

/*
 * Oracle SQL*PLUS VARIABLE statement.
 */
variable_stmt:
	variable_keyword variable_spec
		{
			$$ = $2;
		}
	| variable_keyword opt_varname truncate_char
		{
			/*
			 * The weird behavior of oracle, when encountering these characters 
			 * represented by $3, ignore these characters and the following text.
			 */
			PsqlScanState		state = psql_yyget_extra(yyscanner);
			psqlplus_cmd_var	*var = make_variable_node();

			if ($2)
				var->var_name = $2;

			var->list_bind_var = true;
			state->psqlpluscmd = (psqlplus_cmd *) var;
			return 0;
		}
	;

variable_keyword:
			K_VARIABLE
			| K_VAR
		;

variable_spec:
		bind_varname
			{
				/* list the specified bind variables */
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = $1;
				var->list_bind_var = true;
				$$ = (psqlplus_cmd *) var;
			}
		| bind_varname '=' bind_varvalue
			{
				/* bind variable assignment */
				PsqlScanState		state = psql_yyget_extra(yyscanner); /* BUG#Z352 */
				psqlplus_cmd_var	*var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = $1;
				/* Begin - BUG#Z352 */
				if (state->safe_encoding &&
					state->scanline[strlen(state->scanbuf)] != '\0' &&
					state->scanline[strlen(state->scanbuf)] != ' ' &&
					!(state->scanline[strlen(state->scanbuf)] == ';' &&
					 state->scanline[strlen(state->scanbuf) + 1] == '\0'))
				{
					int		i = 0;
					char	*val = pg_malloc0(strlen(state->scanline) - strlen(state->scanbuf) + 1);
					const char *ptr = state->scanline + strlen(state->scanbuf);

					while(ptr[i] != '\0' && ptr[i] != ' ')
					{
						if (ptr[i] == ';' && ptr[i + 1] == '\0')
							break;

						val[i] = ptr[i];
						i++;
					}
					val[i] = '\0';
					
					var->init_value = psprintf("%s%s", $3, val);
					pg_free(val);
				}
				else
				/* End - BUG#Z352 */
					var->init_value = $3;
				/* Begin - BUG#Z352 */
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
				/* End - BUG#Z352 */
			}
/* Begin - BUG#Z352 */
		| bind_varname '=' bind_varvalue_sconst
			{
				/* bind variable assignment */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var *var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = $1;
				var->init_value = $3;
				$$ = (psqlplus_cmd *) var;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
/* End - BUG#Z352 */
		| bind_varname '='
			{
				/* An error scenario, bind variable assignment without a value */
				psqlplus_cmd_var *var = make_variable_node();
				var->assign_bind_var = true;
				var->var_name = $1;
				$$ = (psqlplus_cmd *) var;
			}
		| bind_varname '=' SINGLE_QUOTE_NO_END
			{
				/*
				 * An error scenario
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->init_value = $3;
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
		| bind_varname '=' DOUBLE_QUOTE_NO_END
			{
				/*
				 * An error scenario
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->init_value = $3;
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
		| bind_varname bind_vartype
			{
				/* declare a bind variable */
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				var->list_bind_var = false;
				$$ = (psqlplus_cmd *) var;
			}
		| bind_varname bind_vartype '=' bind_varvalue
			{
				/* declare a bind variable with an initial value */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				/* Begin - BUG#Z352 */
				if (state->safe_encoding &&
					state->scanline[strlen(state->scanbuf)] != '\0' &&
					state->scanline[strlen(state->scanbuf)] != ' ' &&
					!(state->scanline[strlen(state->scanbuf)] == ';' &&
					 state->scanline[strlen(state->scanbuf) + 1] == '\0'))
				{
					int		i = 0;
					char	*val = pg_malloc0(strlen(state->scanline) - strlen(state->scanbuf) + 1);
					const char *ptr = state->scanline + strlen(state->scanbuf);

					while(ptr[i] != '\0' && ptr[i] != ' ')
					{
						if (ptr[i] == ';' && ptr[i + 1] == '\0')
							break;

						val[i] = ptr[i];
						i++;
					}
					val[i] = '\0';
					
					var->init_value = psprintf("%s%s", $4, val);
					pg_free(val);
				}
				else
				/* End - BUG#Z352 */
					var->init_value = $4;

				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
/* Begin - BUG#Z352 */
		| bind_varname bind_vartype '=' bind_varvalue_sconst
			{
				/* declare a bind variable with an initial value */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				var->init_value = $4;
				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
/* End - BUG#Z352 */

		| bind_varname bind_vartype '='
			{
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				var->init_value = NULL;
				var->list_bind_var = false;
				var->initial_nonnull_value = true;
				$$ = (psqlplus_cmd *) var;
			}
		| bind_varname bind_vartype '=' SINGLE_QUOTE_NO_END
			{
				/*
				 * An error scenario, implemented separately in 
				 * order to reduce interaction with the server.
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var *var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				var->init_value = $4;
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
		| bind_varname bind_vartype '=' DOUBLE_QUOTE_NO_END
			{
				/*
				 * An error scenario, implemented separately in 
				 * order to reduce interaction with the server.
				 */
				PsqlScanState		state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_var	*var = make_variable_node();
				var->var_name = $1;
				var->vartype = $2;
				var->init_value = $4;
				var->list_bind_var = false;
				var->miss_termination_quote = true;
				var->initial_nonnull_value = true;
				state->psqlpluscmd = (psqlplus_cmd *) var;
				return 0;
			}
		| /* empty */
			{
				/* list all bind variables */
				psqlplus_cmd_var *var = make_variable_node();
				var->list_bind_var = true;
				$$ = (psqlplus_cmd *) var;
			}
	;

opt_varname:
		bind_varname
			{  $$ = $1; }
		| /* empty */
			{  $$ = NULL; }
	;

bind_varname: IDENT			{ $$ = $1; }
		| unreserved_keyword	{ $$ = pg_strdup($1); }
	;

bind_varvalue:
		LITERAL				{ $$ = $1; }
		| IDENT					{ $$ = $1; }
		| '+' FCONST			{ $$ = $2; }
		| '-' FCONST			{ $$ = psprintf("-%s", $2); } /* itoa() is not portable */
		| FCONST				{ $$ = $1; }
		| '+' ICONST			{ $$ = psprintf("+%d", $2); }
		| '-' ICONST			{ $$ = psprintf("-%d", $2); }
		| ICONST				{ $$ = psprintf("%d", $1); }
		| unreserved_keyword	{ $$ = pg_strdup($1); }
	;

/* Begin - BUG#Z352 */
bind_varvalue_sconst:
		Sconst					{ $$ = $1; }
	;
/* End - BUG#Z352 */

/*
 * Interact with the server in plsq, call the typmodout function of the datatype
 * to verify the validity of the typmod input and obtain the correct typmod value,
 * but the VARIABLE client command only has the typmod value for the character type,
 * and we already know the header of character datatype is 4 byte, so we don't need
 * to sacrifice this performance consumption.
 */
bind_vartype:
		K_CHAR
			{
				$$ = make_bindvartype(ORACHARCHAROID, 1, $1);
			}
		| K_CHAR '(' ICONST ')'
			{
				if (pg_strcasecmp(get_guc_settings("nls_length_semantics"), "byte") == 0)
					$$ = make_bindvartype(ORACHARBYTEOID, $3, $1);
				else
					$$ = make_bindvartype(ORACHARCHAROID, $3, $1);
			}
		| K_CHAR '(' ICONST K_CHAR ')'
			{
				$$ = make_bindvartype(ORACHARCHAROID, $3, $1);
			}
		| K_CHAR '(' ICONST K_BYTE ')'
			{
				$$ = make_bindvartype(ORACHARBYTEOID, $3, $1);
			}
		| K_NCHAR
			{
				$$ = make_bindvartype(ORACHARCHAROID, 1, $1);
			}
		| K_NCHAR '(' ICONST ')'
			{
				$$ = make_bindvartype(ORACHARCHAROID, $3, $1);
			}
		| K_VARCHAR2
			{
				$$ = make_bindvartype(ORAVARCHARCHAROID, 1, $1);
			}
		| K_VARCHAR2 '(' ICONST ')'
			{
				if (pg_strcasecmp(get_guc_settings("nls_length_semantics"), "byte") == 0)
					$$ = make_bindvartype(ORAVARCHARBYTEOID, $3, $1);
				else
					$$ = make_bindvartype(ORAVARCHARCHAROID, $3, $1);
			}
		| K_VARCHAR2 '(' ICONST K_CHAR ')'
			{
				$$ = make_bindvartype(ORAVARCHARCHAROID, $3, $1);
			}
		| K_VARCHAR2 '(' ICONST K_BYTE ')'
			{
				$$ = make_bindvartype(ORAVARCHARBYTEOID, $3, $1);
			}
		| K_NVARCHAR2
			{
				$$ = make_bindvartype(ORAVARCHARCHAROID, 1, $1);
			}
		| K_NVARCHAR2 '(' ICONST ')'
			{
				$$ = make_bindvartype(ORAVARCHARCHAROID, $3, $1);
			}
		| K_NUMBER
			{
				$$ = make_bindvartype(NUMBEROID, -1, $1);
			}
		| K_BINARY_FLOAT
			{
				$$ = make_bindvartype(BINARY_FLOATOID, -1, $1);
			}
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		| K_REFCURSOR
			{
				$$ = make_bindvartype(SYS_REFCURSOROID, -1, $1);
			}
		/* End - ReqID:SRS-PLSQL-CURSOR */
		| K_BINARY_DOUBLE
			{
				$$ = make_bindvartype(BINARY_DOUBLEOID, -1, $1);
			}
	;

truncate_char:
		'('
		| ')'
		| ','
	;

/*
 * Oracle SQL*PLUS PRINT statement.
 */
print_stmt:
		K_PRINT
			{
				$$ = (psqlplus_cmd *) make_print_node();
			}
		| K_PRINT print_items_list
			{
				psqlplus_cmd_print *n = make_print_node();
				n->print_items = $2;
				$$ = (psqlplus_cmd *) n;
			}
	;

print_items_list:
	print_items
		{
			$$ = $1;
		}
	| print_items_list print_items
		{
			$$ = merge_print_list($1, $2);
		}
	;

print_items:
	IDENT
		{
			$$ = make_print_list1(make_print_item($1, true, false/* BUG#Z351 */));
		}
	| unreserved_keyword
		{
			$$ = make_print_list1(make_print_item(pg_strdup($1), true, false/* BUG#Z351 */));
		}
	| DQCONST
		{
			/*
			 * When PRINT specifies a bind variable name,
			 * it can be surrounded by double quotes but
			 * not single quotes.
			 */
			$$ = make_print_list1(make_print_item($1, true, true/* BUG#Z351 */));
		}
	| SQCONST
		{
			$$ = make_print_list1(make_print_item($1, false, true/* BUG#Z351 */));
		}
	;

/*
 * Oracle SQL*PLUS EXEC[UTE] statement.
 */
exec_stmt: exec_keyword
		{
			PsqlScanState state = psql_yyget_extra(yyscanner);

			if (state->is_sqlplus_cmd)
			{
				psqlplus_cmd_execute *exec = make_exec_node();
				int	offset = strlen(state->scanbuf);
				exec->plisqlstmts = pg_strdup(state->scanline + offset);
				state->psqlpluscmd = (psqlplus_cmd *) exec;
				return 0;
			}
			else
			{
				return 1;
			}
		}
	;

exec_keyword:
		K_EXECUTE
		| K_EXEC
	;

/* Begin - ReqID:SRS-SQL-SYSFUNC */
system_variable_stmt:
		K_SHOW show_system_variable_list
			{
				PsqlScanState state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_sv *show = make_show_node();
				show->sv_list = $2;
				state->psqlpluscmd = (psqlplus_cmd *) show;
				return 0;
			}
		| K_SET set_system_variable_list
			{
				PsqlScanState state = psql_yyget_extra(yyscanner);
				psqlplus_cmd_sv *set = make_set_node();
				set->sv_list = $2;
				state->psqlpluscmd = (psqlplus_cmd *) set;
				return 0;
			}
	;

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
show_system_variable:
		IDENT				{ $$ = $1; }
		| K_SERVEROUT		{ $$ = pg_strdup($1); }
		| K_SERVEROUTP		{ $$ = pg_strdup($1); }
		| K_SERVEROUTPU		{ $$ = pg_strdup($1); }
		| K_SERVEROUTPUT	{ $$ = pg_strdup($1); }
	;
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

show_system_variable_list:
		show_system_variable
			{
				$$ = sv_list_make1(make_system_variable($1));
			}
		| show_system_variable_list show_system_variable
			{
				$$ = sv_list_append($1, make_system_variable($2));
			}
	;

set_system_variable_list:
		set_system_variable
			{
				$$ = sv_list_make1($1);
			}
		| set_system_variable_list set_system_variable
			{
				$$ = sv_list_append($1, $2);
			}
	;

set_system_variable:
		IDENT system_variable_on_off
			{
				sys_var *sv = make_system_variable($1);
				sv->sv_type = SVT_BOOL;
				sv->sv_value.on = $2;
				$$ = sv;
			}
		| IDENT Sconst
			{
				sys_var *sv = make_system_variable($1);
				sv->sv_type = SVT_STRING;
				sv->sv_value.string = pg_strdup($2);
				$$ = sv;
			}
		| IDENT ICONST
			{
				sys_var *sv = make_system_variable($1);
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = $2;
				$$ = sv;
			}
		| IDENT '+' ICONST
			{
				sys_var *sv = make_system_variable($1);
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = $3;
				$$ = sv;
			}
		| IDENT '-' ICONST
			{
				sys_var *sv = make_system_variable($1);
				sv->sv_type = SVT_INTEGER;
				sv->sv_value.ival = -$3;
				$$ = sv;
			}
		/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
		| serveroutput_keyword system_variable_on_off opt_serveroutput_size
			{
				sys_var *sv = make_system_variable("serveroutput");
				csv_serveroutput_node	*data = (csv_serveroutput_node *) pg_malloc0(sizeof(csv_serveroutput_node));
				data->csv_tag = CSV_SERVEROUTPUT;
				data->enable = $2;
				data->size = $3;
				sv->sv_type = SVT_COMPOSITE;
				sv->sv_value.string = (char *)data;
				$$ = sv;
			}
		/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */
	;

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
serveroutput_keyword:
		K_SERVEROUT
		| K_SERVEROUTP
		| K_SERVEROUTPU
		| K_SERVEROUTPUT
	;

/*
 * Keep in sync with dbms_output.c, that is, -1=unlimited.
 *
 * Oracle say:
 * ON uses the SIZE and FORMAT of the previous SET SERVEROUTPUT ON SIZE n FORMAT f,
 * or uses default values if no SET SERVEROUTPUT command was previously issued in
 * the current connection.
 * therefore, we return -2 for the case where the size option is omitted, and -1 is
 * used to indicate unlimited.
 */
opt_serveroutput_size:
		K_SIZE ICONST			{ $$ = $2; }
		| K_SIZE K_UNL			{ $$ = SV_SERVEROUT_BUFSIZE_UNLIMITED; }
		| K_SIZE K_UNLIMITED	{ $$ = SV_SERVEROUT_BUFSIZE_UNLIMITED; }
		| /* empty */			{ $$ = SV_SERVEROUT_NO_SIZE_OPTION; }
	;
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

system_variable_on_off:
		K_ON				{ $$ = true; }
		| K_OFF				{ $$ = false; }
	;
/* End - ReqID:SRS-SQL-SYSFUNC */

/*
 * Single quotes and double quotes have different semantics 
 * in the PRINT command of SQL*PLUS ,  so the tokens of the 
 * quotes are separated separately.
 */
Sconst:
		SQCONST		{ $$ = $1; };
		| DQCONST	{ $$ = $1; };
	;

opt_semi:
		';'
		| /* empty */
	;

unreserved_keyword:
		K_BINARY_DOUBLE
		| K_BINARY_FLOAT
		| K_REFCURSOR	/* ReqID:SRS-PLSQL-CURSOR */
		| K_BYTE
		| K_CHAR
		| K_EXEC
		| K_EXECUTE
		| K_NCHAR
		| K_NUMBER
		| K_NVARCHAR2
		| K_PRINT
		| K_VAR
		| K_VARCHAR2
		| K_VARIABLE
	;

%%

static psqlplus_cmd_var *
make_variable_node(void)
{
	psqlplus_cmd_var *var = pg_malloc0(sizeof(psqlplus_cmd_var));
	var->cmd_type = PSQLPLUS_CMD_VARIABLE;
	var->var_name = NULL;
	var->vartype = NULL;
	var->init_value = NULL;
	var->miss_termination_quote = false;
	var->list_bind_var = false;
	var->assign_bind_var = false;
	var->initial_nonnull_value = false;
	return var;
}

static psqlplus_cmd_print *
make_print_node(void)
{
	psqlplus_cmd_print *print = pg_malloc0(sizeof(psqlplus_cmd_print));
	print->cmd_type = PSQLPLUS_CMD_PRINT;
	print->print_items = NULL;
	return print;
}

static psqlplus_cmd_execute *
make_exec_node(void)
{
	psqlplus_cmd_execute *exec = pg_malloc0(sizeof(psqlplus_cmd_execute));
	exec->cmd_type = PSQLPLUS_CMD_EXECUTE;
	exec->plisqlstmts = NULL;
	return exec;
}

/* Begin - ReqID:SRS-SQL-SYSFUNC */
static psqlplus_cmd_sv *
make_set_node(void)
{
	psqlplus_cmd_sv	*set = pg_malloc0(sizeof(psqlplus_cmd_sv));
	set->cmd_type = PSQLPLUS_CMD_SET;
	set->sv_list = NULL;
	return set;
}

static psqlplus_cmd_sv *
make_show_node(void)
{
	psqlplus_cmd_sv	*show = pg_malloc0(sizeof(psqlplus_cmd_sv));
	show->cmd_type = PSQLPLUS_CMD_SHOW;
	show->sv_list = NULL;
	return show;
}

static sys_var *
make_system_variable(const char *name)
{
	sys_var	*sv = (sys_var *) pg_malloc0(sizeof(sys_var));
	sv->sv_name = pg_strdup(name);
	sv->sv_type = SVT_NONE;
	sv->sv_value.string = NULL;
	return sv;
}

static sys_var_list *
sv_list_make1(sys_var *sv)
{
	sys_var_list	*list = pg_malloc0(sizeof(sys_var_list));
	list->sysvars = (sys_var **) pg_malloc0(sizeof(sys_var *));
	list->sysvars[0] = sv;
	list->length = 1;
	return list;
}

static sys_var_list *
sv_list_append(sys_var_list *list, sys_var *sv)
{
	if (list == NULL)
		return sv_list_make1(sv);

	list->sysvars = (sys_var **) pg_realloc(list->sysvars,
											sizeof(sys_var *) * (list->length+1));
	list->sysvars[list->length++] = sv;
	return list;
}
/* End - ReqID:SRS-SQL-SYSFUNC */

static BindVarType *
make_bindvartype(int32 oid, int32 typmod, const char *typname)
{
	BindVarType *vartype;

	/*
	 * We emulate what anychar_typmodin does here, avoiding the
	 * need to send queries to the server, typmod value need to
	 * add a 4-byte header length for character datatype.
	 */
	if (oid == ORACHARCHAROID ||
		oid == ORACHARBYTEOID ||
		oid == ORAVARCHARCHAROID ||
		oid == ORAVARCHARBYTEOID)
		typmod = typmod + 4;

	vartype = pg_malloc0(sizeof(BindVarType));
	vartype->name = pg_strdup(typname);
	vartype->oid = oid;
	vartype->typmod = typmod;

	return vartype;
}


/*
 * Get the value of the specified GUC.
 */
static char *
get_guc_settings(const char *gucname)
{
	PQExpBuffer query = createPQExpBuffer();
	PGresult	*res;
	char		*val = NULL;

	appendPQExpBuffer(query, "SELECT setting from pg_settings where name = '%s';", gucname);
	res = PQexec(pset.db, query->data);

	if (PQresultStatus(res) == PGRES_TUPLES_OK &&
		PQntuples(res) == 1 &&
		!PQgetisnull(res, 0, 0))
	{		
		val = pg_strdup(PQgetvalue(res, 0, 0));
	}

	PQclear(res);
	destroyPQExpBuffer(query);

	return val;
}

static print_item *
make_print_item(char *name, bool valid, bool need_downcase/* BUG#Z351 */)
{
	print_item *item = pg_malloc0(sizeof(print_item));

	/* Begin - BUG#Z351 */
	/*
	 * When using PRINT to print bind variables in oracle, the semantics
	 * of double quotes is to deal with the problem that the bind variable
	 * has the same name as the keyword.  The name of the bind variable is
	 * case-insensitive and we save it in lowercase,  so downcase needs to
	 * be performed.
	 */
	if (need_downcase)
		item->bv_name = psqlplus_downcase_identifier(name, strlen(name));
	else
	/* End - BUG#Z351 */
		item->bv_name = name;

	item->valid = valid;

	return item;
}

static print_list *
make_print_list1(print_item *item)
{
	print_list	*pl = pg_malloc0(sizeof(print_list));
	pl->items = (print_item **) pg_malloc0(sizeof(print_item *) * 1);
	pl->items[0] = item;
	pl->length = 1;

	return pl;
}

static print_list *
merge_print_list(print_list *list1, print_list *list2)
{
	int	newlen;
	int i;

	if (list1 == NULL)
		return list2;
	if (list2 == NULL)
		return list1;

	newlen = list1->length + list2->length;
	list1->items = (print_item **) pg_realloc(list1->items, sizeof(print_item *) * newlen);

	for (i = 0; i < list2->length; i++)
	{
		list1->items[list1->length] = list2->items[i];
		list1->length++;
	}

	return list1;
}

int
psqlplus_yylex(YYSTYPE *lvalp, yyscan_t yyscanner)
{
	return orapsql_yylex(&(lvalp->psql_yysype), yyscanner);
}

void
psqlplus_yyerror(yyscan_t yyscanner, const char *message)
{
	/* do nothing */
}

/* yylex and the yylval macro, which flex will have its own definition */
#undef yylval
#undef yylex	/* BUG:M0000080 */

#include "psqlplusscan.c"

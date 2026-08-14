/*-------------------------------------------------------------------------
 *
 * parse_param.h
 *	  handle parameters in parser
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_param.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_PARAM_H
#define PARSE_PARAM_H

#include "parser/parse_node.h"


/* Begin - BUG#M0000264 */
extern PGDLLIMPORT List *ora_parameter_list;
extern PGDLLIMPORT List *ora_parameter_name_list;
/* End - BUG#M0000264 */


extern void parse_fixed_parameters(ParseState *pstate,
								   Oid *paramTypes, int numParams);
extern void parse_variable_parameters(ParseState *pstate,
									  Oid **paramTypes, int *numParams);
extern void parse_variable_parameters_ext(ParseState *pstate,
										  Oid **paramTypes, int *numParams,
										  /* Begin - ReqID:SRS-SQL-FUNCOUT */
										  char **paramModes);
										  /* End - ReqID:SRS-SQL-FUNCOUT */
extern void check_variable_parameters(ParseState *pstate, Query *query);
extern bool query_contains_extern_params(Query *query);
/* Begin - ReqID:SRS-SQL-FUNCOUT */
/*
 * type oid (32bits), 2 highest bits; if the first bit is 1,
 * it means OUT mode, if the second bit is 1, it means IN modes.
 * the remaining 30 bits represent the type value.
 */
#define IsModeOut(typeoid) ((typeoid) & 0x80000000)
#define SetModeOut(typeoid) ((typeoid) |= 0x80000000)
#define IsModeIn(typeoid) ((typeoid) & 0x40000000)
#define SetModeIn(typeoid) ((typeoid) |= 0x40000000)
#define UnSetModeOut(typeoid) ((typeoid) &= 0x3FFFFFFF)

/* Begin - BUG#M0000376 */
#define PROARGMODE_NOT_SET		' '
/* End - BUG#M0000376 */

extern Node *ParseParamVariable(Node *arg);
extern char *GetParamMode(Oid *paramTypes, int numParams); /* BUG:M0000083 */
extern void ParseVarParamState(void *arg, Node *param,
							bool *firstOutIsReturn, bool proc_call);
/* End - ReqID:SRS-SQL-FUNCOUT */
/* Begin - ReqID:SRS-SQL-ANONYOUT */
extern int push_oraparam_stack(void);
extern void pop_oraparam_stack(int cur); /* BUG#M0000264 */

extern  int calculate_oraparamnumber(const char* name);
/* Begin - BUG#M0000316 */
extern int pl_calculate_questionmarknumber(void);
extern int calculate_questionmarknumber(void);
extern void reset_oraparam_curpos(void);
/* End - BUG#M0000316 */
/* Begin - BUG#M0000264 */
extern void recalculate_oraparamnumber(List* parsetree_list);
extern int get_current_oraparam_level(void);
extern int recalculate_oraparamname_position(char ***paramnames,
										/* Begin - BUG#M0000376 */
										char **parammodes);
										/* End - BUG#M0000376 */
/* End - BUG#M0000264 */
extern void setdynamic_parser(bool value);
extern bool getdynamic_parser(void);
extern void check_variables_does_match(int variables);
extern void setdynamic_haspgparam(bool value);
extern void setdynamic_docheckvar(bool values);
/* Begin - BUG#M0000316 */
extern void setdynamic_hasquestionmark(bool value);
extern bool getdynamic_doparser(void);
/* End - BUG#M0000316 */
extern bool getdynmaic_docheckvar(void);
extern void setdynamic_doparser(bool value);
extern void set_bindbyname(bool value);
extern bool get_bindbyname(void);
/* End - ReqID:SRS-SQL-ANONYOUT */
extern void setdynamic_callparser(bool value); /* ReqID:SRS-SQL-CALL */
/* Begin - BUG#M0000376 */
extern void set_oraparam_mode(char *refname, char setmode);
/* End - BUG#M0000376 */
#endif							/* PARSE_PARAM_H */

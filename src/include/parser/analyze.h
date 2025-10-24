/*-------------------------------------------------------------------------
 *
 * analyze.h
 *		parse analysis for optimizable statements
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/analyze.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ANALYZE_H
#define ANALYZE_H

#include "parser/parse_node.h"
#include "utils/queryjumble.h"

/* Hook for plugins to get control at end of parse analysis */
typedef void (*post_parse_analyze_hook_type) (ParseState *pstate,
											  Query *query,
											  JumbleState *jstate);
extern PGDLLIMPORT post_parse_analyze_hook_type post_parse_analyze_hook;


extern Query *parse_analyze(RawStmt *parseTree, const char *sourceText,
							Oid *paramTypes, int numParams, QueryEnvironment *queryEnv);
extern Query *parse_analyze_varparams(RawStmt *parseTree, const char *sourceText,
									  Oid **paramTypes, int *numParams,
									  /* Begin - ReqID:SRS-SQL-FUNCOUT */
									  char **paramModes);
									  /* End - ReqID:SRS-SQL-FUNCOUT */
extern Query *parse_sub_analyze(Node *parseTree, ParseState *parentParseState,
								CommonTableExpr *parentCTE,
								bool locked_from_parent,
								bool resolve_unknowns);

extern List *transformInsertRow(ParseState *pstate, List *exprlist,
								List *stmtcols, List *icolumns, List *attrnos,
								bool strip_indirection);
extern List *transformUpdateTargetList(ParseState *pstate,
									   List *targetList);
extern List *transformMultiUpdateTargetList(ParseState *pstate,
									   List *origTlist, List* resultRelations);
extern Query *transformTopLevelStmt(ParseState *pstate, RawStmt *parseTree);
extern Query *transformStmt(ParseState *pstate, Node *parseTree);

extern bool stmt_requires_parse_analysis(RawStmt *parseTree);
extern bool analyze_requires_snapshot(RawStmt *parseTree);

extern const char *LCS_asString(LockClauseStrength strength);
extern void CheckSelectLocking(Query *qry, LockClauseStrength strength);
extern void applyLockingClause(Query *qry, Index rtindex,
							   LockClauseStrength strength,
							   LockWaitPolicy waitPolicy,
							   int32 waitInteger,/* BUG#M0000470: FOR UPDATE WAIT integer */
							   List *lockedColumns,/* BUG#M0000470: FOR UPDATE WAIT integer */
							   bool pushedDown);

extern List *BuildOnConflictExcludedTargetlist(Relation targetrel,
											   Index exclRelIndex);

extern SortGroupClause *makeSortGroupClauseForSetOp(Oid rescoltype, bool require_hash);

/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
extern List *copy_enum_label(Oid enumOid);
/* End - ReqID:SRS-MYSQL-ENUMTYPE */

/* Begin - ReqID:SRS-MYSQL-SET */
extern VariableSetStmt *trans_MysVariable_to_VariableSet(ParseState *pstate, MysVariableSetStmt *stmt);
extern Node *ivy_makeStringConst(char *str, int location);
extern Node *ivy_makeIntConst(int val, int location);
extern Node *ivy_makeFloatConst(char *str, int location);
extern Node *ivy_makeAConst(Value *v, int location);
extern Node *ivy_makeNullAConst(int location);
extern Node *ivy_makeTypeCast(Node *arg, TypeName *typename, int location);
extern Node *ivy_makeBoolAConst(bool state);
extern Node *transformVarAssign(ParseState *pstate, VarAssign *var, bool returnquery);
/* End - ReqID:SRS-MYSQL-SET */
/* Begin - ReqID:SRS-SECURITY-DATADESENSITIZATION */
typedef void (*redact_parse_analyze_hook_type) (ParseState *pstate,
											  Query *query);
extern PGDLLIMPORT redact_parse_analyze_hook_type redact_parse_analyze_hook;
/* End - ReqID:SRS-SECURITY-DATADESENSITIZATION */

#endif							/* ANALYZE_H */

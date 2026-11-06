/*
 * TECC前端解析器节点实现 - 简化版本
 * 只保留必要的make*Stmt函数，移除与tecc_common/tecc_frontend_list.c冲突的重复工具函数
 * 实现详细设计文档第4章要求的数据结构
 */

#include "tecc_nodes.h"
#include "tecc_error.h"
#include "../tecc_common/tecc_frontend_list.h"
#include <string.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <execinfo.h>
#endif

/* ==================== 语句节点创建函数 ==================== */

SelectStmt *makeSelectStmt(int stmt_location, int stmt_len) {
    SelectStmt *stmt = (SelectStmt *)tecc_palloc(sizeof(SelectStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_SelectStmt;
    stmt->distinctClause = NIL;
    stmt->intoClause = NULL;
    stmt->targetList = NIL;
    stmt->valuesLists = NIL;  /* 初始化valuesLists字段 */
    stmt->defaultValues = false;  /* 初始化defaultValues字段 */
    stmt->fromClause = NIL;
    stmt->whereClause = NULL;
    stmt->groupClause = NIL;
    stmt->groupDistinct = false;
    stmt->havingClause = NULL;
    stmt->windowClause = NIL;
    stmt->sortClause = NIL;
    stmt->limitOffset = NULL;
    stmt->limitCount = NULL;
    stmt->lockingClause = NIL;
    stmt->cteList = NIL;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

InsertStmt *makeInsertStmt(int stmt_location, int stmt_len) {
    InsertStmt *stmt = (InsertStmt *)tecc_palloc(sizeof(InsertStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_InsertStmt;
    stmt->relation = NULL;
    stmt->cols = NIL;
    stmt->selectStmt = NULL;
    stmt->onConflictClause = NIL;
    stmt->returningList = NIL;
    stmt->withClause = NIL;
    stmt->override = 0;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

UpdateStmt *makeUpdateStmt(int stmt_location, int stmt_len) {
    UpdateStmt *stmt = (UpdateStmt *)tecc_palloc(sizeof(UpdateStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_UpdateStmt;
    stmt->relation = NULL;
    stmt->targetList = NIL;
    stmt->whereClause = NULL;
    stmt->fromClause = NIL;
    stmt->returningList = NIL;
    stmt->withClause = NIL;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

DeleteStmt *makeDeleteStmt(int stmt_location, int stmt_len) {
    DeleteStmt *stmt = (DeleteStmt *)tecc_palloc(sizeof(DeleteStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_DeleteStmt;
    stmt->relation = NULL;
    stmt->whereClause = NULL;
    stmt->usingClause = NIL;
    stmt->returningList = NIL;
    stmt->withClause = NIL;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

CreateKekStmt *makeCreateKekStmt(int stmt_location, int stmt_len) {
    CreateKekStmt *stmt = (CreateKekStmt *)tecc_palloc(sizeof(CreateKekStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_CreateKekStmt;
    stmt->names = NIL;
    stmt->options = NIL;
    stmt->if_not_exists = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

CreateDekStmt *makeCreateDekStmt(int stmt_location, int stmt_len) {
    CreateDekStmt *stmt = (CreateDekStmt *)tecc_palloc(sizeof(CreateDekStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_CreateDekStmt;
    stmt->names = NIL;
    stmt->options = NIL;
    stmt->if_not_exists = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

DropStmt *makeDropStmt(int stmt_location, int stmt_len) {
    DropStmt *stmt = (DropStmt *)tecc_palloc(sizeof(DropStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_DropStmt;
    stmt->objects = NIL;
    stmt->arguments = NIL;
    stmt->removeType = 0;
    stmt->behavior = 0;  /* DROP_RESTRICT */
    stmt->missing_ok = false;
    stmt->concurrent = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

CreateStmt *makeCreateStmt(int stmt_location, int stmt_len) {
    CreateStmt *stmt = (CreateStmt *)tecc_palloc(sizeof(CreateStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_CreateStmt;
    stmt->relation = NULL;
    stmt->tableElts = NIL;
    stmt->options = NIL;
    stmt->inhRelations = NIL;
    stmt->partbound = NULL;
    stmt->partspec = NIL;
    stmt->ofTypename = NULL;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

AlterTableStmt *makeAlterTableStmt(int stmt_location, int stmt_len) {
    AlterTableStmt *stmt = (AlterTableStmt *)tecc_palloc(sizeof(AlterTableStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_AlterTableStmt;
    stmt->relation = NULL;
    stmt->cmds = NIL;
    stmt->relkind = OBJECT_TABLE;
    stmt->missing_ok = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

AlterTableCmd *makeAlterTableCmd(AlterTableType subtype, Node *def, char *name, DropBehavior behavior, bool missing_ok, int location) {
    AlterTableCmd *cmd = (AlterTableCmd *)tecc_palloc(sizeof(AlterTableCmd));
    if (!cmd) return NULL;

    cmd->type = FE_T_AlterTableCmd;
    cmd->subtype = subtype;
    cmd->def = def;
    cmd->name = name ? tecc_pstrdup(name) : NULL;
    cmd->behavior = behavior;
    cmd->missing_ok = missing_ok;
    cmd->location = location;

    return cmd;
}

/* ==================== 列定义节点创建函数 ==================== */

ColumnDef *makeColumnDef(int location, int len) {
    ColumnDef *col = (ColumnDef *)tecc_palloc(sizeof(ColumnDef));
    if (!col) return NULL;

    col->type = FE_T_ColumnDef;
    col->colname = NULL;
    col->typeName = NULL;
    col->constraints = NIL;
    col->collClause = NULL;
    col->location = location;
    /* Note: Additional fields may need initialization based on ColumnDef structure */

    return col;
}

/* ==================== 类型名节点创建函数 ==================== */

TypeName *makeTypeName(const char *name, int location, int length) {
    TypeName *node = (TypeName *)tecc_palloc(sizeof(TypeName));
    if (!node) return NULL;

    node->type = FE_T_TypeName;
    node->names = list_make1(makeStringConst(name, location, 0));
    node->typemod = -1;  /* 默认无类型修饰符 */
    node->arrayBounds = NIL;
    node->setof = false;
    node->pct_type = false;
    node->location = location;
    node->encryptedOpts = NIL;

    return node;
}

TypeName *makeTypeNameFromList(List *names, int location, int length) {
    TypeName *node = (TypeName *)tecc_palloc(sizeof(TypeName));
    if (!node) return NULL;

    node->type = FE_T_TypeName;
    node->names = names;
    node->typemod = -1;
    node->arrayBounds = NIL;
    node->setof = false;
    node->pct_type = false;
    node->location = location;
    node->encryptedOpts = NIL;

    return node;
}

TypeName *makeTypeNameWithTypmod(const char *name, int typmod, int location, int length) {
    TypeName *node = makeTypeName(name, location, length);
    if (!node) return NULL;

    node->typemod = typmod;
    return node;
}

TypeName *makeTypeNameWithTypmod2(const char *name, int typmod1, int typmod2, int location, int length) {
    /* 简化处理：只使用第一个typmod */
    TypeName *node = makeTypeName(name, location, length);
    if (!node) return NULL;

    node->typemod = typmod1;
    return node;
}

TypeName *makeEncryptedTypeName(const char *name, List *options, int location, int length) {
    TypeName *node = makeTypeName(name, location, length);
    if (!node) return NULL;

    node->encryptedOpts = options;

    return node;
}

TypeName *makeEncryptedTypeNameWithTypmod(const char *name, int typmod, List *options, int location, int length) {
    TypeName *node = makeTypeNameWithTypmod(name, typmod, location, length);
    if (!node) return NULL;

    node->encryptedOpts = options;

    return node;
}

TypeName *makeEncryptedTypeNameWithTypmod2(const char *name, int typmod1, int typmod2, List *options, int location, int length) {
    TypeName *node = makeTypeNameWithTypmod2(name, typmod1, typmod2, location, length);
    if (!node) return NULL;

    node->encryptedOpts = options;

    return node;
}

DefElem *makeDefElem(char *defname, Node *arg, int location) {
    DefElem *node = (DefElem *)tecc_palloc(sizeof(DefElem));
    if (!node) return NULL;

    node->type = FE_T_DefElem;
    node->defnamespace = NULL;
    node->defname = tecc_pstrdup(defname);
    node->arg = arg;
    node->defaction = 0; /* 默认动作 */
    node->location = location;

    return node;
}

/* ==================== 排序规则子句创建函数 ==================== */

Node *makeCollateClause(Node *arg, List *collname, int location) {
    /* ReqID:TECC4.0 - Create CollateClause node for COLLATE expressions */
    CollateClause *node = (CollateClause *)tecc_palloc(sizeof(CollateClause));
    if (!node)
        return NULL;
    node->type = FE_T_CollateClause;
    node->arg = arg;
    node->collname = collname;
    node->location = location;
    return (Node *)node;
}

/* ==================== 约束创建函数 ==================== */

Node *makeConstraint(int constr_type, int location, int length) {
    /* 简化实现：创建空约束节点 */
    (void)constr_type;
    (void)location;
    (void)length;

    /* 返回空节点 */
    return (Node *)makeStringConst("constraint", location, 0);
}

Node *makeConstraintWithExpr(int constr_type, Node *expr, int location, int length) {
    Constraint *n = (Constraint *)tecc_palloc(sizeof(Constraint));
    if (!n) return NULL;

    n->type = FE_T_Constraint;
    n->contype = constr_type;
    n->conname = NULL;
    n->keys = NIL;
    n->raw_expr = expr;
    n->fk_attrs = NIL;
    n->pk_attrs = NIL;
    n->pktable = NULL;
    n->location = location;
    n->length = length;

    return (Node *)n;
}

/* ==================== 表达式节点创建函数 ==================== */

ColumnRef *makeColumnRef(List *fields, int location) {
    ColumnRef *node = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
    if (!node) return NULL;

    node->type = FE_T_ColumnRef;
    node->fields = fields;
    node->location = location;

    return node;
}

FuncCall *makeFuncCall(List *funcname, List *args, int location) {
    FuncCall *node = (FuncCall *)tecc_palloc(sizeof(FuncCall));
    if (!node) return NULL;

    node->type = FE_T_FuncCall;
    node->funcname = funcname;
    node->args = args;
    node->agg_order = NIL;
    node->agg_filter = NULL;
    node->agg_star = false;
    node->agg_distinct = false;
    node->func_variadic = false;
	node->agg_within_group = false;
    node->over = NULL;
    node->location = location;

    return node;
}

A_Expr *makeAExpr(int kind, List *name, Node *lexpr, Node *rexpr, int location) {
    A_Expr *node = (A_Expr *)tecc_palloc(sizeof(A_Expr));
    if (!node) return NULL;

    node->type = FE_T_A_Expr;
    node->kind = kind;
    node->name = name;
    node->lexpr = lexpr;
    node->rexpr = rexpr;
    node->escape = NULL;
    node->location = location;

    return node;
}

A_Expr *makeAExprWithEscape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location) {
    A_Expr *node = (A_Expr *)tecc_palloc(sizeof(A_Expr));
    if (!node) return NULL;

    node->type = FE_T_A_Expr;
    node->kind = kind;
    node->name = name;
    node->lexpr = lexpr;
    node->rexpr = rexpr;
    node->escape = escape;
    node->location = location;

    return node;
}

/* ReqID:TECC4.0 - RowExpr constructor */
RowExpr *makeRowExpr(List *args, int location) {
    RowExpr *node = (RowExpr *)tecc_palloc(sizeof(RowExpr));
    if (!node) return NULL;

    node->type = FE_T_RowExpr;
    node->args = args;
    node->row_format = 0;
    node->location = location;

    return node;
}

BoolExpr *makeBoolExpr(int boolop, List *args, int location) {
    BoolExpr *node = (BoolExpr *)tecc_palloc(sizeof(BoolExpr));
    if (!node) return NULL;

    node->type = FE_T_BoolExpr;
    node->boolop = boolop;
    node->args = args;
    node->location = location;

    return node;
}

NullTest *makeNullTest(Node *arg, int nulltesttype, int location) {
    NullTest *node = (NullTest *)tecc_palloc(sizeof(NullTest));
    if (!node) return NULL;

    node->type = FE_T_NullTest;
    node->arg = arg;
    node->nulltesttype = nulltesttype;
    node->location = location;

    return node;
}

/* ReqID:TECC4.0 - BooleanTest constructor (IS TRUE/FALSE/UNKNOWN) */
BooleanTest *makeBooleanTest(Node *arg, int booltesttype, int location) {
    BooleanTest *node = (BooleanTest *)tecc_palloc(sizeof(BooleanTest));
    if (!node) return NULL;

    node->type = FE_T_BooleanTest;
    node->arg = arg;
    node->booltesttype = booltesttype;
    node->location = location;

    return node;
}

/* ReqID:TECC4.0 - TimeZoneExpr constructor */
TimeZoneExpr *makeTimeZoneExpr(Node *arg, Node *timezone, int location) {
    TimeZoneExpr *node = (TimeZoneExpr *)tecc_palloc(sizeof(TimeZoneExpr));
    if (!node) return NULL;

    node->type = FE_T_TimeZoneExpr;
    node->arg = arg;
    node->timezone = timezone;
    node->location = location;

    return node;
}

/* ReqID:TECC4.0 - SetToDefault constructor */
SetToDefault *makeSetToDefault(int location) {
    SetToDefault *node = (SetToDefault *)tecc_palloc(sizeof(SetToDefault));
    if (!node) return NULL;

    node->type = FE_T_SetToDefault;
    node->location = location;

    return node;
}

/* ReqID:TECC4.0 - makeA_Indices: create array subscript or slice node */
A_Indices *makeA_Indices(bool is_slice, Node *lidx, Node *uidx) {
    A_Indices *node = (A_Indices *)tecc_palloc(sizeof(A_Indices));
    if (!node) return NULL;

    node->type = FE_T_A_Indices;
    node->is_slice = is_slice;
    node->lidx = lidx;
    node->uidx = uidx;

    return node;
}

/* ReqID:TECC4.0 - makeA_Indirection: create indirection expression node */
A_Indirection *makeA_Indirection(Node *arg, List *indirection) {
    A_Indirection *node = (A_Indirection *)tecc_palloc(sizeof(A_Indirection));
    if (!node) return NULL;

    node->type = FE_T_A_Indirection;
    node->arg = arg;
    node->indirection = indirection;

    return node;
}

/* ReqID:TECC4.0 - makeA_Star: create A_Star node for '.*' indirection */
A_Star *makeA_Star(void) {
    A_Star *node = (A_Star *)tecc_palloc(sizeof(A_Star));
    if (!node) return NULL;

    node->type = FE_T_A_Star;

    return node;
}

Param *makeParam(int paramkind, int paramid, int paramtype, int location) {
    Param *node = (Param *)tecc_palloc(sizeof(Param));
    if (!node) return NULL;

    node->type = FE_T_Param;
    node->paramkind = paramkind;
    node->paramid = paramid;
    node->paramtype = paramtype;
    node->location = location;

    return node;
}

Const *makeConst(int consttype, bool constisnull, int location, int data_size) {
    Const *node = (Const *)tecc_palloc(sizeof(Const));
    if (!node) return NULL;

    node->type = FE_T_Const;
    node->consttype = consttype;
    node->consttypmod = -1;
    node->constcollid = 0;
    node->constlen = -1;
    node->constbyval = false;
    node->constisnull = constisnull;
    node->location = location;
    node->data_size = data_size;
    memset(&node->constvalue, 0, sizeof(node->constvalue));

    return node;
}

Var *makeVar(int varno, int varattno, int vartype, int location) {
    Var *node = (Var *)tecc_palloc(sizeof(Var));
    if (!node) return NULL;

    node->type = FE_T_Var;
    node->varno = varno;
    node->varattno = varattno;
    node->vartype = vartype;
    node->vartypmod = -1;
    node->varcollid = 0;
    node->varlevelsup = 0;
    node->varnoold = varno;
    node->varoattno = varattno;
    node->location = location;

    return node;
}

/* ==================== 其他节点创建函数 ==================== */

ResTarget *makeResTarget(char *name, Node *val, int location) {
    ResTarget *node = (ResTarget *)tecc_palloc(sizeof(ResTarget));
    if (!node) return NULL;

    node->type = FE_T_ResTarget;
    node->name = name ? tecc_pstrdup(name) : NULL;
    node->indirection = NIL;
    node->val = val;
    node->location = location;

    return node;
}

RangeVar *makeRangeVar(char *schemaname, char *relname, int location) {
    RangeVar *node = (RangeVar *)tecc_palloc(sizeof(RangeVar));
    if (!node) return NULL;

    node->type = FE_T_RangeVar;
    node->catalogname = NULL;
    node->schemaname = schemaname ? tecc_pstrdup(schemaname) : NULL;
    node->relname = relname ? tecc_pstrdup(relname) : NULL;
    node->inh = true;
    node->relpersistence = 'p';  /* permanent table */
    node->alias = NULL;
    node->location = location;

    return node;
}

RangeVar *makeRangeVarFromQualifiedName(const char *qualified_name, int location, int length) {
    char *schemaname = NULL;
    char *relname = NULL;
    char *copy;
    char *dot;
    RangeVar *rv;

    copy = tecc_pstrdup(qualified_name);
    dot = strchr(copy, '.');

    if (dot) {
        *dot = '\0';
        schemaname = copy;
        relname = dot + 1;
    } else {
        relname = copy;
    }

    rv = makeRangeVar(schemaname, relname, location);
    tecc_pfree(copy);
    return rv;
}

Alias *makeAlias(char *aliasname, List *colnames) {
    Alias *node = (Alias *)tecc_palloc(sizeof(Alias));
    if (!node) return NULL;

    node->type = FE_T_Alias;
    node->aliasname = aliasname ? tecc_pstrdup(aliasname) : NULL;
    node->colnames = colnames;

    return node;
}


/* ==================== 集合操作节点创建函数 ==================== */

SetOperationStmt *makeSetOperationStmt(int op, bool all, Node *larg, Node *rarg, int location, int len) {
    SetOperationStmt *stmt = (SetOperationStmt *)tecc_palloc(sizeof(SetOperationStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_SetOp;
    stmt->op = op;
    stmt->all = all;
    stmt->larg = larg;
    stmt->rarg = rarg;
    stmt->colTypes = NIL;
    stmt->colTypmods = NIL;
    stmt->colCollations = NIL;
    stmt->groupClauses = NIL;
    stmt->stmt_location = location;
    stmt->stmt_len = len;

    return stmt;
}

/* ==================== JOIN表达式节点创建函数 ==================== */

JoinExpr *makeJoinExpr(int jointype, Node *larg, Node *rarg, Node *quals, int location) {
    JoinExpr *node = (JoinExpr *)tecc_palloc(sizeof(JoinExpr));
    if (!node) return NULL;

    node->type = FE_T_JoinExpr;
    node->jointype = jointype;
    node->larg = larg;
    node->rarg = rarg;
    node->quals = quals;
    node->usingClause = NIL;
    node->isNatural = false;
    node->location = location;

    return node;
}

/* ==================== FROM子句子查询节点创建函数 ==================== */

RangeSubselect *makeRangeSubselect(Node *subquery, Alias *alias, int location) {
    RangeSubselect *node = (RangeSubselect *)tecc_palloc(sizeof(RangeSubselect));
    if (!node) return NULL;

    node->type = FE_T_RangeSubselect;
    node->subquery = subquery;
    node->alias = alias;
    node->lateral = false;
    node->location = location;

    return node;
}

/* FROM子句函数调用节点创建函数 */
RangeFunction *makeRangeFunction(Node *funccall, Alias *alias, bool lateral, int location) {
    RangeFunction *node = (RangeFunction *)tecc_palloc(sizeof(RangeFunction));
    if (!node) return NULL;

    node->type = FE_T_RangeFunction;
    node->funccall = funccall;
    node->alias = alias;
    node->lateral = lateral;
    node->location = location;

    return node;
}

/* ==================== CTE (Common Table Expression) 节点创建函数 ==================== */

CommonTableExpr *makeCommonTableExpr(char *ctename, List *aliascolnames, Node *ctequery, int location)
{
    CommonTableExpr *cte = (CommonTableExpr *)tecc_palloc(sizeof(CommonTableExpr));
    if (!cte) return NULL;

    cte->type = FE_T_CommonTableExpr;
    cte->ctename = ctename ? tecc_pstrdup(ctename) : NULL;
    cte->aliascolnames = aliascolnames;
    cte->ctequery = ctequery;
    cte->recursive = false;
    cte->location = location;

    return cte;
}

/* ReqID:TECC4.0 - Set recursive flag on all CTEs in a cte_list */
void set_cte_list_recursive(List *cte_list)
{
    ListCell *lc;
    if (cte_list == NIL)
        return;
    foreach(lc, cte_list)
    {
        CommonTableExpr *cte = (CommonTableExpr *) lfirst(lc);
        if (cte != NULL)
            cte->recursive = true;
    }
}

/* ==================== SubLink (子查询表达式) 节点创建函数 ==================== */

SubLink *makeSubLink(int subLinkType, Node *testexpr, List *operName, Node *subselect, int location)
{
    SubLink *sl = (SubLink *)tecc_palloc(sizeof(SubLink));
    if (!sl) return NULL;

    sl->type = FE_T_SubLink;
    sl->subLinkType = subLinkType;
    sl->testexpr = testexpr;
    sl->operName = operName;
    sl->subselect = subselect;
    sl->location = location;

    return sl;
}

WindowDef *makeWindowDef(char *name, char *refname, List *partitionClause, List *orderClause, int location)
{
    WindowDef *wd = (WindowDef *)tecc_palloc(sizeof(WindowDef));
    if (!wd) return NULL;

    wd->type = FE_T_WindowDef;
    wd->name = name ? tecc_pstrdup(name) : NULL;
    wd->refname = refname ? tecc_pstrdup(refname) : NULL;
    wd->partitionClause = partitionClause;
    wd->orderClause = orderClause;
    wd->location = location;

    return wd;
}

/* ==================== CASE表达式节点创建函数 ==================== */

CaseExpr *makeCaseExpr(Node *arg, Node *defresult, List *args, int location) {
    CaseExpr *node = (CaseExpr *)tecc_palloc(sizeof(CaseExpr));
    if (!node) return NULL;

    node->type = FE_T_CaseExpr;
    node->arg = arg;
    node->defresult = defresult;
    node->args = args;
    node->location = location;

    return node;
}

CaseWhen *makeCaseWhen(Node *expr, Node *result, int location) {
    CaseWhen *node = (CaseWhen *)tecc_palloc(sizeof(CaseWhen));
    if (!node) return NULL;

    node->type = FE_T_CaseWhen;
    node->expr = expr;
    node->result = result;
    node->location = location;

    return node;
}

/* ==================== INSERT ON CONFLICT 节点创建函数 ==================== */

OnConflictClause *makeOnConflictClause(int action, int location) {
    OnConflictClause *node = (OnConflictClause *)tecc_palloc(sizeof(OnConflictClause));
    if (!node) return NULL;

    node->type = FE_T_OnConflictClause;
    node->action = action;
    node->infer = NULL;
    node->targetList = NIL;
    node->whereClause = NULL;
    node->location = location;

    return node;
}

InferClause *makeInferClause(List *indexElems, Node *whereClause, const char *conname, int location) {
    InferClause *node = (InferClause *)tecc_palloc(sizeof(InferClause));
    if (!node) return NULL;

    node->type = FE_T_InferClause;
    node->indexElems = indexElems;
    node->whereClause = whereClause;
    node->conname = conname ? tecc_pstrdup(conname) : NULL;
    node->location = location;

    return node;
}


/*
 * Deduplicate shared val pointers in a target list to prevent double-free.
 * Multi-column SET creates multiple ResTargets sharing the same val pointer
 * (e.g., SET (a,b) = (subquery) shares the same SubLink). This function
 * NULLs out all-but-the-first occurrence of each unique val pointer so that
 * list_free_node_list only frees each unique node once. ReqID:TECC4.0
 */
static void
dedup_res_target_shared_vals(List *targetList)
{
    ListCell *lc1;
    if (targetList == NULL) return;
    for (lc1 = targetList->head; lc1 != NULL; lc1 = lc1->next)
    {
        ResTarget *rt1 = (ResTarget *)lc1->data.ptr_value;
        ListCell *lc2;
        if (rt1 == NULL || rt1->val == NULL) continue;
        for (lc2 = lc1->next; lc2 != NULL; lc2 = lc2->next)
        {
            ResTarget *rt2 = (ResTarget *)lc2->data.ptr_value;
            if (rt2 != NULL && rt2->val == rt1->val)
                rt2->val = NULL;
        }
    }
}

void freeNode(Node *node) {
    if (!node) {
        return;
    }

    /* 根据节点类型调用具体的释放函数 */
    switch (node->type) {
        case FE_T_SelectStmt:
            freeSelectStmt((SelectStmt *)node);
            /* freeSelectStmt已经调用了tecc_pfree，这里不需要再操作已释放的内存 */
            break;
        case FE_T_InsertStmt:
            freeInsertStmt((InsertStmt *)node);
            break;
        case FE_T_UpdateStmt:
            freeUpdateStmt((UpdateStmt *)node);
            break;
        case FE_T_DeleteStmt:
            freeDeleteStmt((DeleteStmt *)node);
            break;
        case FE_T_CreateKekStmt:
            freeCreateKekStmt((CreateKekStmt *)node);
            break;
        case FE_T_CreateDekStmt:
            freeCreateDekStmt((CreateDekStmt *)node);
            break;
        case FE_T_DropStmt:
            freeDropStmt((DropStmt *)node);
            break;
        case FE_T_CreateStmt:
            freeCreateStmt((CreateStmt *)node);
            break;
        case FE_T_PrepareStmt:
            freePrepareStmt((PrepareStmt *)node);
            break;
        case FE_T_ExecuteStmt:
            freeExecuteStmt((ExecuteStmt *)node);
            break;
        case FE_T_DeclareCursorStmt:
            freeDeclareCursorStmt((DeclareCursorStmt *)node);
            break;
        case FE_T_DeallocateStmt:
            freeDeallocateStmt((DeallocateStmt *)node);
            break;
        case FE_T_CreateTableAsStmt:
            freeCreateTableAsStmt((CreateTableAsStmt *)node);
            break;
        case FE_T_CopyStmt:
            freeCopyStmt((CopyStmt *)node);
            break;
        case FE_T_ExplainStmt:
            {
                ExplainStmt *explain = (ExplainStmt *)node;
                if (explain->query)
                    freeNode(explain->query);
                tecc_pfree(node);
            }
            break;
        case FE_T_CommonTableExpr:
            {
                CommonTableExpr *cte = (CommonTableExpr *)node;
                if (cte->ctename)
                    tecc_pfree(cte->ctename);
                if (cte->aliascolnames)
                    list_free_deep(cte->aliascolnames);
                if (cte->ctequery)
                    freeNode(cte->ctequery);
                tecc_pfree(node);
            }
            break;
        case FE_T_SubLink:
            {
                SubLink *sl = (SubLink *)node;
                if (sl->testexpr)
                    freeNode(sl->testexpr);
                if (sl->operName)
                    list_free_deep(sl->operName);
                if (sl->subselect)
                    freeNode(sl->subselect);
                tecc_pfree(node);
            }
            break;
        case FE_T_WindowDef:
            {
                WindowDef *wd = (WindowDef *)node;
                if (wd->name)
                    tecc_pfree(wd->name);
                if (wd->refname)
                    tecc_pfree(wd->refname);
                if (wd->partitionClause)
                    list_free_node_list(wd->partitionClause);
                if (wd->orderClause)
                    list_free_node_list(wd->orderClause);
                tecc_pfree(node);
            }
            break;
        case FE_T_String:
            /* Value节点需要释放字符串 */
            {
                Value *value = (Value *)node;
                if (value->val.str) {
                    tecc_pfree(value->val.str);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_ColumnRef:
            {
                ColumnRef *colref = (ColumnRef *)node;
                if (colref->fields) {
                    list_free_deep(colref->fields);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_FuncCall:
            {
                FuncCall *funccall = (FuncCall *)node;
                if (funccall->funcname) {
                    list_free_deep(funccall->funcname);
                }
                if (funccall->args) {
                    list_free_node_list(funccall->args);
                }
                if (funccall->agg_order) {
                    list_free_node_list(funccall->agg_order);
                }
                if (funccall->agg_filter) {
                    freeNode(funccall->agg_filter);
                }
                if (funccall->over) {
                    freeNode((Node *)(funccall->over));
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_ResTarget:
            {
                ResTarget *restarget = (ResTarget *)node;
                if (restarget->name) {
                    tecc_pfree(restarget->name);
                }
                if (restarget->val) {
                    freeNode(restarget->val);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_RangeVar:
            {
                RangeVar *rangevar = (RangeVar *)node;
                if (rangevar->schemaname) {
                    tecc_pfree(rangevar->schemaname);
                }
                if (rangevar->relname) {
                    tecc_pfree(rangevar->relname);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_DefElem:
            {
                DefElem *defelem = (DefElem *)node;
                if (defelem->defname) {
                    tecc_pfree(defelem->defname);
                }
                if (defelem->arg) {
                    freeNode(defelem->arg);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_SetOp:
            {
                SetOperationStmt *setop = (SetOperationStmt *)node;
                if (setop->larg) freeNode(setop->larg);
                if (setop->rarg) freeNode(setop->rarg);
                if (setop->colTypes) list_free_deep(setop->colTypes);
                if (setop->colTypmods) list_free_deep(setop->colTypmods);
                if (setop->colCollations) list_free_deep(setop->colCollations);
                if (setop->groupClauses) list_free_node_list(setop->groupClauses);
                tecc_pfree(node);
            }
            break;
        case FE_T_JoinExpr:
            {
                JoinExpr *join = (JoinExpr *)node;
                if (join->larg) freeNode(join->larg);
                if (join->rarg) freeNode(join->rarg);
                if (join->quals) freeNode(join->quals);
                tecc_pfree(node);
            }
            break;
        case FE_T_RangeSubselect:
            {
                RangeSubselect *rs = (RangeSubselect *)node;
                if (rs->subquery) freeNode(rs->subquery);
                if (rs->alias) {
                    if (rs->alias->aliasname) tecc_pfree(rs->alias->aliasname);
                    if (rs->alias->colnames) list_free_deep(rs->alias->colnames);
                    tecc_pfree(rs->alias);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_RangeFunction:
            {
                RangeFunction *rf = (RangeFunction *)node;
                if (rf->funccall) freeNode(rf->funccall);
                if (rf->alias) {
                    if (rf->alias->aliasname) tecc_pfree(rf->alias->aliasname);
                    if (rf->alias->colnames) list_free_deep(rf->alias->colnames);
                    tecc_pfree(rf->alias);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_CaseExpr:
            {
                CaseExpr *c = (CaseExpr *)node;
                if (c->arg) freeNode(c->arg);
                if (c->defresult) freeNode(c->defresult);
                if (c->args) list_free_node_list(c->args);
                tecc_pfree(node);
            }
            break;
        case FE_T_CaseWhen:
            {
                CaseWhen *cw = (CaseWhen *)node;
                if (cw->expr) freeNode(cw->expr);
                if (cw->result) freeNode(cw->result);
                tecc_pfree(node);
            }
            break;
        case FE_T_OnConflictClause:
            {
                OnConflictClause *occ = (OnConflictClause *)node;
                if (occ->infer) freeNode(occ->infer);
                /* Prevent double-free from multi-column SET sharing SubLink. ReqID:TECC4.0 */
                dedup_res_target_shared_vals(occ->targetList);
                if (occ->targetList) list_free_node_list(occ->targetList);
                if (occ->whereClause) freeNode(occ->whereClause);
                tecc_pfree(node);
            }
            break;
        case FE_T_InferClause:
            {
                InferClause *infer = (InferClause *)node;
                if (infer->indexElems) list_free_node_list(infer->indexElems);
                if (infer->whereClause) freeNode(infer->whereClause);
                if (infer->conname) tecc_pfree(infer->conname);
                tecc_pfree(node);
            }
            break;
        case FE_T_List:
            {
                /* 多语句List：递归释放所有子节点，然后释放List结构本身。ReqID:TECC4.0 */
                List *list = (List *)node;
                ListCell *lc;
                ListCell *next;
                for (lc = list->head; lc != NULL; lc = next)
                {
                    next = lc->next;
                    if (lc->data.ptr_value != NULL)
                        freeNode((Node *)lc->data.ptr_value);
                    tecc_pfree(lc);
                }
                tecc_pfree(node);
            }
            break;
        case FE_T_PartitionBoundSpec:
            freePartitionBoundSpec((PartitionBoundSpec *)node);
            break;
        case FE_T_PartitionRangeDatum:
            freePartitionRangeDatum((PartitionRangeDatum *)node);
            break;
        default:
            /* 默认只释放节点本身 */
            tecc_pfree(node);
            break;
    }
}

void freeSelectStmt(SelectStmt *stmt) {
    if (!stmt) return;

    if (stmt->distinctClause) list_free_node_list(stmt->distinctClause);
    if (stmt->targetList) list_free_node_list(stmt->targetList);
    if (stmt->fromClause) list_free_node_list(stmt->fromClause);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->groupClause) list_free_node_list(stmt->groupClause);
    if (stmt->havingClause) freeNode(stmt->havingClause);
    if (stmt->windowClause) list_free_node_list(stmt->windowClause);
    if (stmt->sortClause) list_free_node_list(stmt->sortClause);
    if (stmt->limitOffset) freeNode(stmt->limitOffset);
    if (stmt->limitCount) freeNode(stmt->limitCount);
    if (stmt->lockingClause) list_free_node_list(stmt->lockingClause);
    if (stmt->cteList) list_free_node_list(stmt->cteList);

    tecc_pfree(stmt);
}

void freeInsertStmt(InsertStmt *stmt) {
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->cols) list_free_node_list(stmt->cols);
    if (stmt->selectStmt) freeNode(stmt->selectStmt);
    if (stmt->onConflictClause) list_free_node_list(stmt->onConflictClause);
    if (stmt->returningList) list_free_node_list(stmt->returningList);
    if (stmt->withClause) list_free_node_list(stmt->withClause);

    tecc_pfree(stmt);
}

void freeUpdateStmt(UpdateStmt *stmt) {
    if (!stmt) return;

    /* Prevent double-free from multi-column SET sharing SubLink. ReqID:TECC4.0 */
    dedup_res_target_shared_vals(stmt->targetList);
    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->targetList) list_free_node_list(stmt->targetList);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->fromClause) list_free_node_list(stmt->fromClause);
    if (stmt->returningList) list_free_node_list(stmt->returningList);
    if (stmt->withClause) list_free_node_list(stmt->withClause);

    tecc_pfree(stmt);
}

void freeDeleteStmt(DeleteStmt *stmt) {
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->usingClause) list_free_node_list(stmt->usingClause);
    if (stmt->returningList) list_free_node_list(stmt->returningList);
    if (stmt->withClause) list_free_node_list(stmt->withClause);

    tecc_pfree(stmt);
}

void freeCreateKekStmt(CreateKekStmt *stmt) {
    if (!stmt) return;

    if (stmt->names) list_free_deep(stmt->names);
    if (stmt->options) list_free_node_list(stmt->options);

    tecc_pfree(stmt);
}

void freeCreateDekStmt(CreateDekStmt *stmt) {
    if (!stmt) return;

    if (stmt->names) list_free_deep(stmt->names);
    if (stmt->options) list_free_node_list(stmt->options);

    tecc_pfree(stmt);
}

void freeDropStmt(DropStmt *stmt) {
    if (!stmt) return;

    if (stmt->objects) list_free_deep(stmt->objects);
    if (stmt->arguments) list_free_deep(stmt->arguments);

    tecc_pfree(stmt);
}

void freeCreateStmt(CreateStmt *stmt) {
    if (!stmt) {
        return;
    }

    if (stmt->relation) {
        freeNode((Node *)stmt->relation);
    }
    if (stmt->tableElts) {
        list_free_node_list(stmt->tableElts);
    }
    if (stmt->options) {
        list_free_node_list(stmt->options);
    }
    if (stmt->inhRelations) {
        list_free_deep(stmt->inhRelations);
    }
    if (stmt->partbound) {
        freeNode(stmt->partbound);
    }
    if (stmt->partspec) {
        list_free_node_list(stmt->partspec);
    }
    if (stmt->ofTypename) {
        freeNode((Node *)stmt->ofTypename);
    }

    tecc_pfree(stmt);
}

/* ==================== 节点复制函数 ==================== */

Node *tecc_copy_node(const Node *node) {
    if (!node) return NULL;

    /* 简化的节点复制实现 */
    /* 注意：这不是完整的深度复制，只复制基础结构 */
    switch (node->type) {
        case FE_T_SelectStmt: {
            SelectStmt *src = (SelectStmt *)node;
            SelectStmt *dst = (SelectStmt *)tecc_palloc(sizeof(SelectStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(SelectStmt));
            return (Node *)dst;
        }
        case FE_T_InsertStmt: {
            InsertStmt *src = (InsertStmt *)node;
            InsertStmt *dst = (InsertStmt *)tecc_palloc(sizeof(InsertStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(InsertStmt));
            return (Node *)dst;
        }
        case FE_T_UpdateStmt: {
            UpdateStmt *src = (UpdateStmt *)node;
            UpdateStmt *dst = (UpdateStmt *)tecc_palloc(sizeof(UpdateStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(UpdateStmt));
            return (Node *)dst;
        }
        case FE_T_DeleteStmt: {
            DeleteStmt *src = (DeleteStmt *)node;
            DeleteStmt *dst = (DeleteStmt *)tecc_palloc(sizeof(DeleteStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DeleteStmt));
            return (Node *)dst;
        }
        case FE_T_CreateKekStmt: {
            CreateKekStmt *src = (CreateKekStmt *)node;
            CreateKekStmt *dst = (CreateKekStmt *)tecc_palloc(sizeof(CreateKekStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CreateKekStmt));
            return (Node *)dst;
        }
        case FE_T_CreateDekStmt: {
            CreateDekStmt *src = (CreateDekStmt *)node;
            CreateDekStmt *dst = (CreateDekStmt *)tecc_palloc(sizeof(CreateDekStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CreateDekStmt));
            return (Node *)dst;
        }
        case FE_T_DropStmt: {
            DropStmt *src = (DropStmt *)node;
            DropStmt *dst = (DropStmt *)tecc_palloc(sizeof(DropStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DropStmt));
            return (Node *)dst;
        }
        case FE_T_PrepareStmt: {
            PrepareStmt *src = (PrepareStmt *)node;
            PrepareStmt *dst = (PrepareStmt *)tecc_palloc(sizeof(PrepareStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(PrepareStmt));
            return (Node *)dst;
        }
        case FE_T_ExecuteStmt: {
            ExecuteStmt *src = (ExecuteStmt *)node;
            ExecuteStmt *dst = (ExecuteStmt *)tecc_palloc(sizeof(ExecuteStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(ExecuteStmt));
            return (Node *)dst;
        }
        case FE_T_DeclareCursorStmt: {
            DeclareCursorStmt *src = (DeclareCursorStmt *)node;
            DeclareCursorStmt *dst = (DeclareCursorStmt *)tecc_palloc(sizeof(DeclareCursorStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DeclareCursorStmt));
            return (Node *)dst;
        }
        case FE_T_DeallocateStmt: {
            DeallocateStmt *src = (DeallocateStmt *)node;
            DeallocateStmt *dst = (DeallocateStmt *)tecc_palloc(sizeof(DeallocateStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DeallocateStmt));
            return (Node *)dst;
        }
        case FE_T_CreateTableAsStmt: {
            CreateTableAsStmt *src = (CreateTableAsStmt *)node;
            CreateTableAsStmt *dst = (CreateTableAsStmt *)tecc_palloc(sizeof(CreateTableAsStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CreateTableAsStmt));
            return (Node *)dst;
        }
        case FE_T_CopyStmt: {
            CopyStmt *src = (CopyStmt *)node;
            CopyStmt *dst = (CopyStmt *)tecc_palloc(sizeof(CopyStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CopyStmt));
            return (Node *)dst;
        }
        case FE_T_SubLink: {
            SubLink *src = (SubLink *)node;
            SubLink *dst = (SubLink *)tecc_palloc(sizeof(SubLink));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(SubLink));
            return (Node *)dst;
        }
        case FE_T_CommonTableExpr: {
            CommonTableExpr *src = (CommonTableExpr *)node;
            CommonTableExpr *dst = (CommonTableExpr *)tecc_palloc(sizeof(CommonTableExpr));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CommonTableExpr));
            return (Node *)dst;
        }
        case FE_T_WindowDef: {
            WindowDef *src = (WindowDef *)node;
            WindowDef *dst = (WindowDef *)tecc_palloc(sizeof(WindowDef));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(WindowDef));
            return (Node *)dst;
        }
        default:
        {
            /* 对于其他节点类型，只复制基础Node结构 */
            Node *copy = (Node *)tecc_palloc(sizeof(Node));
            if (!copy) return NULL;
            copy->type = node->type;
            return copy;
        }
    }
}

/* ==================== 节点释放函数（别名） ==================== */

void tecc_free_node(Node *node) {
    freeNode(node);
}

/* ==================== Prepared statement node creation functions ==================== */

PrepareStmt *makePrepareStmt(char *name, List *argtypes, Node *query, int location)
{
    PrepareStmt *stmt = (PrepareStmt *)tecc_palloc(sizeof(PrepareStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_PrepareStmt;
    stmt->location = location;
    stmt->length = sizeof(PrepareStmt);
    stmt->name = tecc_pstrdup(name);
    stmt->argtypes = argtypes;
    stmt->query = query;

    return stmt;
}

ExecuteStmt *makeExecuteStmt(char *name, List *params, int nparams, Oid *paramtypes,
                            const char *const *paramValues, int *paramLengths, int *paramFormats, int location)
{
    ExecuteStmt *stmt;
    char **temp_values;
    int i;

    stmt = (ExecuteStmt *)tecc_palloc(sizeof(ExecuteStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_ExecuteStmt;
    stmt->location = location;
    stmt->length = sizeof(ExecuteStmt);
    stmt->name = tecc_pstrdup(name);
    stmt->params = params;
    stmt->nparams = nparams;

    /* 复制参数类型数组 */
    if (nparams > 0 && paramtypes) {
        stmt->paramtypes = (Oid *)tecc_palloc(nparams * sizeof(Oid));
        if (stmt->paramtypes) {
            memcpy(stmt->paramtypes, paramtypes, nparams * sizeof(Oid));
        } else {
            stmt->nparams = 0;
        }
    } else {
        stmt->paramtypes = NULL;
    }

    /* 复制参数值数组 */
    if (nparams > 0 && paramValues) {
        /* 使用临时数组，因为const char *const *不能直接赋值 */
        temp_values = (char **)tecc_palloc(nparams * sizeof(char *));
        if (temp_values) {
            for (i = 0; i < nparams; i++) {
                if (paramValues[i]) {
                    temp_values[i] = tecc_pstrdup(paramValues[i]);
                } else {
                    temp_values[i] = NULL;
                }
            }
            /* 转换为const char *const * */
            stmt->paramValues = (const char *const *)temp_values;
        } else {
            stmt->nparams = 0;
            stmt->paramValues = NULL;
        }
    } else {
        stmt->paramValues = NULL;
    }

    /* 复制参数长度数组 */
    if (nparams > 0 && paramLengths) {
        stmt->paramLengths = (int *)tecc_palloc(nparams * sizeof(int));
        if (stmt->paramLengths) {
            memcpy(stmt->paramLengths, paramLengths, nparams * sizeof(int));
        } else {
            stmt->nparams = 0;
        }
    } else {
        stmt->paramLengths = NULL;
    }

    /* 复制参数格式数组 */
    if (nparams > 0 && paramFormats) {
        stmt->paramFormats = (int *)tecc_palloc(nparams * sizeof(int));
        if (stmt->paramFormats) {
            memcpy(stmt->paramFormats, paramFormats, nparams * sizeof(int));
        } else {
            stmt->nparams = 0;
        }
    } else {
        stmt->paramFormats = NULL;
    }

    return stmt;
}

/* ==================== Prepared statement node free functions ==================== */

void freePrepareStmt(PrepareStmt *stmt)
{
    if (!stmt) return;

    if (stmt->name) tecc_pfree(stmt->name);
    if (stmt->argtypes) list_free_deep(stmt->argtypes);
    if (stmt->query) freeNode(stmt->query);

    tecc_pfree(stmt);
}

void freeExecuteStmt(ExecuteStmt *stmt)
{
    if (!stmt) return;

    if (stmt->name) tecc_pfree(stmt->name);
    if (stmt->params) list_free_deep(stmt->params);

    if (stmt->paramtypes) tecc_pfree(stmt->paramtypes);

    if (stmt->paramValues) {
        int i;
        for (i = 0; i < stmt->nparams; i++) {
            if (stmt->paramValues[i]) {
                tecc_pfree((void *)stmt->paramValues[i]);
            }
        }
        tecc_pfree((const char **)stmt->paramValues);
    }

    if (stmt->paramLengths) tecc_pfree(stmt->paramLengths);
    if (stmt->paramFormats) tecc_pfree(stmt->paramFormats);

    tecc_pfree(stmt);
}

/* ==================== DEALLOCATE statement node functions ==================== */

DeallocateStmt *makeDeallocateStmt(char *name, int location)
{
    DeallocateStmt *stmt;

    stmt = (DeallocateStmt *)tecc_palloc(sizeof(DeallocateStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_DeallocateStmt;
    stmt->location = location;
    stmt->length = sizeof(DeallocateStmt);
    stmt->name = name ? tecc_pstrdup(name) : NULL;

    return stmt;
}

void freeDeallocateStmt(DeallocateStmt *stmt)
{
    if (!stmt) return;

    if (stmt->name) tecc_pfree(stmt->name);

    tecc_pfree(stmt);
}

CreateTableAsStmt *makeCreateTableAsStmt(RangeVar *relation, Node *query, bool if_not_exists, int location)
{
    CreateTableAsStmt *stmt;

    stmt = (CreateTableAsStmt *)tecc_palloc(sizeof(CreateTableAsStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_CreateTableAsStmt;
    stmt->location = location;
    stmt->length = sizeof(CreateTableAsStmt);
    stmt->relation = relation;
    stmt->query = query;
    stmt->if_not_exists = if_not_exists;

    return stmt;
}

void freeCreateTableAsStmt(CreateTableAsStmt *stmt)
{
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->query) freeNode(stmt->query);

    tecc_pfree(stmt);
}

/* ==================== DECLARE CURSOR node functions ==================== */

DeclareCursorStmt *makeDeclareCursorStmt(char *portalname, Node *query, int location)
{
    DeclareCursorStmt *stmt = (DeclareCursorStmt *)tecc_palloc(sizeof(DeclareCursorStmt));
    if (!stmt) return NULL;

    stmt->type = FE_T_DeclareCursorStmt;
    stmt->location = location;
    stmt->length = sizeof(DeclareCursorStmt);
    stmt->portalname = tecc_pstrdup(portalname);
    stmt->query = query;

    return stmt;
}

void freeDeclareCursorStmt(DeclareCursorStmt *stmt)
{
    if (!stmt) return;

    if (stmt->portalname) tecc_pfree(stmt->portalname);
    if (stmt->query) freeNode(stmt->query);

    tecc_pfree(stmt);
}

/* ==================== Alias node free function ==================== */

static void freeAlias(Alias *alias)
{
    if (!alias) return;

    if (alias->aliasname) tecc_pfree(alias->aliasname);
    if (alias->colnames) list_free_deep(alias->colnames);

    tecc_pfree(alias);
}

/* ==================== RangeVar node free function ==================== */

static void freeRangeVar(RangeVar *rangevar)
{
    if (!rangevar) return;

    if (rangevar->catalogname) tecc_pfree(rangevar->catalogname);
    if (rangevar->schemaname) tecc_pfree(rangevar->schemaname);
    if (rangevar->relname) tecc_pfree(rangevar->relname);
    if (rangevar->alias) freeAlias(rangevar->alias);

    tecc_pfree(rangevar);
}

/* ==================== COPY statement node creation function ==================== */

CopyStmt *
makeCopyStmt(RangeVar *relation, Node *query, List *attlist, bool is_from,
            char *filename, List *options,
            int stmt_location, int stmt_len)
{
    CopyStmt *stmt;

    /* 参数验证 */
    /* 1. relation和query应该互斥：不能同时指定表和SELECT查询 */
    if (relation != NULL && query != NULL) {
        tecc_error_report_simple(PARSER_COPY_RELATION_QUERY_CONFLICT_ERROR, stmt_location);
        return NULL;
    }

    /* 2. 必须指定relation或query之一 */
    if (relation == NULL && query == NULL) {
        tecc_error_report_simple(PARSER_COPY_NO_RELATION_OR_QUERY_ERROR, stmt_location);
        return NULL;
    }

    /* 3. COPY FROM不支持SELECT查询 */
    if (is_from && query != NULL) {
        tecc_error_report_simple(PARSER_COPY_FROM_SELECT_NOT_SUPPORTED_ERROR, stmt_location);
        return NULL;
    }

    /* 4. 加密列只支持STDIN/STDOUT，不支持文件 */
    if (filename != NULL) {
        tecc_error_report_simple(PARSER_COPY_FILE_NOT_SUPPORTED_ERROR, stmt_location);
        return NULL;
    }

    stmt = (CopyStmt *)tecc_palloc(sizeof(CopyStmt));
    if (!stmt) {
        tecc_error_report_simple(PARSER_MEMORY_ERROR, stmt_location);
        return NULL;
    }

    stmt->type = FE_T_CopyStmt;
    stmt->relation = relation;
    stmt->query = query;
    stmt->attlist = attlist;
    stmt->is_from = is_from;
    stmt->filename = NULL;  /* 强制为NULL，加密列只支持STDIN/STDOUT */
    stmt->options = options;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

/* ==================== COPY statement node free function ==================== */

void freeCopyStmt(CopyStmt *stmt)
{
    if (!stmt) return;

    if (stmt->relation) freeRangeVar(stmt->relation);
    if (stmt->query) freeNode(stmt->query);
    if (stmt->attlist) list_free_deep(stmt->attlist);
    if (stmt->filename) tecc_pfree(stmt->filename);
    if (stmt->options) list_free_deep(stmt->options);

    tecc_pfree(stmt);
}

/* ==================== List operation functions ==================== */

ListCell *list_head(const List *list)
{
    if (!list) return NULL;
    return list->head;
}

ListCell *lnext(const ListCell *cell)
{
    if (!cell) return NULL;
    return cell->next;
}

/* ==================== Constant creation functions ==================== */

Node *makeIntConst(int64_t val, int location, int data_size)
{
    int consttype;
    int constlen;
    Const *node;

    /* ReqID:TECC4.0 - Choose int type based on value range.
     * makeIntConst is used for ALL integer literals in the TECC frontend
     * grammar. Previously it always assigned INT4OID(23), causing any
     * value outside INT4 range to be mistyped. This fix maps:
     *   [-32768, 32767]   -> INT2OID(21)
     *   [-2^31, 2^31-1]   -> INT4OID(23)
     *   otherwise          -> INT8OID(20) */
    if (val >= INT16_MIN && val <= INT16_MAX)
    {
        consttype = 21;  /* INT2OID */
        constlen = (int)sizeof(int16_t);
    }
    else if (val >= INT32_MIN && val <= INT32_MAX)
    {
        consttype = 23;  /* INT4OID */
        constlen = (int)sizeof(int32_t);
    }
    else
    {
        consttype = 20;  /* INT8OID */
        constlen = (int)sizeof(int64_t);
    }

    node = makeConst(consttype, false, location, data_size);
    if (!node) return NULL;

    node->constvalue.ival = val;
    node->constlen = constlen;
    node->constbyval = true;

    return (Node *)node;
}

Node *makeFloatConst(double val, int location, int data_size)
{
    Const *node = makeConst(701 /* FLOAT8OID */, false, location, data_size);
    if (!node) return NULL;

    node->constvalue.dval = val;
    node->constlen = sizeof(float8);
    node->constbyval = true;

    return (Node *)node;
}

Node *makeNullConst(int location, int data_size)
{
    Const *node = makeConst(705 /* UNKNOWNOID */, true, location, data_size);
    if (!node) return NULL;

    return (Node *)node;
}

Node *makeStringConst(const char *str, int location, int data_size)
{
    Const *node = makeConst(25 /* TEXTOID */, false, location, data_size);
    if (!node) return NULL;

    if (str) {
        node->constvalue.sval = tecc_pstrdup(str);
        node->constlen = -1;  /* 变长字符串 */
        node->constbyval = false;
    } else {
        node->constisnull = true;
    }

    return (Node *)node;
}

/* 值提取函数 */
int intVal(const Value *node)
{
    /* 简化实现：假设节点是整数常量 */
    if (!node) return 0;

    /* 检查节点类型 */
    if (node->type == FE_T_Const) {
        Const *const_node = (Const *)node;
        if (!const_node->constisnull) {
            return (int)const_node->constvalue.ival;
        }
    }

    /* FE_T_Integer: 直接从Value结构体中提取整数值 */
    if (node->type == FE_T_Integer) {
        return node->val.ival;
    }

    return 0;
}

char *strVal(const Value *node)
{
    /* 简化实现：假设节点是字符串常量 */
    if (!node) return NULL;

    /* 检查节点类型 - 需要同时支持FE_T_Const和FE_T_String */
    if (node->type == FE_T_Const) {
        Const *const_node = (Const *)node;
        if (!const_node->constisnull && const_node->constvalue.sval) {
            return const_node->constvalue.sval;
        }
    }

    /* FE_T_String: 直接从Value结构体中提取字符串 */
    if (node->type == FE_T_String) {
        return node->val.str;
    }

    return NULL;
}

double floatVal(const Value *node)
{
    /* 简化实现：假设节点是浮点数常量 */
    if (!node) return 0.0;

    /* 检查节点类型 */
    if (node->type == FE_T_Const) {
        Const *const_node = (Const *)node;
        if (!const_node->constisnull) {
            return const_node->constvalue.dval;
        }
    }

    return 0.0;
}

bool boolVal(const Value *node)
{
    /* 简化实现：假设节点是布尔常量 */
    if (!node) return false;

    /* 检查节点类型 */
    if (node->type == FE_T_Const) {
        Const *const_node = (Const *)node;
        if (!const_node->constisnull) {
            return const_node->constvalue.ival != 0;
        }
    }

    return false;
}

/* ==================== Constraint creation functions (stubs) ==================== */

Node *makeForeignKeyConstraint(List *pktable, List *fk_attrs, List *pk_attrs, int location, int length)
{
    /* Stub implementation - return a simple node */
    Node *node = (Node *)tecc_palloc(sizeof(Node));
    if (!node) return NULL;

    node->type = FE_T_Constraint;  /* Use generic type */
    return node;
}

Node *setConstraintName(Node *constraint, const char *name, int location, int length)
{
    /* Stub implementation - just return the constraint unchanged */
    return constraint;
}

Node *makeConstraintWithColumns(int constr_type, List *columns, int location, int length)
{
    /* Stub implementation - return a simple node */
    Node *node = (Node *)tecc_palloc(sizeof(Node));
    if (!node) return NULL;

    node->type = FE_T_Constraint;  /* Use generic type */
    return node;
}


/* ==================== 分区相关节点创建和释放函数 ==================== */

PartitionBoundSpec *makePartitionBoundSpec(char strategy, bool is_default, int location)
{
    PartitionBoundSpec *spec = (PartitionBoundSpec *)tecc_palloc(sizeof(PartitionBoundSpec));
    if (!spec) return NULL;
    spec->type = FE_T_PartitionBoundSpec;
    spec->strategy = strategy;
    spec->is_default = is_default;
    spec->location = location;
    spec->listdatums = NIL;
    spec->lowerdatums = NIL;
    spec->upperdatums = NIL;
    return spec;
}

PartitionRangeDatum *makePartitionRangeDatum(Node *value, int location)
{
    PartitionRangeDatum *datum = (PartitionRangeDatum *)tecc_palloc(sizeof(PartitionRangeDatum));
    if (!datum) return NULL;
    datum->type = FE_T_PartitionRangeDatum;
    datum->location = location;
    datum->value = value;
    return datum;
}

void freePartitionBoundSpec(PartitionBoundSpec *spec)
{
    if (!spec) return;
    if (spec->listdatums) list_free_node_list(spec->listdatums);
    if (spec->lowerdatums) list_free_node_list(spec->lowerdatums);
    if (spec->upperdatums) list_free_node_list(spec->upperdatums);
    tecc_pfree(spec);
}

void freePartitionRangeDatum(PartitionRangeDatum *datum)
{
    if (!datum) return;
    if (datum->value) freeNode(datum->value);
    tecc_pfree(datum);
}
/* ==================== List utility functions ==================== */

bool list_is_empty(const List *list)
{
    return (list == NULL || list->head == NULL);
}

/* 释放节点列表 - 使用freeNode释放每个节点 */
void list_free_node_list(List *list) {
    ListCell *cell;
    ListCell *next;

    if (!list) return;

    cell = list->head;
    while (cell) {
        next = cell->next;
        /* 释放节点数据 */
        if (cell->data.ptr_value) {
            freeNode((Node *)cell->data.ptr_value);
        }
        tecc_pfree(cell);
        cell = next;
    }

    tecc_pfree(list);
}
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

/* ==================== 语句节点创建函数 ==================== */

SelectStmt *makeSelectStmt(int stmt_location, int stmt_len) {
    SelectStmt *stmt = (SelectStmt *)tecc_palloc(sizeof(SelectStmt));
    if (!stmt) return NULL;

    stmt->type = T_SelectStmt;
    stmt->distinctClause = NIL;
    stmt->intoClause = NULL;
    stmt->targetList = NIL;
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

    stmt->type = T_InsertStmt;
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

    stmt->type = T_UpdateStmt;
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

    stmt->type = T_DeleteStmt;
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

    stmt->type = T_CreateKekStmt;
    stmt->kek_name = NULL;
    stmt->options = NIL;
    stmt->if_not_exists = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

CreateDekStmt *makeCreateDekStmt(int stmt_location, int stmt_len) {
    CreateDekStmt *stmt = (CreateDekStmt *)tecc_palloc(sizeof(CreateDekStmt));
    if (!stmt) return NULL;

    stmt->type = T_CreateDekStmt;
    stmt->dek_name = NULL;
    stmt->options = NIL;
    stmt->if_not_exists = false;
    stmt->stmt_location = stmt_location;
    stmt->stmt_len = stmt_len;

    return stmt;
}

DropStmt *makeDropStmt(int stmt_location, int stmt_len) {
    DropStmt *stmt = (DropStmt *)tecc_palloc(sizeof(DropStmt));
    if (!stmt) return NULL;

    stmt->type = T_DropStmt;
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

    stmt->type = T_CreateStmt;
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

    stmt->type = T_AlterTableStmt;
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

    cmd->type = T_AlterTableCmd;
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

    col->type = T_ColumnDef;
    col->colname = NULL;
    col->typeName = NULL;
    col->constraints = NIL;
    col->location = location;
    /* Note: Additional fields may need initialization based on ColumnDef structure */

    return col;
}

/* ==================== 表达式节点创建函数 ==================== */

ColumnRef *makeColumnRef(List *fields, int location) {
    ColumnRef *node = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
    if (!node) return NULL;

    node->type = T_ColumnRef;
    node->fields = fields;
    node->location = location;

    return node;
}

FuncCall *makeFuncCall(List *funcname, List *args, int location) {
    FuncCall *node = (FuncCall *)tecc_palloc(sizeof(FuncCall));
    if (!node) return NULL;

    node->type = T_FuncCall;
    node->funcname = funcname;
    node->args = args;
    node->agg_order = NIL;
    node->agg_filter = NULL;
    node->agg_star = false;
    node->agg_distinct = false;
    node->func_variadic = false;
    node->over = NULL;
    node->location = location;

    return node;
}

A_Expr *makeAExpr(int kind, List *name, Node *lexpr, Node *rexpr, int location) {
    A_Expr *node = (A_Expr *)tecc_palloc(sizeof(A_Expr));
    if (!node) return NULL;

    node->type = T_A_Expr;
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

    node->type = T_A_Expr;
    node->kind = kind;
    node->name = name;
    node->lexpr = lexpr;
    node->rexpr = rexpr;
    node->escape = escape;
    node->location = location;

    return node;
}

BoolExpr *makeBoolExpr(int boolop, List *args, int location) {
    BoolExpr *node = (BoolExpr *)tecc_palloc(sizeof(BoolExpr));
    if (!node) return NULL;

    node->type = T_BoolExpr;
    node->boolop = boolop;
    node->args = args;
    node->location = location;

    return node;
}

NullTest *makeNullTest(Node *arg, int nulltesttype, int location) {
    NullTest *node = (NullTest *)tecc_palloc(sizeof(NullTest));
    if (!node) return NULL;

    node->type = T_NullTest;
    node->arg = arg;
    node->nulltesttype = nulltesttype;
    node->location = location;

    return node;
}

Param *makeParam(int paramkind, int paramid, int paramtype, int location) {
    Param *node = (Param *)tecc_palloc(sizeof(Param));
    if (!node) return NULL;

    node->type = T_Param;
    node->paramkind = paramkind;
    node->paramid = paramid;
    node->paramtype = paramtype;
    node->location = location;

    return node;
}

Const *makeConst(int consttype, bool constisnull, int location, int data_size) {
    Const *node = (Const *)tecc_palloc(sizeof(Const));
    if (!node) return NULL;

    node->type = T_Const;
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

    node->type = T_Var;
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

    node->type = T_ResTarget;
    node->name = tecc_pstrdup(name);
    node->indirection = NIL;
    node->val = val;
    node->location = location;

    return node;
}

RangeVar *makeRangeVar(char *schemaname, char *relname, int location) {
    RangeVar *node = (RangeVar *)tecc_palloc(sizeof(RangeVar));
    if (!node) return NULL;

    node->type = T_RangeVar;
    node->catalogname = NULL;
    node->schemaname = tecc_pstrdup(schemaname);
    node->relname = tecc_pstrdup(relname);
    node->inh = true;
    node->relpersistence = 'p';  /* permanent table */
    node->alias = NULL;
    node->location = location;

    return node;
}

RangeVar *makeRangeVarFromQualifiedName(const char *qualified_name, int location, int length) {
    char *schemaname = NULL;
    char *relname = NULL;
    char *copy = tecc_pstrdup(qualified_name);
    char *dot = strchr(copy, '.');

    if (dot) {
        *dot = '\0';
        schemaname = copy;
        relname = dot + 1;
    } else {
        relname = copy;
    }

    RangeVar *rv = makeRangeVar(schemaname, relname, location);
    tecc_pfree(copy);
    return rv;
}

Alias *makeAlias(char *aliasname, List *colnames) {
    Alias *node = (Alias *)tecc_palloc(sizeof(Alias));
    if (!node) return NULL;

    node->type = T_Alias;
    node->aliasname = tecc_pstrdup(aliasname);
    node->colnames = colnames;

    return node;
}

DefElem *makeDefElem(char *defname, Node *arg, int location) {
    DefElem *node = (DefElem *)tecc_palloc(sizeof(DefElem));
    if (!node) return NULL;

    node->type = T_DefElem;
    node->defnamespace = NULL;
    node->defname = tecc_pstrdup(defname);
    node->arg = arg;
    node->defaction = 0;  /* unspecified action */
    node->location = location;

    return node;
}

/* ==================== 节点释放函数 ==================== */

void freeNode(Node *node) {
    if (!node) return;

    /* 根据节点类型调用具体的释放函数 */
    switch (node->type) {
        case T_SelectStmt:
            freeSelectStmt((SelectStmt *)node);
            break;
        case T_InsertStmt:
            freeInsertStmt((InsertStmt *)node);
            break;
        case T_UpdateStmt:
            freeUpdateStmt((UpdateStmt *)node);
            break;
        case T_DeleteStmt:
            freeDeleteStmt((DeleteStmt *)node);
            break;
        case T_CreateKekStmt:
            freeCreateKekStmt((CreateKekStmt *)node);
            break;
        case T_CreateDekStmt:
            freeCreateDekStmt((CreateDekStmt *)node);
            break;
        case T_DropStmt:
            freeDropStmt((DropStmt *)node);
            break;
        case T_PrepareStmt:
            freePrepareStmt((PrepareStmt *)node);
            break;
        case T_ExecuteStmt:
            freeExecuteStmt((ExecuteStmt *)node);
            break;
        case FE_T_CopyStmt:
            freeCopyStmt((CopyStmt *)node);
            break;
        case T_Value:
            /* Value节点需要释放字符串 */
            {
                Value *value = (Value *)node;
                if (value->type == T_String && value->val.str) {
                    tecc_pfree(value->val.str);
                }
                tecc_pfree(node);
            }
            break;
        case T_ColumnRef:
            {
                ColumnRef *colref = (ColumnRef *)node;
                if (colref->fields) {
                    list_free_deep(colref->fields);
                }
                tecc_pfree(node);
            }
            break;
        case T_FuncCall:
            {
                FuncCall *funccall = (FuncCall *)node;
                if (funccall->funcname) {
                    list_free_deep(funccall->funcname);
                }
                if (funccall->args) {
                    list_free_deep(funccall->args);
                }
                tecc_pfree(node);
            }
            break;
        case T_ResTarget:
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
        case T_RangeVar:
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
        case T_DefElem:
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
        default:
            /* 默认只释放节点本身 */
            tecc_pfree(node);
            break;
    }
}

void freeSelectStmt(SelectStmt *stmt) {
    if (!stmt) return;

    if (stmt->distinctClause) list_free_deep(stmt->distinctClause);
    if (stmt->targetList) list_free_deep(stmt->targetList);
    if (stmt->fromClause) list_free_deep(stmt->fromClause);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->groupClause) list_free_deep(stmt->groupClause);
    if (stmt->havingClause) freeNode(stmt->havingClause);
    if (stmt->windowClause) list_free_deep(stmt->windowClause);
    if (stmt->sortClause) list_free_deep(stmt->sortClause);
    if (stmt->limitOffset) freeNode(stmt->limitOffset);
    if (stmt->limitCount) freeNode(stmt->limitCount);
    if (stmt->lockingClause) list_free_deep(stmt->lockingClause);
    if (stmt->cteList) list_free_deep(stmt->cteList);

    tecc_pfree(stmt);
}

void freeInsertStmt(InsertStmt *stmt) {
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->cols) list_free_deep(stmt->cols);
    if (stmt->selectStmt) freeNode(stmt->selectStmt);
    if (stmt->onConflictClause) list_free_deep(stmt->onConflictClause);
    if (stmt->returningList) list_free_deep(stmt->returningList);
    if (stmt->withClause) list_free_deep(stmt->withClause);

    tecc_pfree(stmt);
}

void freeUpdateStmt(UpdateStmt *stmt) {
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->targetList) list_free_deep(stmt->targetList);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->fromClause) list_free_deep(stmt->fromClause);
    if (stmt->returningList) list_free_deep(stmt->returningList);
    if (stmt->withClause) list_free_deep(stmt->withClause);

    tecc_pfree(stmt);
}

void freeDeleteStmt(DeleteStmt *stmt) {
    if (!stmt) return;

    if (stmt->relation) freeNode((Node *)stmt->relation);
    if (stmt->whereClause) freeNode(stmt->whereClause);
    if (stmt->usingClause) list_free_deep(stmt->usingClause);
    if (stmt->returningList) list_free_deep(stmt->returningList);
    if (stmt->withClause) list_free_deep(stmt->withClause);

    tecc_pfree(stmt);
}

void freeCreateKekStmt(CreateKekStmt *stmt) {
    if (!stmt) return;

    if (stmt->kek_name) tecc_pfree(stmt->kek_name);
    if (stmt->options) list_free_deep(stmt->options);

    tecc_pfree(stmt);
}

void freeCreateDekStmt(CreateDekStmt *stmt) {
    if (!stmt) return;

    if (stmt->dek_name) tecc_pfree(stmt->dek_name);
    if (stmt->options) list_free_deep(stmt->options);

    tecc_pfree(stmt);
}

void freeDropStmt(DropStmt *stmt) {
    if (!stmt) return;

    if (stmt->objects) list_free_deep(stmt->objects);
    if (stmt->arguments) list_free_deep(stmt->arguments);

    tecc_pfree(stmt);
}

/* ==================== 节点复制函数 ==================== */

Node *tecc_copy_node(const Node *node) {
    if (!node) return NULL;

    /* 简化的节点复制实现 */
    /* 注意：这不是完整的深度复制，只复制基础结构 */
    switch (node->type) {
        case T_SelectStmt: {
            SelectStmt *src = (SelectStmt *)node;
            SelectStmt *dst = (SelectStmt *)tecc_palloc(sizeof(SelectStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(SelectStmt));
            return (Node *)dst;
        }
        case T_InsertStmt: {
            InsertStmt *src = (InsertStmt *)node;
            InsertStmt *dst = (InsertStmt *)tecc_palloc(sizeof(InsertStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(InsertStmt));
            return (Node *)dst;
        }
        case T_UpdateStmt: {
            UpdateStmt *src = (UpdateStmt *)node;
            UpdateStmt *dst = (UpdateStmt *)tecc_palloc(sizeof(UpdateStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(UpdateStmt));
            return (Node *)dst;
        }
        case T_DeleteStmt: {
            DeleteStmt *src = (DeleteStmt *)node;
            DeleteStmt *dst = (DeleteStmt *)tecc_palloc(sizeof(DeleteStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DeleteStmt));
            return (Node *)dst;
        }
        case T_CreateKekStmt: {
            CreateKekStmt *src = (CreateKekStmt *)node;
            CreateKekStmt *dst = (CreateKekStmt *)tecc_palloc(sizeof(CreateKekStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CreateKekStmt));
            return (Node *)dst;
        }
        case T_CreateDekStmt: {
            CreateDekStmt *src = (CreateDekStmt *)node;
            CreateDekStmt *dst = (CreateDekStmt *)tecc_palloc(sizeof(CreateDekStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CreateDekStmt));
            return (Node *)dst;
        }
        case T_DropStmt: {
            DropStmt *src = (DropStmt *)node;
            DropStmt *dst = (DropStmt *)tecc_palloc(sizeof(DropStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(DropStmt));
            return (Node *)dst;
        }
        case T_PrepareStmt: {
            PrepareStmt *src = (PrepareStmt *)node;
            PrepareStmt *dst = (PrepareStmt *)tecc_palloc(sizeof(PrepareStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(PrepareStmt));
            return (Node *)dst;
        }
        case T_ExecuteStmt: {
            ExecuteStmt *src = (ExecuteStmt *)node;
            ExecuteStmt *dst = (ExecuteStmt *)tecc_palloc(sizeof(ExecuteStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(ExecuteStmt));
            return (Node *)dst;
        }
        case FE_T_CopyStmt: {
            CopyStmt *src = (CopyStmt *)node;
            CopyStmt *dst = (CopyStmt *)tecc_palloc(sizeof(CopyStmt));
            if (!dst) return NULL;
            memcpy(dst, src, sizeof(CopyStmt));
            return (Node *)dst;
        }
        default:
            /* 对于其他节点类型，只复制基础Node结构 */
            Node *copy = (Node *)tecc_palloc(sizeof(Node));
            if (!copy) return NULL;
            copy->type = node->type;
            return copy;
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

    stmt->type = T_PrepareStmt;
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

    stmt->type = T_ExecuteStmt;
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

/* ==================== Alias node free function ==================== */

void freeAlias(Alias *alias)
{
    if (!alias) return;

    if (alias->aliasname) tecc_pfree(alias->aliasname);
    if (alias->colnames) list_free_deep(alias->colnames);

    tecc_pfree(alias);
}

/* ==================== RangeVar node free function ==================== */

void freeRangeVar(RangeVar *rangevar)
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
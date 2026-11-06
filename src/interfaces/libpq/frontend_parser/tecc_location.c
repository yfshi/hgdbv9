/*
 * TECC前端解析器位置信息管理
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的SQL改写需求
 *
 * 设计原则：
 * 1. 位置信息准确：支持精确的SQL元素定位
 * 2. 映射关系完整：支持原始位置到新位置的映射
 * 3. 性能高效：支持大规模SQL的实时改写
 * 4. 内存紧凑：最小化位置信息的内存占用
 */

#include "tecc_location.h"
#include "tecc_nodes.h"
#include "tecc_parser.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ==================== 内部数据结构 ==================== */

/* 位置映射节点 */
typedef struct LocationMapNode {
    int original_start;     /* 原始起始位置 */
    int original_end;       /* 原始结束位置 */
    int new_start;          /* 新起始位置 */
    int new_end;            /* 新结束位置 */
    struct LocationMapNode *next;  /* 下一个映射节点 */
} LocationMapNode;

/* 位置管理器 */
typedef struct LocationManager {
    LocationMapNode *mappings;      /* 位置映射链表 */
    int mapping_count;              /* 映射数量 */
    size_t total_mapped_chars;      /* 总映射字符数 */
    int current_offset;             /* 当前偏移量（用于增量改写） */
    bool tracking_enabled;          /* 是否启用位置跟踪 */
} LocationManager;

/* ==================== 全局位置管理器 ==================== */

/* 注：全局位置管理器目前未使用，保留为未来扩展 */
/* static LocationManager g_location_manager = { */
/*     .mappings = NULL, */
/*     .mapping_count = 0, */
/*     .total_mapped_chars = 0, */
/*     .current_offset = 0, */
/*     .tracking_enabled = true */
/* }; */

/* ==================== 内部辅助函数 ==================== */

/* 创建位置映射节点 */
static LocationMapNode *create_location_map(int orig_start, int orig_end,
                                           int new_start, int new_end) {
    LocationMapNode *node = (LocationMapNode *)tecc_palloc(sizeof(LocationMapNode));
    if (!node) return NULL;

    node->original_start = orig_start;
    node->original_end = orig_end;
    node->new_start = new_start;
    node->new_end = new_end;
    node->next = NULL;

    return node;
}

/* 添加位置映射 */
static void add_location_mapping(LocationManager *manager,
                                int orig_start, int orig_end,
                                int new_start, int new_end) {
    LocationMapNode *node;

    if (!manager || !manager->tracking_enabled) return;

    node = create_location_map(orig_start, orig_end, new_start, new_end);
    if (!node) return;

    /* 添加到链表头部 */
    node->next = manager->mappings;
    manager->mappings = node;
    manager->mapping_count++;
    manager->total_mapped_chars += (new_end - new_start);
}

/* 查找位置映射 */
static const LocationMapNode *find_location_mapping(const LocationManager *manager,
                                                   int original_pos) {
    LocationMapNode *current;

    if (!manager || !manager->mappings) return NULL;

    current = manager->mappings;
    while (current) {
        if (original_pos >= current->original_start &&
            original_pos <= current->original_end) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/* 计算新位置 */
static int calculate_new_position(const LocationManager *manager, int original_pos) {
    const LocationMapNode *mapping;
    int relative_pos;
    int mapping_length;
    int new_mapping_length;

    if (!manager || !manager->tracking_enabled) return original_pos;

    mapping = find_location_mapping(manager, original_pos);
    if (!mapping) {
        /* 没有找到映射，应用全局偏移 */
        return original_pos + manager->current_offset;
    }

    /* 计算在映射区间内的相对位置 */
    relative_pos = original_pos - mapping->original_start;
    mapping_length = mapping->original_end - mapping->original_start;
    new_mapping_length = mapping->new_end - mapping->new_start;

    if (mapping_length == 0) {
        /* 零长度映射，直接使用新起始位置 */
        return mapping->new_start;
    }

    /* 线性插值计算新位置 */
    return mapping->new_start + (relative_pos * new_mapping_length) / mapping_length;
}

/* ==================== 公共接口实现 ==================== */

/* 初始化位置管理器 */
void tecc_location_init(LocationManager **manager_ptr) {
    LocationManager *manager;

    if (!manager_ptr) return;

    manager = (LocationManager *)tecc_palloc(sizeof(LocationManager));
    if (!manager) return;

    memset(manager, 0, sizeof(LocationManager));
    manager->tracking_enabled = true;

    *manager_ptr = manager;
}

/* 清理位置管理器 */
void tecc_location_cleanup(LocationManager *manager) {
    LocationMapNode *current;
    LocationMapNode *next;

    if (!manager) return;

    current = manager->mappings;
    while (current) {
        next = current->next;
        tecc_pfree(current);
        current = next;
    }

    tecc_pfree(manager);
}

/* 启用/禁用位置跟踪 */
void tecc_location_set_tracking(LocationManager *manager, bool enabled) {
    if (manager) {
        manager->tracking_enabled = enabled;
    }
}

/* 添加文本替换映射 */
void tecc_location_add_replacement(LocationManager *manager,
                                  int orig_start, int orig_end,
                                  const char *new_text) {
    int new_start;
    int new_end;
    int orig_length;
    int new_length;

    if (!manager || !manager->tracking_enabled || !new_text) return;

    new_start = manager->current_offset + orig_start;
    new_end = new_start + strlen(new_text);

    add_location_mapping(manager, orig_start, orig_end, new_start, new_end);

    /* 更新当前偏移量 */
    orig_length = orig_end - orig_start;
    new_length = strlen(new_text);
    manager->current_offset += (new_length - orig_length);
}

/* 添加文本插入映射 */
void tecc_location_add_insertion(LocationManager *manager,
                                int position,
                                const char *inserted_text) {
    if (!manager || !manager->tracking_enabled || !inserted_text) return;

    /* 插入可以看作是从position到position的零长度替换 */
    tecc_location_add_replacement(manager, position, position, inserted_text);
}

/* 添加文本删除映射 */
void tecc_location_add_deletion(LocationManager *manager,
                               int start, int end) {
    if (!manager || !manager->tracking_enabled) return;

    /* 删除可以看作是用空字符串替换 */
    tecc_location_add_replacement(manager, start, end, "");
}

/* 获取新位置 */
int tecc_location_get_new_position(const LocationManager *manager, int original_pos) {
    return calculate_new_position(manager, original_pos);
}

/* 获取位置信息的新位置 */
LocationInfo tecc_location_get_new_location(const LocationManager *manager,
                                           const LocationInfo *original_loc) {
    LocationInfo new_loc = {0};

    if (!manager || !original_loc) return new_loc;

    new_loc.start_location = tecc_location_get_new_position(manager, original_loc->start_location);
    new_loc.end_location = tecc_location_get_new_position(manager, original_loc->end_location);
    new_loc.start_line = original_loc->start_line;
    new_loc.start_column = original_loc->start_column;
    new_loc.end_line = original_loc->end_line;
    new_loc.end_column = original_loc->end_column;

    return new_loc;
}

/* 批量更新节点位置 */
void tecc_location_update_node_positions(const LocationManager *manager, Node *node) {
    if (!manager || !node || !manager->tracking_enabled) return;

    /* 根据节点类型递归更新位置 */
    switch (node->type) {
        case FE_T_SelectStmt: {
            SelectStmt *stmt = (SelectStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            /* 注意：stmt_len 不需要更新，因为它是长度不是位置 */
            break;
        }
        case FE_T_InsertStmt: {
            InsertStmt *stmt = (InsertStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_UpdateStmt: {
            UpdateStmt *stmt = (UpdateStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_DeleteStmt: {
            DeleteStmt *stmt = (DeleteStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_CreateKekStmt: {
            CreateKekStmt *stmt = (CreateKekStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_CreateDekStmt: {
            CreateDekStmt *stmt = (CreateDekStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_DropStmt: {
            DropStmt *stmt = (DropStmt *)node;
            stmt->stmt_location = tecc_location_get_new_position(manager, stmt->stmt_location);
            break;
        }
        case FE_T_ColumnRef: {
            ColumnRef *colref = (ColumnRef *)node;
            colref->location = tecc_location_get_new_position(manager, colref->location);
            break;
        }
        case FE_T_FuncCall: {
            FuncCall *funccall = (FuncCall *)node;
            funccall->location = tecc_location_get_new_position(manager, funccall->location);
            break;
        }
        case FE_T_A_Expr: {
            A_Expr *aexpr = (A_Expr *)node;
            aexpr->location = tecc_location_get_new_position(manager, aexpr->location);
            break;
        }
        case FE_T_BoolExpr: {
            BoolExpr *boolexpr = (BoolExpr *)node;
            boolexpr->location = tecc_location_get_new_position(manager, boolexpr->location);
            break;
        }
        case FE_T_NullTest: {
            NullTest *nulltest = (NullTest *)node;
            nulltest->location = tecc_location_get_new_position(manager, nulltest->location);
            break;
        }
        case FE_T_Param: {
            Param *param = (Param *)node;
            param->location = tecc_location_get_new_position(manager, param->location);
            break;
        }
        case FE_T_Const: {
            Const *const_node = (Const *)node;
            const_node->location = tecc_location_get_new_position(manager, const_node->location);
            break;
        }
        case FE_T_Var: {
            Var *var = (Var *)node;
            var->location = tecc_location_get_new_position(manager, var->location);
            break;
        }
        case FE_T_ResTarget: {
            ResTarget *restarget = (ResTarget *)node;
            restarget->location = tecc_location_get_new_position(manager, restarget->location);
            break;
        }
        case FE_T_RangeVar: {
            RangeVar *rangevar = (RangeVar *)node;
            rangevar->location = tecc_location_get_new_position(manager, rangevar->location);
            break;
        }
        case FE_T_DefElem: {
            DefElem *defelem = (DefElem *)node;
            defelem->location = tecc_location_get_new_position(manager, defelem->location);
            break;
        }
        case FE_T_String:
        case FE_T_Integer:
        case FE_T_Float:
        case FE_T_Null: {
            Value *value = (Value *)node;
            value->location = tecc_location_get_new_position(manager, value->location);
            break;
        }
        default:
            /* 其他节点类型不需要位置更新 */
            break;
    }

    /* 递归处理列表中的节点 */
    if (node->type == FE_T_List) {
        List *list = (List *)node;
        ListCell *cell = list_head(list);
        while (cell) {
            if (list->list_type == FE_T_List && cell->data.ptr_value) {
                tecc_location_update_node_positions(manager, (Node *)cell->data.ptr_value);
            }
            cell = cell->next;
        }
    }
}

/* 获取统计信息 */
void tecc_location_get_statistics(const LocationManager *manager,
                                 int *mapping_count,
                                 size_t *total_mapped_chars,
                                 int *current_offset) {
    if (!manager) return;

    if (mapping_count) *mapping_count = manager->mapping_count;
    if (total_mapped_chars) *total_mapped_chars = manager->total_mapped_chars;
    if (current_offset) *current_offset = manager->current_offset;
}

/* 重置位置管理器 */
void tecc_location_reset(LocationManager *manager) {
    LocationMapNode *current;
    LocationMapNode *next;

    if (!manager) return;

    /* 清理现有映射 */
    current = manager->mappings;
    while (current) {
        next = current->next;
        tecc_pfree(current);
        current = next;
    }

    /* 重置状态 */
    manager->mappings = NULL;
    manager->mapping_count = 0;
    manager->total_mapped_chars = 0;
    manager->current_offset = 0;
}

/* 导出位置映射 */
LocationMapping *tecc_location_export_mappings(const LocationManager *manager) {
    LocationMapping *head = NULL;
    LocationMapping *tail = NULL;
    LocationMapNode *current;
    LocationMapping *mapping;

    if (!manager || !manager->mappings) return NULL;

    current = manager->mappings;
    while (current) {
        mapping = (LocationMapping *)tecc_palloc(sizeof(LocationMapping));
        if (!mapping) break;

        mapping->original_start = current->original_start;
        mapping->original_end = current->original_end;
        mapping->new_start = current->new_start;
        mapping->new_end = current->new_end;
        mapping->next = NULL;

        if (!head) {
            head = mapping;
            tail = mapping;
        } else {
            tail->next = mapping;
            tail = mapping;
        }

        current = current->next;
    }

    return head;
}

/* 导入位置映射 */
void tecc_location_import_mappings(LocationManager *manager,
                                  const LocationMapping *mappings) {
    const LocationMapping *current;

    if (!manager || !mappings) return;

    tecc_location_reset(manager);

    current = mappings;
    while (current) {
        add_location_mapping(manager,
                            current->original_start,
                            current->original_end,
                            current->new_start,
                            current->new_end);
        current = current->next;
    }
}
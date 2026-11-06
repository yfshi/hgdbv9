/*
 * TECC前端解析器位置信息管理头文件
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的SQL改写需求
 */

#ifndef TECC_LOCATION_H
#define TECC_LOCATION_H

#include "tecc_nodes.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 位置管理器句柄 ==================== */

/* 位置管理器不透明类型 */
typedef struct LocationManager LocationManager;

/* ==================== 初始化与清理 ==================== */

/*
 * tecc_location_init - 初始化位置管理器
 *
 * 功能：创建并初始化位置管理器实例
 * 兼容性要求：
 * 1. 内存分配方式必须与处理器模块兼容
 * 2. 初始化状态必须支持处理器模块的多次使用
 *
 * @param manager_ptr 管理器指针输出参数
 */
void tecc_location_init(LocationManager **manager_ptr);

/*
 * tecc_location_cleanup - 清理位置管理器
 *
 * 功能：释放位置管理器占用的所有内存
 * 兼容性要求：
 * 1. 内存释放必须彻底，避免内存泄漏
 * 2. 必须支持NULL指针的安全处理
 *
 * @param manager 要清理的位置管理器
 */
void tecc_location_cleanup(LocationManager *manager);

/* ==================== 位置跟踪控制 ==================== */

/*
 * tecc_location_set_tracking - 启用/禁用位置跟踪
 *
 * 功能：控制位置跟踪的启用状态
 * 兼容性要求：
 * 1. 必须支持处理器模块的动态跟踪控制
 * 2. 状态切换必须不影响现有映射
 *
 * @param manager 位置管理器
 * @param enabled 是否启用位置跟踪
 */
void tecc_location_set_tracking(LocationManager *manager, bool enabled);

/* ==================== 位置映射操作 ==================== */

/*
 * tecc_location_add_replacement - 添加文本替换映射
 *
 * 功能：记录文本替换操作的位置映射
 * 兼容性要求：
 * 1. 映射必须准确，支持处理器模块的精确改写
 * 2. 必须支持重叠和嵌套的替换操作
 * 3. 性能必须满足处理器模块的实时改写需求
 *
 * @param manager 位置管理器
 * @param orig_start 原始起始位置
 * @param orig_end 原始结束位置
 * @param new_text 新文本
 */
void tecc_location_add_replacement(LocationManager *manager,
                                  int orig_start, int orig_end,
                                  const char *new_text);

/*
 * tecc_location_add_insertion - 添加文本插入映射
 *
 * 功能：记录文本插入操作的位置映射
 * 兼容性要求：
 * 1. 插入位置必须准确，支持处理器模块的增量改写
 * 2. 必须支持多次插入操作
 *
 * @param manager 位置管理器
 * @param position 插入位置
 * @param inserted_text 插入的文本
 */
void tecc_location_add_insertion(LocationManager *manager,
                                int position,
                                const char *inserted_text);

/*
 * tecc_location_add_deletion - 添加文本删除映射
 *
 * 功能：记录文本删除操作的位置映射
 * 兼容性要求：
 * 1. 删除范围必须准确，支持处理器模块的精确删除
 * 2. 必须支持部分删除和完全删除
 *
 * @param manager 位置管理器
 * @param start 删除起始位置
 * @param end 删除结束位置
 */
void tecc_location_add_deletion(LocationManager *manager,
                               int start, int end);

/* ==================== 位置查询 ==================== */

/*
 * tecc_location_get_new_position - 获取新位置
 *
 * 功能：根据原始位置计算新位置
 * 兼容性要求：
 * 1. 计算必须准确，考虑所有映射关系
 * 2. 性能必须满足处理器模块的实时查询需求
 *
 * @param manager 位置管理器
 * @param original_pos 原始位置
 * @return 新位置
 */
int tecc_location_get_new_position(const LocationManager *manager, int original_pos);

/*
 * tecc_location_get_new_location - 获取位置信息的新位置
 *
 * 功能：计算位置信息结构的新位置
 * 兼容性要求：
 * 1. 必须同时更新起始和结束位置
 * 2. 行号和列号保持不变（除非文本结构发生变化）
 *
 * @param manager 位置管理器
 * @param original_loc 原始位置信息
 * @return 新的位置信息
 */
LocationInfo tecc_location_get_new_location(const LocationManager *manager,
                                           const LocationInfo *original_loc);

/* ==================== 节点位置更新 ==================== */

/*
 * tecc_location_update_node_positions - 批量更新节点位置
 *
 * 功能：递归更新解析树中所有节点的位置信息
 * 兼容性要求：
 * 1. 必须支持所有节点类型的位置更新
 * 2. 必须正确处理嵌套结构和列表
 * 3. 性能必须满足处理器模块的批量更新需求
 *
 * @param manager 位置管理器
 * @param node 要更新的节点
 */
void tecc_location_update_node_positions(const LocationManager *manager, Node *node);

/* ==================== 统计信息 ==================== */

/*
 * tecc_location_get_statistics - 获取统计信息
 *
 * 功能：获取位置管理器的统计信息
 * 兼容性要求：
 * 1. 统计信息必须详细，支持处理器模块的性能分析
 * 2. 必须支持处理器模块的资源监控需求
 *
 * @param manager 位置管理器
 * @param mapping_count 映射数量输出参数
 * @param total_mapped_chars 总映射字符数输出参数
 * @param current_offset 当前偏移量输出参数
 */
void tecc_location_get_statistics(const LocationManager *manager,
                                 int *mapping_count,
                                 size_t *total_mapped_chars,
                                 int *current_offset);

/* ==================== 重置与重用 ==================== */

/*
 * tecc_location_reset - 重置位置管理器
 *
 * 功能：重置位置管理器到初始状态
 * 兼容性要求：
 * 1. 必须彻底清理所有映射
 * 2. 重置后必须可以重新使用
 *
 * @param manager 要重置的位置管理器
 */
void tecc_location_reset(LocationManager *manager);

/* ==================== 导入导出 ==================== */

/*
 * tecc_location_export_mappings - 导出位置映射
 *
 * 功能：导出位置映射到标准格式
 * 兼容性要求：
 * 1. 导出格式必须与处理器模块兼容
 * 2. 必须支持处理器模块的映射共享需求
 *
 * @param manager 位置管理器
 * @return 位置映射链表
 */
LocationMapping *tecc_location_export_mappings(const LocationManager *manager);

/*
 * tecc_location_import_mappings - 导入位置映射
 *
 * 功能：从标准格式导入位置映射
 * 兼容性要求：
 * 1. 必须支持处理器模块的映射恢复需求
 * 2. 导入必须准确，保持映射关系不变
 *
 * @param manager 位置管理器
 * @param mappings 要导入的位置映射
 */
void tecc_location_import_mappings(LocationManager *manager,
                                  const LocationMapping *mappings);

#ifdef __cplusplus
}
#endif

#endif /* TECC_LOCATION_H */
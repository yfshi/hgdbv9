/*
 * TECC前端解析器语义分析器头文件
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的语义检查需求
 */

#ifndef TECC_SEMANTIC_H
#define TECC_SEMANTIC_H

#include "tecc_nodes.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 语义分析接口 ==================== */

/*
 * tecc_semantic_analyze - 语义分析主函数
 *
 * 功能：对解析树进行全面的语义分析
 * 兼容性要求：
 * 1. 检查必须全面，覆盖所有SQL语义约束
 * 2. 错误报告必须详细，支持处理器模块的错误处理
 * 3. 性能必须满足处理器模块的实时分析需求
 *
 * @param parse_tree 解析树
 * @param strict_mode 是否启用严格模式
 * @param error_count 错误数量输出参数
 * @param warning_count 警告数量输出参数
 * @return 错误信息链表
 */
ParserError *tecc_semantic_analyze(const Node *parse_tree,
                                  bool strict_mode,
                                  int *error_count,
                                  int *warning_count);

/*
 * tecc_semantic_check - 快速语义检查
 *
 * 功能：快速检查解析树的关键语义错误
 * 兼容性要求：
 * 1. 只检查关键错误，性能优先
 * 2. 必须支持处理器模块的快速验证需求
 *
 * @param parse_tree 解析树
 * @return 是否通过语义检查
 */
bool tecc_semantic_check(const Node *parse_tree);

/*
 * tecc_semantic_get_statistics - 获取语义分析统计信息
 *
 * 功能：获取语义分析的统计信息
 * 兼容性要求：
 * 1. 统计信息必须详细，支持处理器模块的性能分析
 * 2. 必须支持处理器模块的监控需求
 *
 * @param parse_tree 解析树
 * @param nodes_checked 检查的节点数输出参数
 * @param constraints_validated 验证的约束数输出参数
 */
void tecc_semantic_get_statistics(const Node *parse_tree,
                                 int *nodes_checked,
                                 int *constraints_validated);

/* ==================== 语义检查配置 ==================== */

/*
 * 语义检查级别枚举
 */
typedef enum SemanticCheckLevel {
    SEMANTIC_CHECK_NONE = 0,      /* 不进行语义检查 */
    SEMANTIC_CHECK_BASIC = 1,     /* 基础检查（语法级错误） */
    SEMANTIC_CHECK_STANDARD = 2,  /* 标准检查（语义级错误） */
    SEMANTIC_CHECK_STRICT = 3,    /* 严格检查（所有警告） */
    SEMANTIC_CHECK_FULL = 4       /* 完整检查（包括性能建议） */
} SemanticCheckLevel;

/*
 * 语义检查选项结构
 */
typedef struct SemanticCheckOptions {
    SemanticCheckLevel check_level;       /* 检查级别 */
    bool enable_encryption_checks;        /* 是否启用加密检查 */
    bool enable_performance_checks;       /* 是否启用性能检查 */
    bool enable_security_checks;          /* 是否启用安全检查 */
    int max_error_count;                  /* 最大错误数 */
    int max_warning_count;                /* 最大警告数 */
    void *user_data;                      /* 用户数据 */
} SemanticCheckOptions;

/*
 * tecc_semantic_analyze_ex - 扩展语义分析
 *
 * 功能：使用指定选项进行语义分析
 * 兼容性要求：
 * 1. 必须支持处理器模块的定制化检查需求
 * 2. 选项配置必须灵活，支持多种检查场景
 *
 * @param parse_tree 解析树
 * @param options 语义检查选项
 * @param error_count 错误数量输出参数
 * @param warning_count 警告数量输出参数
 * @return 错误信息链表
 */
ParserError *tecc_semantic_analyze_ex(const Node *parse_tree,
                                     const SemanticCheckOptions *options,
                                     int *error_count,
                                     int *warning_count);

/*
 * tecc_semantic_init_options - 初始化语义检查选项
 *
 * 功能：使用默认值初始化语义检查选项
 * 兼容性要求：
 * 1. 默认值必须与处理器模块的默认行为一致
 * 2. 必须支持处理器模块的选项初始化需求
 *
 * @param options 要初始化的选项结构
 */
void tecc_semantic_init_options(SemanticCheckOptions *options);

/* ==================== 加密相关语义检查 ==================== */

/*
 * tecc_semantic_check_encryption - 加密相关语义检查
 *
 * 功能：专门检查与加密相关的语义约束
 * 兼容性要求：
 * 1. 必须严格执行TECC加密约束
 * 2. 必须支持处理器模块的加密验证需求
 *
 * @param parse_tree 解析树
 * @param kek_list 可用的KEK列表
 * @param dek_list 可用的DEK列表
 * @param errors 错误信息输出参数
 * @return 是否通过加密检查
 */
bool tecc_semantic_check_encryption(const Node *parse_tree,
                                   List *kek_list,
                                   List *dek_list,
                                   ParserError **errors);

/*
 * tecc_semantic_validate_kek - 验证KEK定义
 *
 * 功能：验证KEK定义的语义正确性
 * 兼容性要求：
 * 1. 必须检查KEK名称、算法、密钥大小等约束
 * 2. 必须支持处理器模块的KEK验证需求
 *
 * @param kek_stmt CREATE KEK语句
 * @param existing_keks 现有的KEK列表
 * @param errors 错误信息输出参数
 * @return 是否通过验证
 */
bool tecc_semantic_validate_kek(const CreateKekStmt *kek_stmt,
                               List *existing_keks,
                               ParserError **errors);

/*
 * tecc_semantic_validate_dek - 验证DEK定义
 *
 * 功能：验证DEK定义的语义正确性
 * 兼容性要求：
 * 1. 必须检查DEK名称、关联的KEK、算法等约束
 * 2. 必须支持处理器模块的DEK验证需求
 *
 * @param dek_stmt CREATE DEK语句
 * @param existing_deks 现有的DEK列表
 * @param available_keks 可用的KEK列表
 * @param errors 错误信息输出参数
 * @return 是否通过验证
 */
bool tecc_semantic_validate_dek(const CreateDekStmt *dek_stmt,
                               List *existing_deks,
                               List *available_keks,
                               ParserError **errors);

/* ==================== 性能相关语义检查 ==================== */

/*
 * tecc_semantic_check_performance - 性能相关语义检查
 *
 * 功能：检查可能影响性能的语义问题
 * 兼容性要求：
 * 1. 必须识别常见的性能反模式
 * 2. 必须提供性能优化建议
 *
 * @param parse_tree 解析树
 * @param warnings 警告信息输出参数
 * @return 发现的性能问题数量
 */
int tecc_semantic_check_performance(const Node *parse_tree,
                                   ParserError **warnings);

/*
 * tecc_semantic_check_security - 安全相关语义检查
 *
 * 功能：检查安全相关的语义问题
 * 兼容性要求：
 * 1. 必须识别潜在的安全风险
 * 2. 必须提供安全加固建议
 *
 * @param parse_tree 解析树
 * @param warnings 警告信息输出参数
 * @return 发现的安全问题数量
 */
int tecc_semantic_check_security(const Node *parse_tree,
                                ParserError **warnings);

#ifdef __cplusplus
}
#endif

#endif /* TECC_SEMANTIC_H */
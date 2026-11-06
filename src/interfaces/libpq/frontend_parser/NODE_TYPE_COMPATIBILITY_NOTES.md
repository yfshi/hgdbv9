# TECC前端解析器节点类型兼容性注意事项

## 概述

本文档记录TECC前端解析器在节点类型兼容性方面的注意事项，特别是前端解析树需要传递给后端处理器的场景。

## 关键问题

### 1. 节点类型值兼容性

#### 问题描述
前端解析器最初使用自定义的节点类型值范围（1000-1020），但这些值与PostgreSQL的NodeTag枚举不兼容。当解析树需要传递给后端处理器时，节点类型值必须与PostgreSQL兼容。

#### 解决方案
更新节点类型值为PostgreSQL兼容的值：

**标准SQL语句节点类型（使用PostgreSQL值）**：
- `T_SelectStmt`: 241
- `T_InsertStmt`: 238  
- `T_UpdateStmt`: 240
- `T_DeleteStmt`: 239
- `T_DropStmt`: 256

**TECC专用语句节点类型（使用专用值，避免冲突）**：
- `T_CreateKekStmt`: 1000
- `T_CreateDekStmt`: 1001

**基础节点类型（使用PostgreSQL值）**：
- `T_Invalid`: 0
- `T_List`: 231
- `T_Value`: 225
- `T_Integer`: 226
- `T_String`: 228
- `T_Float`: 227
- `T_Null`: 230

### 2. 节点结构内存布局兼容性

#### 问题描述
前端Node结构包含`type`、`location`、`length`三个字段，而PostgreSQL的Node结构只包含`type`字段。

#### 影响
- **前端到后端传递**：前端解析树可以传递给后端，但后端需要忽略`location`和`length`字段
- **后端到前端传递**：后端生成的节点可能缺少位置信息
- **SQL改写支持**：前端需要位置信息进行SQL改写

#### 当前实现
前端Node结构：
```c
typedef struct Node {
    NodeTag type;      /* 节点类型 */
    int location;      /* 在SQL中的起始位置 */
    int length;        /* 节点长度 */
} Node;
```

### 3. 已更新的文件

#### 头文件
- `tecc_nodes.h`：更新节点类型常量定义

#### 实现文件
- `tecc_parser_unified.c`：更新节点类型验证逻辑（第522行）
- `test_processor_call.c`：更新switch语句中的节点类型值
- `simple_processor_test.c`：更新switch语句中的节点类型值
- `verify_processor_integration.sh`：更新模拟处理器中的节点类型值

#### 测试文件
- `test_node_compatibility.c`：验证节点类型兼容性

#### 文档
- `INTEGRATION_COMPLETION_SUMMARY.md`：更新节点类型常量描述
- `TECC_PROCESSOR_INTEGRATION_GUIDE.md`：更新节点类型常量定义

### 4. 验证测试

已运行的验证测试：
1. **节点类型兼容性测试**：`test_node_compatibility.c`
   - 验证标准SQL语句类型值在PostgreSQL范围内（<400）
   - 验证TECC专用类型在专用范围内（≥1000）

2. **处理器集成模拟测试**：`verify_processor_integration.sh`
   - 模拟处理器调用解析器的完整流程
   - 验证解析树格式符合处理器要求

### 5. 待解决的问题

#### 1. Bison生成文件
需要确保Bison生成的解析器代码使用正确的节点类型值。

#### 2. 处理器模块适配
处理器模块需要：
- 正确处理前端Node结构的`location`和`length`字段
- 在遍历解析树时使用正确的节点类型值

#### 3. 错误处理兼容性
前端解析器的错误码定义需要与后端兼容。

### 6. 最佳实践

#### 前端开发
1. 始终使用`tecc_nodes.h`中定义的节点类型常量
2. 在switch语句中为节点类型值添加注释说明
3. 处理节点时检查`location`和`length`字段的有效性

#### 后端集成
1. 忽略前端Node结构中的`location`和`length`字段
2. 使用`nodeTag()`宏获取节点类型（兼容前后端）
3. 处理TECC专用节点类型时进行特殊处理

### 7. 相关文件

- `tecc_nodes.h`：节点类型定义主文件
- `fe_adapter.h`：前端适配器接口
- `tecc_parser_unified.c`：统一版解析器实现
- `test_node_compatibility.c`：兼容性测试
- `parse_node_tags.py`：PostgreSQL节点类型解析脚本

## 总结

TECC前端解析器的节点类型兼容性问题已基本解决：
- ✅ 节点类型值与PostgreSQL兼容
- ✅ 前端解析树可以传递给后端处理器
- ✅ 位置信息支持SQL改写功能
- ✅ 测试验证通过

剩余工作需要确保Bison生成的代码和处理器模块完全适配新的节点类型值。

**最后更新**: 2026-04-13
**状态**: ✅ 主要问题已解决，待最终集成验证
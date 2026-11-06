# TECC HSM Adapter Module

## 概述
硬件安全模块（HSM）适配器模块，用于集成同智伟业TZHSM-G002密码机到TECC系统中。

## 目录结构
```
tecc_hsm_adapter/
├── fe-hsm-adapter.h          # HSM适配器接口定义
├── fe-hsm-adapter.c          # HSM适配器实现
├── test_hsm_adapter.c        # 测试程序
├── test_hsm_adapter.mk       # 测试Makefile
├── HSM_ADAPTER_IMPLEMENTATION.md  # 实现文档
└── README.md                 # 本文件
```

## 功能特性

### 1. 统一的HSM适配器接口
- `HsmAdapterInterface`: 统一的适配器接口
- 支持设备管理（初始化、连接、断开、清理）
- 支持密钥管理（检查密钥存在、获取密钥信息、导出公钥）
- 支持密钥操作（加密、解密、签名、验证）

### 2. 双适配器实现
- **同智伟业适配器** (`get_tzhsm_adapter()`): 基于`sdf_client.h`的框架实现
- **模拟适配器** (`get_mock_hsm_adapter()`): 用于测试和开发

### 3. KEY_PATH解析
- 严格格式: `hsm:id=<index>`
- 索引范围: 1-65535
- 解析函数: `parse_hsm_key_path()`

### 4. HSM KEK设置
- 核心函数: `setup_hsm_kek()`
- 功能: 检查密码机中指定索引的密钥是否存在
- 错误处理: 密钥不存在时提示用户先在密码机中生成

## 使用示例

### 1. 基本使用
```c
#include "tecc_hsm_adapter/fe-hsm-adapter.h"

// 设置HSM KEK
int result = setup_hsm_kek("my_kek", "hsm:id=1", NULL);
if (result == 0) {
    printf("HSM KEK setup successful\n");
}
```

### 2. SQL语句
```sql
CREATE KEK my_hsm_kek WITH (
    kms_type = 'hsmkms',
    key_path = 'hsm:id=1',
    kms_args = ''
);
```

### 3. 测试
```bash
# 进入目录
cd tecc_hsm_adapter

# 编译测试
make -f test_hsm_adapter.mk

# 运行测试
./test_hsm_adapter
```

## 依赖关系
- 被 `tecc_processor/create_kek_processor.c` 调用
- 集成到 `stmt_processor.c` 的CREATE KEK处理流程中

## 编译说明
模块已集成到主Makefile中：
- 包含路径: `-I$(srcdir)/tecc_hsm_adapter`
- 源文件: `tecc_hsm_adapter/fe-hsm-adapter.o`

## 设计原则
1. **仅硬件加密**: 仅支持`hsmkms`模式
2. **不生成密钥**: 只检查密钥存在性
3. **前端实现**: 位于libpq前端
4. **简化配置**: 使用标准`client_config.xml`

## 待完成工作
1. 同智伟业SDK实际集成
2. 生产环境适配（连接池、故障转移）
3. 完整集成测试
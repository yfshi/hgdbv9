# 三未信安 (SWXA) HSM 适配器概要设计

## 1. 概述

三未信安密码机采用国密标准 SDF (SM2/SM3/SM4) 接口规范，提供硬件加速的密钥管理和密码运算服务。本适配器的目标是将三未信安密码机集成到 TECC 的密钥管理体系中，支持：

- `swxakms` KEY_STORE 类型
- CREATE KEK 时检查密码机中密钥是否存在
- CREATE DEK 时使用密码机生成和加密 DEK
- 列加密时使用密码机进行 SM4 加密/解密

## 2. API 映射

三未信安 SDF 接口定义在 `swsds.h`（`src/include/hsm/swsds.h`），驱动库为 `/usr/local/lib/libswsds.so`。

### 2.1 核心 API 映射

| HsmAdapterInterface | swsds.h API | 说明 |
|---|---|---|
| `init` | `SDF_OpenDeviceWithPathAndName` | 通过 ini 配置文件打开设备 |
| `connect` | `SDF_OpenSession` | 打开会话 |
| `disconnect` | `SDF_CloseSession` | 关闭会话 |
| `cleanup` | `SDF_CloseDevice` | 关闭设备 |
| `check_key_exists` | `GetKeyStatus` | 检查指定索引的密钥是否存在 |
| `encrypt` | `SDF_Encrypt` | 对称加密 |
| `decrypt` | `SDF_Decrypt` | 对称解密 |
| `sm4_encrypt` | `SDF_Encrypt` (SGD_SMS4_CBC) | SM4-CBC 加密 |
| `sm4_decrypt` | `SDF_Decrypt` (SGD_SMS4_CBC) | SM4-CBC 解密 |

### 2.2 KEK 管理 API

| 操作 | swsds.h API | 说明 |
|---|---|---|
| 检查 KEK 是否存在 | `GetKeyStatus(session, SGD_SMS4, keyIndex, &status)` | status=1 表示存在 |
| 用 KEK 生成 DEK | `SDF_GenerateKeyWithKEK(session, 256, SGD_SMS4_CBC, kekIndex, key, &keyLen, &hKey)` | 生成密钥并用 KEK 加密输出 |
| 用 KEK 加密数据 | `SDF_Encrypt(session, hKey, algID, iv, data, &encData)` | — |
| 用 KEK 解密数据 | `SDF_Decrypt(session, hKey, algID, iv, encData, &data)` | — |

## 3. 文件结构

```
src/interfaces/libpq/tecc_hsm_adapter/
├── fe-swxa-adapter.h               # SWXA 适配器接口
├── fe-swxa-adapter.c               # SWXA 适配器实现
├── fe-hsm-adapter.h                # 已有（添加 get_swxa_adapter 声明）
├── fe-hsm-adapter.c                # 已有（添加 get_swxa_adapter 导出）
├── SWXA_HSM_ADAPTER_DESIGN.md      # 本文档（概设）
├── SWXA_HSM_ADAPTER_DETAIL.md      # 详细设计
└── HSM_ADAPTER_IMPLEMENTATION.md   # 已有

src/include/hsm/
└── swsds.h                         # 三未信安 SDF 头文件
```

## 4. 配置机制

- **`swsds.ini`**：用户放置在 `$PGDATA/` 目录下，适配器自动加载（IP/端口/密码等）
- **`KEY_PATH`**：与 `hsmkms` 相同的 `hsm:id=<index>` 格式，复用 `parse_hsm_key_path()` 解析索引

```sql
CREATE KEK my_kek WITH (
    KEY_STORE = swxakms,
    KEY_PATH = 'hsm:id=1'
);
```

## 5. 处理流程

### 5.1 CREATE KEK

```
用户输入: CREATE KEK kek1 WITH (KEY_STORE = swxakms, KEY_PATH = 'hsm:id=1')
    │
    ├─ 解析 KEY_PATH → parse_hsm_key_path("hsm:id=1") → key_index = 1
    │
    ├─ 获取 SWXA 适配器 → get_swxa_adapter()
    │
    ├─ adapter->init()
    │   ├─ 构造 $PGDATA/swsds.ini 路径
    │   └─ stat() 校验文件存在
    │
    ├─ adapter->connect()
    │   ├─ SDF_OpenDeviceWithPathAndName(ini_path, &hDevice)
    │   └─ SDF_OpenSession(hDevice, &hSession)
    │
    ├─ adapter->check_key_exists(1)
    │   └─ GetKeyStatus(hSession, SGD_SMS4, 1, &status) → status==1?
    │
    ├─ adapter->disconnect() + cleanup()
    │
    └─ 成功/失败返回
```

### 5.2 CREATE DEK

```
用户输入: CREATE DEK dek1 WITH (KEK = kek1)
    │
    ├─ 从目录表获取 KEK 信息（KEY_STORE=swxakms, KEY_INDEX=1）
    │
    ├─ init → connect → SDF_GenerateKeyWithKEK → destroyKey → disconnect → cleanup
    │
    └─ 加密后的 DEK 作为 ENCRYPTED_VALUE 注入 SQL → 发送到后端
```

### 5.3 列加解密

```
写入：明文 → init → connect → SDF_Encrypt(CBC) → disconnect → cleanup → 密文入库
读取：密文 → init → connect → SDF_Decrypt(CBC) → disconnect → cleanup → 明文返回
```

## 6. 驱动集成

```makefile
# src/interfaces/libpq/Makefile

# 头文件（已有）
override CPPFLAGS += -I$(top_srcdir)/src/include/hsm

# 新源文件
OBJS += tecc_hsm_adapter/fe-swxa-adapter.o

# 链接库
SHLIB_LINK += -lswsds
```

## 7. 实现任务

| 阶段 | 任务 | 文件 | 优先级 |
|---|---|---|---|
| 一 | 复制 swsds.h 到 src/include/hsm/ | — | 高 |
| 一 | fe-swxa-adapter.h 接口定义 | 新增 | 高 |
| 一 | 适配器实现（init/connect/disconnect/cleanup/check_key_exists） | fe-swxa-adapter.c | 高 |
| 一 | create_kek_processor.c 增加 swxakms 分支 | 已有 | 高 |
| 一 | Makefile 集成 | Makefile | 高 |
| 二 | SDF_GenerateKeyWithKEK 封装 | fe-swxa-adapter.c | 中 |
| 二 | CREATE DEK 集成 | create_dek_processor.c | 中 |
| 三 | SDF_Encrypt/SDF_Decrypt 封装 | fe-swxa-adapter.c | 低 |
| 三 | 列加密/解密集成 | 后端加密模块 | 低 |

## 8. 使用示例

```bash
# 准备
cp /path/to/swsds.ini $PGDATA/swsds.ini

# CREATE KEK
CREATE KEK my_kek WITH (KEY_STORE = swxakms, KEY_PATH = 'hsm:id=1');

# CREATE DEK
CREATE DEK my_dek WITH VALUES (KEK = my_kek);

# 创建加密表
CREATE TABLE t1 (id int, name text_enc ENCRYPTED WITH (dek = my_dek));
```

## 9. 设计决策

| 决策 | 选择 | 理由 |
|---|---|---|
| KEY_PATH 格式 | `hsm:id=<index>` | 与 hsmkms 一致，复用 parse_hsm_key_path |
| ini 配置方式 | 自动从 `$PGDATA/swsds.ini` 加载 | 用户只需放文件，SQL 中不传路径 |
| 密钥存在检查 | `GetKeyStatus` 扩展接口 | 标准 SDF_GetKeyStatus 不支持按索引查询 |
| 设备打开 | `SDF_OpenDeviceWithPathAndName` | 扩展接口，支持传配置文件路径 |

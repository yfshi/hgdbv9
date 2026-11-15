# SoftSDF 软件 SDF 设备集成方案

## 1. 背景与目标

TECC 的硬加密场景（swxakms/tzwykms）依赖真实密码机，前期调测长期受制于无密码机环境。为此引入开源 SoftSDF（GmSSL 生态，Apache-2.0）作为软件 SDF 设备模拟器，vendored 到 `tecc_kms/`，编译产出 `libteccsdf.so`，以软件 stub 形式实现标准 SDF 接口，供无密码机环境下的联调与回归测试。

本方案的目标：让 swxa 适配器（运行时 `dlopen` 机制）能够加载 `libteccsdf.so` 替代真实三未信安加密机 `libswsds.so`，覆盖对称 SM4 场景（RND/DET/OPE/SEARCH 的 KEK/DEK 加解密）。

**范围界定**：按约定本次只接 swxa，不接 tzwy；HE（sm2）场景暂不涉及。

## 2. 关键前提（接口对齐结论）

经核实，swxa 适配器与 SoftSDF 在关键维度上天然兼容，集成成本很低：

| 项 | swxa 适配器 | SoftSDF | 结论 |
|---|---|---|---|
| 句柄类型 | `void *`（自定义 SwxaSdfApi） | `void *` | 无差异，无需桥接层 |
| 算法常量 | `SGD_SMS4_CBC = 0x00000402` | `SGD_SM4_CBC = 0x00000402` | 数值一致，运行时兼容 |
| 裸导入 | `SDF_ImportKey`（4 参，厂商扩展） | 已补齐 | 已就绪 |
| 加解密/随机数/会话/销毁 | 标准 SDF | 已具备 | 已就绪 |

注：README 中的「桥接适配层」是针对 tzwy 的 `long long` 句柄（`sdf_client.h`）而言的，不适用于 swxa 的 `void *` 句柄。因此本次只需补齐 2 个 swxa 厂商扩展接口，无需桥接层。

## 3. 总体设计

### 3.1 libteccsdf.so 安装路径

真实加密机 so 的标准位置是 `/usr/lib64`（`libsdf.client.so` 的 configure 默认查找目录、`libswsds.so` 的 dlopen 回退目录）。为便于统一管理，`libteccsdf.so` 也安装到 **`/usr/lib64/libteccsdf.so`**，与 `libsdf.client.so`、`libswsds.so` 同目录。

`tecc_kms/build/CMakeLists.txt` 增加 install 规则，`LIBRARY DESTINATION` 默认 `lib64`（可用 `CMAKE_INSTALL_LIBDIR` 覆盖，兼容 `/usr/local/lib64` 等）。`teccsdfinit`、`teccsdftest` 一并安装。

### 3.2 加载切换（环境变量 TECC_SDF_LIB）

`swxa_load_sdf_api()` 在现有 `dlopen("/usr/local/lib/libswsds.so")` 之前，优先检查环境变量 `TECC_SDF_LIB`：

- `TECC_SDF_LIB=/usr/lib64/libteccsdf.so` → 加载 SoftSDF；
- 未设置 → 维持现有 `libswsds.so` 默认顺序，不影响真实环境。

## 4. 接口补齐（2 个 swxa 厂商扩展接口）

swxa `dlsym` 列表中 SoftSDF 缺失的接口：

1. `SDF_OpenDeviceWithPathAndName(char *cfg, void **phDevice)`：包装标准 `SDF_OpenDevice`，忽略 `cfg` 配置路径；
2. `SDF_GetSymmKeyHandle(session, key_index, phKeyHandle)`：模拟加密机内预置对称 KEK——读 `kek-<index>.key` 文件（由 `teccsdfinit -kek <index>` 预生成）加载到 `session->key_list` 返回句柄。

两者实现方式均复用 SoftSDF 现有 KEK 文件机制（`SDF_GenerateKeyWithKEK` 内部已读 `kek-<index>.key`），与 SoftSDF 既有模型、与真实加密机预置模型一致。

## 5. 预置 KEK 模拟

真实 swxa 加密机的 KEK 是「预置」的：管理员用加密机管理工具把对称密钥注入到密码机某个 index，客户端 `CREATE KEK ... key_path='hsm:id=N'` 时仅 `check_key_exists(N)` 校验，加解密时 `SDF_GetSymmKeyHandle(N)` 引用。客户端不生成、不导入 KEK。

SoftSDF 对应：`teccsdfinit -kek <index>` 生成 `kek-<index>.key`（16 字节随机密钥），等价于真实加密机的「管理员注入密钥」。`SDF_GetSymmKeyHandle` 读该文件。

预置密钥的来源分两类：

- **一般用例**：提交一组固定的 `kek-*.key` 到仓库（如 `tecc_kms/keys/kek-1.key` …），供回归用例直接引用；
- **KEK 密钥相关测例**（后续针对 SoftSDF 新增）：由测试脚本现场跑 `teccsdfinit` 生成。

## 6. 配置检测放宽

`swxa_init()` 目前强制要求 `swsds.ini`（真实加密机配置）。SoftSDF 模式（`TECC_SDF_LIB` 指向 `libteccsdf.so`）下跳过该检查。

## 7. 回归测试识别 softsdf 模式

`setup_swsds_ini()` 增加对 `TECC_SDF_LIB` 的识别：设置且 `libteccsdf.so` 存在 → 启用 `swxa_enc.sql`/`swxa_search.sql` 等用例；否则按现有逻辑 skip。

## 8. 实施步骤

1. tecc_kms：补齐 `SDF_OpenDeviceWithPathAndName` + `SDF_GetSymmKeyHandle`，更新 `sdf.h`/`softsdf.lds`/`include/`，CMakeLists 加 install 规则；
2. 编译 `libteccsdf.so` 并安装到 `/usr/lib64`；
3. `fe-swxa-adapter.c`：`swxa_load_sdf_api` 加 `TECC_SDF_LIB` 优先，`swxa_init` 放宽 ini 检查；
4. 提交固定 `kek-*.key` 到仓库；
5. `tecc_test_utils.pm`：`setup_swsds_ini` 识别 softsdf 模式；
6. 手动 psql 验证（CREATE KEK swxakms + RND/DET 加解密）；
7. 回归用例 + 全量回归。

## 9. 风险与注意

- SoftSDF 是软件 stub，SM4 加解密、随机数均为真实算法实现（基于 GmSSL），但无真实密码机的物理安全边界，仅用于调测与回归，不用于生产；
- `kek-*.key` 为明文密钥文件，仅用于测试，仓库中提交的固定密钥仅作测试用途；
- SoftSDF 与真实 swxa 的差异仅在于厂商扩展接口的语义模拟，需在后续真实密码机联调时验证行为一致性。

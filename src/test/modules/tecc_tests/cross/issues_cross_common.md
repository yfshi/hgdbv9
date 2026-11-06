# 跨驱动（cross）测试问题记录

## 当前测试内容

cross/sql/ 下的 3 个 SQL 文件覆盖全类型 × 全算法：

- **001_create_table.sql**: 创建 KEK/DEK（localkms）+ 4 张加密表
  - cross_rnd: 9 种密态类型，RND（默认算法）
  - cross_det: int4/float8/text/varchar，DET
  - cross_ope: int4/numeric/text，OPE
  - cross_search: text/varchar，SEARCH
- **002_insert.sql**: 向各表插入测试数据
- **003_select.sql**: 全量查询 + DET等值查询 + OPE范围/排序 + SEARCH模糊匹配 + 清理

## 测试方式

`make check-tecc-cross` 启动全新 PG 实例，两轮交替执行：
- Round 1: 001→libpq, 002→jdbc(psql), 003→libpq
- Round 2: 001→jdbc(psql), 002→libpq, 003→jdbc(psql)
- 无 expected 文件时当作空串比对，输出实际内容供分析

## 当前状态（2026-05-24）

**测试结果**: 6 测试中 5 失败，1 通过（rev[0] 因 Round 2 全失败 → 输出为空匹配空预期）

### 实际输出摘要

**Round 1 001 (libpq)** — ✅ 成功
```
DROP KEK
CREATE KEK
CREATE DEK  (×4)
DROP TABLE  (×4)
CREATE TABLE (×4)
```
KEK/DEK/表全部创建成功。KEK args 有 KEY_STORE=localkms 和 KEY_PATH。

**Round 1 002 (jdbc/psql)** — ❌ 全失败
```
insert done  ← 只有最后的 SELECT 'insert done' 成功
```
所有 INSERT 语句失败，错误：
```
Failed to decrypt DEK 'dek_rnd'
TECC query preprocessing failed: Encryption failed: Cannot encrypt: Failed to decrypt DEK 'dek_rnd'
```
DEK 用 KEK 解密时参数无效。

**Round 1 003 (libpq)** — ❌ 数据为空
```
=== RND ALL ===
=== DET EQUAL ===
=== OPE RANGE ===
=== OPE ORDER ===
=== SEARCH LIKE ===
DROP TABLE
DROP DEK
DROP KEK
```
SELECT 无数据（因 002 未插入）。清理语句（DROP TABLE/DEK/KEK）执行成功。

**Round 2 所有** — ❌ 全空
因 Round 1 的 003 清理成功，Round 2 001 重新创建一切，但 002 INSERT 同样失败。

## 已分析的问题

### 问题 #1（已解决）: HSM 反射警告
**现象**: 启动时 `WARNING: Failed to read SymAlgFlag.SGD_SM4_ECB via reflection`
**根因**: `TzwyHsmDeviceManager` 的 `readAlgField()` 是 `static final` 字段，在类加载时执行反射，无 HSM JAR 时 ClassNotFoundException。
**修复**: 改用内部静态类 `AlgFlagHolder` 懒加载，仅在首次调用 `sgdSm4()` 时触发反射。
**涉及文件**: `TzwyHsmDeviceManager.java`
**状态**: ✅ 已修复并提交

### 问题 #2（待分析）: INSERT 加密列失败 — Failed to decrypt DEK
**现象**: 所有加密 INSERT 均报 `Failed to decrypt DEK 'dek_xxx'`
```
[tecc_key/high_level_api.c:171 tecc_key_decrypt_dek] Invalid parameters for DEK decryption
```
**错误链**: `tecc_key_manager_adapter.c:442` → `high_level_api.c:488 tecc_key_get_kek_with_state()` → `tecc_key_decrypt_dek` 返回参数无效

**已确认能正常工作的环节**:
1. CREATE KEK → `hg_kek` 有记录 ✅
2. KEK args → `hg_kek_args` 有 KEY_STORE + KEY_PATH（bytea 格式） ✅
3. KEK key file → `/tmp/kek_cross.key` 已生成 ✅
4. CREATE DEK → `hg_dek` 有记录，ENCRYPTED_VALUE 已生成 ✅
5. CREATE TABLE → `hg_encrypted_columns` 有记录 ✅

**分析**:
- DEK 解密失败可能有以下原因：
  a. `tecc_key_get_kek_with_state()` 加载 KEK 明文成功但长度为 0
  b. DEK 的 `encrypted_key` 格式与 `tecc_key_decrypt_dek` 期望的格式不匹配
  c. CREATE DEK 时前端生成的 ENCRYPTED_VALUE 与后端存储的格式不一致
- 需要注意：KEK key file `/tmp/kek_cross.key` 格式为 `TECCKEK...`（148 字节）

**复现**: `make check-tecc-cross`
**涉及文件**:
- `src/common/tecc_key/high_level_api.c` — `tecc_key_decrypt_dek`
- `src/common/tecc_key/high_level_api.c:488` — `tecc_key_get_kek_with_state`
- `src/interfaces/libpq/tecc_scheduler/tecc_key_manager_adapter.c:442` — 错误报告点

### 问题 #3（待分析）: Round 2 DROP KEK CASCADE 失败
**现象**: Round 2 001 中 `DROP KEK IF EXISTS kek_cross CASCADE` 报错：
```
ERROR: cannot drop key encryption key "kek_cross" because encrypted columns depend on it
```
**根因**: Round 1 的 003 清理未执行（因 INSERT 失败），导致 KEK/DEK/表在 Round 2 时仍存在。
DROP KEK CASCADE 检查到 `hg_encrypted_columns` 仍有记录指向 DEK，拒绝删除。

**注意**: `DROP TABLE ... CASCADE` 本身功能正常——能正确清理 `hg_encrypted_columns` 记录。
但 `DROP KEK ... CASCADE` 在 dropkek.c 中有保护检查，如果 DEK 仍有加密列引用（通过查询 `hg_encrypted_columns`），即使 CASCADE 也会拒绝删除。

**状态**: 待问题 #2 修复后自动解决（003 的清理能正常执行后，Round 2 即可正常重跑）

### 关于 `-C on` 框架改动
PSQL 的 TECC 加密模式默认已启用（`fe-connect.c:347` 中 `enable_tecc` 默认值为 `"true"`），所有框架中的 `-C on` 已移除：
- `tecc_test_utils.pm` 中 3 处 `'-C', 'on'` 已删除
- `001_tecc_libpq_test.pl` 注释中的 `-C` 已更新
- 共享用例中 `-C on` 本身未在 SQL 中出现（仅在 TAP 脚本中）

### 预期文件生成机制改造
所有回归测试中，当 expected 文件不存在时，行为改为：当作空串比对，让测试自然失败并输出实际内容，而不是直接 fail 跳过。涉及文件：
- `003_tecc_cross_test.pl` — `run_step`
- `002_tecc_jdbc_test.pl` — raw 和 common 比较
- `tecc_test_utils.pm` — `run_sql_diff_test` 和 common 比较

## 环境信息

- PG源码: `/home/wxp/tecc4_on_pg14` (branch `tecc4-feature-pg14`)
- JDBC源码: `/home/wxp/pgjdbc-release-42.5.x` (branch `tecc4-feature-pg14`)
- 构建: `make -j$(nproc) && make install`（或 `make check-*` 自动 temp-install）
- TAP测试: `make check-tecc-cross` 自动编译安装 + 启动临时 PG 实例

## 框架改造记录（2026-05-24 第二次）

### cross 测试改用真实 JDBC 交替执行

`003_tecc_cross_test.pl` `run_step` 增加 `$driver` 参数：
- `libpq` → `psql -X -A -t`，合并 stderr
- `jdbc` → `TeccSqlTestRunner --output-format psql`，去除 `>>>` 头部，合并 stderr
- 两端统一 `normalize_common_output()` 后比对 expected

启动时校验 JDBC 环境（`TECC_JDBC_HOME`、jar、test classes），缺一即 `skip_all`，无降级。

### normalize_common_output() 公共归一化函数

`tecc_test_utils.pm` 新增导出函数，消除 `run_sql_dir_libpq_dual_diff()` 和 `002_tecc_jdbc_test.pl` 中的 inline 重复代码：

1. Hex 密文隐藏（完整 hex `E'\\x...'` + 截断 hex `...'::int4_enc`）
2. psql 路径缩短 + NOTICE/WARNING/ERROR 前缀归一化
3. 去除尾部空行

### TeccSqlTestRunner PSQL 模式进度噪音修复

`TeccSqlTestRunner.java` 中 `[mode] 完成, N 条语句` 和 `\copy` 进度输出改为仅在非 PSQL 模式打印。

## 最新测试结果（2026-05-24 第二次，框架验证）

**环境**: `make check-tecc-cross`，全新 temp-install + 临时 PG 实例
**TECC_JDBC_HOME**: `/home/wxp/pgjdbc-release-42.5.x`

| 用例 | 驱动 | 结果 | 说明 |
|------|------|------|------|
| fwd[0] 001_create_table | libpq | 内容正确 | KEK/DEK/表全部创建成功，因无 expected 文件判定失败 |
| fwd[1] 002_insert | jdbc | **23 条 INSERT 全部成功** | 输出 `INSERT 0 1` × 23 + `insert done` |
| fwd[2] 003_select | libpq | SELECT 返回 0 行 | 数据未读到 |
| rev[0] 001_create_table | jdbc | 内容正确 | JDBC 创建 KEK/DEK/表成功 |
| rev[1] 002_insert | libpq | 全部失败 | `scheduler returned no result`（同问题 #2） |
| rev[2] 003_select | jdbc | SELECT 返回 0 行 + 清理成功 | 因 rev[1] 未插入数据 |

**框架验证结论**:
- ✅ libpq/JDBC 交替执行正常，正向/反向轮次按预期分配驱动
- ✅ TCP/IP 连接正常（`listen_addresses` + `pg_hba.conf` 配置）
- ✅ TeccSqlTestRunner 通过 JDBC 成功执行 SQL
- ✅ stderr 合并输出正常（NOTICE/WARNING/ERROR 全部捕获）
- ✅ normalize_common_output 归一化正常工作
- ❌ 6/6 失败（均因无 expected 文件，空串比对）

### 问题 #4（新发现）: JDBC INSERT 成功但 libpq SELECT 查不到数据

**现象**: fwd[1] JDBC 执行 23 条 INSERT 全部返回 `INSERT 0 1`，但 fwd[2] libpq 的 SELECT 返回 0 行。

**可能原因**:
- JDBC 侧的加密结果未正确持久化到存储层（尽管 `INSERT 0 1` 表明服务器接受了数据）
- JDBC 和 libpq 对 DEK 的解密路径不同，导致 libpq 无法读取 JDBC 加密的数据
- 或 temp-install 未包含最新的 C 代码改动（但 JDBC jar 是独立编译的）

**与问题 #2 的关系**: 问题 #2 中 libpq INSERT 全线失败。现在发现 JDBC INSERT 能成功，说明 DEK/KEK 本身是可用的，问题可能出在 libpq 端的加解密调度器初始化（`scheduler returned no result`）。

**涉及文件**:
- `src/interfaces/libpq/tecc_scheduler/tecc_crypto_scheduler.c:643` — `Failed to get crypto scheduler from connection`

### 问题 #5（新发现）: rev[0] JDBC 建表后 rev[1] libpq INSERT 同样失败

**现象**: 即使 KEK/DEK/表全部由 JDBC 创建，后续 libpq 的 INSERT 仍然失败，错误与问题 #2 一致。

**意义**: 排除了 "libpq 创建的 DEK 格式不对" 的可能性。JDBC 创建的 DEK 同样被 libpq 拒绝，说明问题在 libpq 端自身，与 DEK 来源无关。

---

## 2026-05-30：跨驱动解密失败根因分析

### 已验证对齐的组件 ✓

| 组件 | libpq | JDBC | 结论 |
|------|-------|------|------|
| SM4-CBC | tecc_sm4_internal.c | Sm4Cipher.java | 相同key+IV+plain→相同cipher ✓ |
| PBKDF2-HMAC-SHA256 | OpenSSL PKCS5_PBKDF2_HMAC | 自定义 pbkdf2HmacSha256 | 相同输入→相同输出 ✓ |
| KEK文件格式 | TECCKEK magic+PBKDF2+SM4-CBC+HMAC | encryptLibpqKekFile/decryptLibpqKekFile | 一致 ✓ |
| HKDF-SM3 | tecc_hkdf_sm3.c | HkdfSm3.java | 参数一致 ✓ |
| RND数据格式 | version(4BE)+auth_tag(32)+iv(16)+ciphertext_len(8LE)+data | 同上 | 对齐 ✓ |
| CDT容器格式 | 28B header(vl_len_+ver+dek_dist_oid+rnd/det/ope/search_len) | 同上 | 对齐 ✓ |

### 关键发现：KEK文件跨驱动不兼容

两边 KEK_PLAINTEXT 值在同一次运行中不同：

| 来源 | KEK 明文 | 产生方 |
|------|----------|--------|
| libpq (fwd) | 25B1D4216726051E94AD0D279D5D0551... | libpq 创建 |
| JDBC (rev) | 26954175D05908365BD99DAFA7765BF8... | JDBC 创建 |

虽然各边自洽，但相互不兼容——说明两边对 KEK 文件的格式理解不统一。

### 其他发现

**JDBC `AES_KEY_BYTES=16` 截断问题** (`LocalKMSEncryptionProvider.java:755-756`):
```java
byte[] key = new byte[AES_KEY_BYTES];  // 16
System.arraycopy(raw, 0, key, 0, AES_KEY_BYTES);  // 截断32→16
```
但实际加密走**预派生密钥路径**（derivedEncKey/derivedAuthKey 来自完整32B DEK），此截断不影响主线流程。仅在没有预派生密钥的回退路径中会出问题。<"/parameter>

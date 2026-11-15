# TECC 回归测试问题记录

> 更新日期: 2026-05-27

---

## 已修复问题

### P1-1: OPE文本INSERT全部静默失败 — 连接池泄漏 ✅
- 提交: `21de2387ea`
- SQL: `ope.sql`
- 根因: 连接池耗尽导致所有新连接无法获取KEK，文本类型OPE加密全部静默失败
- 修复: 连接池泄漏修复

### P1-2: COPY测试ORDER BY on DET失败 ✅
- 提交: `44da2fac1c`
- SQL: `copy_test.sql`
- 根因: DET加密不支持ORDER BY，3个SELECT全部失败（错误进stderr未被捕获）
- 修复: 去掉SELECT中的ORDER BY id

### P1-3: cache.sql权限测试未切换用户 ✅
- 提交: `095d90c3d1`
- SQL: `cache.sql` 第4节
- 根因: GRANT后没有`\c`重新连接为无权限用户，所有操作仍是超级用户
- 修复: 添加`\c postgres test_noperm_user`切换，创建已知超级用户`test_postgres`切回

### P1-3b: `\c`输出一致性 ✅
- 提交: `095d90c3d1` (与上面相同)
- SQL: `cache.sql` 第4节
- 根因: 用`:super_user`变量做`\c`目标导致连接消息中的用户名随环境变化
- 修复: 统一用确定角色名`test_postgres`

### P2-1: 前端加密类型输入校验 ✅
- 提交: `f26250b519`
- SQL: `crypto_verify.sql` 第4节
- 覆盖类型: int2_enc, int4_enc, int8_enc, float4_enc, float8_enc, numeric_enc
- 校验规则:
  - INTEGER值: int2范围[-32768, 32767], int4范围[-2147483648, 2147483647]
  - FLOAT值: 拒绝赋值给整数加密类型（int2_enc/int4_enc/int8_enc）
  - STRING值: 解析后做范围/格式校验
- 技术方案: 通过`raw_value->type_oid`传递基础类型OID，`get_enc_type_name_from_base_oid()`纯静态映射查表

### P3-1: 测试框架 raw 模式错误走 SEARCH 剥离逻辑 ✅
- 提交: 待提交
- 代码: `tecc_test_utils.pm` `run_sql_dir_libpq_dual_diff()` raw 分支
- 根因: raw 比较调用 `run_sql_diff_test` 时传了 `$sql_base`，触发 `strip_non_deterministic_rows()`，违背 raw 模式语义
- 修复: raw 分支不再传 `$sql_base`

### P3-2: SAFETY CHECK 日志含不确定性行号 ✅
- 提交: 待提交
- 代码: `fe-tecc-integration.c`, `tecc_logging.h`
- 根因: `TECC_LOG` 宏输出 `__FILE__:__LINE__`，源码行号变化导致所有含 SAFETY CHECK 的 expected 文件失效
- 修复: 新增 `TECC_LOG_MSG` 宏（不含文件行号），SAFETY CHECK 改用此宏
- 影响用例: `cache.sql`, `crypto_verify.sql`, `grammar.sql`

---

## 已知问题（待修复）

### \copy + TECC 交互问题 ⚠️
- SQL: `copy_test.sql`
- 现象: `\copy test_copy_enc TO` 输出密文二进制（非明文），`\copy FROM` 报 "invalid hex string"
- 根因: \copy 在 TECC 开启时未正确处理加解密
- 当前: 服务端 COPY TO/FROM 正常工作，\copy 路径不处理

### 测试框架不支持stderr验证 ⚠️
- SQL: 涉及ERROR预期的所有用例
- 现象: 框架只比较stdout，预期错误测试无法验证错误是否实际发生
- 需要: 新增stderr断言机制

### SEARCH LIKE/NOT LIKE Heisenbug — 间歇性匹配失败 ⚠️

> 分析日期: 2026-05-26 ~ 2026-05-27
> 提交: `e68b7106ab6` (三层修复)
> 状态: 三层确定性修复已提交验证通过，但间歇性根因仍未定位

---

#### 问题现象

`make check-tecc-libpq-raw` 全套回归时，`crypto_verify.sql` 或 `det.sql` 的 SEARCH LIKE 查询
随机返回 0 行。单测（`TECC_TEST=xxx`）或直接 psql 单文件均正常。

最小复现 (`/tmp/test_like_nodebug.sql`):
```sql
DROP KEK IF EXISTS kek_det CASCADE;
CREATE KEK kek_det WITH (key_store = 'localkms', key_path = '/tmp/kek_det.key');
CREATE DEK dek_det WITH VALUES(KEK = kek_det);
DROP TABLE IF EXISTS tecc_test CASCADE;
CREATE TABLE tecc_test (id INT4, name TEXT_ENC, col_text TEXT_ENC
    ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'DET,SEARCH'));
INSERT INTO tecc_test VALUES (1, 'Alice', 'Apple'), (2, 'Bob', 'Banana'),
    (3, 'Charlie', 'Cherry'), (4, 'David', 'Date'), (5, 'Eve', 'Elderberry');
SELECT '5f_like_ana', id, name, col_text FROM tecc_test
    WHERE col_text LIKE '%ana%' ORDER BY id;
SELECT '5g_like_erry', id, name, col_text FROM tecc_test
    WHERE col_text LIKE '%erry%' ORDER BY id;
SELECT '5h_like_e', id, name, col_text FROM tecc_test
    WHERE col_text LIKE '%e%' ORDER BY id;
```

循环 100 次，原始失败率 ~10%，三层修复后 ~3%。失败仅出现在 `%erry%` 和 `%ana%`，
`%e%`（3字节）100次全通过。

---

#### 已提交的三层修复 (`e68b7106ab6`)

##### 1. varlena 1字节头导致 CIPHERDATA 宏偏移错位 ✅ 已确认生效

**根因**: CipherDataType < 126 字节时 PG 用 1 字节 varlena header，VARDATA_ANY 返回
ptr+1 而非 ptr+4。所有 CIPHERDATA_DATUM_* 宏字段读偏移差 3 字节，SEARCH_LEN 读到
垃圾值。LIKE pattern CipherDataType 很小（`%e%` 仅 33 字节），必然触发。

**修复**:
- `src/backend/utils/tecc/cipherdatatype.c`: SET_VARSIZE → SET_VARSIZE_4B
- `src/backend/utils/tecc/cipherdata_operators.c`: 9 处 PG_GETARG_POINTER → PG_DETOAST_DATUM

**验证**: 
- 解析服务端存储的 raw cipherdata，所有行 vl_len_ 低 2 位为 0 → 全 4 字节头
- 服务端调试日志中 LIKE pattern 的 pat_raw 结构正确（如 `%e%` → `259fa5de25`），
  若 1 字节头则 SEARCH_PTR 偏移错 3 字节，读出的应是垃圾

**结论**: 此修复对 LIKE pattern 参数（< 126 字节，旧代码用 1 字节头）起关键作用。
但此 bug 是确定性的——同一 pattern 同一大小同一头类型，不能解释间歇性。

##### 2. SM3 哈希值与 LIKE 通配符碰撞 ✅ 已确认生效

**根因**: SM3(char||key) 输出 3 字节可随机等于 `%`(0x25) 或 `_`(0x5F) 或 `\`(0x5C)，
概率 ~2/256 ≈ 0.78%/字节。bytealike 无法区分真实通配符和哈希碰撞产生的 `%`/`_`。

**修复**:
- `src/common/tecc_algorithm/tecc_search_sm4.c`: 碰撞字节（等于 `%`/`_`/`\`）前缀 `\` 转义，
  真实通配符不转义
- `src/backend/utils/tecc/cipherdata_operators.c`: cipherdata_like/nlike 中解转义 str 和 pat 双方，
  然后传 bytealike

**验证** (逐字节，服务端调试日志):
- `\` 碰撞 (hash('B')=5ac25c): str_raw=5ac2**5c5c**66 → str_unesc=5ac2**5c**66 ✓
- `_` 碰撞 (hash('L')=b8ce5c5f): str_raw 包含 5c5f → unesc 恢复 5f ✓
- `%` 碰撞 (新密钥下 hash('#')=34d825): str_raw=34d8**5c25** → str_unesc=34d8**25** ✓
- LIKE pattern 中真实通配符不转义: pat_raw `%ana%`=`25`66ac5e...`25`（25 独立出现）✓
- bytealike(unesc_str, unesc_pat) 对三种碰撞全部匹配正确 ✓
- 专用测试 `test_collision_targeted.sql` (L/_ 碰撞) 和 `test_pct_collision_verify.sql`
  (% 碰撞) 全部 PASS

**碰撞随密钥变化**: hash=SM3(char||key)，换密钥则碰撞字符不同。
原密钥 (dek_det): `_`→'L', `\`→'U', `%`→无。
新密钥 (dek_pct_test): `%`→'#', `_`→无, `\`→无。

**结论**: 转义机制正确工作。但同一密钥下碰撞是确定的（同字符同hash），
不能解释间歇性。

##### 3. LIKE 模式的 raw_value 未加入 raw_values_list ❓ 实际非 bug

旧代码在 `where_clause_processor.c:process_comparison` 中 AEXPR_LIKE 路径创建
raw_value 加密后直接 `tecc_raw_value_destroy`，注释称 "process phase already added it"。

**进一步分析**: process phase（`where_process_*` 函数，line ~1494）遍历 Const 节点时
已经将 LIKE pattern 加密并加入 raw_values_list。process_comparison 中创建的是冗余 copy，
销毁不影响 SQL 改写。原注释正确，此修改不是 bug 修复，仅避免冗余加密。

---

#### 排除的其他可能

##### SEARCH 算法本身
- 算法确定（无 IV/random）：`sm3_hash_bytes()` 接收 pos_bytes 但未使用，
  hash 仅含 char+key，无位置信息。无 RAND_bytes/rand/random 调用。
- 不同字符密文碰撞：3 字节=24 位=1677 万种。100 个字符生日悖论碰撞 ~0.03%，
  不太可能是 3% 失败的主因。
- `_` 通配符粒度不匹配：bytealike 中 `_` 匹配 1 字节，但 SEARCH 每字符 3 字节。
  `LIKE 'a_'` 只匹配 1+1/3 个字符。这是已知缺陷但测试用 `%` 不用 `_`，不相关。

##### cipherdata_in 服务端解析
`cipherdata_in` 总是 `ptr += sizeof(int32)` 跳过 4 字节 varlena。客户端旧代码
SET_VARSIZE 创建 1 字节头时，hex 数据小 3 字节，服务端大小校验
`decoded_len != CIPHERDATA_FIXED_SIZE + ... + search_len` 会失败 → ERROR。
这是确定性的，不产生间歇性 0 行返回。

##### 连接/密钥一致性
INSERT 和 SELECT 共用 `tecc_search_sm4_encrypt`，同一连接同一密钥，加密结果一致。
排除密钥不一致导致匹配失败。

---

#### 未解决的间歇性根因

三层修复全部是**确定性**的——给定相同输入必然相同输出。但原始 bug 是 ~10% 间歇性的，
说明**真正的 Heisenbug 根因未被这三层修复触及**。

可能的间歇性根因方向:
1. **内存越界/use-after-free**: palloc/pfree 配对错误导致随机内存覆写
2. **未初始化内存读取**: 某处 palloc 后未 memset，读到随机值
3. **多连接状态泄漏**: 回归测试多连接场景下全局/静态变量污染
4. **其他未发现的代码路径**: CipherDataType 在某些路径仍用 1 字节头

建议后续排查手段:
- ASAN (`--enable-asan`) 构建运行回归
- 审查所有 CIPHERDATA_DATUM_* 宏调用点是否都已 detoast
- 检查 cipherdata_in/out/send/recv 等非操作符函数中的 CipherDataType 访问
- 使用 rr 录制失败运行回放调试

---

#### 测试环境

- PG 实例: `/home/wxp/pg14`, 端口 5432, 数据目录 `/home/wxp/pg14/data`
- 构建: `make -j && make install` (全量)
- 测试脚本: `/tmp/test_like_nodebug.sql`, `/tmp/test_collision_targeted.sql`,
  `/tmp/test_pct_collision_verify.sql`
- 原始密文查看: `PGENABLETECC=false psql ...`
- 密钥文件: `/tmp/kek_det.key`, `/tmp/kek_crypto.key`, `/tmp/kek_pct_test.key`

#### 受影响用例
| SQL | 现象 |
|-----|------|
| `crypto_verify.sql` 5b3/5b4 | 匹配返回空 |
| `det.sql` 5f/5g/5h/5n/5p | 匹配返回空 |
| `search.sql` | 待复现 |
| `express.sql` 3a/3b | 待复现 |
| `collation.sql` 3d/3f | 待复现 |

### TECC 客户端不支持 INSERT INTO ... SELECT（倍增路径不可用）⚠️

- SQL: `INSERT INTO enc_int4 ... SELECT ... FROM enc_int4 CROSS JOIN generate_series(...)`
- 现象: 乘法 SQL 被客户端 TECC 层拦截，不发送到服务端；回执 `0 rows` 无错误
- 根因: `fe-exec.c:1404-1422` — 处理器标记 `needs_rewrite=1`，但 `process_insert_select_typecast()` 发现 SELECT 目标列是 ColumnRef 而非 TypeCast 无法重写 → SAFETY CHECK 拦截 (`fe-tecc-integration.c:1116-1140`)
- 影响: 性能测试框架 `init_encrypted_data()` 的服务器端倍增路径完全不可用，任何 `INSERT INTO encrypted_table ... SELECT FROM encrypted_table` 都会触发此拦截
- 解决: 乘法阶段通过 `psql -C off` 临时禁用 TECC 客户端处理
- 状态: 待修复

### 缓存auto-recovery在DML路径触发数据库查询 ⚠️
- 代码: `tecc_cache_get_encrypted_column_by_name()` 缓存未命中时查数据库
- 风险: DML处理中触发数据库查询可能导致TECC处理器递归或性能问题
- 需要: 剥离auto-recovery逻辑，DML路径只读缓存

---

## 测试用例设计文档

### 共享用例13类分组
- SQL目录: `src/test/modules/tecc_tests/shared/sql/`
- Expected目录: `src/test/modules/tecc_tests/shared/expected/`
- 测试框架: TAP (`t/001_tecc_libpq_test.pl`, `t/tecc_test_utils.pm`)

#### 用例列表
| 文件名 | 覆盖内容 | 当前状态 |
|--------|---------|---------|
| cache.sql | 缓存/密钥/权限管理 | ✅ 已修复 (expected刷新) |
| collation.sql | ICU排序规则 | ⚠️ SEARCH匹配失败(3d/3f LIKE) |
| copy_test.sql | COPY/\\copy加解密 | ✅ 通过 |
| crypto_verify.sql | NaN/Infinity/边界值/全类型 | ✅ 已修复 (SAFETY CHECK格式对齐) |
| det.sql | DET等值比较 | ⚠️ SEARCH匹配失败(5f/5g/5h/5n/5p LIKE) |
| enc_indexes.sql | 加密索引 | ✅ 已修复 (EXPLAIN hex归一化) |
| enc_types.sql | 加密类型系统 | ✅ 通过 |
| express.sql | 表达式/布尔/CTE/子查询 | ⚠️ SEARCH匹配失败(3a/3b LIKE) |
| grammar.sql | 语法覆盖 | ⚠️ OPE normalization失败（其他差异均为预期行为） |
| number_order.sql | 数字类型ORDER BY | ✅ 通过 |
| ope.sql | OPE范围查询 | ✅ 通过 |
| pbe.sql | Prepared Statement | ✅ 通过 |
| search.sql | SEARCH模糊匹配 | ⚠️ SEARCH匹配失败(raw SKIPPED) |
| update_delete.sql | UPDATE/DELETE | ✅ 通过 |

### 独立测试
- ~~`test_only_insert`~~ — 原 libpq INSERT端到端测试 (通过 ✅)。**2026-09-20 退役**：
  只有一张无加密列的普通表 + 一次 INSERT/SELECT，两个驱动都测不到密态能力；
  随 `jdbc/` 目录解散一并归档到 `archive/jdbc_dir_retired_20260920/`。

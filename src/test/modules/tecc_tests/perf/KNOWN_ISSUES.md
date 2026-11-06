# TECC 性能测试已知问题

## 1. 新连接TECC缓存KEK查找失败（阻断性）

**现象**: 客户端TECC处理器无法解密DEK，WHERE加密全部失败。

```
tecc_key_decrypt_dek: Invalid parameters for DEK decryption
Failed to decrypt DEK: dek_perf
```

**根因**: KEK在缓存中查找失败，GUC未加载：
```
KEK 'kek_perf' not found in current_schema/search_path (GUC not loaded?)
```

TECC客户端缓存在新建连接后，需要通过 `SELECT current_schema()` 获取当前模式才能查找KEK。但新连接上这个GUC值未能正确加载，导致KEK在 `public` 模式中无法被找到→KEK key material为NULL→DEK无法解密→WHERE常量加密失败。

**影响**: 所有带WHERE条件的加密列查询（DET等值/OPE范围/SEARCH模糊）在新psql连接中全部报错。只有全表扫描（无WHERE条件）的9条用例可运行。

**推测原因**: 连接初始化时序问题。`tecc_cache_init` 在GUC加载完成前尝试查找KEK，或 `current_schema` 查询在特定连接参数下未正确执行。

## 2. DET/SEARCH列不支持BTree索引（已修复）

**现象**: DET和SEARCH列创建BTree索引失败。
```
ERROR: cannot create B-tree index on encrypted column without OPE encryption
```

**根因**: 服务端 `tecc_index.c:check_index_support_for_encrypted_column()` 限制BTree索引只允许OPE加密列。

**修复方案**:
- DET列 → HASH索引（支持等值查询 `=`）
- OPE列 → BTree索引（支持范围查询 `>=` `<`）
- SEARCH列 → 无支持的索引类型（LIKE模糊查询必须全表扫描）

**状态**: 已修复（perf_ddl.sql中已修改索引类型）

## 3. WHERE条件使用 `encrypted with` 显式语法不受支持

**现象**: `WHERE col = 1::int4_enc encrypted with (dek = dek_perf)` 在服务端报错。

**根因**: 服务端语法虽可解析 `encrypted with` 的typecast，但WHERE上下文中该语法未正确处理。

**解决方案**: 使用 `WHERE col = 1`，由前端TECC处理器自动识别加密列并加密常量。

**状态**: 已修复（33个SQL文件已移除显式语法）

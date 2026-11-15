# Hash 聚合每组 DISTINCT 去重表的内存开销评估

- 关联需求/缺陷：Bug 9324（GROUP BY 密态列搭配 `COUNT(DISTINCT)` 报排序错误），记录见 `plan/瀚高全密态数据库-未关闭Bug907.md`
- 关联提交：`a8489c26195`（修复）、`2fecd53080c`（文档）
- 关联代码：`src/include/executor/nodeAgg.h`、`src/backend/executor/nodeAgg.c`、`src/backend/executor/execExprInterp.c`、`src/backend/optimizer/path/costsize.c`、`src/backend/utils/adt/selfuncs.c`
- 文档性质：**开销评估与备选方案分析**（非设计文档，当前实现未变更）
- 撰写日期：2026-09-21

**结论摘要**：该改动只增加**内存**，不增加任何磁盘存储；量级为 `8 字节 × 聚合数 × 分组数`，对 HashAggregate 条目的增幅约 6%~35%（取决于聚合数量与转移状态大小）；性能上只有"落盘阈值位移"和"规划器代价微调"两条间接通道，没有额外 CPU 开销。经权衡，建议**保留当前实现**；如需压缩，优先考虑方案 C（4 字节槽位索引），其次方案 B（附加区），两者的收益上限相同、风险面不同。

---

## 1. 背景

### 1.1 Bug 9324 的修复要点

`COUNT(DISTINCT)` 在规划期被计入 `numOrderedAggs`，规划器因此禁用 HashAggregate、强制 GroupAggregate + Sort。当分组键本身是 DET 列时，报错发生在 Agg 之前的 Sort 节点（`cipherdata_cmp_text` 报 `cannot compare encrypted values for ordering`），聚合根本执行不到。

修复分两处：

1. **规划器**（`src/backend/optimizer/prep/prepagg.c`）：纯 DISTINCT（有 `aggdistinct` 且无 `aggorder`）不再计入 `numOrderedAggs`，但仍置 `hasNonPartialAggs`；带 `aggorder` 的聚合与有序集聚合语义不变。
2. **执行器**：DET 等不可排序列的 DISTINCT 去重改用 `TupleHashTable`（哈希去重 + 等值判断，二者对 DET 密文都可用），并把去重表的作用域从"每个 grouping set 一份"改为"**每个分组一份**"。

设计依据：`tecc_doc/加密查询语法边界概要设计.md` F4 表 —— `GROUP BY enc_col` 与 `COUNT(DISTINCT)` 对 DET 列为**支持**，`ORDER BY enc_col` 对 DET 列为**不支持**。因此本方案不改动密文排序能力，"让 DET 密文可排序"的方向是错的（见 §5）。

### 1.2 为什么去重表必须按分组隔离

这是本次开销的根源，也是不可回避的设计约束：

- **HashAggregate / AGG_MIXED**：`advance_aggregates()` 先消费完全部输入，之后才逐组 finalize。一张表服务该 grouping set 下所有组时，不同组的值会混在一起 —— 实测结果为错误值（`RD=2, MK=0`）。
- **AGG_SORTED / AGG_PLAIN**：pergroup 数组按组复用，可以"建一张表、每组 Reset 后复用"，不需要按组各一份（当前实现即如此，`nodeAgg.c:651-668` 建一次，`nodeAgg.c:969-1013` 每组 `ResetTupleHashTable`）。
- **关键约束**：hashed 模式下每条输入元组都可能落到不同分组（`lookup_hash_entries()` 每个元组调用一次），"当前分组"随时切换，因此**单个 per-trans 的"当前表指针"不够用**，指针必须存放在分组状态里。

运行期判别 hashed 与 sorted 的判据是 `aggstate->curaggcontext == aggstate->hashcontext`（`nodeAgg.c:626`、`nodeAgg.c:1006`）。

### 1.3 由此引入的字段

```c
typedef struct AggStatePerGroupData
{
    Datum        transValue;
    bool         transValueIsNull;
    bool         noTransValue;
    /* ReqID:TECC4.0 - per-group DISTINCT dedup table (use_hash_distinct only) */
    TupleHashTable distinctHashtable;      /* nodeAgg.h:293 */
} AggStatePerGroupData;
```

x86-64 布局：改前 `8 + 1 + 1 + 6(补齐) = 16` 字节；新增 8 字节对齐的指针后尾部补齐不再需要，`8 + 1 + 1 + 6 + 8 = 24` 字节。**结构体 16 → 24 字节，每个 (分组 × 聚合) 各一份。**

同时删除了 `AggStatePerTransData.distinct_hashtables`（原按 grouping set 的数组），净增 1 个指针字段。

---

## 2. 影响评估

### 2.1 是内存还是存储 —— 只增加内存

**事实一**：全树 `sizeof(AggStatePerGroupData)` 只出现在 6 处，全部是内存分配或内存清零：

| 位置 | 用途 | 性质 |
|---|---|---|
| `nodeAgg.c:1665` | `additionalsize = numtrans * sizeof(...)`，传给 `BuildTupleHashTableExt` | 仅估算（影响初始桶数） |
| `nodeAgg.c:1845` | `hash_agg_entry_size()` 内算 `pergroupSize` | 仅估算 |
| `nodeAgg.c:2204` | `MemoryContextAlloc(hashtable->tablecxt, sizeof * numtrans)` | **真正的每组分配（RAM）** |
| `nodeAgg.c:3788` | `palloc0(sizeof * numaggs)`，sorted/plain 的固定数组 | RAM，与组数无关 |
| `nodeAgg.c:4856` | `MemSet(..., sizeof * node->numaggs)`，ReScan 清零 | RAM |
| `execExprInterp.c` | 不出现，只读字段 | — |

**事实二**：唯一的落盘路径 `hashagg_spill_tuple()`（`nodeAgg.c:3129-3188`）只写两样东西：

```c
LogicalTapeWrite(tapeset, tapenum, (void *) &hash, sizeof(uint32));  /* 4 字节哈希值 */
LogicalTapeWrite(tapeset, tapenum, (void *) tuple, tuple->t_len);    /* MinimalTuple：所需列 */
```

没有任何一处序列化分组状态结构。

**结论**：该字段**不进入磁盘**，spill 文件格式、记录长度、磁盘用量都不变。唯一的间接存储效应是 §2.3 的阈值位移 —— 恰好卡在 `hash_mem` 边界上的查询，可能从"完全不落盘"变为"落一批盘"。

**另需区分**（易被误认为同一件事）：DISTINCT 去重表**本身**的内存是记在 `hashcontext->ecxt_per_tuple_memory` 的子上下文里的（`nodeAgg.c:630`），而 `hashagg_update_metrics()` 用递归统计 `MemoryContextMemAllocated(..., true)`（`nodeAgg.c:2076`）把它算进 `hashkey_mem` —— 所以去重表的内存**受 `hash_mem` 限额约束**，不会无界增长，是一处正面属性。

### 2.2 占用量化

`hash_agg_entry_size()`（`nodeAgg.c:1837-1864`）返回：

```
TupleHashEntryData(24) + [CHUNKHDRSZ(16) + 8 + W] + [CHUNKHDRSZ(16) + 16k] + [16 + ts]
 = 80 + W + 16k + ts          ← 改前（W=键宽，k=聚合数，ts=转移状态字节，ts>0 时）
```

改后 `16k → 24k`，即：

> **恒定增量 = 8 字节 × 聚合数（k）× 分组数**

条目增幅随场景变化（k=1 约 6%~10%，k 大且 `ts=0` 可达 35%）：

| 场景 | 改前条目 | 改后条目 | 增幅 |
|---|---|---|---|
| k=1, W=4, ts=0 | 84 | 92 | +9.5% |
| k=1, W=8, ts=0 | 88 | 96 | +9.1% |
| k=1, W=8, ts=16 | 120 | 128 | +6.7% |
| k=1, W=64, ts=0 | 144 | 152 | +5.6% |
| k=3, W=8, ts=0 | 120 | 144 | +20.0% |
| k=10, W=8, ts=0 | 232 | 312 | +34.5% |

绝对量举例：100 万分组 × 3 聚合 ≈ **+24 MB**；`hash_mem` 默认值（2×work_mem = 8 MB）下，单聚合 8 字节键的可容纳分组数按同一模型下降约 **9%**（95.4k → 87.4k）。

**注意**：按同一模型比较，分组容量下降的相对幅度就等于条目增幅，因此上表的"增幅"列可直接读作"容量损失"。

### 2.3 性能影响 —— 两条间接通道，均不涉及 CPU

1. **执行器落盘阈值位移**：`hash_agg_entry_size()`（`nodeAgg.c:3816`）→ `hash_agg_set_limits()`（`nodeAgg.c:1942-1956`）得到 `ngroups_limit` / `mem_limit` → `hash_agg_check_limits()` 决定何时 spill。每组涨价 8 字节 ⇒ 边界查询多一次 spill/replay 的 IO。
   *自我修正机制*：`hashagg_update_metrics()`（`nodeAgg.c:2097-2103`）会用**实测** `MemoryContextMemAllocated` 重算 `hashentrysize`，运行中阈值自适应，初始估算误差不累积。
2. **规划器代价微调**：`costsize.c:2682` 用同一函数算 `nbatches → spill_cost`（HashAggregate 代价略升）；`selfuncs.c:3875` 的 `estimate_hashagg_tablesize()` 也用它（供 HashAgg 与 Sort/GroupAggregate 比较）。只在两者代价本就非常接近时可能换计划，非普遍现象。

**没有 CPU 开销**：字段只在 `use_hash_distinct` 为真时被访问（`execExprInterp.c:4309`、`nodeAgg.c:651/969`），非 DISTINCT 查询一次都不读，也没有新增分支。

**不受影响的部分**：`AGG_SORTED`/`AGG_PLAIN` 只有一处固定大小分配（`nodeAgg.c:3788`，规模为 `numaggs` 而非分组数）；无 DISTINCT 的聚合不访问该字段；磁盘格式不变。

### 2.4 影响范围上的一个要点

该字段是**无条件**存在的，因此**所有 HashAggregate 查询（含完全不含加密列的普通 SQL）都承担这份内存**，而不只是 TECC 查询。这是当前方案真正的代价所在。

---

## 3. 备选方案

### 方案 A：保留现状（推荐）

`AggStatePerGroupData` 内直接放指针，`sizeof` 参与全部估算。

- 优点：去重表"按分组隔离"这一语义约束由数据结构直接表达；规划器（`costsize.c`/`selfuncs.c`）与执行器（`nodeAgg.c:3816`）共用 `hash_agg_entry_size()`，两边**天然一致**，不存在估算失配；改动面最小，已通过全量回归。
- 缺点：非 DISTINCT 查询也付 8 字节/(分组×聚合)。

### 方案 B：附加区（把指针挪出结构体）

结构体回到 16 字节；分组状态块尾部**追加** `numtrans × sizeof(TupleHashTable)` 字节，仅在 `use_hash_distinct` 时追加。

- 收益：非 DISTINCT 查询（含全部非 TECC 查询）零开销，收益上限即 §2.2。
- 风险见 §4，最大的一条是规划器接口。

### 方案 C：4 字节槽位索引（结构体大小不变的零开销方案）

利用结构体原有的 6 字节尾部补齐：把 8 字节指针换成 **`uint32` 槽位号**（`8+1+1+4 = 14 → 补齐到 16`，**结构体大小不变**），去重表本体放在 `AggState` 的一个按需增长的槽位数组里。

- 收益：与方案 B 相同（非 DISTINCT 查询零开销），且**不需要改 `hash_agg_entry_size()` 的签名、不需要动规划器**。
- 代价/风险：见 §4 —— 风险从"跨模块接口对齐"换成"槽位生命周期管理"。

### 方案 D：不改执行器，只改规划器（不可行，已排除）

实测结果为错误值（`RD=2, MK=0`）。原因见 §1.2：hashed 聚合先消费全部输入，per-set 表串组。

### 方案 E：让 DET 密文可排序（方向错误，已排除）

改 `cipherdata_cmp_internal`/`cipherdata_cmp_text`/`cipherdata_cmp_bpchar` 让 DET 密文参与排序比较。三重问题：

1. 违反设计文档 F4（`ORDER BY enc_col` 对 DET 列明确不支持），等于顺带放开 `ORDER BY DET`；
2. DET 密文序跨运行不固定（每次生成新 DEK），会使多个 expected 文件不稳定；
3. 与其绕开 Bug，不如按其真实语义（等值/哈希）解决。

该方向曾尝试并已全部回退。

---

## 4. 风险对照

| 维度 | A 保留现状 | B 附加区 | C 4 字节槽位索引 |
|---|---|---|---|
| 非 DISTINCT 查询开销 | 8B/(分组×聚合) | 0 | 0 |
| 结构体大小 | 24B | 16B | **16B（不变）** |
| 规划器接口改动 | 无 | **需要**（`hash_agg_entry_size()` 加参 + `AggClauseCosts` 传递） | 无 |
| 规划器/执行器估算失配风险 | 无（同一 `sizeof`） | **有**（估小→真超 `hash_mem` 不落盘；估大→提前落盘） | 无 |
| 取址安全性 | 编译期字段访问 | 手工指针算术，两种布局位置不同 | 槽位间接寻址（仍越界风险较低） |
| 生命周期风险 | 无 | 无 | **有**（槽位回收/别名） |
| 需重新验证的面 | — | 规划器+执行器接口、5 个访问点、3 处分配 | 全部访问点 + 槽位回收路径 |
| 实现复杂度 | 最低 | 中 | 中高 |

### 方案 B 的具体风险

1. **`hash_agg_entry_size()` 签名不足（最大的一条）**。签名只有 `(numTrans, tupleWidth, transitionSpace)`，三个调用者都不掌握"本查询是否使用哈希去重"：执行器 `nodeAgg.c:3816`、规划器 `costsize.c:2682`、`selfuncs.c:3875`。要么按最坏情况总加（收益归零），要么加参数并经 `preprocess_aggref` → `AggClauseCosts` → `cost_agg` 把标志传下来，属**跨模块接口改动**；一旦两边不一致，就会出现"实际内存超 `hash_mem` 却不落盘"或"无谓提前落盘"。
2. **取址退化为手工指针算术，且两种布局不同**：hashed 是 `entry->additional + numtrans`；sorted 是 `all_pergroups[setno]` 整个数组**末尾** `+ numaggs`（不是每个元素之后）。现有 5 个访问点全部要改：`nodeAgg.c:634`（写）、`651/655`、`969`、`1007`，以及 `execExprInterp.c:4309`（**每个输入元组都走的热路径**）。**失去编译期类型检查**：越界不会编译报错，而是写到相邻分组的 `transValue` 或读到别的分组的表指针，表现为数据错乱而非易定位的崩溃。
3. **三处"整体大小"必须同步**：`nodeAgg.c:2204` 分配、`3788` `palloc0`、`4856` `MemSet`。并且必须保证**只有 `use_hash_distinct` 时**才访问附加区 —— sorted 路径的 pergroup 是 `numaggs` 个 16 字节连续结构体，无条件读附加区就是越界读相邻元素（UB）。
4. **验证成本**：改动面由"一个字段"扩为规划器/执行器接口 + 5 个访问点 + 3 处分配大小；hashed 的 spill/rescan 需要重新验证（`AGG_MIXED` 的 sort 阶段漏建去重上下文导致崩溃，已在 Bug 9324 修复过程中踩过一次）；需重跑全量回归。

### 方案 C 的具体风险

1. **槽位生命周期**：分组被丢弃时必须释放槽位，否则泄漏或槽位别名（两个分组共用一张表 → 串组，即 §1.2 的错误复现）。回收点必须与 hash 表重置/重建的路径严格对齐，漏一处即出错。
2. **`ExecReScanAgg` 复用已填充 hash 表**时槽位必须仍然有效（当前实现是靠"把表指针置 NULL 而非 `ResetTupleHashTable`"来避免重复累加，`nodeAgg.c:1006-1007`；换槽位后要保证同一语义）。
3. **热路径多一次间接寻址**：`execExprInterp.c:4309` 由"读一个字段"变为"读槽位号 + 查数组"。
4. **同样失去 `sizeof` 的自洽性**：槽位数组不在 hash 条目里，其内存由 `hash_mem` 统计（子上下文）覆盖，但初始估算与实际的对应关系不如方案 A 直接。

---

## 5. 结论与建议

1. **保留方案 A**。它是"去重表按分组隔离"这一语义的直接表达，`sizeof` 参与所有估算使规划器与执行器天然一致；代价 `8B/(分组×聚合)` 只对 HashAggregate 生效，且被执行器的实测自适应部分吸收。
2. **何时才需要动**：出现"HashAggregate 内存吃紧"的具体现场证据（客户压测报告、`explain analyze` 显示因该开销提前落盘并造成可测量的 IO 退化）时再评估。届时优先方案 C（不动规划器接口），方案 B 次之。
3. **`nodeAgg.h:269` 的注释需要更正**：现写"相对整个条目约 8~18%"，该区间只覆盖 `k ≤ 2` 的情形；当聚合数多、转移状态为 0（如多个 `count(*)`）时，实测模型下可达 34.5%（k=10）。建议改为"约 6%~35%，取决于聚合数量与转移状态大小"。

## 6. 遗留相关项（非本次引入）

- `GROUP BY enc_col`（不带 `DISTINCT`）在代价模型选择排序路径时仍可能报同样的排序错误（例：`work_mem` 很小时规划器选 Sort）。这是 Bug 9324 修复前就存在的缺口，与本次改动无关。
- 要一并治理需在规划器引入"列加密元数据感知"能力（规划器目前拿不到列的加密信息），属独立课题。

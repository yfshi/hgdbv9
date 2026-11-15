# 三未信安密码机（SWXA / SWSDS）客户端使用规范性评估

> 评估对象：TECC 客户端对三未信安网络密码机的调用方式（libpq 侧适配器）与客户端配置。
> 评估依据：厂商《三未信安服务器密码机应用接口手册（通用版本）V2.4》（2022-04，下称"手册"）、
> 交付包 `SWSDS_V1.6.58_GMT0018` 中的官方 `swsds.h`、`swsds.ini` 与 demo 源码（`demo/*.c`）、
> 以及本仓库 `src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.c`。
> 评估日期：2026-09-26。

## 结论摘要

主体用法**符合**手册与官方 demo 的规范：设备/会话/密钥句柄的生命周期与手册 2.5 的
线程/进程安全约定一致，密钥句柄与私钥授权索引的用法也对得上手册条文。存在
**1 处实质性偏离**（会话密钥句柄驻留到连接结束，与手册 4.2.21「运算完成后应销毁」的
措辞相左，属性能与规范的显式取舍）与 **1 处次要偏离**（从不调用
`SDF_ReleasePrivateKeyAccessRight`）。配置侧有若干手工偏离与缺项，其中
`[KeepAlive]`、`[OpenDevice]` 两节已于本次评估中补齐。

## 一、符合手册/规范的做法

| 我们的做法 | 依据 | 代码位置 |
|---|---|---|
| 设备句柄**进程内只打开一次**并复用，进程退出时关闭 | 手册 2.5 句柄表：设备句柄「进程安全=是、线程安全=是」；官方 demo 亦为 `SDF_OpenDevice` 1 次 / `SDF_CloseDevice` 1 次 | `fe-swxa-adapter.c` 的 `swxa_device_get` / `swxa_device_close_at_exit` |
| 每个连接一个会话，不跨线程复用会话 | 手册 2.5：会话句柄「进程安全=是、线程安全=是，**不建议多线程复用**」 | `swxa_connect` 中 `SDF_OpenSession` |
| 会话密钥句柄缓存在**连接级**，且所有设备调用经同一把锁串行化 | 手册 2.5：密钥句柄「进程安全=否、线程安全=否，**禁止多线程复用**」 | `SwxaDevicePrivate`（`SWXA_KEY_CACHE_SLOTS`）、`swxa_device_op_lock` |
| `SDF_ImportKey` / `SDF_GetSymmKeyHandle` → 运算 → `SDF_DestroyKey` 成对；断连时**先逐个 `SDF_DestroyKey` 再 `SDF_CloseSession`**（会话仍有效时销毁） | 手册 4.2.21「在对称算法运算完成后，应调用本函数销毁会话密钥」；demo 中 ImportKey 10 次 / DestroyKey 20 处 | `swxa_disconnect`、`swxa_key_cache_reset(device, true)` |
| 私钥授权索引传 `key_index + 10000` | 手册 2.4 `algorithmArrangement`：「SM2：SM2 密钥索引 +10000」 | `SWXA_AUTH_KEY_INDEX_OFFSET = 10000` |
| 无口令时 `pucPassword` 传 `NULL`/长度 0；需要口令时口令长度 ≥ 8 字节 | 手册 4.1.7「私钥访问控制码的长度，不少于 8 字节」 | 取权限处两种形态；占位口令 `"tecc0000"`（8 字节） |
| 对称加密的 IV 取自设备随机数 | 手册 4.1.6 `SDF_GenerateRandom` | `swxa_encrypt` 第 2 步 |
| 用厂商扩展 `SDF_OpenDeviceWithPathAndName` 指定配置文件打开设备 | 手册 5.1.2（厂商扩展接口，允许使用） | `swxa_connect` |
| 配置文件部署在 `/etc/` 或程序当前目录 | 手册 2.3：首选「应用程序当前路径」，Linux 次选 `/etc/` | 回归框架的 ini 搜索顺序 |
| 会话密钥用内部 KEK 句柄（`SDF_GetSymmKeyHandle`，厂商扩展）取设备内预置 KEK | 手册 5.2.1「获取内部 KEK 对称密钥句柄」 | `swxa_sm4_*` 路径 |
| `controlAuthority=1`、`SemStatus=1`、`[ErrorLog] level=2` | 手册 2.4 默认值；手册注明 level「建议业务系统正式运行时设置为 2」 | `/etc/swsds.ini` |

## 二、偏离项

### P1（实质偏离）会话密钥句柄驻留到连接结束，而非「运算完成后立即销毁」

手册 4.2.21 的备注是规范措辞：「在对称算法运算完成后，**应**调用本函数销毁会话密钥」；
官方 demo 也是 `ImportKey`/`GetSymmKeyHandle` → 运算 → `DestroyKey` 就地配对
（`demo/symm.c:825/838`、`demo/symm.c:1095`）。

我们的实现为消除性能瓶颈（真机单次设备调用约 3.2 ms，原先每个值要
`SDF_ImportKey` + 运算 + `SDF_DestroyKey` = 3 次往返 ≈ 9.6 ms/值），把
`SDF_ImportKey` 得到的**会话密钥句柄缓存在连接级**（最多 16 个槽位），
因此同一 DEK 的会话密钥在设备内**驻留至连接结束**才销毁
（见 `fe-swxa-adapter.c:140-180` 的说明与 `swxa_key_cache_reset`）。
`TECC_SDF_KEY_CACHE=0`（或 `off`/`false`/`no`）可退回「每值导入/销毁」的旧行为。

风险与建议：会话密钥在设备内长期驻留属资源与安全管理的取舍，等保/安全测评可能追问
「会话密钥是否用完即毁」。建议：① 向厂商确认该形态是否可接受；② 若不接受，把销毁边界
由「连接」改为「语句/事务」；③ 在交付文档中显式登记该偏离与开关。

### P2（次要偏离）从不调用 `SDF_ReleasePrivateKeyAccessRight` —— 2026-09-26 已修

手册 4.1.8 提供「释放私钥使用权限」，官方 demo 在 ECC 流程中成对调用
（`demo/ecc.c:1171`、`:1195`、`:1212`）。此前我们取到私钥使用权后不再释放，权限保持到会话
关闭才随会话回收；在 `controlAuthority=1`（启用私钥访问控制）下，等于本连接内长期持有
该私钥使用权。

**修法**：适配器新增私钥权限记录表，会话内取一次后复用，`swxa_disconnect` 在关会话前逐个
`SDF_ReleasePrivateKeyAccessRight(session, 索引 + 10000)`；释放失败静默（收尾动作，且
断连路径往 stderr 打字会污染金标准比较）；该 SDF 符号按**可选**加载，老库不导出时跳过。

顺带修掉 SoftSDF 一处不对称缺陷：其授权函数把索引归一化（`uiKeyIndex -= 10000`，容器按裸
索引登记），但释放函数没有同样归一化，导致按真机规范传 `索引+10000` 时释放永远找不到容器
（真机上取/放用同一个 +10000 索引、两次都成功）。

**验证**（2026-09-26）：软模式 `swxa_enc.sql`、`swxa_alter_kek_he.sql` PASS；真机
`swxa_enc.sql` PASS（59 s，设备错误日志零条目）、`swxa_alter_kek_he.sql` PASS（36 s，零条目）。

### P3 配置项偏离与缺项

| 配置项 | 手册默认 | 我们的值 | 说明 |
|---|---|---|---|
| `[KeepAlive] heartbeat` | 手册未记载（交付包 2024-10 的 ini 有，值 60） | **2026-09-26 已补 60，真机验证无异常** | 本客户端把设备连接池长期持有到进程退出，无心跳时空闲连接可能被设备/防火墙回收；不排除与现场偶发 `CheckServer Error…connecttimeout` 相关 |
| `[OpenDevice] OpenDeviceMode` | 同上，交付包为 0 | **2026-09-26 已补 0** | 语义（厂商注释）：「多台设备只有一台设备正常时 OpenDevice 是否算成功」，0=不成功（默认）。单设备部署取默认值；此前缺项导致设备库每次启动记 `SDIF_ParseConfigFile->Read ITEM_OPENDEVICE_MODE error, use default:[0]` ——**补齐后真机实测该条已消失** |
| `[OpenDevice] SessionHandleManage` | 同上，交付包为 0 | **2026-09-26 已补 0** | 语义：「opensession 打开句柄数量，0=不限制，1=与连接池相同」。**必须保持 0**：并发连接数可能超过 poolsize，设为 1 会把会话数限到 poolsize |
| `[ErrorLog] logjsonerror` / `logjsoninfo` | 交付包有 | 缺 | 已被默认值兜住（`swsds_error.log` 照常产生），建议按交付包补齐 |
| `[Timeout] connect` / `service` | 30 / 30 | **120 / 120** | 为适配 VPN 慢网放宽 4 倍；手册未设上限，但会拉长故障暴露时间 |
| `[Hash] HashMode` | 1（服务端 HASH） | **0（客户端 HASH）** | 手工偏离默认值，建议确认与密码服务侧配置一致 |
| `[SessionKey] Synchronize` | 0（手册注明「建议使用会话密钥同步」，面向多机负载） | 0 | 单设备部署取默认值，可接受 |
| `[ConnectionPool] poolsize` | 50（手册仅说明它是「与密码设备建立的网络连接数」） | 10 | 手册**没有**「10 的倍数且 ≤100」这条约束（该约束来自 Java 侧 JAR 内的校验字符串）；取最小值以降低共享设备占用，属主动优化 |
| `[Global] Protocol` | tcp | tcp | 默认值。tcp 模式下 `sslmode`/`sslAlgorithm`/`CipherList` 均不生效，即与密码机走明文 TCP 通道；手册要求与密码服务侧设置保持一致，生产建议改 ssl |
| `[Device] device=CMD` | 手册与厂商交付 ini **均无此节** | 有 | 来源不明的历史遗留，设备库似乎忽略；建议以交付包那份为基准清理 |

### P4 从不调用 `SDF_GetDeviceInfo`（手册 4.1.5）

建议连接建立时调用一次并记录设备型号/版本（`DeviceName`/`DeviceVersion`/
`StandardVersion`），现场排障与版本核对会用到。非必需项。

### P6（本次真机验证暴露）私钥授权先试 `NULL` 口令，真机必失败一次并写设备错误日志 —— 2026-09-26 已修

`swxa_sm2_decrypt_core` 取私钥使用权时按「环境变量口令 → `NULL` → 8 字节占位口令」
顺序试探（`fe-swxa-adapter.c:1240-1276`）。手册 4.1.7 要求 `pucPassword` 为不少于 8 字节
的访问控制码；真机对 `NULL` 直接返回参数错误，日志逐条记录：

```
SDF_GetPrivateKeyAccessRight->input parameters Error, keyindex[10003], pucPassword[(null)] sdf.c:6648
```

实测 `TECC_TEST=swxa_enc.sql make check-tecc-libpq`（真机）一次运行产生 **49 条**此类
记录——即该用例里每一次 HE 私钥信封解密都先浪费一次注定失败的设备调用（另占一次往返），
且全部写进 `swsds_error.log`，运维读设备日志时会看到成片"错误"。另外该调用是**按操作**
重复取权限（同一会话内 49 次），却从未释放（见 P2），取/放不成比例。

建议：真机模式下跳过 `NULL` 试探（直接使用环境变量口令或 8 字节占位口令），
并在会话内取一次权限后复用（配合 P2 在结束时释放）；软模式保留 `NULL` 形态即可。

**修法（已实施）**：授权口令的**第一轮试探按模式决定**——SoftSDF 用空口令、真机用 8 字节
占位口令（第二轮换另一种形态兜底，`TECC_SDF_PASS` 设了则只试它），于是真机不再发生注定
失败的调用；同时会话内**取一次权限后复用**，重复解信封不再重复取权限。**验证**：真机
`TECC_TEST=swxa_enc.sql make check-tecc-libpq` 的设备错误日志里
`SDF_GetPrivateKeyAccessRight` 由修复前的 **49 条降为 0 条**，用例 PASS。

### P5 配置文件权限与注释编码

- `/etc/swsds.ini` 权限为 `0644 root:root`（所有用户可读），其中含设备连接口令
  （虽为手册要求的 PIN 密文形式）。建议收紧为 `0600`。
- 该文件（及仓库内副本）的中文注释**已损坏**：原始 GBK 注释字节在早前某次编辑中按
  UTF-8 解码，变成了 U+FFFD 替换字符（`od` 可见大量 `EF BF BD`），文件因此被 `file`
  判为 UTF-8。注释会被设备库忽略，功能无影响，但可读性已失。建议以交付包中
  `SWSDS_V1.6.58_GMT0018/swsds.ini`（GBK，注释完好）为基准重建本地 ini。

## 三、软模式（SoftSDF）的能力盲区

`tecc_kms/src/softsdf.c` 是我们自研的软件密码机替身，属**功能级**模拟：它按会话维护
`key_list` 并校验会话归属本设备（`softsdf.c:1415-1422`），但**不模拟**设备侧连接池、
会话密钥/句柄容量上限，也无法模拟「禁止跨线程复用」这类约束。因此 P1、P3 中与设备资源
相关的问题**只能在真机模式下暴露**——软模式全绿不能外推为真机合规。

## 四、处置建议（按优先级）

1. 决定 P1 的会话密钥销毁边界（保持连接级驻留并登记，或改为语句级销毁），必要时向厂商确认。
   ——**未做**（属性能与规范的取舍，待决策）。
2. 补 P2 的 `SDF_ReleasePrivateKeyAccessRight`。——**2026-09-26 已做**（会话内取一次 + 断连释放，见 P2）。
3. ini 侧：`[KeepAlive]`/`[OpenDevice]` 已补；继续补 `[ErrorLog] logjson*`，并确认
   `HashMode` / `Synchronize` / `Timeout` / `Protocol` 与密码服务侧一致；清理
   `[Device]` 节；以厂商交付 ini 为基准恢复可读注释。
4. `/etc/swsds.ini` 权限收为 `0600`。
5. 连接建立时调用 `SDF_GetDeviceInfo` 并记日志。
6. P6：真机模式跳过 `NULL` 口令试探（直接用环境变量口令或 8 字节占位口令），并把取私钥权限
   由"每操作一次"改为"每会话一次后复用"，与第 2 项一起改。——**2026-09-26 已做**（见 P6）。

## 附录 A：官方 demo 的规范用法计数（`SWSDS_V1.6.58_GMT0018/demo/*.c`）

| 接口 | 次数 | 说明 |
|---|---|---|
| `SDF_OpenDevice` / `SDF_CloseDevice` | 1 / 1 | 设备打开一次、关闭一次 |
| `SDF_OpenSession` / `SDF_CloseSession` | 53 / 107 | 会话多次打开、各分支均关闭 |
| `SDF_ImportKey` / `SDF_DestroyKey` | 10 / 20 | 导入后即销毁 |
| `SDF_GetSymmKeyHandle` | 1 | 另在 `symm.c:1095` 配 `SDF_DestroyKey` |
| `SDF_GetPrivateKeyAccessRight` / `SDF_ReleasePrivateKeyAccessRight` | 35 / 6 | demo 亦非全部成对释放 |
| `SDF_OpenDeviceWithPathAndName` | 0 | demo 用标准 `SDF_OpenDevice` + 默认配置文件路径 |

## 附录 B：本轮核对手册的条目

| 手册位置 | 内容要点 |
|---|---|
| 2.3 配置文件部署 | 首选应用程序当前目录，Linux 次选 `/etc/` |
| 2.4 配置文件说明 | `connect`/`service` 默认 30、`poolsize` 默认 50、`controlAuthority` 默认 1、SM2 授权索引 +10000、`Synchronize` 默认 0、`SemStatus` 默认 1、`HashMode` 默认 1、`Protocol` 默认 tcp |
| 2.5 多线程和多进程支持 | 设备句柄 进程/线程安全=是；会话句柄 安全=是但不建议多线程复用；密钥句柄 均否、禁止多线程复用 |
| 4.1.1–4.1.8 设备管理 | 打开/关闭设备、创建/关闭会话、获取设备信息、产生随机数、获取/释放私钥使用权限 |
| 4.2.20 `SDF_ImportKey` | 导入明文会话密钥，密钥长度 {8,16,24,32,64} |
| 4.2.21 `SDF_DestroyKey` | 「在对称算法运算完成后，应调用本函数销毁会话密钥」 |
| 5.1.1–5.1.4 打开设备扩展 | `..._WithPath` / `..._WithPathAndName` / `..._WithParameter` / `..._WithParameter_Ex`（含 `poolSize` 入参） |
| 5.2.1 `SDF_GetSymmKeyHandle` | 获取内部 KEK 对称密钥句柄 |

> 注：手册 V2.4（2022-04）早于交付的库/ini（1.6.58，ini 为 2024-10），
> `[KeepAlive]`、`[OpenDevice]`、JSON 日志项等只出现在交付 ini 的注释里，手册未记载。
> 建议向厂商索取与 1.6.58 对应的新版接口手册。

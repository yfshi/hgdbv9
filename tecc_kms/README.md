# tecc_kms — TECC 软件密码机（SDF 设备模拟器）

## 定位

`tecc_kms` 是 TECC 全密态数据库的**软件密码机组件**，以软件方式实现国密密码设备应用接口
（GM/T 0018-2012，即 SDF 接口），用于在无真实密码机（同智伟业 tzwykms / 三未信安 swxakms）
的环境下，为硬加密场景提供可真实加解密的 SDF 设备模拟，支撑前期开发与联调。

同时，本组件是 TECC 未来自研密钥管理系统（KMS）的孵化地——目录保持与 PG 内核、libpq
的松耦合，将来可整体抽离为独立密钥管理组件。

## 设计原则

1. **松耦合**：位于仓库顶层 `tecc_kms/`，不在 `src/` 下，与 PG 内核、libpq 无构建依赖；
2. **标准接口**：SDF 接口对齐 GM/T 0018，客户端（`src/interfaces/libpq/tecc_hsm_adapter/`）
   通过标准 SDF 接口调用本组件，将来切换真实密码机仅替换动态库；
3. **真实算法**：基于 GmSSL 国密算法库（SM2/SM3/SM4）实现，非 stub 假加密；
4. **独立构建**：使用独立的 CMake 构建体系，不纳入 PG 的 autoconf/make。

## 目录结构

```
tecc_kms/
├── src/            # 软件 SDF 设备实现源码（源自 GmSSL/SoftSDF，Apache-2.0）
│   ├── softsdf.c       # SDF 设备接口实现
│   ├── softsdfinit.c   # 虚拟设备初始化工具源码
│   ├── softsdftest.c   # 接口自测源码
│   ├── sdf.h / sgd.h   # SDF 接口头文件（GM/T 0018）
│   ├── softsdf.lds     # 导出符号表（Linux）
│   └── CMakeLists.txt  # SoftSDF 原始构建（参考）
├── include/        # SDF 接口头文件副本（供客户端引用）
├── build/          # tecc_kms 构建脚本（CMakeLists.txt）
├── jce/            # SoftSDF 的 JCE 门面（**回归测试专用，非产品件**，详见 jce/README.md）
├── bin/            # 编译产物输出目录
│   ├── libteccsdf.so    # SDF 设备动态库（客户端链接目标）
│   ├── teccsdfinit      # 虚拟设备初始化工具
│   └── libteccjce.jar   # JCE 门面（make tecc-sdf-jce-jar 生成；仅 SoftSDF 模式的 JDBC 回归加载）
└── README.md
```

## 命名约定

| 项 | 名称 |
|---|---|
| 目录 | `tecc_kms` |
| SDF 设备动态库 | `libteccsdf.so` |
| 设备初始化工具 | `teccsdfinit` |

## 依赖

- **GmSSL**（github.com/guanzhi/GmSSL，BSD 许可，可闭源商用）：提供 SM2/SM3/SM4 国密算法。
- **SoftSDF**（github.com/GmSSL/SoftSDF，**Apache-2.0** 许可）：本组件的软件 SDF 实现来源，
  源码已就位于 `src/`（HEAD 提交 `cb93b876`）。
- 客户端侧：`src/interfaces/libpq/tecc_hsm_adapter/fe-hsm-adapter.c`（同智伟业适配器）与
  `fe-swxa-adapter.c`（三未信安适配器）通过标准 SDF 接口链接本组件产出的 `libteccsdf.so`。

## 构建

### 前置：安装 GmSSL（只需一次）

```bash
git clone https://github.com/guanzhi/GmSSL.git        # 网络不稳可换 gitee 镜像 gitee.com/mirrors/GmSSL
cd GmSSL && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/gmssl-install ..   # 装到自定义目录（避免污染系统/需 sudo）
make -j && make install
# 确认存在：$HOME/gmssl-install/include/gmssl/sm2.h 与 $HOME/gmssl-install/lib/libgmssl.so
```

> 也可 `sudo make install` 装到 /usr/local；推荐自定义前缀，构建 tecc_kms 时用
> `-DGmSSL_ROOT_DIR` 指向即可（CMakeLists 按 `.so/.a` 后缀搜索，GmSSL 仅有 .so 也能找到）。

### 构建 tecc_kms

```bash
cd tecc_kms/build          # CMakeLists.txt 在 build/ 下，用 `.` 而非 `..`
cmake -DGmSSL_ROOT_DIR=$HOME/gmssl-install .
make -j
# 产物输出至 tecc_kms/bin/libteccsdf.so、teccsdfinit、teccsdftest
```

常见报错：`GmSSL headers not found` / `GmSSL static library not found` → `GmSSL_ROOT_DIR`
路径不对或 GmSSL 未执行 make install。

### 安装（可选）

```bash
sudo make install    # 默认 libteccsdf.so → /usr/lib64、头文件 → /usr/include/teccsdf
```

### 初始化虚拟设备（联调/回归前必做）

密钥文件按**相对路径**生成在**当前工作目录**，请先进入目标目录：

```bash
cd tecc_kms/bin
./teccsdfinit -kek 1 -key 1 -pass <口令>   # 带访问口令的 SM2 密钥对
./teccsdfinit -kek 3 -key 3                # 省略 -pass：无口令 SM2 密钥对
# 生成 kek-<n>.key（预置 KEK）、sm2enc-<n>.pem / sm2encpub-<n>.pem（SM2 加密密钥对）等
# 密钥索引 1..4096，对应 hsm:id=<index>
# 省略 -pass 时生成非加密 PEM，等价真机密码机内不带访问口令的设备密钥
```

自测：`./teccsdftest`（期望输出 `test ok`）。

### 接入回归测试

回归框架**默认就走 SoftSDF**（`init_sdf_mode` 在 initdb 前把 `TECC_SDF_LIB` 填成
`tecc_kms/bin/libteccsdf.so` 的绝对路径），因此 HSM 用例开箱即跑，不需要任何环境变量：

```bash
make check-tecc-libpq      # 硬加密用例（shared 7 个 + libpq 2 个）真跑
make check-tecc-jdbc       # 需 TECC_JDBC_HOME；自动构建并加载 JCE 门面
make check-tecc-cross
```

变体：

```bash
# 指定别的 SoftSDF 库（相对路径会被归一成绝对路径）
TECC_SDF_LIB=$PWD/tecc_kms/bin/libteccsdf.so make check-tecc-libpq
# 本机接了真实加密机：显式关掉 SoftSDF，改走 swsds.ini + TCP 探测
# （密码机连不上时，硬加密用例整体不执行，报告里打印「用例跳过 N 个（原因）: ...」）
TECC_SDF_LIB=off make check-tecc-libpq

# 预置密钥（keys/kek-{1,2,3}.key、keys/sm2*-3.pem）中的 SM2 密钥对不带访问口令，
# 无需设置 TECC_SDF_PASS；若改用自己生成的口令保护的密钥文件，再用
# TECC_SDF_PASS=<口令> 传入（须与 teccsdfinit -pass 一致）。
```

### JDBC 侧的对应物：`jce/`（JCE 门面）

libpq 走 `dlopen(libteccsdf.so)`，而 **JDBC 走的是 JCE Provider 路线**——驱动反射加载
`com.sansec.jce.provider.SwxaProvider`，SoftSDF 无法直接插进去。`jce/` 就是补这一层：
一个 drop-in Provider（同名类/同算法集），后端直接读 SoftSDF 的 `kek-<index>.key` 与
`sm2enc-<index>.pem` 做标准 SM4-CBC / SM2（已与 native SoftSDF 逐字节核对），
**无需改动 pgjdbc 任何产品代码**。

```bash
TECC_JDBC_HOME=<pgjdbc 项目根> \
TECC_SDF_LIB=$PWD/tecc_kms/bin/libteccsdf.so \
make check-tecc-jdbc
```

细节（为什么必须独立成 JAR、元契约、构建、使用）见 **`jce/README.md`**。

### Windows 构建

CMakeLists 已含 MSVC 分支（Ws2_32/Advapi32、WINDOWS_EXPORT_ALL_SYMBOLS）：

```bat
cd tecc_kms\build
cmake -G "Visual Studio 17 2022" -A x64 -DGmSSL_ROOT_DIR=<gmssl安装路径> .
cmake --build . --config Release
```

## 接口覆盖与差异（重要）

SoftSDF 已实现大部分 GM/T 0018 标准接口（设备/会话管理、`SDF_GenerateRandom`、
`SDF_Encrypt/Decrypt`、`SDF_DestroyKey`、`SDF_ImportKeyWithKEK`、`SDF_ExportSignPublicKey_ECC`、
`SDF_ExportEncPublicKey_ECC`、`SDF_GetPrivateKeyAccessRight`、`SDF_InternalDecrypt_ECC`、
`SDF_GenerateKeyPair_ECC`、数字信封、密钥协商、哈希等）。

**工程对接所需的厂商扩展接口已全部补齐（`src/softsdf.c` 实现 + `src/softsdf.lds` 导出）：**

| 接口 | 用途 | 实现 |
|---|---|---|
| `SDF_ImportKey`（裸明文导入会话密钥）| 用传入的 DEK 明文做 SM4 加解密（对称数据路径）| 已补：裸导入明文密钥到会话 |
| `SDF_OpenDeviceWithPathAndName`（swxa 厂商扩展）| 按配置文件打开设备 | 已补：幂等封装 `SDF_OpenDevice`（重复打开返回既有句柄）|
| `SDF_GetSymmKeyHandle`（swxa 厂商扩展）| 引用预置对称密钥（KEK）| 已补：读 `kek-<index>.key` 文件 |

> 注：SM2 外部公钥加密/内部私钥解密 SoftSDF 均已实现（`SDF_ExternalEncrypt_ECC` /
> `SDF_InternalDecrypt_ECC`）；SM2 私钥授权 `SDF_GetPrivateKeyAccessRight` 已有，
> 口令由 `TECC_SDF_PASS` 传入（见上节）。

## 与客户端的衔接

客户端（libpq 内 HSM 适配器）通过**动态库加载**方式调用本组件（复用 swxa 适配器已有的
`dlopen` 机制），由环境变量指定 SDF 库与 SM2 私钥口令：

| 环境变量 | 含义 | 取值示例 | 说明 |
|---|---|---|---|
| `TECC_SDF_LIB` | 指定 dlopen 加载的 SDF 动态库路径 | 产品/生产：不设 → 加载真实厂商库（`libswsds.so` 等）；回归：不设 → 默认 `tecc_kms/bin/libteccsdf.so`（SoftSDF），`=off` → 走真机 | 产品侧**非空即 dlopen 该路径**；**回归框架**（`t/tecc_test_utils.pm` 的 `init_sdf_mode`）在此之上包一层：不设 = SoftSDF（默认，跳过 swsds.ini 检查），`=off`（同义 `real`/`none`/`0`）= 真机（子进程删掉该变量；swsds.ini/TCP 探测不通过则硬加密用例不执行并打印说明）|
| `TECC_SDF_PASS` | SoftSDF 模式下**带口令**密钥文件的访问口令 | `TECC_SDF_PASS=test`（须与 `teccsdfinit -pass` 一次生成时一致）| 客户端解密 HE 私钥信封前会**无条件**调用 `SDF_GetPrivateKeyAccessRight`（真机同样必须授权）；设了该变量则用它作口令，未设则传空口令，对应无口令密钥文件（等价真机无口令设备密钥）。两种形态 SoftSDF 均支持 |

> 生产（真实 swxa 密码机）**不设** `TECC_SDF_LIB`，走厂商库与设备内密钥；上述两个变量仅影响测试路径。
> 注意：**回归框架把「不设」解释为 SoftSDF**，所以要在回归里打真机得写 `TECC_SDF_LIB=off`。

## SoftSDF 对 swxa（三未信安）接口的模拟对接方案

swxa 适配器（`src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.c`）通过 `dlopen/dlsym`
按 **swxa 真实接口（`src/include/hsm/sdf_client.h`）签名**调用 SDF 函数；SoftSDF
（`libteccsdf.so`）实现同名函数模拟设备行为。对接要点：

| 层面 | swxa 真实接口（sdf_client.h）| SoftSDF 模拟 | 对接说明 |
|---|---|---|---|
| 句柄类型 | `long long` | `void *` | 64 位 ABI 等价（均 8 字节值传递），适配器统一声明 `void *` 兼容两者 |
| ECC 结构 | `ECCrefPublicKey`/`ECCCipher`（`pack(1)`、`ECCref_MAX_LEN=64`、`C[1]`）| 同布局（`pack(1)`、`ECCref_MAX_LEN=64`、含 `C_[254]` 扩展）| 字段偏移一致，swxa 忽略尾部扩展；SM2 有效 32 字节写在 x/y 预留尾部 |
| 对称密钥 | `SDF_GetSymmKeyHandle`（厂商扩展）| 已补：读 `kek-<index>.key` 文件 | 预置 KEK 模拟（`teccsdfinit -kek N` 生成）|
| 设备打开 | `SDF_OpenDeviceWithPathAndName`（厂商扩展）| 已补：幂等封装 `SDF_OpenDevice` | 重复打开返回既有句柄，避免单例冲突 |
| 裸密钥导入 | `SDF_ImportKey`（厂商扩展）| 已补：裸明文导入会话密钥 | DEK 数据路径 `sm4_encrypt/decrypt` |
| SM2 公钥导出 | `SDF_ExportSignPublicKey_ECC` / `SDF_ExportEncPublicKey_ECC` | 已有：读 `sm2signpub-<index>.pem` / `sm2encpub-<index>.pem` | 适配器 **enc 优先**（与解密 enc 私钥匹配）|
| SM2 私钥授权 | `SDF_GetPrivateKeyAccessRight` | 已有：先按键文件头判别——无口令 PEM 直接解析、口令加密 PEM 用口令解密 | 口令来自 `TECC_SDF_PASS`；未设置时传空口令，对应无口令设备密钥（真机常见形态）|
| SM2 设备内解密 | `SDF_InternalDecrypt_ECC`（`SGD_SM2_3`）| 已有：GmSSL `sm2_do_decrypt` | 输入 ECCCipher（DER 信封解析后填入）|

调用链示例（HE 私钥信封解密 / KEK 轮换）：

```
tecc_he_load_dek_privkey → hsm_adapter->sm2_decrypt（swxa_sm2_decrypt）
  → SDF_GetPrivateKeyAccessRight(session, index, TECC_SDF_PASS ?: "")  # 授权加载 SM2 私钥（真机设备密钥无口令，传空口令）
  → SDF_InternalDecrypt_ECC(session, index, SGD_SM2_3, ECCCipher, ...)
```

> 密文格式注意：`tecc_sm2_envelope_encrypt`（OpenSSL SM2 EVP）输出 **DER（ASN.1 SEQUENCE）**
> 信封（`30 81 <len> | 02 <x> | 02 <y> | 04 <M> | 04 <C2>`），非裸 C1||C3||C2；适配器
> `swxa_sm2_decrypt` 需先做 DER TLV 解析再填 ECCCipher。

## 编译验证记录（已完成）

- GmSSL 3.3 从 gitee 镜像（`gitee.com/mirrors/GmSSL`）拉取编译，本地安装至 `~/gmssl-install`；
- `tecc_kms` 以 `cmake -DGmSSL_ROOT_DIR=~/gmssl-install . && make` 构建成功，
  产物 `libteccsdf.so` + `teccsdfinit` + `teccsdftest`；
- `teccsdfinit -kek 1 -key 1 -pass ...` 成功生成 KEK 与 SM2 签名/加密密钥对（真实算法）；
- `teccsdftest` 自测通过（`test ok`）。

## 关键实现要点（已完成/后续）

- ~~补齐裸 `SDF_ImportKey` 接口，并同步更新 `src/softsdf.lds` 导出符号表~~（已完成，`libteccsdf.so` 已导出）；
- ~~补齐 swxa 厂商扩展 `SDF_OpenDeviceWithPathAndName` / `SDF_GetSymmKeyHandle`~~（已完成，见"接口覆盖与差异"）；
- SM2 密文格式：已实测确认 `tecc_sm2_envelope_encrypt`（OpenSSL SM2 EVP）输出 **DER（ASN.1 SEQUENCE）** 信封（非裸 C1||C3||C2），`swxa_sm2_decrypt` 按 DER TLV 解析；公钥点为 65 字节 `04||X||Y`；
- 接口对齐：见下节"与工程 SDF 接口对齐（已实施）"。

## 与工程 SDF 接口对齐（已实施：fe-swxa-adapter.c / sdf_client.h）

工程 swxa 适配器 `fe-swxa-adapter.c` 按厂商 SDK 抽取的 `src/include/hsm/sdf_client.h` 签名
调用 SDF 函数，`libteccsdf.so`（SoftSDF）实现同名接口。差异及已落地的对齐方式：

| 差异项 | swxa sdf_client.h（厂商 SDK）| SoftSDF sdf.h（标准）| 已实施的对齐 |
|---|---|---|---|
| 句柄类型 | `long long` | `void *` | 64 位 ABI 等价（均 8 字节值传递），适配器统一声明 `void *`（与 SoftSDF 匹配、与 swxa 兼容）|
| 调用约定 | `SDF_CALL __stdcall`（Windows）| 默认 cdecl | Linux 无影响 |
| 结构体对齐 | `#pragma pack(1)`、`ECCref_MAX_LEN=64` | `pack(1)`、`ECCref_MAX_LEN=64`、`ECCCipher` 含 `C_[254]` 扩展 | 适配器本地定义与 SoftSDF 同布局结构（swxa 忽略尾部扩展，字段偏移一致；SM2 有效 32 字节写在 x/y 预留尾部）|
| 厂商扩展接口 | `SDF_OpenDeviceWithPathAndName`、`SDF_GetSymmKeyHandle`、`SDF_ImportKey` | 已补齐同名接口 | 无需桥接，直接 dlsym 调用 |
| 常量 | `SGD_SM2_3 = 0x00020800`（sdf_def.h）| 一致（sgd.h）| 一致 |

**结论**：无需桥接适配层——swxa 适配器与 SoftSDF 在 64 位下直接对接（`void *` 句柄 +
同布局结构 + 补齐的厂商扩展接口），`TECC_SDF_LIB` 在 SoftSDF 与真实厂商库之间切换。

---

## 附录：JDBC 接入 tecc_kms 方案（JNA，仅文档）

JDBC 驱动（`~/pg14_tecc_dev/pgjdbc-release-42.5.x`）已有完整的 HSM vendor 分发架构：
`HsmVendorClassLoader`（加载厂商 JAR）→ `HsmVendorDekDecryptor/Encryptor`（vendor 接口）→
`HsmVendorDekDecryptorRegistry`（EnumMap 按 vendor 分发）。现有实现 `SwxaHsmVendorDekDecryptor`
（JCE Provider 路线）、`TzwyHsmVendorDekDecryptor`（反射 + JNA 路线）。

`libteccsdf.so` 是标准 SDF 动态库，JDBC 通过 **JNA 直接加载**即可复用，无需 Java 版 SoftSDF。

### JNA 绑定接口定义（新增 `TeccKmsSdfLibrary`）

```java
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.PointerByReference;
import com.sun.jna.ptr.IntByReference;

/** JNA 映射 libteccsdf.so 的标准 SDF 接口（GM/T 0018）。 */
public interface TeccKmsSdfLibrary extends Library {
    TeccKmsSdfLibrary INSTANCE =
        Native.load("teccsdf", TeccKmsSdfLibrary.class);  // 运行时 -Djna.library.path=tecc_kms/bin

    int SDF_OpenDevice(PointerByReference phDeviceHandle);
    int SDF_CloseDevice(Pointer hDeviceHandle);
    int SDF_OpenSession(Pointer hDeviceHandle, PointerByReference phSessionHandle);
    int SDF_CloseSession(Pointer hSessionHandle);
    int SDF_GenerateRandom(Pointer hSessionHandle, int uiLength, byte[] pucRandom);
    int SDF_ImportKeyWithKEK(Pointer hSessionHandle, int uiAlgID, int uiKEKIndex,
                             byte[] pucKey, int uiKeyLength, PointerByReference phKeyHandle);
    int SDF_Encrypt(Pointer hSessionHandle, Pointer hKeyHandle, int uiAlgID,
                    byte[] pucIV, byte[] pucData, int uiDataLength,
                    byte[] pucEncData, IntByReference puiEncDataLength);
    int SDF_Decrypt(Pointer hSessionHandle, Pointer hKeyHandle, int uiAlgID,
                    byte[] pucIV, byte[] pucEncData, int uiEncDataLength,
                    byte[] pucData, IntByReference puiDataLength);
    int SDF_DestroyKey(Pointer hSessionHandle, Pointer hKeyHandle);
    int SDF_ExportSignPublicKey_ECC(Pointer hSessionHandle, int uiKeyIndex, byte[] pucPublicKey);
}
```

### 新增类清单（JDBC 侧改动）

| 类 | 位置 | 说明 |
|---|---|---|
| `HsmConfig.Vendor.TECC_KMS` | `tecc/key/device/HsmConfig.java` | 新增枚举值；`fromKeyStore` 映射 `tecckms`/`softsdf` → TECC_KMS |
| `TeccKmsSdfLibrary` | `tecc/key/security/hsm/` | JNA 绑定（上表），`Native.load("teccsdf", ...)` |
| `TeccKmsDeviceManager` | 同上 | 封装 openDevice/openSession/importKey/encrypt/decrypt/destroyKey 生命周期，参考 `TzwyHsmDeviceManager` |
| `TeccKmsVendorDekDecryptor` | 同上 | 实现 `HsmVendorDekDecryptor`：`vendor()=TECC_KMS`，`decrypt()` 调 JNA 解开 DEK 信封 |
| `TeccKmsVendorDekEncryptor` | 同上 | 实现 `HsmVendorDekEncryptor`（若有） |
| 注册 | `HsmDekSecurityOps` 构造处 | 向 `HsmVendorDekDecryptorRegistry` 注册 `TeccKmsVendorDekDecryptor` |

### 接入步骤

1. 实现上述 JNA 绑定与 vendor 实现类（参考 `TzwyHsmVendorDekDecryptor` 的 SDF 调用链）；
2. 运行时以 `-Djna.library.path=<tecc_kms>/bin` 让 JNA 找到 `libteccsdf.so`；
3. 连接串 `key_store=tecckms`（或新增映射），触发 `HsmConfig.Vendor.TECC_KMS` 分发；
4. 联调通过后，生产切换回厂商实现（`swxa`/`tzwy`），仅改配置与 vendor 类。

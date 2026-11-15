# tecc_kms/jce — SoftSDF 的 JCE 门面（回归测试专用）

> **定位**：不是产品件。它只为**在没有真实加密机的 Linux 上跑通 JDBC 侧的硬加密
> （`key_store='swxakms'`）回归**而存在。

---

## 一、为什么需要它

TECC 的加密/解密在**客户端**完成。libpq 侧通过 `TECC_SDF_LIB=<libteccsdf.so>` 就能把
三未信安（swxa）适配器的设备调用切到软件密码机 SoftSDF；但 JDBC 侧走的是**另一条路线**：

```
HsmConfig.Vendor.SWXA  →  反射加载 com.sansec.jce.provider.SwxaProvider（厂商 JCE Provider）
                       →  KeyGenerator("SM4").init(keyIndex<<16)            取设备内对称 KEK
                       →  Cipher("SM4/CBC/NoPadding")                       封装/解封 DEK、列级加解密
                       →  SecureRandom("RND")                               生成封装 IV
                       →  KeyPairGenerator("SM2").initialize(keyIndex<<16)  取设备内 SM2 密钥对
                       →  Cipher("SM2")                                     解 HE 私钥信封
```

也就是说 JDBC 侧吃的是 **JCE Provider**，而不是 SDF 动态库——SoftSDF 无法直接插进去。
本组件就是补上这一层：提供**同名类、同 `PROVIDER_NAME`、同算法集**的 drop-in Provider，
让上述全部代码路径原样工作，**无需改动 pgjdbc 任何产品代码**。

## 二、密码学后端：读 SoftSDF 的密钥资产（纯 Java，无 native）

| 驱动请求 | 本 Provider 的实现 |
|---|---|
| `KeyGenerator("SM4").init(keyIndex<<16)` | 读 CWD 下 `kek-<index>.key`（裸 16B SM4 密钥，与 `SDF_GetSymmKeyHandle` 同一份资产），包成 `SecretKeySpec("SM4")` |
| `Cipher("SM4/CBC/NoPadding")` | `Sm4Cipher.encryptCbcNoPadding/decryptCbcNoPadding`（驱动内已有） |
| `SecureRandom("RND")` | JVM 强随机源（只用于生成 16B IV，不参与密钥派生） |
| `KeyPairGenerator("SM2").initialize(keyIndex<<17)` | 读 CWD 下 `sm2enc-<index>.pem`（**加密**对，与 libpq `SDF_ExportEncPublicKey_ECC`/`SGD_SM2_3` 同一把），公钥由 `Sm2Cipher.derivePublicKey` 推导 |
| `KeyPairGenerator("SM2").initialize(keyIndex<<16)` | 读 CWD 下 `sm2sign-<index>.pem`（**签名**对，与 libpq `SGD_SM2_1` 同一把；驱动 HE 解封回退路径） |
| `Cipher("SM2")` | `Sm2Cipher.envelopeDecrypt(privDer, envelope)`，DER 与 raw `04‖C1x‖C1y‖C3‖C2` 两种信封都接受 |

> **位移约定（与 libpq 对齐）**：驱动 `SwxaSm2PublicKeyExporter` 用 `index<<17` 取加密对、
> `SwxaHeEnvelopeDecryptor` 先试 `<<17` 再回退 `<<16`；SoftSDF/`fe-swxa-adapter` 则按
> `sm2enc-<n>.pem`（加密）与 `sm2sign-<n>.pem`（签名）分文件。旧门面只认 `<<16` 且一律读
> `sm2enc`，驱动改用 `<<17` 后会把索引 3 解成 6（`3<<17 >>> 16 = 6`）导致找不到密钥。

**字节等价依据**（不是"差不多"，是逐字节核对过的）：

- SoftSDF 的 `SDF_Encrypt/Decrypt` 在 `SGD_SM4_CBC` 分支直接调 GmSSL 的
  `sm4_cbc_encrypt_blocks`——标准 SM4-CBC、无填充、无 HMAC；`Sm4Cipher` 同语义。
  已用 native 交叉校验：同一 `kek-1.key` + 同一 IV + 同一明文，
  `SDF_Encrypt` 与 Java Provider 输出**完全相同的密文**。
- SM2 走标准算法；`sm2enc-<index>.pem` 即 `SDF_InternalDecrypt_ECC` 使用的
  `container->enc_key`（libpq 侧同样是 `SDF_ExportEncPublicKey_ECC` 导出的那把加密密钥）。
  已校验：由私钥推导的 65B 公钥与 `sm2encpub-<index>.pem` 内的点一致。
- `kek-<index>.key` 与 `sm2enc-<index>.pem` 的**文件名/路径约定与 SoftSDF 完全一致**
  （`softsdf.c` 同样是 `fopen("kek-%u.key")` / `fopen("sm2enc-%u.pem")`），
  且都是**相对 CWD**——回归框架已把这些资产预置到 runner 的工作目录 `tmp_check/tmp`。

## 三、驱动侧的元契约（三条都不能破）

驱动用「类是否来自含 `org/postgresql/Driver.class` 的 JAR」判断 stub，因此：

1. `com.sansec.jce.provider.SwxaProvider` 必须**独立成 JAR**（不能打进 pgjdbc）；
2. 必须有 `public static final String PROVIDER_NAME`（本实现为 `SwxaJCE`）与 public 无参构造；
3. `getServices()` 必须非空。

另：`SecureRandom.<alg>` 注册的类必须**继承 `SecureRandomSpi`** 且为 public
（参照 SUN provider 的 `SecureRandom.SHA1PRNG → sun.security.provider.SecureRandom`），
否则 `SecureRandom.getInstance()` 直接拒绝——这是本组件开发中踩过的坑。

## 四、构建

```bash
# 需先有 JDBC 驱动的 jar（编译期要用到 Sm4Cipher/Sm2Cipher）
make -C src/test/modules/tecc_tests tecc-sdf-jce-jar
# 产物：tecc_kms/bin/libteccjce.jar（构建中间件在 tecc_kms/jce/build/）
```

`make check-tecc-jdbc` 会自动带上这一步（在 `jdbc-jar` 之后，因为编译期依赖驱动 jar）。

## 五、使用

**仅在 SoftSDF 模式下**由 `t/002_tecc_jdbc_test.pl` 自动接入（SoftSDF 是回归默认模式，
所以下面这条命令不设 `TECC_SDF_LIB` 即可）：

```bash
TECC_JDBC_HOME=<pgjdbc 项目根> \
make check-tecc-jdbc
# 换别的 SoftSDF 库：TECC_SDF_LIB=/path/to/libteccsdf.so
# 打真机（本 JAR 不参与）：TECC_SDF_LIB=off
```

harness 做三件事：把本 JAR 放在 runner classpath **首位**、写入 `hsm.properties`
（`swxa.ini.path` 只做存在性校验，内容无关）并传 `--tecc-config`。

> ⚠ **非 SoftSDF 模式（真实加密机，`TECC_SDF_LIB=off`）绝不能把本 JAR 放进 classpath**
> ——它会把真厂商的 `com.sansec.jce.provider.SwxaProvider` 顶掉。harness 已按此把关
> （只在 `sdf_soft_mode()` 为真时加入）。

## 六、目录

```
tecc_kms/jce/
├── src/com/sansec/jce/provider/
│   ├── SwxaProvider.java                  # Provider 注册（5 个 Service）
│   ├── SoftSdfKeyAssets.java              # 读 kek-<index>.key / sm2enc-<n>.pem / sm2sign-<n>.pem（相对 CWD）
│   ├── SoftSdfSm4KeyGeneratorSpi.java     # KeyGenerator.SM4（入参 = keyIndex<<16）
│   ├── SoftSdfSm4CipherSpi.java           # Cipher.SM4/CBC/NoPadding
│   ├── SoftSdfSm2KeyPairGeneratorSpi.java # KeyPairGenerator.SM2（入参 = keyIndex<<16）
│   ├── SoftSdfSm2CipherSpi.java           # Cipher.SM2
│   ├── SoftSdfSm2Curve.java               # sm2p256v1 域参数（供 ECPublicKey 用）
│   ├── SoftSdfSm2PrivateKey.java / SoftSdfSm2PublicKey.java
│   └── SoftSdfSecureRandomSpi.java        # SecureRandom.RND
└── build/                                 # 构建中间产物（不入库）
```

> 编译时 javac 会打一条"使用了已过时的 API"提示：Provider 构造用的是
> `(name, double, info)`（JDK 8 可用），而 `(name, String, info)` 要 JDK 9+。
> 为保住 Java 8 兼容性而刻意保留，不是问题。

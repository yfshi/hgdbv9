package com.sansec.jce.provider;

import java.security.Provider;

/**
 * SoftSDF 的 JCE 门面（回归测试专用，非产品件）。
 *
 * ReqID:TECC4.0
 *
 * 背景：TECC 的 JDBC 客户端在硬加密（key_store='swxakms'）时走 JCE Provider 路线
 * ——{@code HsmVendorClassLoader} 按全限定名 {@code com.sansec.jce.provider.SwxaProvider}
 * 反射加载厂商类（优先扫 classpath 上的非驱动 JAR），读其 {@code PROVIDER_NAME} 静态字段，
 * 再 {@code Security.addProvider}。因此本类只要提供**同名类 + 同名字段 + 同算法集**，
 * 就能顶替真实 swxajce 接管整条 SWXA 路径，无需改动 pgjdbc 任何产品代码。
 *
 * 密钥资产沿用 SoftSDF 的约定（**相对 CWD**）：{@code kek-<index>.key}（裸 16B SM4 密钥）、
 * {@code sm2enc-<index>.pem}（PKCS#8 SM2 私钥）。密码学实现复用驱动内已有的
 * {@code Sm4Cipher}（标准 SM4-CBC NoPadding，与 SoftSDF 的 {@code SDF_Encrypt(SGD_SM4_CBC)} 字节等价）
 * 与 {@code Sm2Cipher}（标准 SM2 信封解密），故输出与 libpq 的 SoftSDF 路径一致。
 *
 * 元契约（三条都不能破，否则会被驱动判为 stub 或注册失败）：
 * <ul>
 *   <li>{@code public static final String PROVIDER_NAME}</li>
 *   <li>public 无参构造</li>
 *   <li>{@code getServices()} 非空</li>
 * </ul>
 * 另：本 JAR 必须**独立于 pgjdbc JAR**，因为驱动用「类是否来自含
 * {@code org/postgresql/Driver.class} 的 JAR」来判定 stub。
 */
public final class SwxaProvider extends Provider {

  private static final long serialVersionUID = 1L;

  /** Provider 名：与真实三未信安厂商类同名，驱动按此名注册并据此查找算法。 */
  public static final String PROVIDER_NAME = "SwxaJCE";

  /** 注册 SoftSDF 支持的算法集（恰好覆盖驱动 SWXA 路径的全部 5 个调用点）。 */
  public SwxaProvider() {
    // 用 (name, double, info) 旧构造：本 JAR 需能在 JDK 8 下编译（pgjdbc 42.5 支持 Java 8），
    // 而 (name, String, info) 是 JDK 9+ 才有的。代价是 javac 会打一条过时 API 提示。
    super(PROVIDER_NAME, 1.0, "TECC SoftSDF JCE facade (regression only)");

    // 取设备内对称 KEK：驱动以 keyIndex<<16 作 KeyGenerator.init 的实参传入。
    put("KeyGenerator.SM4", SoftSdfSm4KeyGeneratorSpi.class.getName());

    // SM4-CBC/NoPadding：既承载 DEK 封装/解封（key 来自上面那把 KEK），
    // 也承载列级加解密（key 是裸 SecretKeySpec 形态的用户 DEK）。
    put("Cipher.SM4/CBC/NoPadding", SoftSdfSm4CipherSpi.class.getName());

    // 设备随机数：驱动用 SecureRandom.getInstance("RND") 生成 DEK 封装的 IV。
    // 注册的类必须继承 SecureRandomSpi 且为 public，否则 SecureRandom.getInstance 会拒绝。
    put("SecureRandom.RND", SoftSdfSecureRandomSpi.class.getName());

    // 取设备内 SM2 密钥对：驱动以 keyIndex<<16 作 KeyPairGenerator.initialize 的实参传入；
    // 公钥用于 CREATE KEK 注入 PUBLIC_KEY，私钥用于解 HE 私钥信封。
    put("KeyPairGenerator.SM2", SoftSdfSm2KeyPairGeneratorSpi.class.getName());
    put("Cipher.SM2", SoftSdfSm2CipherSpi.class.getName());
  }
}

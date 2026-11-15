package com.sansec.jce.provider;

import java.security.SecureRandom;
import java.security.SecureRandomSpi;

/**
 * {@code RND} 算法的随机源（设备随机数）。
 *
 * ReqID:TECC4.0
 *
 * 对应 SoftSDF 的 {@code SDF_GenerateRandom}。驱动只用它生成 DEK 封装的 16B IV，
 * 不参与密钥派生，故此处用 JVM 的强随机源即可——真机走密码机硬件随机数，但两者
 * 都只需满足「不可预测且每次不同」，且 IV 差异会被回归的密文 hex 归一化吸收。
 *
 * 注意：{@code SecureRandom.<alg>} 注册的类必须继承 {@link SecureRandomSpi}
 * （参照 SUN provider 的 {@code SecureRandom.SHA1PRNG → sun.security.provider.SecureRandom}），
 * 且必须是 public 类——两者缺一会被 {@code SecureRandom.getInstance()} 拒绝。
 */
public final class SoftSdfSecureRandomSpi extends SecureRandomSpi {

  private static final long serialVersionUID = 1L;

  // 用 JDK 默认强随机源（本 provider 追加在 Provider 列表末尾，不会被选作默认，故无递归）
  private final SecureRandom delegate = new java.security.SecureRandom();

  @Override
  protected void engineSetSeed(byte[] seed) {
    delegate.setSeed(seed);
  }

  @Override
  protected void engineNextBytes(byte[] bytes) {
    delegate.nextBytes(bytes);
  }

  @Override
  protected byte[] engineGenerateSeed(int numBytes) {
    return delegate.generateSeed(numBytes);
  }
}

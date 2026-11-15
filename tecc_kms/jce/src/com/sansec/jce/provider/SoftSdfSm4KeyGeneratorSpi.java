package com.sansec.jce.provider;

import java.io.IOException;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidParameterException;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;

import javax.crypto.KeyGeneratorSpi;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

/**
 * 设备内对称 KEK 的 KeyGenerator（算法名 {@code SM4}）。
 *
 * ReqID:TECC4.0
 *
 * 入参约定（对齐真实三未信安 JCE）：{@code init()} 收到的是 {@code keyIndex << 16}，
 * 不是密钥位长——驱动侧 {@code HsmCipherSupport.getOrCreateSwxaSm4Key} 就是这么调的。
 * 本实现取出低 16 位之外的索引，读 {@code kek-<index>.key} 作为 KEK 返回。
 */
public final class SoftSdfSm4KeyGeneratorSpi extends KeyGeneratorSpi {

  /** 设备内密钥索引的编码位移（与驱动约定一致）。 */
  private static final int INDEX_SHIFT = 16;

  // 设备内密钥索引；-1 表示尚未 init
  private int keyIndex = -1;

  @Override
  protected void engineInit(int keysize, SecureRandom random) {
    // 驱动约定：实参 = keyIndex << 16，故低 16 位必须为 0，否则不是本 provider 的用法
    if ((keysize & ((1 << INDEX_SHIFT) - 1)) != 0) {
      throw new InvalidParameterException("SM4 key size must encode keyIndex<<16, got: " + keysize);
    }
    int index = keysize >>> INDEX_SHIFT;
    if (index <= 0) {
      throw new InvalidParameterException("SM4 keyIndex must be positive, got: " + index);
    }
    this.keyIndex = index;
  }

  @Override
  protected void engineInit(SecureRandom random) {
    // 无索引无从定位设备内密钥
    throw new InvalidParameterException("SM4 KeyGenerator requires keyIndex<<16 via init(int)");
  }

  @Override
  protected void engineInit(AlgorithmParameterSpec params, SecureRandom random)
      throws InvalidAlgorithmParameterException {
    throw new InvalidAlgorithmParameterException("SM4 KeyGenerator requires keyIndex<<16 via init(int)");
  }

  @Override
  protected SecretKey engineGenerateKey() {
    if (keyIndex < 0) {
      throw new IllegalStateException("SM4 KeyGenerator not initialized");
    }
    try {
      // 设备内 KEK 就是文件里的裸 16 字节；包成 SecretKeySpec 供 Cipher 取用
      return new SecretKeySpec(SoftSdfKeyAssets.readKek(keyIndex), "SM4");
    } catch (IOException e) {
      throw new IllegalStateException("failed to load device KEK (index=" + keyIndex + "): " + e.getMessage(), e);
    }
  }
}

package com.sansec.jce.provider;

import java.io.IOException;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidParameterException;
import java.security.KeyPair;
import java.security.KeyPairGeneratorSpi;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;

import org.postgresql.tecc.algorithm.key.Sm2Cipher;

/**
 * 设备内 SM2 密钥对的 KeyPairGenerator（算法名 {@code SM2}）。
 *
 * ReqID:TECC4.0
 *
 * 入参约定（对齐驱动 SwxaSm2PublicKeyExporter / SwxaHeEnvelopeDecryptor，与 libpq
 * 的 sign/enc 密钥对分离同一语义）：
 * <ul>
 *   <li>{@code initialize(index << 17)} —— **加密**密钥对 → {@code sm2enc-<index>.pem}
 *       （libpq：{@code SDF_ExportEncPublicKey_ECC} / {@code SGD_SM2_3}）</li>
 *   <li>{@code initialize(index << 16)} —— **签名**密钥对 → {@code sm2sign-<index>.pem}
 *       （libpq：{@code SDF_ExportSignPublicKey_ECC} / {@code SGD_SM2_1}；驱动 HE 解封回退）</li>
 * </ul>
 *
 * 公钥由对应私钥推导（{@code Sm2Cipher.derivePublicKey}），该方法在 DER 内嵌公钥时会校验
 * 公私钥自洽，因此不会出现"pem 被换过却静默用错密钥"的分叉。
 */
public final class SoftSdfSm2KeyPairGeneratorSpi extends KeyPairGeneratorSpi {

  /** 设备内密钥索引的低位编码位移（sign = 16，enc = 17，与驱动约定一致）。 */
  private static final int SIGN_SHIFT = 16;
  private static final int ENC_SHIFT = 17;

  /** 低 16 位必须为 0（两种位移都要求）。 */
  private static final int LOW16_MASK = (1 << SIGN_SHIFT) - 1;

  // 解析结果；null 表示尚未 initialize
  private SoftSdfKeyAssets.Sm2KeyKind kind;
  private int keyIndex = -1;

  @Override
  public void initialize(int keysize, SecureRandom random) {
    if ((keysize & LOW16_MASK) != 0) {
      throw new InvalidParameterException(
          "SM2 key size must encode keyIndex<<16 (sign) or keyIndex<<17 (enc), got: " + keysize);
    }
    int idx16 = keysize >>> SIGN_SHIFT;
    if (idx16 <= 0) {
      throw new InvalidParameterException("SM2 keyIndex must be positive, got: " + idx16);
    }

    // 奇数 idx16 只能来自 <<16（<<17 后 idx16 恒为偶数）→ 签名对
    if ((idx16 & 1) != 0) {
      this.kind = SoftSdfKeyAssets.Sm2KeyKind.SIGN;
      this.keyIndex = idx16;
      return;
    }

    // 偶数 idx16：可能是 <<17 加密对（index = idx16/2），也可能是偶数索引的 <<16 签名对。
    // 按文件存在性消歧，与 SoftSDF 预置密钥布局一致（HE 用 3/4，且 enc/sign 成对预置）。
    int idx17 = keysize >>> ENC_SHIFT;
    if (idx17 > 0 && SoftSdfKeyAssets.sm2PrivateExists(SoftSdfKeyAssets.Sm2KeyKind.ENC, idx17)
        && (idx17 << ENC_SHIFT) == keysize) {
      this.kind = SoftSdfKeyAssets.Sm2KeyKind.ENC;
      this.keyIndex = idx17;
      return;
    }
    if (SoftSdfKeyAssets.sm2PrivateExists(SoftSdfKeyAssets.Sm2KeyKind.SIGN, idx16)) {
      this.kind = SoftSdfKeyAssets.Sm2KeyKind.SIGN;
      this.keyIndex = idx16;
      return;
    }
    if (SoftSdfKeyAssets.sm2PrivateExists(SoftSdfKeyAssets.Sm2KeyKind.ENC, idx16)) {
      // 兼容旧门面：仅预置 sm2enc 且驱动曾统一按 <<16 送入
      this.kind = SoftSdfKeyAssets.Sm2KeyKind.ENC;
      this.keyIndex = idx16;
      return;
    }
    throw new InvalidParameterException(
        "SM2 key size " + keysize + " resolves to sign-index=" + idx16
            + " / enc-index=" + idx17
            + " but neither sm2sign-<n>.pem nor sm2enc-<n>.pem exists in CWD");
  }

  @Override
  public void initialize(AlgorithmParameterSpec params, SecureRandom random)
      throws InvalidAlgorithmParameterException {
    throw new InvalidAlgorithmParameterException(
        "SM2 KeyPairGenerator requires keyIndex<<16 (sign) or keyIndex<<17 (enc) via initialize(int)");
  }

  @Override
  public KeyPair generateKeyPair() {
    if (keyIndex < 0 || kind == null) {
      throw new IllegalStateException("SM2 KeyPairGenerator not initialized");
    }
    byte[] der;
    try {
      der = SoftSdfKeyAssets.readSm2PrivateDer(kind, keyIndex);
    } catch (IOException e) {
      throw new IllegalStateException(
          "failed to load device SM2 " + kind + " key (index=" + keyIndex + "): "
              + e.getMessage(), e);
    }
    return new KeyPair(
        new SoftSdfSm2PublicKey(Sm2Cipher.derivePublicKey(der)),
        new SoftSdfSm2PrivateKey(der));
  }
}

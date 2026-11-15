package com.sansec.jce.provider;

import java.security.PrivateKey;

/**
 * 设备内 SM2 私钥的载体（模拟件）。
 *
 * ReqID:TECC4.0
 *
 * 真机形态下私钥不出设备，只以句柄存在；本模拟件把 {@code sm2enc-<index>.pem} 的
 * PKCS#8 DER 直接持有，供 {@code Cipher("SM2")} 解 HE 私钥信封。用 DER 作为载体是因为
 * 驱动侧 {@code Sm2Cipher.envelopeDecrypt} 的入参契约就是 PKCS#8 DER。
 */
public final class SoftSdfSm2PrivateKey implements PrivateKey {

  private static final long serialVersionUID = 1L;

  // sm2enc-<index>.pem 去 PEM 封装后的 PKCS#8 DER
  private final byte[] der;

  SoftSdfSm2PrivateKey(byte[] der) {
    this.der = der;
  }

  /** PKCS#8 DER 私钥字节。 */
  byte[] der() {
    return der;
  }

  @Override
  public String getAlgorithm() {
    return "SM2";
  }

  @Override
  public String getFormat() {
    return "PKCS#8";
  }

  @Override
  public byte[] getEncoded() {
    return der.clone();
  }
}

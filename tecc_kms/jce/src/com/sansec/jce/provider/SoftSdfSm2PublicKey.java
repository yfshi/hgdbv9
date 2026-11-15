package com.sansec.jce.provider;

import java.security.interfaces.ECPublicKey;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;

/**
 * 设备内 SM2 公钥的载体（模拟件）。
 *
 * ReqID:TECC4.0
 *
 * 实现 {@link ECPublicKey} 而非裸 {@code PublicKey}，是因为驱动侧
 * {@code SwxaSm2PublicKeyExporter.publicKeyToUncompressedPoint} **优先**走
 * {@code ECPublicKey.getW()} 取 X/Y 组装 65B 未压缩点；只有非 EC 公钥时才退到
 * 扫 X.509 编码。走主路径可以顺带验证该分支。
 */
public final class SoftSdfSm2PublicKey implements ECPublicKey {

  private static final long serialVersionUID = 1L;

  // 65B 未压缩点 04‖X‖Y
  private final byte[] point65;
  // 曲线点
  private final ECPoint w;

  SoftSdfSm2PublicKey(byte[] point65) {
    this.point65 = point65;
    this.w = SoftSdfSm2Curve.decodePoint(point65);
  }

  @Override
  public ECPoint getW() {
    return w;
  }

  @Override
  public ECParameterSpec getParams() {
    return SoftSdfSm2Curve.params();
  }

  @Override
  public String getAlgorithm() {
    return "SM2";
  }

  @Override
  public String getFormat() {
    // 非 X.509：本模拟件的编码就是曲线点本身
    return "RAW";
  }

  @Override
  public byte[] getEncoded() {
    // 返回原始点。驱动侧兜底分支（扫 65B 未压缩点）同样接受这一形态
    return point65.clone();
  }
}

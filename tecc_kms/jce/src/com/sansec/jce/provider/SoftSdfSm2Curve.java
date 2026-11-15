package com.sansec.jce.provider;

import java.math.BigInteger;
import java.security.spec.ECFieldFp;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;
import java.security.spec.EllipticCurve;

/**
 * sm2p256v1 曲线参数（仅供 {@link SoftSdfSm2PublicKey} 组装 {@code ECPublicKey} 用）。
 *
 * ReqID:TECC4.0
 *
 * 常量与驱动内 {@code Sm2Cipher} 完全一致（同一条曲线，避免"公钥点用一条曲线编码、
 * 上层按另一条曲线解读"的静默分叉）。
 */
final class SoftSdfSm2Curve {

  /** 素域 p。 */
  private static final BigInteger P = new BigInteger(
      "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF", 16);
  /** 曲线系数 a。 */
  private static final BigInteger A = new BigInteger(
      "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC", 16);
  /** 曲线系数 b。 */
  private static final BigInteger B = new BigInteger(
      "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93", 16);
  /** 基点阶 n。 */
  private static final BigInteger N = new BigInteger(
      "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123", 16);
  /** 基点 G。 */
  private static final ECPoint G = new ECPoint(
      new BigInteger("32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7", 16),
      new BigInteger("BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0", 16));

  /** 坐标字节数（sm2p256v1 的 X/Y 各 32B）。 */
  static final int COORD_BYTES = 32;
  /** 未压缩点长度（04‖X‖Y）。 */
  static final int POINT_BYTES = 65;

  private static final ECParameterSpec PARAMS =
      new ECParameterSpec(new EllipticCurve(new ECFieldFp(P), A, B), G, N, 1);

  private SoftSdfSm2Curve() {
  }

  /** sm2p256v1 域参数。 */
  static ECParameterSpec params() {
    return PARAMS;
  }

  /** 拆 65B 未压缩点 {@code 04‖X‖Y} 为曲线点。 */
  static ECPoint decodePoint(byte[] point65) {
    return new ECPoint(
        new BigInteger(1, slice(point65, 1, COORD_BYTES)),
        new BigInteger(1, slice(point65, 1 + COORD_BYTES, COORD_BYTES)));
  }

  /** 曲线点 → 65B 未压缩点 {@code 04‖X‖Y}。 */
  static byte[] encodePoint(ECPoint w) {
    byte[] out = new byte[POINT_BYTES];
    out[0] = 0x04;
    copyFixed32(w.getAffineX(), out, 1);
    copyFixed32(w.getAffineY(), out, 1 + COORD_BYTES);
    return out;
  }

  // BigInteger 坐标 → 定长 32B 大端（剥符号字节、左补零）
  private static void copyFixed32(BigInteger v, byte[] dst, int off) {
    byte[] b = v.toByteArray();
    int start = (b.length > 0 && b[0] == 0) ? 1 : 0;
    int len = b.length - start;
    if (len > COORD_BYTES) {
      throw new IllegalArgumentException("SM2 coordinate exceeds " + COORD_BYTES + " bytes");
    }
    System.arraycopy(b, start, dst, off + COORD_BYTES - len, len);
  }

  private static byte[] slice(byte[] src, int off, int len) {
    byte[] out = new byte[len];
    System.arraycopy(src, off, out, 0, len);
    return out;
  }
}

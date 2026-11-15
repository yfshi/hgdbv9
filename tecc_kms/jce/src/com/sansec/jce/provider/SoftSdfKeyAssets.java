package com.sansec.jce.provider;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Base64;

/**
 * SoftSDF 密钥资产的读取（相对 CWD，与 {@code softsdf.c} 的文件名约定一致）。
 *
 * ReqID:TECC4.0
 *
 * 对应关系（与 libpq / SoftSDF 一致）：
 * <ul>
 *   <li>{@code kek-<index>.key} —— {@code SDF_GetSymmKeyHandle()}（裸 16B SM4 密钥，无头无 IV）</li>
 *   <li>{@code sm2enc-<index>.pem} —— 设备内**加密**密钥对私钥，
 *       对应 libpq {@code SDF_ExportEncPublicKey_ECC} / {@code SDF_InternalDecrypt_ECC(SGD_SM2_3)}</li>
 *   <li>{@code sm2sign-<index>.pem} —— 设备内**签名**密钥对私钥，
 *       对应 libpq {@code SDF_ExportSignPublicKey_ECC} / {@code SGD_SM2_1}</li>
 * </ul>
 *
 * SoftSDF 用 {@code fopen("kek-N.key")} 即相对路径读，故这里同样相对 CWD；回归框架已把
 * 这些资产预置到 runner 的工作目录（{@code tmp_check/tmp}）。
 */
final class SoftSdfKeyAssets {

  /** 对称 KEK 长度（SM4 主密钥）。 */
  static final int KEK_BYTES = 16;

  /** 设备内 SM2 密钥对类型：与驱动 JCE 位移约定 / libpq 算法 ID 对齐。 */
  enum Sm2KeyKind {
    /** 加密对：驱动 {@code initialize(index<<17)} ↔ libpq {@code SGD_SM2_3} / {@code sm2enc-<n>.pem}。 */
    ENC,
    /** 签名对：驱动 {@code initialize(index<<16)} ↔ libpq {@code SGD_SM2_1} / {@code sm2sign-<n>.pem}。 */
    SIGN
  }

  private SoftSdfKeyAssets() {
  }

  /** 取设备内对称 KEK：{@code kek-<index>.key}，裸 16 字节。 */
  static byte[] readKek(int keyIndex) throws IOException {
    Path p = Paths.get("kek-" + keyIndex + ".key");
    byte[] kek = Files.readAllBytes(p);
    if (kek.length != KEK_BYTES) {
      throw new IOException("invalid KEK file " + p.toAbsolutePath()
          + ": expected " + KEK_BYTES + " bytes, got " + kek.length);
    }
    return kek;
  }

  /**
   * 取设备内 SM2 私钥（DER）：按类型读 {@code sm2enc-<index>.pem} 或 {@code sm2sign-<index>.pem}。
   *
   * 返回 DER 而非密钥对象，因为驱动侧 {@code Sm2Cipher.envelopeDecrypt/derivePublicKey}
   * 的入参契约就是 PKCS#8 DER（与软 KMS 路径同一形态）。
   */
  static byte[] readSm2PrivateDer(Sm2KeyKind kind, int keyIndex) throws IOException {
    Path p = Paths.get(prefix(kind) + keyIndex + ".pem");
    return pemToDer(Files.readAllBytes(p), p.toString());
  }

  /** 指定类型的 SM2 私钥文件是否存在于 CWD（用于 <<16/<<17 歧义消解）。 */
  static boolean sm2PrivateExists(Sm2KeyKind kind, int keyIndex) {
    return Files.isRegularFile(Paths.get(prefix(kind) + keyIndex + ".pem"));
  }

  private static String prefix(Sm2KeyKind kind) {
    return kind == Sm2KeyKind.SIGN ? "sm2sign-" : "sm2enc-";
  }

  /** 剥 PEM 定界行并 base64 解码。 */
  private static byte[] pemToDer(byte[] pem, String name) throws IOException {
    StringBuilder b64 = new StringBuilder();
    for (String line : new String(pem, StandardCharsets.US_ASCII).split("\n")) {
      String t = line.trim();
      if (t.isEmpty() || t.startsWith("-----")) {
        continue;
      }
      b64.append(t);
    }
    if (b64.length() == 0) {
      throw new IOException("empty PEM body: " + name);
    }
    try {
      return Base64.getDecoder().decode(b64.toString());
    } catch (IllegalArgumentException e) {
      throw new IOException("invalid base64 in PEM: " + name, e);
    }
  }
}

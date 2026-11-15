package com.sansec.jce.provider;

import java.security.AlgorithmParameters;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.CipherSpi;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.ShortBufferException;

import org.postgresql.tecc.algorithm.key.Sm2Cipher;

/**
 * {@code SM2} 的 Cipher（用设备 SM2 私钥解 HE 私钥信封）。
 *
 * ReqID:TECC4.0
 *
 * 对齐 SoftSDF 的 {@code SDF_InternalDecrypt_ECC}：libpq 侧把 DER 信封拆成
 * {@code ECCCipher} 再交设备解密，本实现直接解 DER——两者都是标准 SM2 解密，
 * 明文一致。
 *
 * 信封两种形态都接受：DER（驱动主用，OpenSSL EVP_PKEY_encrypt 风格）与 raw
 * {@code 04‖C1x‖C1y‖C3‖C2}（驱动侧解封装失败时的回退形态）。
 */
public final class SoftSdfSm2CipherSpi extends CipherSpi {

  // SM2 非分组密码
  private static final int NOT_A_BLOCK_CIPHER = 0;
  // raw wire 最短长度：C1(65) + C3(32)
  private static final int RAW_WIRE_MIN = 97;
  // ReqID:TECC4.0 - 真机 ECCCipher.C 容量上限（swsds.h 与厂商头文件均为 136 字节），
  // 装不下的信封设备解不开（返回 0x01000804）。与 native SoftSDF 的
  // SDF_InternalDecrypt_ECC 同限，见 tecc_kms/src/softsdf.c。
  private static final int DEVICE_MAX_C2 = 136;

  // 解密方向与设备私钥 DER
  private int opmode = -1;
  private byte[] privDer;

  // 累积输入
  private byte[] buf = new byte[128];
  private int bufLen;

  @Override
  protected void engineSetMode(String mode) throws NoSuchAlgorithmException {
    throw new NoSuchAlgorithmException("SM2 cipher does not support mode: " + mode);
  }

  @Override
  protected void engineSetPadding(String padding) throws NoSuchPaddingException {
    if (!"NoPadding".equalsIgnoreCase(padding)) {
      throw new NoSuchPaddingException("SM2 cipher does not support padding: " + padding);
    }
  }

  @Override
  protected int engineGetBlockSize() {
    return NOT_A_BLOCK_CIPHER;
  }

  @Override
  protected int engineGetOutputSize(int inputLen) {
    // 解密输出不超过输入（信封含 C1/C3 开销）；给足上界即可
    return inputLen;
  }

  @Override
  protected byte[] engineGetIV() {
    return null;
  }

  @Override
  protected AlgorithmParameters engineGetParameters() {
    return null;
  }

  @Override
  protected void engineInit(int opmode, Key key, SecureRandom random) throws InvalidKeyException {
    engineInit(opmode, key, (AlgorithmParameterSpec) null, random);
  }

  @Override
  protected void engineInit(int opmode, Key key, AlgorithmParameterSpec params, SecureRandom random)
      throws InvalidKeyException {
    if (opmode != Cipher.DECRYPT_MODE) {
      // 设备只持有 SM2 私钥，封 HE 信封是客户端用 PUBLIC_KEY 做的（驱动软件路径）
      throw new InvalidKeyException("SoftSDF SM2 cipher only supports DECRYPT_MODE");
    }
    if (!(key instanceof SoftSdfSm2PrivateKey)) {
      throw new InvalidKeyException("expected device SM2 private key, got: "
          + (key == null ? "null" : key.getClass().getName()));
    }
    this.opmode = opmode;
    this.privDer = ((SoftSdfSm2PrivateKey) key).der();
    this.bufLen = 0;
  }

  @Override
  protected void engineInit(int opmode, Key key, AlgorithmParameters params, SecureRandom random)
      throws InvalidKeyException {
    engineInit(opmode, key, (AlgorithmParameterSpec) null, random);
  }

  @Override
  protected byte[] engineUpdate(byte[] input, int inputOffset, int inputLen) {
    append(input, inputOffset, inputLen);
    return new byte[0];
  }

  @Override
  protected int engineUpdate(byte[] input, int inputOffset, int inputLen,
      byte[] output, int outputOffset) {
    append(input, inputOffset, inputLen);
    return 0;
  }

  @Override
  protected byte[] engineDoFinal(byte[] input, int inputOffset, int inputLen)
      throws IllegalBlockSizeException, BadPaddingException {
    append(input, inputOffset, inputLen);
    byte[] envelope = new byte[bufLen];
    System.arraycopy(buf, 0, envelope, 0, bufLen);
    bufLen = 0;

    if (opmode != Cipher.DECRYPT_MODE) {
      throw new IllegalStateException("cipher not initialized");
    }
    if (envelope.length == 0) {
      throw new IllegalBlockSizeException("empty SM2 envelope");
    }
    try {
      // DER 信封首字节恒 0x30（SEQUENCE）；raw wire 首字节是未压缩点标记 0x04
      byte[] der = ((envelope[0] & 0xFF) == 0x04 && envelope.length >= RAW_WIRE_MIN)
          ? Sm2Cipher.rawWireToDerEnvelope(envelope) : envelope;

      // ReqID:TECC4.0 - 按真机容量上限拒绝超长信封：真机 ECCCipher.C 固定 136 字节，
      // 装不下的信封设备返回 0x01000804 解不开。门面若不加同一限制，同一信封会在
      // JDBC 侧绿、在真机上红（假绿）。当前设备 KEK 下的 HE 私钥信封已按 128 字节
      // 分块（TECC_HE_ENV_CHUNK_MAX），正常用例不会触及；本校验拦的是分块机制之前
      // 创建的旧信封。C2 长度 = raw wire 去掉 04‖C1(64)‖C3(32) 的余量。
      int c2Len = Sm2Cipher.derEnvelopeToRawWire(der).length - RAW_WIRE_MIN;
      if (c2Len > DEVICE_MAX_C2) {
        throw new BadPaddingException("SM2 envelope C2 length " + c2Len
            + " exceeds the device ECCCipher.C limit (" + DEVICE_MAX_C2 + ")");
      }
      return Sm2Cipher.envelopeDecrypt(privDer, der);
    } catch (RuntimeException e) {
      // C3 校验失败/信封结构非法 → 按 JCE 契约抛 BadPaddingException
      throw new BadPaddingException("SM2 envelope decrypt failed: " + e.getMessage());
    }
  }

  @Override
  protected int engineDoFinal(byte[] input, int inputOffset, int inputLen,
      byte[] output, int outputOffset) throws ShortBufferException, IllegalBlockSizeException,
      BadPaddingException {
    byte[] out = engineDoFinal(input, inputOffset, inputLen);
    if (output.length - outputOffset < out.length) {
      throw new ShortBufferException("output buffer too small: need " + out.length);
    }
    System.arraycopy(out, 0, output, outputOffset, out.length);
    return out.length;
  }

  private void append(byte[] input, int inputOffset, int inputLen) {
    if (inputLen <= 0) {
      return;
    }
    if (buf.length < bufLen + inputLen) {
      byte[] nb = new byte[Math.max(buf.length * 2, bufLen + inputLen)];
      System.arraycopy(buf, 0, nb, 0, bufLen);
      buf = nb;
    }
    System.arraycopy(input, inputOffset, buf, bufLen, inputLen);
    bufLen += inputLen;
  }
}

package com.sansec.jce.provider;

import java.security.AlgorithmParameters;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;
import java.security.spec.InvalidParameterSpecException;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.CipherSpi;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.ShortBufferException;
import javax.crypto.spec.IvParameterSpec;

import org.postgresql.tecc.algorithm.crypto.Sm4Cipher;

/**
 * {@code SM4/CBC/NoPadding} 的 Cipher（DEK 封装/解封 + 列级数据加解密共用）。
 *
 * ReqID:TECC4.0
 *
 * 与 SoftSDF 的字节等价依据：{@code softsdf.c} 的 {@code SDF_Encrypt/Decrypt} 在
 * {@code SGD_SM4_CBC} 分支直接调 {@code sm4_cbc_encrypt_blocks}，即标准 SM4-CBC、
 * 无填充、无 HMAC；驱动内 {@code Sm4Cipher.encryptCbcNoPadding/decryptCbcNoPadding} 同语义。
 * 填充由驱动上层自己做（PKCS7），此处一律 NoPadding 原样进出。
 */
public final class SoftSdfSm4CipherSpi extends CipherSpi {

  // 分组长度兼 IV 长度
  private static final int BLOCK_BYTES = 16;

  // 加解密方向与参数
  private int opmode = -1;
  private byte[] key;
  private byte[] iv;

  // 累积输入（NoPadding 下与输出等长，故仅在 doFinal 时一次处理）
  private byte[] buf = new byte[BLOCK_BYTES];
  private int bufLen;

  @Override
  protected void engineSetMode(String mode) throws NoSuchAlgorithmException {
    if (!"CBC".equalsIgnoreCase(mode)) {
      throw new NoSuchAlgorithmException("unsupported mode: " + mode);
    }
  }

  @Override
  protected void engineSetPadding(String padding) throws NoSuchPaddingException {
    if (!"NoPadding".equalsIgnoreCase(padding)) {
      throw new NoSuchPaddingException("unsupported padding: " + padding);
    }
  }

  @Override
  protected int engineGetBlockSize() {
    return BLOCK_BYTES;
  }

  @Override
  protected int engineGetOutputSize(int inputLen) {
    // NoPadding：不增不减，恒与输入等长
    return inputLen;
  }

  @Override
  protected byte[] engineGetIV() {
    return iv == null ? null : iv.clone();
  }

  @Override
  protected AlgorithmParameters engineGetParameters() {
    if (iv == null) {
      return null;
    }
    try {
      AlgorithmParameters p = AlgorithmParameters.getInstance("IV");
      p.init(new IvParameterSpec(iv));
      return p;
    } catch (NoSuchAlgorithmException | InvalidParameterSpecException e) {
      return null;
    }
  }

  @Override
  protected void engineInit(int opmode, Key key, SecureRandom random) throws InvalidKeyException {
    // 本 provider 只用于 CBC，无 IV 无法工作
    throw new InvalidKeyException("SM4/CBC/NoPadding requires an IV");
  }

  @Override
  protected void engineInit(int opmode, Key key, AlgorithmParameterSpec params, SecureRandom random)
      throws InvalidKeyException, InvalidAlgorithmParameterException {
    if (!(params instanceof IvParameterSpec)) {
      throw new InvalidAlgorithmParameterException("SM4/CBC/NoPadding requires IvParameterSpec");
    }
    engineInit(opmode, key, ((IvParameterSpec) params).getIV(), random);
  }

  @Override
  protected void engineInit(int opmode, Key key, AlgorithmParameters params, SecureRandom random)
      throws InvalidKeyException, InvalidAlgorithmParameterException {
    IvParameterSpec spec;
    try {
      spec = params == null ? null : params.getParameterSpec(IvParameterSpec.class);
    } catch (InvalidParameterSpecException e) {
      throw new InvalidAlgorithmParameterException("SM4/CBC/NoPadding requires IvParameterSpec", e);
    }
    if (spec == null) {
      throw new InvalidAlgorithmParameterException("SM4/CBC/NoPadding requires IvParameterSpec");
    }
    engineInit(opmode, key, spec.getIV(), random);
  }

  // 参数齐备后落位；key/IV 长度必须是 16
  private void engineInit(int opmode, Key key, byte[] iv, SecureRandom random)
      throws InvalidKeyException {
    if (opmode != Cipher.ENCRYPT_MODE && opmode != Cipher.DECRYPT_MODE) {
      throw new InvalidKeyException("unsupported opmode: " + opmode);
    }
    if (key == null) {
      throw new InvalidKeyException("key is null");
    }
    byte[] rawKey = key.getEncoded();
    if (rawKey == null || rawKey.length != BLOCK_BYTES) {
      throw new InvalidKeyException("SM4 key must be " + BLOCK_BYTES + " bytes");
    }
    if (iv == null || iv.length != BLOCK_BYTES) {
      throw new InvalidKeyException("IV must be " + BLOCK_BYTES + " bytes");
    }
    this.opmode = opmode;
    this.key = rawKey;
    this.iv = iv.clone();
    this.bufLen = 0;
  }

  @Override
  protected byte[] engineUpdate(byte[] input, int inputOffset, int inputLen) {
    append(input, inputOffset, inputLen);
    return new byte[0];
  }

  @Override
  protected int engineUpdate(byte[] input, int inputOffset, int inputLen,
      byte[] output, int outputOffset) throws ShortBufferException {
    append(input, inputOffset, inputLen);
    return 0;
  }

  @Override
  protected byte[] engineDoFinal(byte[] input, int inputOffset, int inputLen)
      throws IllegalBlockSizeException, BadPaddingException {
    append(input, inputOffset, inputLen);
    byte[] data = new byte[bufLen];
    System.arraycopy(buf, 0, data, 0, bufLen);
    bufLen = 0;
    return process(data);
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

  // NoPadding 必须整块；不整块说明调用方漏了自身填充
  private byte[] process(byte[] data) throws IllegalBlockSizeException {
    if (opmode < 0) {
      throw new IllegalStateException("cipher not initialized");
    }
    if (data.length == 0 || (data.length % BLOCK_BYTES) != 0) {
      throw new IllegalBlockSizeException("SM4/CBC/NoPadding input must be block-aligned, got "
          + data.length + " bytes");
    }
    return opmode == Cipher.ENCRYPT_MODE
        ? Sm4Cipher.encryptCbcNoPadding(key, iv, data)
        : Sm4Cipher.decryptCbcNoPadding(key, iv, data);
  }
}

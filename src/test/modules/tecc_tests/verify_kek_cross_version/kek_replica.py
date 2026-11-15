#!/usr/bin/env python3
"""
KEK 文件格式独立复刻解密器（跨版本验证用）。

不依赖任何 TECC 源码/库，只用 Python 标准库（hashlib/hmac/subprocess）复刻
security.c 与 kms_adapter.c 的 PBKDF2 派生链 + SM4-CBC 解密 + HMAC 校验。

文件格式（两分支逐字节一致，来自源码 diff）：
  magic(7) "TECCKEK" | ver(1) | reserved(8) | algo_id(4) | salt(16) | iv(16)
  | SM4密文[自带16B IV] | HMAC-SHA256(32)

派生链（与 security.c 一致）：
  derived = PBKDF2-HMAC-SHA256(pw=MEK||"KEK_ENCRYPTION", salt=文件salt, iters=10000, dklen=32)
  sm4_key = derived[0:16]
  hmac_key = PBKDF2-HMAC-SHA256(pw=derived||"HMAC_KEY", salt=HMAC_KEY_DERIVATION_SALT, iters=10000, dklen=32)
  hmac = HMAC-SHA256(key=hmac_key, msg=文件[0:len-32])
  密文主体 = 文件[52:len-32]  (SM4 输出 = IV(16)||CBC密文, PKCS7)
"""
import hashlib
import hmac
import struct
import subprocess
import sys

# 与 security.c HMAC_KEY_DERIVATION_SALT 一致
HMAC_SALT = (b"TECC_HMAC_KEY_DERIVATION_SALT" + bytes([0x00, 0x46, 0x61]))
PBKDF2_ITERS = 10000
HEADER = 16          # magic7+ver1+reserved8
ALGO_ID = 4
SALT_LEN = 16
IV_FIELD_LEN = 16
HMAC_LEN = 32
SM4_KEY_LEN = 16
SM4_BLOCK = 16


def derive(mek: bytes, context: bytes, salt: bytes, dklen=32) -> bytes:
    """security_derive_key_from_kek 的复刻：password = MEK||context, PBKDF2-HMAC-SHA256."""
    return hashlib.pbkdf2_hmac("sha256", mek + context, salt, PBKDF2_ITERS, dklen)


def sm4_cbc_decrypt(cipher_body: bytes, key: bytes) -> bytes:
    """SM4-CBC 解密（密文主体 = IV(16)||CBC密文），用 openssl CLI."""
    iv = cipher_body[:SM4_BLOCK]
    ct = cipher_body[SM4_BLOCK:]
    p = subprocess.run(
        ["openssl", "enc", "-d", "-sm4-cbc", "-K", key.hex(),
         "-iv", iv.hex(), "-nopad"],
        input=ct, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError(f"openssl SM4-CBC decrypt failed: {p.stderr.decode()}")
    return p.stdout


def strip_pkcs7(pt: bytes) -> bytes:
    pad = pt[-1]
    if pad < 1 or pad > SM4_BLOCK:
        raise RuntimeError(f"bad PKCS7 padding byte 0x{pad:02x}")
    body = pt[:-pad]
    if body != pt[:-pad]:
        raise RuntimeError("PKCS7 padding mismatch")
    # 校验 padding 字节都等于 pad
    if pt[len(pt) - pad:] != bytes([pad]) * pad:
        raise RuntimeError("PKCS7 padding bytes inconsistent")
    return body


def decrypt_kek_file(path: str, mek: bytes) -> bytes:
    """完整复刻 decrypt_kek_from_storage."""
    data = open(path, "rb").read()
    if len(data) < HEADER + ALGO_ID + SALT_LEN + IV_FIELD_LEN + SM4_BLOCK + HMAC_LEN:
        raise RuntimeError(f"file too small: {len(data)}")
    if data[:7] != b"TECCKEK":
        raise RuntimeError(f"bad magic: {data[:7]!r}")
    if data[7] != 1:
        raise RuntimeError(f"bad version: {data[7]}")
    algo_id = struct.unpack(">I", data[HEADER:HEADER + ALGO_ID])[0]
    # algo_id: 高16位 encrypt_algo(SM4-CBC=1) | 低16位 kdf_algo(PBKDF2-SHA256=1)
    encrypt_algo = algo_id >> 16
    kdf_algo = algo_id & 0xFFFF
    if encrypt_algo != 1 or kdf_algo != 1:
        raise RuntimeError(f"unexpected algo_id: encrypt={encrypt_algo} kdf={kdf_algo}")

    salt = data[HEADER + ALGO_ID: HEADER + ALGO_ID + SALT_LEN]
    iv_field = data[HEADER + ALGO_ID + SALT_LEN: HEADER + ALGO_ID + SALT_LEN + IV_FIELD_LEN]
    cipher_body = data[HEADER + ALGO_ID + SALT_LEN + IV_FIELD_LEN: -HMAC_LEN]
    stored_hmac = data[-HMAC_LEN:]

    # 1. 派生 SM4 密钥
    derived = derive(mek, b"KEK_ENCRYPTION", salt)
    sm4_key = derived[:SM4_KEY_LEN]

    # 2. 派生 HMAC 密钥并校验
    hmac_key = derive(derived, b"HMAC_KEY", HMAC_SALT)
    calc_hmac = hmac.new(hmac_key, data[:-HMAC_LEN], hashlib.sha256).digest()
    if calc_hmac != stored_hmac:
        raise RuntimeError("HMAC MISMATCH: file tampered or wrong MEK")

    # 3. SM4-CBC 解密 + 去 PKCS7
    pt = sm4_cbc_decrypt(cipher_body, sm4_key)
    return strip_pkcs7(pt)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit("usage: kek_replica.py <file.key> <mek_hex>")
    path, mek_hex = sys.argv[1], sys.argv[2]
    mek = bytes.fromhex(mek_hex)
    plain = decrypt_kek_file(path, mek)
    print(f"OK   {path}: len={len(plain)}B")
    print(f"      plaintext = {plain.hex()}")

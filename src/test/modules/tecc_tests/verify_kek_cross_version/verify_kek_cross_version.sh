#!/bin/bash
# ============================================================================
# verify_kek_cross_version.sh — 验证 4.0.0 生成的 KEK 文件在 4.0.1 可用
#
# 原理（无需切换分支，全程从 git 对象读取）：
#   KEK 文件兼容性取决于三件事，本脚本逐项验证：
#   [A] 两分支的 MEK 一致（FIXED_MASTER_KEY == get_mek）
#   [B] 两分支的 KEK 文件加解密实现逐字节一致
#       （encrypt/decrypt_kek_from_storage + security 派生链 + HMAC）
#   [C] 用独立算法复刻（不依赖任何 TECC 库）解密一个真实 KEK 文件，
#       证明"格式正确 + 算法理解正确"，从而 4.0.0 产出的文件必能被
#       4.0.1 的同一套解密逻辑解开。
#
# 依赖：git / python3 / openssl(≥3.0, 支持 sm4-cbc)
# ============================================================================
set -u

B40="release-tecc4.0.0"
B41="tecc4-feature-pg14"
REPO="$(cd "$(dirname "$0")" && cd ../../../../../ && pwd)"   # verify_kek_cross_version → 仓库根
TMP="$(mktemp -d)"
PASS=0; FAIL=0

ok()   { echo "  [PASS] $1"; PASS=$((PASS+1)); }
bad()  { echo "  [FAIL] $1"; FAIL=$((FAIL+1)); }
assert_eq() { # $1=label $2=left $3=right
  if [ "$2" = "$3" ]; then ok "$1"; else bad "$1"; echo "         left:  $2"; echo "         right: $3"; fi
}

echo "=== 验证环境 ==="
python3 --version >/dev/null 2>&1  || { echo "需要 python3"; exit 1; }
openssl version >/dev/null 2>&1   || { echo "需要 openssl"; exit 1; }
echo "  仓库: $REPO"
echo "  对比: $B40  vs  $B41"
echo

# ---------- [A] MEK 一致性 ----------
echo "=== [A] MEK 一致性 ==="
mek_400=$(git -C "$REPO" show "$B40:src/common/tecc_key/kms_adapter.c" \
    | sed -n '/FIXED_MASTER_KEY\[/,/};/p' | grep -oE '0x[0-9a-f]{2}' | sed 's/0x//' | tr -d '\n')
mek_401=$(git -C "$REPO" show "$B41:src/common/tecc_key/kms_adapter.c" \
    | sed -n '/FIXED_MASTER_KEY\[/,/};/p' | grep -oE '0x[0-9a-f]{2}' | sed 's/0x//' | tr -d '\n')
mek_ktool=$(grep -A8 'static const unsigned char fixed_mek' "$REPO/src/bin/tecc_ktool/tecc_ktool.c" \
    | grep -oE '0x[0-9a-f]{2}' | sed 's/0x//' | tr -d '\n')

assert_eq "4.0.0 FIXED_MASTER_KEY == 4.0.1 FIXED_MASTER_KEY" "$mek_400" "$mek_401"
assert_eq "4.0.1 FIXED_MASTER_KEY == tecc_ktool get_mek"    "$mek_401" "$mek_ktool"
MEK_HEX="$mek_401"
echo "  MEK = $MEK_HEX"
echo

# ---------- [B] 加解密实现一致性 ----------
echo "=== [B] 两分支加解密实现逐字节一致 ==="

# B1: encrypt/decrypt_kek_from_storage 函数体
extract_fns() { # $1=branch
  git -C "$REPO" show "$1:src/common/tecc_key/kms_adapter.c" \
    | sed -n '/^encrypt_kek_for_storage/,/^}/p;/^decrypt_kek_from_storage/,/^}/p'
}
extract_fns "$B40" > "$TMP/kms_40.txt"
extract_fns "$B41" > "$TMP/kms_41.txt"
if diff -q "$TMP/kms_40.txt" "$TMP/kms_41.txt" >/dev/null; then
  ok "encrypt/decrypt_kek_from_storage 函数体一致（$(wc -l < "$TMP/kms_40.txt") 行）"
else
  bad "encrypt/decrypt_kek_from_storage 函数体有差异"
  diff "$TMP/kms_40.txt" "$TMP/kms_41.txt" | head -20
fi

# B2: 关键常量
for pat in 'KEK_FILE_HEADER_SIZE 16' 'KEK_FILE_ALGO_ID_SIZE 4' \
           'KEK_FILE_SALT_SIZE 16' 'KEK_FILE_IV_SIZE 16' 'KEK_FILE_HMAC_SIZE 32' \
           'PBKDF2_ITERATIONS 10000'; do
  c40=$(git -C "$REPO" show "$B40:src/common/tecc_key/kms_adapter.c" | grep -oE "$pat" | head -1)
  c41=$(git -C "$REPO" show "$B41:src/common/tecc_key/kms_adapter.c" | grep -oE "$pat" | head -1)
  assert_eq "kms_adapter 常量: $pat" "$c40" "$c41"
done

# B3: security.c 派生链与 HMAC 盐
s40=$(git -C "$REPO" show "$B40:src/common/tecc_key/security.c")
s41=$(git -C "$REPO" show "$B41:src/common/tecc_key/security.c")
assert_eq "PBKDF2_ITERATIONS(security)" \
  "$(echo "$s40" | grep -oE 'PBKDF2_ITERATIONS 10000' | head -1)" \
  "$(echo "$s41" | grep -oE 'PBKDF2_ITERATIONS 10000' | head -1)"
h40=$(echo "$s40" | sed -n '/HMAC_KEY_DERIVATION_SALT\[/,/};/p' | grep -oE "'[A-Z_]+'|0x[0-9a-f]{2}" | tr -d "'\n" | tr -d ' ')
h41=$(echo "$s41" | sed -n '/HMAC_KEY_DERIVATION_SALT\[/,/};/p' | grep -oE "'[A-Z_]+'|0x[0-9a-f]{2}" | tr -d "'\n" | tr -d ' ')
assert_eq "HMAC_KEY_DERIVATION_SALT" "$h40" "$h41"

# B4: security_generate_kek_file_hmac 函数体
g40=$(echo "$s40" | sed -n '/^security_generate_kek_file_hmac/,/^}/p')
g41=$(echo "$s41" | sed -n '/^security_generate_kek_file_hmac/,/^}/p')
if [ "$g40" = "$g41" ]; then
  ok "security_generate_kek_file_hmac 函数体一致"
else
  bad "security_generate_kek_file_hmac 有差异"
fi
echo

# ---------- [C] 独立算法复刻：解密真实 KEK 文件 ----------
echo "=== [C] 独立复刻解密（tecc_ktool 生成的文件）==="
KTOOL="$REPO/src/bin/tecc_ktool/tecc_ktool"
KKEY="$TMP/verify.key"
if [ ! -x "$KTOOL" ]; then
  echo "  [SKIP] tecc_ktool 未编译，跳过生成步骤（改用已有文件）"
  KKEY=/tmp/kek-xverify/xv_sm4.key
else
  "$KTOOL" generate --key-name verify --key-algo sm4 --key-path "$TMP" \
      >/dev/null 2>&1
fi
if [ -f "$KKEY" ]; then
  # 先用 tecc_ktool validate 验 HMAC（走源码路径）
  if "$KTOOL" validate --key-path "$KKEY" >/dev/null 2>&1; then
    ok "tecc_ktool validate（源码 HMAC 校验）通过"
  else
    bad "tecc_ktool validate 失败"
  fi
  # 独立复刻解密（kek_replica.py 与本脚本同目录）
  REPLICA="$(cd "$(dirname "$0")" && pwd)/kek_replica.py"
  out=$(python3 "$REPLICA" "$KKEY" "$MEK_HEX" 2>&1)
  if [ $? -eq 0 ]; then
    plen=$(echo "$out" | grep -oE 'len=[0-9]+' | head -1)
    if [ "$plen" = "len=32" ]; then
      ok "独立复刻解密成功：32B KEK 明文，HMAC 校验通过"
      echo "      $out"
    else
      bad "解密结果长度非 32B：$out"
    fi
  else
    bad "独立复刻解密失败：$out"
  fi
else
  bad "没有可用的 KEK 文件：$KKEY"
fi
echo

# ---------- 汇总 ----------
echo "=============================="
echo "结果: PASS=$PASS  FAIL=$FAIL"
[ "$FAIL" -eq 0 ] && echo "结论: 4.0.0 生成的 KEK 文件在 4.0.1 上可用 ✓" \
                 || echo "结论: 存在不一致，请检查上表"
rm -rf "$TMP"
exit $FAIL

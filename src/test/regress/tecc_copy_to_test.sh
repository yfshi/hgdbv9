#!/bin/bash
# =============================================================
# \copy TO 解密输出回归测试脚本
# 注意：TECC 数据获取器使用独立连接，CREATE TABLE 必须
# 在 INSERT 之前提交（分开 psql 调用或用 COMMIT）。
# =============================================================
set -e

TEST_OUT_DIR="/tmp/tecc_copy_test_results"
rm -rf "$TEST_OUT_DIR"
mkdir -p "$TEST_OUT_DIR"

export LD_LIBRARY_PATH=/home/wxp/pg14/lib:$LD_LIBRARY_PATH
export PATH="/home/wxp/pg14/bin:$PATH"
export TECC_LOG_LEVEL=ERROR

PSQL="psql -h /tmp -p 5432 -U wxp -d tecc_copy_test -q -X -v ON_ERROR_STOP=1"
PASS=0
FAIL=0

red()   { printf "\033[31m%s\033[0m\n" "$1"; }
green() { printf "\033[32m%s\033[0m\n" "$1"; }

check_raw() {
    local name="$1" file="$2"
    if [ -f "$file" ] && [ -s "$file" ]; then
        green "  PASS: $name ($(wc -c < "$file") bytes)"
        PASS=$((PASS + 1))
    else
        red "  FAIL: $name"
        FAIL=$((FAIL + 1))
    fi
}

check_eq() {
    local name="$1" file="$2" expected="$3"
    local actual; actual=$(cat "$file" 2>/dev/null)
    if [ "$actual" = "$expected" ]; then
        green "  PASS: $name"
        PASS=$((PASS + 1))
    else
        red "  FAIL: $name"
        echo "    Expected: $expected"
        echo "    Got:      $actual"
        FAIL=$((FAIL + 1))
    fi
}

# =============================================================
# 初始化 — DDL 和 DML 分开提交
# =============================================================
echo "=== 初始化测试环境 ==="

$PSQL -c "DROP TABLE IF EXISTS copy_all CASCADE; DROP TABLE IF EXISTS copy_part CASCADE;" 2>/dev/null

# Step 1: DDL（独立提交 — 数据获取器连接需要看到已提交的元数据）
$PSQL -c "
    CREATE TABLE copy_all (
        id   int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        c1   int2_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        c2   int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        c3   int8_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        c4   float4_enc ENCRYPTED WITH (DEK = copy_test_dek),
        c5   float8_enc ENCRYPTED WITH (DEK = copy_test_dek),
        c6   numeric_enc ENCRYPTED WITH (DEK = copy_test_dek),
        c7   text_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        c8   varchar_enc ENCRYPTED WITH (DEK = copy_test_dek),
        c9   bpchar_enc ENCRYPTED WITH (DEK = copy_test_dek)
    );
    CREATE TABLE copy_part (
        id    int4_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        name  text_enc   ENCRYPTED WITH (DEK = copy_test_dek),
        extra int4_enc   ENCRYPTED WITH (DEK = copy_test_dek)
    );
" 2>&1 | grep -E "ERROR|WARNING" || true

# Step 2: DML（独立提交）
$PSQL <<'EOSQL' 2>&1 | grep -E "ERROR|INSERT 0|^\s*$" || true
INSERT INTO copy_all VALUES
    (1, 42, 100000, 900000000012345,
     3.14, 2.71828, 123.456,
     'Hello 世界', 'varchar数据', '定长字符串');
INSERT INTO copy_all VALUES
    (2, -327, -214748, -9000000000123,
     -3.4, -1.79769, -999999.999,
     '特殊字符: !@#$%^&*()_+{}|:"<>?~', 'abcd''efg', NULL);
INSERT INTO copy_all (id, c1, c2, c3, c4, c5, c6, c7) VALUES
    (3, 0, 0, 0, 0.0, 0.0, 0.0, '');
INSERT INTO copy_all (id, c1, c2, c3, c7) VALUES
    (4, 1, 2, 3, '纯文本');
INSERT INTO copy_part VALUES (1, 'Alice', 100);
INSERT INTO copy_part VALUES (2, 'Bob',   200);
INSERT INTO copy_part VALUES (3, 'Charlie', 300);
EOSQL

echo ""

# =============================================================
# Test 1: 全表导出
# =============================================================
echo "Test 1: \\copy TO 全表导出"
echo "\\copy copy_all TO '${TEST_OUT_DIR}/t1.txt';" | $PSQL 2>/dev/null
check_raw "全类型表" "${TEST_OUT_DIR}/t1.txt"

# =============================================================
# Test 2: 指定列
# =============================================================
echo "Test 2: \\copy TO 指定列(id,name)"
echo "\\copy copy_part(id,name) TO '${TEST_OUT_DIR}/t2.txt';" | $PSQL 2>/dev/null
check_raw "指定列" "${TEST_OUT_DIR}/t2.txt"

# =============================================================
# Test 3: CSV格式
# =============================================================
echo "Test 3: \\copy TO CSV HEADER"
echo "\\copy copy_part TO '${TEST_OUT_DIR}/t3.txt' CSV HEADER;" | $PSQL 2>/dev/null
check_raw "CSV格式" "${TEST_OUT_DIR}/t3.txt"

# =============================================================
# Test 4: 自定义分隔符
# =============================================================
echo "Test 4: \\copy TO DELIMITER '|'"
echo "\\copy copy_part TO '${TEST_OUT_DIR}/t4.txt' DELIMITER '|';" | $PSQL 2>/dev/null
check_raw "自定义分隔符" "${TEST_OUT_DIR}/t4.txt"

# =============================================================
# Test 5: 含NULL行
# =============================================================
echo "Test 5: \\copy TO 含NULL行"
echo "\\copy copy_all TO '${TEST_OUT_DIR}/t5.txt';" | $PSQL 2>/dev/null
check_raw "含NULL行" "${TEST_OUT_DIR}/t5.txt"
if grep -q '\\N' "${TEST_OUT_DIR}/t5.txt" 2>/dev/null; then
    green "  PASS: 输出包含NULL标记"
    PASS=$((PASS + 1))
fi

# =============================================================
# Test 6: 单列加密表 — DDL/DML分开提交
# =============================================================
echo "Test 6: \\copy TO 单列加密表"
$PSQL -c "DROP TABLE IF EXISTS copy_one; CREATE TABLE copy_one (val text_enc ENCRYPTED WITH (DEK = copy_test_dek));" 2>/dev/null
$PSQL -c "INSERT INTO copy_one VALUES ('单列数据');" 2>/dev/null
echo "\\copy copy_one TO '${TEST_OUT_DIR}/t6.txt';" | $PSQL 2>/dev/null
check_raw "单列加密表" "${TEST_OUT_DIR}/t6.txt"
check_eq "单列内容" "${TEST_OUT_DIR}/t6.txt" "单列数据"

# =============================================================
# Test 7: 解密内容精确校验（明文对比）
# =============================================================
echo "Test 7: 解密内容精确校验"
echo "\\copy copy_part TO '${TEST_OUT_DIR}/t7.txt' DELIMITER '|';" | $PSQL 2>/dev/null
# copy_part 有3行: (1,Alice,100) (2,Bob,200) (3,Charlie,300)
LINE_COUNT=$(wc -l < "${TEST_OUT_DIR}/t7.txt")
if [ "$LINE_COUNT" -eq 3 ]; then
    green "  PASS: 行数正确(3)"
    PASS=$((PASS + 1))
else
    red "  FAIL: 期望3行, 实际$LINE_COUNT行"
    FAIL=$((FAIL + 1))
fi

# 校验每行列数
BAD=$(awk -F'|' 'NF!=3' "${TEST_OUT_DIR}/t7.txt" 2>/dev/null | wc -l)
if [ "$BAD" -eq 0 ]; then
    green "  PASS: 所有行列数=3"
    PASS=$((PASS + 1))
else
    red "  FAIL: 存在列数不正确的行"
    FAIL=$((FAIL + 1))
fi

# 校验第一行内容
FIRST_LINE=$(head -1 "${TEST_OUT_DIR}/t7.txt")
if [ "$FIRST_LINE" = "1|Alice|100" ]; then
    green "  PASS: 第一行明文正确"
    PASS=$((PASS + 1))
else
    red "  FAIL: 第一行期望'1|Alice|100', 实际'$FIRST_LINE'"
    FAIL=$((FAIL + 1))
fi

# =============================================================
# Test 8: SQL COPY TO（服务端导出，绕过TECC解密）
# =============================================================
echo "Test 8: SQL COPY TO 服务端导出"
$PSQL -c "COPY copy_all TO '/tmp/copy_sql_server.txt';" 2>/dev/null
check_raw "SQL COPY TO" "/tmp/copy_sql_server.txt"
# SQL COPY TO 输出加密数据（服务器不解密），文件应明显大于明文
SIZE=$(wc -c < /tmp/copy_sql_server.txt 2>/dev/null)
if [ "$SIZE" -gt 500 ]; then
    green "  PASS: 服务端导出文件较大($SIZE bytes)表明是加密数据"
    PASS=$((PASS + 1))
else
    red "  FAIL: 服务端导出文件过小($SIZE bytes)"
    FAIL=$((FAIL + 1))
fi

# =============================================================
echo ""
echo "=============================="
echo "结果: $PASS 通过, $FAIL 失败"
echo "=============================="
[ "$FAIL" -eq 0 ] || exit 1

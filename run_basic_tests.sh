#!/bin/bash
#
# TECC基础用例自动测试脚本
#

# ---- 环境配置（按需修改） ----
PREFIX="/home/highgo/test/tecc4"
export LD_PRELOAD="${PREFIX}/lib/libpq.so"
export PATH="${PREFIX}/bin:$PATH"
export TECC_LOG_LEVEL=${TECC_LOG_LEVEL:-WARNING}

PG_HOST="localhost"
PG_PORT="5866"
PG_USER="highgo"
PG_DB="postgres"
OUT_DIR="/tmp/tecc_test_results"

PASS=0
FAIL=0
TOTAL=0

# 测试集定义: "组名:文件名"
TESTS=(
  "冒烟:test_insert_simple_no_comments.sql"
  "冒烟:test_insert_final.sql"
  "CRUD:test_insert_e2e.sql"
  "CRUD:test_select_e2e.sql"
  "CRUD:test_tecc_simple.sql"
  "全类型:test_all_types.sql"
  "全类型:test_all_types_where.sql"
)

mkdir -p "$OUT_DIR"

echo "=============================================="
echo " TECC 基础用例自动测试"
echo " 开始时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo " 目标数据库: ${PG_HOST}:${PG_PORT}/${PG_DB}"
echo "=============================================="
echo ""

# 检查连接
if ! psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -c "SELECT 1" > /dev/null 2>&1; then
    echo "[FAIL] 数据库连接失败，请检查服务状态"
    exit 1
fi
echo "[OK] 数据库连接正常"
echo ""

# 逐文件执行
for entry in "${TESTS[@]}"; do
    GROUP="${entry%%:*}"
    FILE="${entry##*:}"
    TOTAL=$((TOTAL + 1))
    LOG="${OUT_DIR}/${FILE}.log"

    if [ ! -f "$FILE" ]; then
        echo "[SKIP] ${GROUP} | ${FILE} — 文件不存在"
        continue
    fi

    printf "[%02d/%02d] %-6s | %-40s ... " "$TOTAL" "${#TESTS[@]}" "$GROUP" "$FILE"

    psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" \
         -v ON_ERROR_STOP=1 \
         -f "$FILE" > "$LOG" 2>&1

    if [ $? -eq 0 ]; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        FAIL=$((FAIL + 1))
        # 失败后停止后续测试
        echo ""
        echo "=============================================="
        echo " 失败日志: ${LOG}"
        echo "=============================================="
        tail -30 "$LOG"
        echo ""
        break
    fi
done

echo ""
echo "=============================================="
echo " 测试结果: 通过 ${PASS} / 失败 ${FAIL} / 总计 ${TOTAL}"
echo " 日志目录: ${OUT_DIR}"
echo " 结束时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

if [ $FAIL -gt 0 ]; then
    exit 1
else
    exit 0
fi

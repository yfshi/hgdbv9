#!/bin/bash
# TECC Partition Table 综合测试
# 测试：PARTITION OF, ATTACH/DETACH PARTITION, MAXVALUE/MINVALUE, 数据路由
# 不使用 set -e，手动处理错误

export LD_LIBRARY_PATH=/home/why/pg14/lib:$LD_LIBRARY_PATH
PSQL_ON="/home/why/pg14/bin/psql -h localhost -p 5432 -U why -d postgres -X -k on -q -v ON_ERROR_STOP=1"
PSQL_OFF="/home/why/pg14/bin/psql -h localhost -p 5432 -U why -d postgres -X -k off -q -v ON_ERROR_STOP=1"
PASS=0
FAIL=0

ok()   { echo "  PASS"; ((PASS++)); }
nook() { echo "  FAIL: $1"; ((FAIL++)); }

sql_on() {
    local desc="$1" sql="$2"
    printf "%-65s" "$desc"
    if echo "$sql" | $PSQL_ON 2>/dev/null > /tmp/tecc_test_out.txt; then
        ok
    else
        nook "$(cat /tmp/tecc_test_out.txt | head -3 | tr '\n' ' ')"
    fi
}

sql_off() {
    local desc="$1" sql="$2"
    printf "%-65s" "$desc"
    if echo "$sql" | $PSQL_OFF 2>/dev/null > /tmp/tecc_test_out.txt; then
        ok
    else
        nook "$(cat /tmp/tecc_test_out.txt | head -3 | tr '\n' ' ')"
    fi
}

sql_on_fail() {
    local desc="$1" sql="$2"
    printf "%-65s" "$desc"
    if echo "$sql" | $PSQL_ON 2>/dev/null > /tmp/tecc_test_out.txt; then
        nook "expected failure but succeeded"
    else
        ok
    fi
}

sql_on_contains() {
    local desc="$1" sql="$2" expected="$3"
    printf "%-65s" "$desc"
    local out
    out=$(echo "$sql" | $PSQL_ON 2>/dev/null)
    if echo "$out" | grep -q "$expected"; then
        ok
    else
        nook "expected output containing '$expected', got: $(echo "$out" | head -3 | tr '\n' ' ')"
    fi
}

echo "============================================================"
echo " TECC Partition Table 综合测试"
echo "============================================================"
echo ""

# ======================================
echo "=== 环境清理 ==="
# 删除所有可能的遗留表
$PSQL_OFF -c "DROP TABLE IF EXISTS t_pt_list_parent, t_pt_range_parent, t_pt_min_parent, t_pt_attach_parent, t_pt_int_parent, t_pt_plain_parent, t_pt_comp_parent CASCADE;" > /dev/null 2>&1 || true
$PSQL_OFF -c "DROP TABLE IF EXISTS t_pt_attach_child CASCADE;" > /dev/null 2>&1 || true
# 清理 KEK (CASCADE 会自动删除依赖的 DEK)
sql_off "Drop KEK test_kek CASCADE" "DROP KEK IF EXISTS test_kek CASCADE;"
rm -f /tmp/test_kek.key

echo ""
echo "=== 初始化 KEK/DEK ==="
sql_on "CREATE KEK"     "CREATE KEK test_kek WITH (KEY_STORE = 'localkms', KEY_PATH = '/tmp/test_kek.key');"
sql_on "CREATE DEK det" "CREATE DEK det_dek WITH VALUES(KEK = 'test_kek');"
sql_on "CREATE DEK ope" "CREATE DEK ope_dek WITH VALUES(KEK = 'test_kek');"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 1: LIST 分区 + DET"
echo "========================================================================"
PREFIX="t_pt_list"

sql_on "1a 创建 LIST 父表 (DET)" \
    "CREATE TABLE ${PREFIX}_parent (status text_enc ENCRYPTED WITH (dek = 'det_dek', support_algo = 'det')) PARTITION BY LIST (status);"

sql_on "1b PARTITION OF IN ('active','pending')" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES IN ('active', 'pending');"

sql_on "1c PARTITION OF IN ('inactive','archived')" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES IN ('inactive', 'archived');"

sql_on "1d PARTITION OF DEFAULT" \
    "CREATE TABLE ${PREFIX}_pdefault PARTITION OF ${PREFIX}_parent DEFAULT;"

sql_on "1e INSERT active"  "INSERT INTO ${PREFIX}_parent VALUES ('active');"
sql_on "1f INSERT pending" "INSERT INTO ${PREFIX}_parent VALUES ('pending');"
sql_on "1g INSERT inactive" "INSERT INTO ${PREFIX}_parent VALUES ('inactive');"
sql_on "1h INSERT unknown (→default)" "INSERT INTO ${PREFIX}_parent VALUES ('unknown');"

sql_on_contains "1i SELECT all (2+1+1=4 rows)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "4"

sql_on_contains "1j SELECT active only" \
    "SELECT status FROM ${PREFIX}_parent WHERE status = 'active';" "active"

sql_on "1k DELETE inactive" "DELETE FROM ${PREFIX}_parent WHERE status = 'inactive';"
sql_on_contains "1l SELECT after DELETE (3 rows)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "3"

sql_on "1m DROP TABLES" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 2: RANGE 分区 + OPE (含 MAXVALUE)"
echo "========================================================================"
PREFIX="t_pt_range"

sql_on "2a 创建 RANGE 父表 (OPE)" \
    "CREATE TABLE ${PREFIX}_parent (id INT, name text_enc ENCRYPTED WITH (dek = 'ope_dek', support_algo = 'ope')) PARTITION BY RANGE (name);"

sql_on "2b PARTITION OF FROM ('aaa') TO ('mmm')" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('aaa') TO ('mmm');"

sql_on "2c PARTITION OF FROM ('mmm') TO ('zzz')" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('mmm') TO ('zzz');"

sql_on "2d PARTITION OF FROM ('zzz') TO (MAXVALUE)" \
    "CREATE TABLE ${PREFIX}_p3 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('zzz') TO (MAXVALUE);"

sql_on "2e INSERT bbb (→p1)"  "INSERT INTO ${PREFIX}_parent VALUES (1, 'bbb');"
sql_on "2f INSERT nnn (→p2)"  "INSERT INTO ${PREFIX}_parent VALUES (2, 'nnn');"
sql_on "2g INSERT zzzz (→p3)" "INSERT INTO ${PREFIX}_parent VALUES (3, 'zzzz');"
sql_on "2h INSERT aaa (→p1)"  "INSERT INTO ${PREFIX}_parent VALUES (4, 'aaa');"

sql_on_contains "2i SELECT count(*)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "4"

sql_on_contains "2j ORDER BY name (OPE preserves order)" \
    "SELECT id FROM ${PREFIX}_parent ORDER BY name;" "1
        4
        2
        3"

sql_on "2k UPDATE" "UPDATE ${PREFIX}_parent SET name = 'ccc' WHERE id = 4;"
sql_on_contains "2l SELECT after UPDATE" \
    "SELECT name FROM ${PREFIX}_parent WHERE id = 4;" "ccc"

sql_on "2m DROP TABLES" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 3: RANGE 分区 + OPE + MINVALUE"
echo "========================================================================"
PREFIX="t_pt_min"

sql_on "3a 创建 RANGE 父表 (OPE)" \
    "CREATE TABLE ${PREFIX}_parent (id INT, v text_enc ENCRYPTED WITH (dek = 'ope_dek', support_algo = 'ope')) PARTITION BY RANGE (v);"

sql_on "3b PARTITION OF FROM (MINVALUE) TO ('mmm')" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES FROM (MINVALUE) TO ('mmm');"

sql_on "3c PARTITION OF FROM ('mmm') TO (MAXVALUE)" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('mmm') TO (MAXVALUE);"

sql_on "3d INSERT" "INSERT INTO ${PREFIX}_parent VALUES (1, 'aaa'); INSERT INTO ${PREFIX}_parent VALUES (2, 'zzz');"
sql_on_contains "3e SELECT (2 rows)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "2"

sql_on "3f DROP" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 4: ATTACH PARTITION + OPE"
echo "========================================================================"
PREFIX="t_pt_attach"

sql_on "4a 创建 RANGE 父表 (OPE)" \
    "CREATE TABLE ${PREFIX}_parent (id INT, name text_enc ENCRYPTED WITH (dek = 'ope_dek', support_algo = 'ope')) PARTITION BY RANGE (name);"

# 子表需要 -k off，因为 text_enc 列必须带 ENCRYPTED WITH 子句
sql_off "4b 创建子表 (ATTACH 用)" \
    "CREATE TABLE ${PREFIX}_child (id INT, name text_enc ENCRYPTED WITH (dek = 'ope_dek', support_algo = 'ope'));"

sql_on "4c ATTACH PARTITION FOR VALUES FROM ('aaa') TO ('mmm')" \
    "ALTER TABLE ${PREFIX}_parent ATTACH PARTITION ${PREFIX}_child FOR VALUES FROM ('aaa') TO ('mmm');"

sql_on "4d INSERT INTO attached partition" \
    "INSERT INTO ${PREFIX}_parent VALUES (1, 'bbb');"

sql_on_contains "4e SELECT" \
    "SELECT id, name FROM ${PREFIX}_parent ORDER BY id;" "bbb"

sql_on "4f DETACH PARTITION" \
    "ALTER TABLE ${PREFIX}_parent DETACH PARTITION ${PREFIX}_child;"

sql_off "4g DROP detached child" "DROP TABLE ${PREFIX}_child;"
sql_on "4h DROP parent" "DROP TABLE ${PREFIX}_parent;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 5: 整数分区键 + DET (LIST)"
echo "========================================================================"
PREFIX="t_pt_int"

sql_on "5a 创建 INT LIST 父表" \
    "CREATE TABLE ${PREFIX}_parent (id int4_enc ENCRYPTED WITH (dek = 'det_dek', support_algo = 'det'), val TEXT) PARTITION BY LIST (id);"

sql_on "5b PARTITION OF IN (10, 20, 30)" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES IN (10, 20, 30);"

sql_on "5c PARTITION OF IN (40, 50, 60)" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES IN (40, 50, 60);"

sql_on "5d INSERT 20"   "INSERT INTO ${PREFIX}_parent VALUES (20, 'twenty');"
sql_on "5e INSERT 50"  "INSERT INTO ${PREFIX}_parent VALUES (50, 'fifty');"
sql_on_contains "5f SELECT (2 rows)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "2"

sql_on_contains "5g SELECT val=50" \
    "SELECT val FROM ${PREFIX}_parent WHERE id = 50;" "fifty"

sql_on "5h DROP" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 6: 非加密列分区键 (plain text)"
echo "========================================================================"
PREFIX="t_pt_plain"

sql_on "6a 创建 plain RANGE 父表" \
    "CREATE TABLE ${PREFIX}_parent (id INT, name TEXT) PARTITION BY RANGE (id);"

sql_on "6b PARTITION OF FROM (1) TO (100)" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES FROM (1) TO (100);"

sql_on "6c PARTITION OF FROM (100) TO (MAXVALUE)" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES FROM (100) TO (MAXVALUE);"

sql_on "6d INSERT" "INSERT INTO ${PREFIX}_parent VALUES (50, 'plain');"
sql_on_contains "6e SELECT" \
    "SELECT name FROM ${PREFIX}_parent WHERE id = 50;" "plain"

sql_on "6f DROP" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 7: 非法加密类型拒绝 (rand → partition key 应报错)"
echo "========================================================================"

sql_on_fail "7a RAND 列 + PARTITION OF (应报错)" \
    "CREATE TABLE t_pt_rand_parent (name text_enc ENCRYPTED WITH (dek = 'det_dek', support_algo = 'rand')) PARTITION BY RANGE (name);"

# ======================================
echo ""
echo "========================================================================"
echo " 测试组 8: 多分区键列 (复合分区)"
echo "========================================================================"
PREFIX="t_pt_comp"

sql_on "8a 创建复合分区父表" \
    "CREATE TABLE ${PREFIX}_parent (a INT, b text_enc ENCRYPTED WITH (dek = 'ope_dek', support_algo = 'ope'), c text_enc ENCRYPTED WITH (dek = 'det_dek', support_algo = 'det')) PARTITION BY RANGE (b);"

sql_on "8b PARTITION OF FROM ('a') TO ('m')" \
    "CREATE TABLE ${PREFIX}_p1 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('a') TO ('m');"

sql_on "8c PARTITION OF FROM ('m') TO (MAXVALUE)" \
    "CREATE TABLE ${PREFIX}_p2 PARTITION OF ${PREFIX}_parent FOR VALUES FROM ('m') TO (MAXVALUE);"

sql_on "8d INSERT" "INSERT INTO ${PREFIX}_parent VALUES (1, 'bbb', 'secret1');"
sql_on "8e INSERT" "INSERT INTO ${PREFIX}_parent VALUES (2, 'xxx', 'secret2');"
sql_on_contains "8f SELECT (2 rows)" \
    "SELECT count(*) FROM ${PREFIX}_parent;" "2"

sql_on "8g DROP" "DROP TABLE ${PREFIX}_parent CASCADE;"

# ======================================
echo ""
echo "========================================================================"
echo " 测试统计"
echo "========================================================================"
echo " 通过: $PASS"
echo " 失败: $FAIL"
echo " 总计: $((PASS + FAIL))"
echo "========================================================================"

echo ""
echo "=== 最终清理 ==="
$PSQL_OFF -c "DROP TABLE IF EXISTS t_pt_list_parent, t_pt_range_parent, t_pt_min_parent, t_pt_attach_parent, t_pt_int_parent, t_pt_plain_parent, t_pt_comp_parent CASCADE;" > /dev/null 2>&1 || true
$PSQL_OFF -c "DROP TABLE IF EXISTS t_pt_attach_child CASCADE;" > /dev/null 2>&1 || true
sql_off "Drop KEK test_kek CASCADE" "DROP KEK IF EXISTS test_kek CASCADE;"

exit $([ $FAIL -eq 0 ])

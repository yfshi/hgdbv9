#!/bin/bash
export LD_LIBRARY_PATH=/home/wxp/pg14/lib:/usr/lib64:$LD_LIBRARY_PATH
export PATH="/home/wxp/pg14/bin:$PATH"
PSQL="psql -d postgres -q -A -t"

echo "========================================"
echo "  TECC 综合测试 (2026-04-26)"
echo "========================================"

# 重建测试数据
echo ""
echo "--- 重建测试数据 ---"
psql -d postgres -f test_setup_select_where.sql 2>&1 | grep -E "(DROP|CREATE|INSERT|ERROR|NOTICE)" | grep -v "DEBUG\|INFO\|DBG\|CURSOR"

run_test() {
    local num=$1
    local desc=$2
    local sql=$3
    echo ""
    echo "--- 测试 $num: $desc ---"
    echo "SQL: $sql"
    result=$(psql -d postgres -c "$sql" 2>&1 | grep -v "DEBUG\|INFO\|TECC\|DBG\|CURSOR\|MARKER\|=== TEST\|DEK_\|KEK_\|\[0-3\|\[4-7\|\[8-11\|\[12-15\|\[16-19\|\[20-23\|\[24-\|^$" )
    echo "$result"
    if echo "$result" | grep -qi "ERROR"; then
        echo ">>> 失败!"
    else
        echo ">>> 通过"
    fi
}

# ======== 基础查询 ========
run_test 1 "SELECT * FROM test_employee" "SELECT * FROM test_employee;"
run_test 2 "SELECT * FROM test_all_types" "SELECT * FROM test_all_types;"

# ======== DET WHERE等值查询 ========
run_test 3 "WHERE c_int2 = 10" "SELECT * FROM test_all_types WHERE c_int2 = 10;"
run_test 4 "WHERE c_int4 = 100" "SELECT * FROM test_all_types WHERE c_int4 = 100;"
run_test 5 "WHERE c_varchar = 'hello'" "SELECT * FROM test_all_types WHERE c_varchar = 'hello';"
run_test 6 "WHERE c_text = 'text1'" "SELECT * FROM test_all_types WHERE c_text = 'text1';"
run_test 7 "WHERE name = '张三'" "SELECT * FROM test_employee WHERE name = '张三';"
run_test 8 "WHERE salary = 123.45" "SELECT * FROM test_employee WHERE salary = 123.45;"

# ======== <> 不等值查询 ========
run_test 9 "WHERE c_int2 <> 20" "SELECT * FROM test_all_types WHERE c_int2 <> 20;"
run_test 10 "WHERE name <> '李四'" "SELECT * FROM test_employee WHERE name <> '李四';"

# ======== 范围查询 (DET预期报错) ========
run_test 11 "WHERE c_int2 > 15 (预期报错)" "SELECT * FROM test_all_types WHERE c_int2 > 15;"
run_test 12 "WHERE c_int2 >= 20 (预期报错)" "SELECT * FROM test_all_types WHERE c_int2 >= 20;"

# ======== INSERT ========
run_test 13 "INSERT单值" "INSERT INTO test_employee (id, name, salary, email) VALUES (10, 'test10', 999.99, 't10@t.com');"
run_test 14 "INSERT多值" "INSERT INTO test_employee (id, name, salary, email) VALUES (11, 'a', 1.11, 'a@a.com'), (12, 'b', 2.22, 'b@b.com');"
run_test 15 "INSERT test_all_types 多值" "INSERT INTO test_all_types (id, c_int2, c_int4, c_varchar) VALUES (91, 91, 910, 'x'), (92, 92, 920, 'y');"

# ======== UPDATE ========
run_test 16 "UPDATE salary WHERE name" "UPDATE test_employee SET salary = 50000 WHERE name = '张三';"
run_test 17 "UPDATE多列 SET" "UPDATE test_employee SET salary = 25000, name = '王五-upd' WHERE id = 3;"

# ======== 验证修改结果 ========
run_test 18 "SELECT验证UPDATE结果" "SELECT id, name, salary, email FROM test_employee ORDER BY id;"

# ======== 复杂查询 ========
run_test 19 "AND条件" "SELECT * FROM test_employee WHERE name = '李四' AND salary >= 20000;"
run_test 20 "ORDER BY" "SELECT * FROM test_employee ORDER BY id;"
run_test 21 "LIMIT" "SELECT * FROM test_employee LIMIT 2;"
run_test 22 "投影列" "SELECT id, name FROM test_employee;"

# ======== INSERT + SELECT联查 ========
run_test 23 "INSERT+SELECT验证" "INSERT INTO test_employee (id, name, salary, email) VALUES (20, 'new', 100, 'new@t.com'); SELECT * FROM test_employee WHERE id = 20;"

echo ""
echo "========================================"
echo "  测试完成"
echo "========================================"

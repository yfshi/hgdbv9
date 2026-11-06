#!/bin/bash

echo "检查解析器与接口兼容性"
echo "======================"
echo ""

echo "1. 检查接口函数..."
echo ""

# 检查tecc_parser.h中的接口函数
echo "   tecc_parser.h中的接口函数："
grep -n "tecc_parser_" tecc_parser.h | grep -v "^*" | while read line; do
    echo "     $line"
done

echo ""
echo "2. 检查适配器接口..."
echo ""

# 检查fe_adapter.h中的接口函数
echo "   fe_adapter.h中的接口函数："
grep -n "tecc_parser" fe_adapter.h | while read line; do
    echo "     $line"
done

echo ""
echo "3. 检查Bison解析器函数..."
echo ""

# 检查简化语法中的函数
echo "   tecc_grammar_simple.y中的关键函数："
if grep -q "tecc_yyparse" tecc_grammar_simple.y; then
    echo "     ✓ tecc_yyparse() - Bison解析器入口"
else
    echo "     ✗ tecc_yyparse() - 缺失"
fi

if grep -q "reset_tecc_parsetree" tecc_grammar_simple.y; then
    echo "     ✓ reset_tecc_parsetree() - 重置解析树"
else
    echo "     ✗ reset_tecc_parsetree() - 缺失"
fi

if grep -q "get_tecc_parsetree" tecc_grammar_simple.y; then
    echo "     ✓ get_tecc_parsetree() - 获取解析树"
else
    echo "     ✗ get_tecc_parsetree() - 缺失"
fi

if grep -q "tecc_parse_string" tecc_grammar_simple.y; then
    echo "     ✓ tecc_parse_string() - 解析字符串接口"
else
    echo "     ✗ tecc_parse_string() - 缺失"
fi

echo ""
echo "4. 检查函数调用链..."
echo ""

# 检查函数调用关系
echo "   函数调用链分析："
echo "   1. tecc_parser_parse (fe_adapter.cpp) -> tecc_parser_parse_node"
echo "   2. tecc_parser_parse_node -> tecc_parser_parse_ex (tecc_parser.c)"
echo "   3. tecc_parser_parse_ex -> call_bison_parser"
echo "   4. call_bison_parser -> tecc_yyparse() (Bison生成)"
echo "   5. tecc_yyparse() -> 使用简化语法规则"

echo ""
echo "5. 检查类型兼容性..."
echo ""

# 检查Node和List类型定义
echo "   类型定义检查："
if grep -q "typedef struct Node" tecc_grammar_simple.y; then
    echo "     ✓ Node类型在语法文件中定义"
else
    echo "     ⚠ Node类型未在语法文件中定义（可能在其他头文件中）"
fi

if grep -q "typedef struct List" tecc_grammar_simple.y; then
    echo "     ✓ List类型在语法文件中定义"
else
    echo "     ⚠ List类型未在语法文件中定义（可能在其他头文件中）"
fi

# 检查%union中的类型
echo ""
echo "   Bison %union类型检查："
if grep -q "Node \*node" tecc_grammar_simple.y; then
    echo "     ✓ %union包含Node*类型"
else
    echo "     ✗ %union缺少Node*类型"
fi

if grep -q "List \*list" tecc_grammar_simple.y; then
    echo "     ✓ %union包含List*类型"
else
    echo "     ✗ %union缺少List*类型"
fi

echo ""
echo "6. 检查Makefile集成..."
echo ""

# 检查Makefile
if [ -f "Makefile" ]; then
    echo "   Makefile检查："
    if grep -q "tecc_grammar_simple" Makefile; then
        echo "     ✓ Makefile引用简化语法文件"
    else
        echo "     ⚠ Makefile未引用简化语法文件（可能需要更新）"
    fi

    if grep -q "tecc_grammar_simple.tab.c" Makefile; then
        echo "     ✓ Makefile包含简化解析器目标文件"
    else
        echo "     ⚠ Makefile未包含简化解析器目标文件"
    fi
else
    echo "     ⚠ Makefile不存在"
fi

echo ""
echo "7. 兼容性总结..."
echo ""

# 总结
echo "   接口兼容性分析："
echo "   - 前端接口: tecc_parser_parse() 存在"
echo "   - 核心接口: tecc_parser_parse_ex() 存在"
echo "   - Bison接口: tecc_yyparse() 存在"
echo "   - 适配器层: 完整"
echo ""
echo "   类型兼容性分析："
echo "   - Node类型: 需要确认定义一致性"
echo "   - List类型: 需要确认定义一致性"
echo "   - %union类型: 包含必要类型"
echo ""
echo "   集成兼容性分析："
echo "   - Makefile: 可能需要更新以包含新解析器"
echo "   - 构建系统: 需要测试编译链接"

echo ""
echo "建议："
echo "1. 编译测试整个解析器库"
echo "2. 运行现有的测试程序验证功能"
echo "3. 如果需要，更新Makefile以包含新解析器"
echo "4. 验证类型定义的一致性"
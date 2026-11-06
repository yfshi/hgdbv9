#!/bin/bash

echo "开始修复tecc_grammar.y中的TODO注释..."

# 修复所有表达式TODO注释
sed -i 's/expr PLUS expr     { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("\+")), \$1, \$3, @2.first_line); \*\/ }/expr PLUS expr     { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("\+")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr MINUS expr    { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("-")), \$1, \$3, @2.first_line); \*\/ }/expr MINUS expr    { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("-")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr STAR expr     { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("\*")), \$1, \$3, @2.first_line); \*\/ }/expr STAR expr     { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("\*")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr SLASH expr    { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("\/")), \$1, \$3, @2.first_line); \*\/ }/expr SLASH expr    { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("\/")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr PERCENT expr  { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("%")), \$1, \$3, @2.first_line); \*\/ }/expr PERCENT expr  { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("%")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr EQUAL expr    { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("=")), \$1, \$3, @2.first_line); \*\/ }/expr EQUAL expr    { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("=")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr NOT_EQUAL expr { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("<>")), \$1, \$3, @2.first_line); \*\/ }/expr NOT_EQUAL expr { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("<>")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr LESS expr     { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("<")), \$1, \$3, @2.first_line); \*\/ }/expr LESS expr     { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("<")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr GREATER expr  { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString(">")), \$1, \$3, @2.first_line); \*\/ }/expr GREATER expr  { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString(">")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr LESS_EQUAL expr { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString("<=")), \$1, \$3, @2.first_line); \*\/ }/expr LESS_EQUAL expr { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString("<=")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr GREATER_EQUAL expr { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_OP, list_make1(makeString(">=")), \$1, \$3, @2.first_line); \*\/ }/expr GREATER_EQUAL expr { \$\$ = (Node *)makeAExpr(AEXPR_OP, list_make1(makeString(">=")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr AND expr      { \$\$ = NULL; \/\* TODO: make_bool_expr(AND_EXPR, list_make2(\$1, \$3), @2.first_line); \*\/ }/expr AND expr      { \$\$ = (Node *)makeBoolExpr(AND_EXPR, list_make2(\$1, \$3), @2.first_line); }/' tecc_grammar.y

sed -i 's/expr OR expr       { \$\$ = NULL; \/\* TODO: make_bool_expr(OR_EXPR, list_make2(\$1, \$3), @2.first_line); \*\/ }/expr OR expr       { \$\$ = (Node *)makeBoolExpr(OR_EXPR, list_make2(\$1, \$3), @2.first_line); }/' tecc_grammar.y

sed -i 's/NOT expr           { \$\$ = NULL; \/\* TODO: make_bool_expr(NOT_EXPR, list_make1(\$2), @1.first_line); \*\/ }/NOT expr           { \$\$ = (Node *)makeBoolExpr(NOT_EXPR, list_make1(\$2), @1.first_line); }/' tecc_grammar.y

sed -i 's/expr IS NULL_P     { \$\$ = NULL; \/\* TODO: make_null_test(\$1, IS_NULL, @2.first_line); \*\/ }/expr IS NULL_P     { \$\$ = (Node *)makeNullTest(\$1, IS_NULL, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr IS NOT NULL_P { \$\$ = NULL; \/\* TODO: make_null_test(\$1, IS_NOT_NULL, @2.first_line); \*\/ }/expr IS NOT NULL_P { \$\$ = (Node *)makeNullTest(\$1, IS_NOT_NULL, @2.first_line); }/' tecc_grammar.y

sed -i 's/expr LIKE expr     { \$\$ = NULL; \/\* TODO: make_a_expr(AEXPR_LIKE, list_make1(makeString("LIKE")), \$1, \$3, @2.first_line); \*\/ }/expr LIKE expr     { \$\$ = (Node *)makeAExpr(AEXPR_LIKE, list_make1(makeString("LIKE")), \$1, \$3, @2.first_line); }/' tecc_grammar.y

sed -i 's/PARAM              { \$\$ = NULL; \/\* TODO: makeParam(PARAM_EXTERN, \$1, 0, @1.first_line); \*\/ }/PARAM              { \$\$ = (Node *)makeParam(PARAM_EXTERN, \$1, 0, @1.first_line); }/' tecc_grammar.y

echo "修复完成！"
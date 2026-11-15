/* Begin - ReqID:TECC4.0 */
/*
 * File: stmt_processor.c
 *
 * Abstract:
 *   Statement processor implementation for TECC processor module.
 *   Implements SQL statement type identification, routing logic,
 *   and coordination with specialized processors.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/stmt_processor.c
 */

#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include <unistd.h>
#include <sys/stat.h>
#include "libpq-fe.h"  /* For PQdb() function */
#include "stmt_processor.h"
#include "values_processor.h"
#include "where_clause_processor.h"
#include "create_stmt_processor.h"
#include "alter_stmt_processor.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* tecc_unified_encrypt_string */
/* Modify - ReqID:TECC4.0: tecc_he_load_dek_key_hex / tecc_he_load_kek_arg_hex
 * 声明在 tecc_algorithm_scheduler.h，显式包含避免隐式声明 */
#include "../tecc_scheduler/tecc_algorithm_scheduler.h"
#include "create_kek_processor.h"

/* Include expression processor interface for TeccColumnRefData */
#include "column_ref_data.h"
#include "../frontend_parser/tecc_nodes.h"
#include "expr_processor.h"

/* Include frontend parser definitions for CreateStmt and ColumnDef */
#include "../frontend_parser/fe_adapter.h"  /* For PostgreSQL types */
#include "../frontend_parser/tecc_nodes.h"  /* For node type definitions */

/* Forward declarations for external module interfaces */
/* Cache module interface */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "catalog/pg_type_d.h"   /* ReqID:TECC4.0 - TEXTOID etc. for type-category check */
#include "catalog/pg_type_enc.h"  /* ReqID:TECC4.0 - TEXT_ENCOID etc. for type check */
/* ReqID:TECC4.0 - Bug 9327: 字符型密态 cast 的长度定型（tecc_char_cast_len_clip） */
#include "../tecc_formatter/tecc_type_output.h"
#include "mb/pg_wchar.h"   /* ReqID:TECC4.0 - PG_UTF8（客户端编码回退值） */

/* TECC state management */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* For tecc_get_state() */

/* ReqID:TECC4.0 - Bug 9255: ALTER KEK 前端 USAGE 权限预检 */
#include "../tecc_data_fetcher/include/tecc_permission_check.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"  /* ReqID:TECC4.0 - tecc_fetcher_execute_query() 列存在性探测 */

/* Security and key management interfaces */
#include "../../../common/tecc_key/security.h"
#include "../../../common/tecc_key/high_level_api.h"
#include "../../../common/tecc_key/kms_adapter.h"
/* Modify - ReqID:TECC4.0: ALTER KEK 轮换使用 SM2 信封加解密，需显式包含
 * tecc_sm2.h（TECC_SM2_SUCCESS / tecc_sm2_envelope_encrypt 等声明） */
#include "../../../src/include/common/tecc_key/tecc_sm2.h"
/* ReqID:TECC4.0 - tecc_bn_to_fixed() 用到 BN_num_bytes/BN_bn2bin，
 * 显式包含 bn.h（跨 1.0.2/1.1.x/3.0 均可用）。 */
#include <openssl/bn.h>

/* ReqID:TECC4.0 - HE 密钥对生成接口（同态算法组提供） */
#include "../../../src/include/common/tecc_algorithm/tecc_ec_elgamal.h"
#include "../../../src/include/common/tecc_algorithm/tecc_elgamal.h"
/* ReqID:TECC4.0 - SM2 信封加解密（HE 私钥保护、ALTER KEK 重加密） */
#include "../../../src/include/common/tecc_key/tecc_sm2.h"
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"
#include "tecc_expression_walker.h"

/* ReqID:TECC4.0 - Encryption type bitmask flags (from tecc_processor_internal.h) */
#ifndef TECC_ENC_TYPE_DET
#define TECC_ENC_TYPE_DET    0x01
#define TECC_ENC_TYPE_OPE    0x02
#define TECC_ENC_TYPE_SEARCH 0x04
#endif

/*
 * Forward declarations for internal functions
 */
static TeccStmtProcessorResult stmt_extract_and_check_columns(const Node *stmt,
                                                             TeccStatementData *statement_data,
                                                             List **encrypted_columns);
static TeccStmtProcessorResult stmt_extract_select_columns(const SelectStmt *select_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_insert_columns(const InsertStmt *insert_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_update_columns(const UpdateStmt *update_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_delete_columns(const DeleteStmt *delete_stmt,
                                                          List **columns,
                                                          const TeccStatementData *statement_data);
static TeccStmtProcessorResult stmt_extract_create_columns(const CreateStmt *create_stmt,
                                                          List **columns);
static TeccStmtProcessorResult stmt_extract_copy_columns(const CopyStmt *copy_stmt,
                                                        List **columns,
                                                        const TeccStatementData *statement_data);
static void stmt_extract_columns_from_expression(const Node *expr, List **columns,
                                                const TeccStatementData *statement_data);

/* Helper functions for metadata loading and checking */
static bool stmt_load_table_metadata(const RangeVar *range_var, TeccStatementData *statement_data, TeccColumnCacheItem ***encrypted_columns_ptr, int *column_count_ptr);
static bool stmt_check_column_encrypted(const TeccStatementData *statement_data, const char *schema_name, const char *table_name, const char *column_name);
static bool stmt_query_metadata_cache_simple(const TeccStatementData *statement_data, const char *schema_name, const char *table_name, const char *column_name);
static bool stmt_is_column_in_encrypted_list(const char *column_name, TeccColumnCacheItem **encrypted_columns, int column_count);
/* ReqID:TECC4.0 - 子查询/CTE 改写状态向父语句传播（Bug 9152 家族） */
static void stmt_propagate_sub_rewrite_state(TeccStatementData *statement_data,
                                            const TeccStatementData *sub_data,
                                            const char *sub_alias);
static void stmt_propagate_sub_encrypted_columns(TeccStatementData *statement_data,
                                                const TeccStatementData *sub_data,
                                                const char *sub_alias);

/*
 * ReqID:TECC4.0 - OpenSSL 1.0.2 兼容的定长大端序列化。
 * BN_bn2binpad 在内网(1.0.2)环境缺失，改用 BN_bn2bin 手动左补零。
 * 语义与 BN_bn2binpad(bn, buf, size) 一致：写入 size 字节大端(左补零)，
 * 返回 size；bn 超过 size 字节则失败返回 0。
 */
static int
tecc_bn_to_fixed(const BIGNUM *bn, unsigned char *buf, size_t size)
{
	int			nbytes = BN_num_bytes(bn);

	if (bn == NULL || nbytes < 0 || (size_t) nbytes > size)
		return 0;
	memset(buf, 0, size);
	if (nbytes > 0)
		BN_bn2bin(bn, buf + (size - (size_t) nbytes));
	return (int) size;
}

/* End - ReqID:TECC4.0 */

/* Process specific statement type */

TeccStmtProcessorResult tecc_stmt_processor_process_alter_kek(const AlterKekStmt *alter_kek_stmt,
                                             TeccStatementData *statement_data);

/* ReqID:TECC4.0 - 前向声明：按 KEK 类型（HSM/非对称/对称）加密 DEK，
 * ALTER KEK 轮换复用（定义在文件后部）。 */
static int encrypt_dek_based_on_kek_type(const unsigned char *plain_dek, size_t plain_dek_len,
                                         const unsigned char *kek, size_t kek_len,
                                         bool is_asym,
                                         unsigned char **encrypted_dek, size_t *encrypted_dek_len);

/* End - ReqID:TECC4.0 */

/* Forward declaration */
static TeccStmtProcessorResult
process_insert_select_typecast(SelectStmt *select_stmt,
                              TeccStatementData *statement_data,
                              const InsertStmt *insert_stmt,
                              TeccColumnCacheItem **table_columns,
                              int table_column_count);

/*
 * Process constants in FuncCall arguments for encryption.
 *
 * These functions can mix encrypted columns with plaintext constants.
 * Constants that may be returned to an outer expression (COALESCE/GREATEST/
 * LEAST) are encrypted with maximal algorithms (RND+OPE+SEARCH for char types)
 * so they work in any outer context. NULLIF constants are only compared,
 * not returned, so DET is sufficient.
 *
 * Multi-column DEK consistency is checked: all encrypted columns in the
 * same function call must share the same DEK. ReqID:TECC4.0
 */
/*
 * ReqID:TECC4.0 - 收集标量函数参数中的加密列（递归下钻内层 FuncCall，
 * 支持 COALESCE(MIN(c_int4), 0) 这类嵌套聚合）。ColumnRef 收集成功且
 * DEK 一致返回 true；DEK 不一致时设置错误并返回 false。
 */
static bool
stmt_collect_funcall_enc_columns(Node *arg, TeccStatementData *statement_data,
								 const char *func_name,
								 bool *has_encrypted_col,
								 TeccColumnCacheItem **enc_cache,
								 const char **common_dek_name,
								 int *union_encryption_type,
								 bool *is_char_type)
{
	ListCell   *ilc;

	if (arg == NULL)
		return true;

	/* 递归下钻内层 FuncCall（MIN/MAX/COALESCE 等嵌套调用） */
	if (nodeTag(arg) == FE_T_FuncCall)
	{
		FuncCall   *inner_fc = (FuncCall *) arg;

		foreach(ilc, inner_fc->args)
		{
			Node	   *iarg = (Node *) ilc->data.ptr_value;

			if (!stmt_collect_funcall_enc_columns(iarg, statement_data,
												  func_name,
												  has_encrypted_col, enc_cache,
												  common_dek_name,
												  union_encryption_type,
												  is_char_type))
				return false;
		}
		return true;
	}

	if (nodeTag(arg) != FE_T_ColumnRef)
		return true;

	{
		struct ColumnRef *col_ref = (struct ColumnRef *) arg;
		char	   *col_name = NULL;

		if (col_ref->fields != NIL)
		{
			int			num_fields = list_length(col_ref->fields);
			Node	   *field = (Node *) list_nth(col_ref->fields, num_fields - 1);

			col_name = strVal((Value *) field);
		}
		if (col_name == NULL)
			return true;

		/* Check in statement_data->encrypted_columns */
		foreach(ilc, statement_data->encrypted_columns)
		{
			TeccColumnRefData *ec = (TeccColumnRefData *) ilc->data.ptr_value;

			if (ec != NULL && ec->column_name[0] != '\0' &&
				strcmp(ec->column_name, col_name) == 0)
			{
				TeccColumnCacheItem *col_cache = NULL;

				/* Look up cache for full metadata */
				if (statement_data->conn != NULL)
				{
					TeccState *s = tecc_get_state(statement_data->conn);
					const char *db = PQdb(statement_data->conn);

					/* ReqID:TECC4.0 - 用列登记时的 schema（FROM 分析
					 * fallback 为 public），不依赖 current_schema GUC：
					 * search_path 首项为用户 schema（如 highgo）时 GUC
					 * 与表实际 schema 不一致，会命中该 schema 下的负缓存
					 * 哨兵导致加密列漏检（COALESCE on HE 列静默不加密）。 */
					const char *schema = (ec->schema_name[0] != '\0') ?
						ec->schema_name : "public";

					if (s != NULL && db != NULL && schema != NULL &&
						ec->table_name[0] != '\0')
					{
						col_cache = tecc_cache_get_encrypted_column_by_name(
							s, db, schema, ec->table_name, col_name);

					}
				}

				/* ReqID:TECC4.0 - 纯 HE 列（HE_ADD_DEK/HE_MUL_DEK）的
				 * dek_name 为 NULL 是合法状态（一列多 DEK 设计），
				 * 不得当作"非加密列"跳过。 */
				if (col_cache == NULL)
					break;

				if (!*has_encrypted_col)
				{
					/* First encrypted column — use as reference */
					*has_encrypted_col = true;
					*enc_cache = col_cache;
					*common_dek_name = (col_cache->dek_name != NULL) ?
						col_cache->dek_name :
						(col_cache->he_add_dek_name != NULL ?
						 col_cache->he_add_dek_name :
						 (col_cache->he_mul_dek_name != NULL ?
						  col_cache->he_mul_dek_name : ""));
					*union_encryption_type = col_cache->encryption_type;
					if (col_cache->original_type_oid == VARCHAR_ENCOID ||
						col_cache->original_type_oid == BPCHAR_ENCOID ||
						col_cache->original_type_oid == TEXT_ENCOID)
						*is_char_type = true;
				}
				else
				{
					/* Additional encrypted column — check DEK consistency */
					*union_encryption_type |= col_cache->encryption_type;
					if (!*is_char_type &&
						(col_cache->original_type_oid == VARCHAR_ENCOID ||
						 col_cache->original_type_oid == BPCHAR_ENCOID ||
						 col_cache->original_type_oid == TEXT_ENCOID))
						*is_char_type = true;
					{
						const char *cur_dek = (col_cache->dek_name != NULL) ?
							col_cache->dek_name :
							(col_cache->he_add_dek_name != NULL ?
							 col_cache->he_add_dek_name :
							 (col_cache->he_mul_dek_name != NULL ?
							  col_cache->he_mul_dek_name : ""));

						if (strcmp(*common_dek_name, cur_dek) != 0)
						{
							tecc_processor_set_error(statement_data,
								TECC_PROCESSOR_ERROR,
								"function \"%s\" has encrypted columns with "
								"different DEKs: \"%s\" and \"%s\"",
								func_name, *common_dek_name, cur_dek);
							return false;
						}
					}
				}
				break;
			}
		}
	}
	return true;
}

/*
 * ReqID:TECC4.0 - 显式 cast 常量支持（GREATEST/LEAST/COALESCE/NULLIF）
 *
 * 识别 TypeCast(Const) 形态（如 0::int8、'x'::varchar），把常量按
 * cast 目标基类型加密（DEK/算法策略仍取密态列），整段 cast 表达式
 * 改写为 E'\x..'::<enc类型> 字面量，接入服务端隐式拓宽格：与密态列
 * 可转换的组合正常比较，洞/跨族组合由服务端按既有矩阵报错。
 */

/* cast 目标类型名 → 基类型 OID（仅 9 种密态基类型 + 常用别名；
 * 未知类型返回 InvalidOid，调用方跳过、维持服务端兜底报错）。 */
static Oid
stmt_cast_name_to_base_oid(const TypeName *typeName)
{
	static const struct {
		const char *name;
		Oid			base_oid;
	} cast_map[] = {
		{"int2", INT2OID}, {"smallint", INT2OID},
		{"int4", INT4OID}, {"int", INT4OID}, {"integer", INT4OID},
		{"int8", INT8OID}, {"bigint", INT8OID},
		{"float4", FLOAT4OID}, {"real", FLOAT4OID},
		{"float8", FLOAT8OID}, {"double precision", FLOAT8OID},
		{"numeric", NUMERICOID}, {"decimal", NUMERICOID},
		{"text", TEXTOID},
		{"varchar", VARCHAROID}, {"character varying", VARCHAROID},
		{"bpchar", BPCHAROID}, {"char", BPCHAROID}, {"character", BPCHAROID},
		{NULL, InvalidOid}
	};
	char		name_buf[64];
	int			i;
	ListCell   *lc;
	size_t		off = 0;

	if (typeName == NULL || typeName->names == NIL)
		return InvalidOid;

	/* 拼接类型名（多词类型名以空格连接） */
	foreach(lc, typeName->names)
	{
		const char *part = strVal((Value *) lfirst(lc));
		size_t		plen;

		if (part == NULL)
			return InvalidOid;
		plen = strlen(part);
		if (off > 0 && off + 1 < sizeof(name_buf))
			name_buf[off++] = ' ';
		if (off + plen >= sizeof(name_buf))
			return InvalidOid;
		memcpy(name_buf + off, part, plen);
		off += plen;
	}
	name_buf[off] = '\0';

	for (i = 0; cast_map[i].name != NULL; i++)
	{
		if (pg_strcasecmp(cast_map[i].name, name_buf) == 0)
			return cast_map[i].base_oid;
	}

	/* 限定名（pg_catalog.int8）整体未命中时，退化按最后一段匹配 */
	if (list_length(typeName->names) == 2)
	{
		const char *last_name = strVal((Value *) llast(typeName->names));

		if (last_name != NULL)
		{
			for (i = 0; cast_map[i].name != NULL; i++)
			{
				if (pg_strcasecmp(cast_map[i].name, last_name) == 0)
					return cast_map[i].base_oid;
			}
		}
	}
	return InvalidOid;
}

static bool
stmt_is_type_name_char(char ch)
{
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
		(ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '"')
		return true;
	return false;
}

/* 从 inner 常量文本末尾起扫描 '::typename[(typmod)]' 的结束位置
 * （跳过空白；typmod 括号配平）。找不到 '::' 返回 0。 */
static size_t
stmt_scan_type_cast_extent(const char *query, size_t from)
{
	size_t		pos = from;
	int			depth;

	if (query == NULL)
		return 0;

	while (query[pos] == ' ' || query[pos] == '\t' ||
		   query[pos] == '\n' || query[pos] == '\r')
		pos++;
	if (query[pos] != ':' || query[pos + 1] != ':')
		return 0;
	pos += 2;
	while (query[pos] == ' ' || query[pos] == '\t' ||
		   query[pos] == '\n' || query[pos] == '\r')
		pos++;

	for (;;)
	{
		char		ch = query[pos];

		if (ch == '\0')
			return 0;
		if (ch == '(')
		{
			depth = 1;
			pos++;
			while (depth > 0)
			{
				ch = query[pos];
				if (ch == '\0')
					return 0;
				if (ch == '(')
					depth++;
				else if (ch == ')')
					depth--;
				pos++;
			}
			break;				/* typmod 消费完，即 cast 表达式末尾 */
		}
		if (!stmt_is_type_name_char(ch))
			break;
		pos++;
	}
	return pos;
}

/* 解析 TypeCast(Const) 参数：返回 true 并填充改写区间/目标基类型/
 * 待加密文本（字符串常量用解析后的解码值，无外层引号）。
 * 内层支持 Const 与 UMINUS(Const)（-1::int8 的负号在 cast 内侧）。 */
static bool
stmt_parse_cast_const_arg(Node *arg, const char *query,
						  size_t *out_loc, size_t *out_dsize,
						  Oid *out_base_oid, char **out_value_text)
{
	TypeCast   *tc = (TypeCast *) arg;
	Const	   *inner = NULL;
	size_t		value_loc = 0;
	size_t		value_size = 0;
	Oid			base_oid;
	size_t		inner_end;
	size_t		extent;

	if (arg == NULL || nodeTag(arg) != FE_T_TypeCast)
		return false;
	if (tc->location < 0 || tc->typeName == NULL)
		return false;

	/* 内层表达式：Const 或 UMINUS(Const) */
	if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_Const)
	{
		inner = (Const *) tc->arg;
		value_loc = (size_t) inner->location;
	}
	else if (tc->arg != NULL && nodeTag(tc->arg) == FE_T_A_Expr)
	{
		A_Expr	   *aexpr = (A_Expr *) tc->arg;

		if (aexpr->kind == AEXPR_OP && aexpr->lexpr == NULL &&
			aexpr->rexpr != NULL && nodeTag(aexpr->rexpr) == FE_T_Const &&
			aexpr->location >= 0)
		{
			inner = (Const *) aexpr->rexpr;
			value_loc = (size_t) aexpr->location;	/* 含负号 */
		}
	}
	if (inner == NULL)
		return false;
	if (inner->constisnull || inner->location < 0 || inner->data_size <= 0)
		return false;
	value_size = (size_t) inner->location + (size_t) inner->data_size -
		value_loc;

	base_oid = stmt_cast_name_to_base_oid(tc->typeName);
	if (!OidIsValid(base_oid))
		return false;

	/* 改写区间 = 整段 cast 表达式（inner 常量结束后的 ::类型名 含 typmod） */
	inner_end = (size_t) inner->location + (size_t) inner->data_size;
	extent = stmt_scan_type_cast_extent(query, inner_end);
	if (extent <= (size_t) tc->location)
		return false;

	*out_loc = (value_loc < (size_t) tc->location) ?
		value_loc : (size_t) tc->location;
	*out_dsize = extent - *out_loc;
	*out_base_oid = base_oid;
	if (inner->consttype == TEXTOID && inner->constvalue.sval != NULL)
		*out_value_text = tecc_processor_strdup(inner->constvalue.sval);
	else
	{
		/* ReqID:TECC4.0 - 数值常量取内层表达式自身的文本切片
		 * （仅数字/负号部分），不能取整段 cast 表达式（含 ::类型名）。 */
		char   *slice = (char *) pg_malloc(value_size + 1);

		if (slice == NULL)
			return false;
		memcpy(slice, query + value_loc, value_size);
		slice[value_size] = '\0';
		*out_value_text = slice;
	}
	return true;
}

void
stmt_process_funcall_constants(FuncCall *fc, TeccStatementData *statement_data)
{
    const char *func_name;
    ListCell *lc;
    bool has_encrypted_col = false;
    TeccColumnCacheItem *enc_cache = NULL;  /* first encrypted column's cache */
    const char *common_dek_name = NULL;
    bool may_return_const;                  /* true=constant may be returned to outer expr */
    bool is_char_type = false;              /* any encrypted column is character type */
    int union_encryption_type = 0;          /* union of algo bits across all columns */

    if (fc == NULL || statement_data == NULL)
        return;
    if (fc->funcname == NIL || fc->args == NIL)
        return;

    /* Get function name */
    {
        Node *name_node = (Node *) linitial(fc->funcname);
        Value *v;
        if (name_node == NULL)
            return;
        v = (Value *) name_node;
        func_name = strVal(v);
    }

    /* Only process specific functions that mix encrypted cols with constants */
    if (strcasecmp(func_name, "coalesce") != 0 &&
        strcasecmp(func_name, "nullif") != 0 &&
        strcasecmp(func_name, "greatest") != 0 &&
        strcasecmp(func_name, "least") != 0 &&
        strcasecmp(func_name, "decode") != 0)
        return;

    /* ReqID:TECC4.0 - NULLIF only compares, never returns the constant.
     * COALESCE/GREATEST/LEAST/decode may return the constant to outer
     * expressions. */
    may_return_const = (strcasecmp(func_name, "nullif") != 0);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "stmt_process_funcall_constants: processing %s(), may_return_const=%d",
                       func_name, may_return_const);

    /*
     * Step 1: Collect ALL encrypted columns from arguments.
     * Check DEK consistency — all must share the same DEK.
     * Track whether any column is character type for SEARCH decision.
     * ReqID:TECC4.0 - 嵌套聚合（如 COALESCE(MIN(c_int4), 0)）的加密列
     * 藏在内层 FuncCall 参数里，递归下钻收集。
     */
    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        if (!stmt_collect_funcall_enc_columns(arg, statement_data, func_name,
                                              &has_encrypted_col, &enc_cache,
                                              &common_dek_name,
                                              &union_encryption_type,
                                              &is_char_type))
            return;
    }

    if (!has_encrypted_col)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "stmt_process_funcall_constants: no encrypted column in %s()",
                           func_name);
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "stmt_process_funcall_constants: found encrypted col in %s(), "
                       "DEK=%s, char_type=%d, union_algo=0x%02x",
                       func_name, common_dek_name, is_char_type,
                       union_encryption_type);

    /* Step 2: Encrypt constant arguments with appropriate strategy */
    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        Oid const_oid = InvalidOid;
        Oid cast_target_oid = InvalidOid;   /* ReqID:TECC4.0 - cast 常量目标基类型 */
        size_t const_loc = 0, const_dsize = 0;
        char *const_text = NULL;
        /* ReqID:TECC4.0 - 该常量是否为"数值文本形态"的浮点字面量
         * （前端为保精度保留为 TEXTOID 字符串，见 tecc_grammar.y FLOAT 规则）。 */
        bool const_text_numeric = false;

        if (arg == NULL)
            continue;

        /* ReqID:TECC4.0 - 递归下钻内层标量函数（如
         * GREATEST(COALESCE(MIN(c_int4), 0), ...)）：内层函数的常量
         * 同样需要按列加密。加密结果按 location 记录、统一重写，
         * 递归安全。MIN/SUM 等非标量函数在入口过滤处直接返回。 */
        if (nodeTag(arg) == FE_T_FuncCall)
        {
            stmt_process_funcall_constants((FuncCall *) arg, statement_data);
            continue;
        }

        /* ReqID:TECC4.0 - 常量识别支持三种形态：裸 Const、UMINUS+Const、
         * 显式 TypeCast(Const)（如 0::int8、'x'::varchar）。cast 常量按
         * 目标基类型加密（列 DEK/算法不变），整段 cast 改写为
         * E'\x..'::<enc类型>，接入服务端隐式拓宽格；跨族 cast 维持
         * 现有跳过行为（服务端兜底报错）。 */
        cast_target_oid = InvalidOid;
        if (nodeTag(arg) == FE_T_Const)
        {
            Const *cnode = (Const *) arg;
            if (cnode->constisnull || cnode->location < 0 || cnode->data_size <= 0)
                continue;
            const_loc = (size_t)cnode->location;
            const_dsize = (size_t)cnode->data_size;
            const_oid = cnode->consttype;
            /* ReqID:TECC4.0 - 浮点字面量：前端保留其文本但标记了
             * text_numeric_literal，语义上是数值常量而非字符串常量。
             * 若不识别，下面的 char/non-char 类别检查会把它当字符串跳过
             * 加密，常量原样发给服务端后报
             * "GREATEST types <enc> and numeric cannot be matched"。 */
            if (cnode->consttype == TEXTOID && cnode->text_numeric_literal)
                const_text_numeric = true;
            /* ReqID:TECC4.0 - 字符串常量改用解析后的解码值（剥外层引号、
             * '' 还原），避免引号随内容一起加密导致比较语义错误。 */
            if (cnode->consttype == TEXTOID && cnode->constvalue.sval != NULL)
                const_text = tecc_processor_strdup(cnode->constvalue.sval);
        }
        else if (nodeTag(arg) == FE_T_A_Expr)
        {
            A_Expr *aexpr = (A_Expr *) arg;
            Node *inner;
            if (aexpr->kind != AEXPR_OP)
                continue;
            if (aexpr->lexpr == NULL && aexpr->rexpr != NULL)
                inner = aexpr->rexpr;
            else if (aexpr->rexpr == NULL && aexpr->lexpr != NULL)
                inner = aexpr->lexpr;
            else
                continue;
            if (nodeTag(inner) == FE_T_TypeCast)
            {
                /* ReqID:TECC4.0 - 负数 cast 常量（-1::int8 解析为
                 * UMINUS(TypeCast(Const))）：改写区间从负号起。 */
                if (!stmt_parse_cast_const_arg(inner, statement_data->query,
                                               &const_loc, &const_dsize,
                                               &cast_target_oid, &const_text))
                    continue;
                if (aexpr->location >= 0 &&
                    (size_t)aexpr->location < const_loc)
                {
                    const_dsize += const_loc - (size_t)aexpr->location;
                    const_loc = (size_t)aexpr->location;
                }
                const_oid = cast_target_oid;
            }
            else if (nodeTag(inner) == FE_T_Const)
            {
                Const *ic = (Const *) inner;
                if (ic->constisnull || ic->location < 0 || ic->data_size <= 0)
                    continue;
                if (aexpr->location < 0)
                    continue;
                const_loc = (size_t)aexpr->location;
                const_dsize = (size_t)(ic->location + ic->data_size) -
                              (size_t)aexpr->location;
                const_oid = ic->consttype;
            }
            else
                continue;
        }
        else if (nodeTag(arg) == FE_T_TypeCast)
        {
            if (!stmt_parse_cast_const_arg(arg, statement_data->query,
                                           &const_loc, &const_dsize,
                                           &cast_target_oid, &const_text))
                continue;
            const_oid = cast_target_oid;
        }
        else
            continue;

        {
            const char *query = statement_data->query;
            TeccUnifiedEncryptResult *encrypt_result;
            TeccRawValue *raw_value;
            const char *enc_type_name;
            char *table_name;
            char *column_name;
            char *dek_name;
            const char *schema_name;
            Oid encrypt_base_oid = InvalidOid;

            /* ReqID:TECC4.0 - 解析节点未提供文本时，回退原始 SQL 切片 */
            if (const_text == NULL)
            {
                const_text = (char *) pg_malloc(const_dsize + 1);
                if (const_text == NULL)
                    continue;
                memcpy(const_text, query + const_loc, const_dsize);
                const_text[const_dsize] = '\0';
            }

            /* Determine table/column/dek for encryption.
             * ReqID:TECC4.0 - 纯 HE 列 dek_name 为 NULL 是合法状态，
             * 只需 enc_cache 有效；HE 分支按 he_add/he_mul DEK 名加密。 */
            if (enc_cache != NULL)
            {
                table_name = enc_cache->table_name;
                column_name = enc_cache->column_name;
                dek_name = enc_cache->dek_name;
                schema_name = enc_cache->schema_name;
            }
            else
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "stmt_process_funcall_constants: "
                                   "enc_cache or DEK name not available, "
                                   "skipping constant encryption for %s()",
                                   func_name);
                pg_free(const_text);
                continue;
            }

            encrypt_base_oid = enc_cache->original_type_oid;

            /*
             * ReqID:TECC4.0 - Skip constants whose type category (char vs
             * non-char) doesn't match the encrypted column.  This prevents
             * decode() result-text constants from being RND-normalised as
             * integers — which would fail and trigger heap corruption.
             */
            {
                /* ReqID:TECC4.0 - 数值文本形态的浮点字面量虽以 TEXTOID 承载，
                 * 类别上属数值，不参与 char 判定。但它只能按浮点/数值列加密：
                 * 整型列无法表示小数（按 int 归一化 "1.5" 会失败），字符列更
                 * 不适用。这两类列维持既有的"跳过加密"行为、交由服务端报错，
                 * 避免归一化失败路径触发堆损坏。 */
                bool col_is_float_or_numeric =
                    (enc_cache->original_type_oid == FLOAT4OID ||
                     enc_cache->original_type_oid == FLOAT8OID ||
                     enc_cache->original_type_oid == NUMERICOID);
                bool const_is_char = (!const_text_numeric &&
                                      (const_oid == TEXTOID ||
                                       const_oid == VARCHAROID ||
                                       const_oid == BPCHAROID));
                bool col_is_char = (enc_cache->original_type_oid == TEXTOID ||
                                    enc_cache->original_type_oid == VARCHAROID ||
                                    enc_cache->original_type_oid == BPCHAROID);

                if (const_text_numeric
                        ? !col_is_float_or_numeric
                        : (const_is_char != col_is_char))
                {
                    pg_free(const_text);
                    continue;
                }
            }

            /* ReqID:TECC4.0 - cast 常量的加密基类型改用 cast 目标类型，
             * 使加密字面量携带目标 enc 类型、经服务端隐式拓宽格与密态列
             * 比较；HE 分支不使用（同态仅整型，加密需按列类型）。 */
            if (cast_target_oid != InvalidOid)
                encrypt_base_oid = cast_target_oid;

            /*
             * ReqID:TECC4.0 - Encrypt with strategy based on function semantics.
             * COALESCE/GREATEST/LEAST: max encryption — constant may be returned
             * to an outer expression and face any operator (=, >, LIKE, ...).
             * Use RND + DET + OPE + SEARCH(for char types) to ensure the
             * encrypted constant can match regardless of outer context.
             * NULLIF: constant is only compared for equality, never returned.
             * Uses column's actual algorithm support (DET preferred, OPE fallback).
             */
            if (may_return_const)
            {
                /*
                 * ReqID:TECC4.0 - HE 列（he-add/he-mul）上的常量必须按列
                 * 算法加密：COALESCE 可能把常量返回给外层表达式，服务器端
                 * 要求与列密文同类型。传 DET/OPE 标志会让调度器走 SM4-DEK
                 * 取密钥路径，非对称 KEK 无该材料导致加密失败（ret=1000）。
                 */
                bool has_he_add = (enc_cache->encryption_type & 0x08) != 0;
                bool has_he_mul = (enc_cache->encryption_type & 0x10) != 0;

                if (has_he_add || has_he_mul)
                {
                    /* ReqID:TECC4.0 - 纯 HE 列 dek_name 为 NULL，传 HE DEK 名
                     * 供调度器加载 EC/FF 公钥（HE 加密按公钥进行，不需要
                     * SM4 DEK 材料）。 */
                    const char *he_dek = has_he_add ?
                        (enc_cache->he_add_dek_name != NULL ?
                         enc_cache->he_add_dek_name : dek_name) :
                        (enc_cache->he_mul_dek_name != NULL ?
                         enc_cache->he_mul_dek_name : dek_name);

                    encrypt_result = tecc_unified_encrypt_string_with_options(
                        statement_data->conn,
                        const_text,
                        table_name,
                        column_name,
                        NULL, /* default KEK */
                        he_dek,
                        enc_cache->original_type_oid,
                        enc_cache->original_type_mod,
                        false,          /* supports_det */
                        false,          /* supports_ope */
                        false,          /* supports_search */
                        has_he_add,     /* supports_he_add */
                        has_he_mul,     /* supports_he_mul */
                        NULL
                    );
                }
                else
                {
                    encrypt_result = tecc_unified_encrypt_string_with_options(
                        statement_data->conn,
                        const_text,
                        table_name,
                        column_name,
                        NULL, /* default KEK */
                        dek_name,
                        encrypt_base_oid,
                        enc_cache->original_type_mod,
                        true,           /* supports_det — for equality */
                        true,           /* supports_ope — for ordering */
                        is_char_type,    /* supports_search — for pattern matching */
                        false,          /* supports_he_add */
                        false,          /* supports_he_mul */
                        NULL
                    );
                }
            }
            else
            {
                /*
                 * NULLIF: equality comparison only.
                 * Must match the column's actual algorithm support:
                 * prefer DET if available, fall back to OPE (covers equality),
                 * error if neither is available.
                 */
                bool has_det = (enc_cache->encryption_type & 0x01) != 0;
                bool has_ope = (enc_cache->encryption_type & 0x02) != 0;

                if (!has_det && !has_ope)
                {
                    tecc_processor_set_error(statement_data,
                        TECC_PROCESSOR_ERROR,
                        "function \"%s\" requires DET or OPE encryption "
                        "on column \"%s\", but neither is available",
                        func_name, column_name);
                    pg_free(const_text);
                    continue;
                }

                encrypt_result = tecc_unified_encrypt_string_with_options(
                    statement_data->conn,
                    const_text,
                    table_name,
                    column_name,
                    NULL, /* default KEK */
                    dek_name,
                    encrypt_base_oid,
                    enc_cache->original_type_mod,
                    has_det,        /* supports_det — prefer if available */
                    !has_det,       /* supports_ope — fallback when DET missing */
                    false,           /* supports_search — not needed */
                    false,           /* supports_he_add */
                    false,           /* supports_he_mul */
                    NULL
                );
            }

            pg_free(const_text);
            const_text = NULL;

            if (encrypt_result == NULL ||
                encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "stmt_process_funcall_constants: encryption failed for const in %s()",
                                   func_name);
                continue;
            }

            /* Create raw value for SQL rewriting */
            raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
            if (raw_value != NULL)
            {
                raw_value->location = const_loc;
                raw_value->data_size = const_dsize;
                raw_value->state = TECC_RAW_VALUE_STATE_ENCRYPTED;

                /* Set column type for ::typename cast
                 * ReqID:TECC4.0 - 浮点字面量的 const_oid 是其"承载用"的 TEXTOID，
                 * 直接据此取 enc 类型名会让改写出的密文带上 ::text_enc，服务端随即报
                 * "GREATEST could not convert type text_enc to <列类型>"。
                 * 这类常量的语义类型是数值，改按列基类型（或 cast 目标类型）取名。 */
                enc_type_name = get_enc_type_name_from_base_oid(
                    const_text_numeric ? encrypt_base_oid : const_oid);
                tecc_raw_value_set_column_info(raw_value, column_name,
                                               enc_type_name, InvalidOid);
                if (table_name != NULL)
                {
                    tecc_raw_value_set_table_info(raw_value, schema_name, table_name);
                }

                /* Store encrypted hex value */
                raw_value->processed.encrypted_value =
                    tecc_processor_strdup(encrypt_result->hex_string);

                /* Add to rewrite list */
                if (statement_data->raw_values_list == NULL)
                    statement_data->raw_values_list =
                        tecc_raw_values_list_create(16);
                if (statement_data->raw_values_list != NULL)
                {
                    if (tecc_raw_values_list_add(
                            statement_data->raw_values_list, raw_value) < 0)
                        tecc_raw_value_destroy(raw_value);
                }
                statement_data->needs_rewrite = 1;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "stmt_process_funcall_constants: encrypted const in %s() at loc=%zu",
                                   func_name, const_loc);
            }
        }
    }
}

/*
 * ===========================================================================
 * ReqID:TECC4.0 - Aggregate function classification and validation
 *
 * Aggregate functions on encrypted columns are gated by column encryption
 * algorithm type:
 *   - MIN/MAX: require OPE (ciphertext order comparison)
 *   - COUNT/COUNT(DISTINCT): always supported (no value content needed)
 *   - array_agg: rejected (returns *_encarray OID, no decryption mapping)
 *   - SUM: supported on he-add int2/int4/int8 columns (server-side
 *     homomorphic accumulation, 设计 F5); rejected otherwise
 *   - AVG/string_agg: rejected (no homomorphic support)
 *
 * Only the 7 tracked aggregate names are validated; all other functions
 * (scalar, user-defined, json_agg, xmlagg, etc.) pass through untouched.
 * ===========================================================================
 */

/* Classification of tracked aggregate functions */
typedef enum TeccAggregateClass
{
	TECC_AGG_CLASS_NONE = 0,		/* not a tracked aggregate — pass through */
	TECC_AGG_CLASS_COUNT,			/* count — always supported (incl. DISTINCT) */
	TECC_AGG_CLASS_MINMAX,			/* min/max — requires OPE on encrypted cols */
	TECC_AGG_CLASS_SUM,				/* sum — requires HE_ADD on encrypted cols */
	TECC_AGG_CLASS_UNSUPPORTED		/* avg/string_agg/array_agg — always rejected */
} TeccAggregateClass;

/* Classification tables — case-insensitive matching via strcasecmp */
static const char *const stmt_count_aggregates[] = {"count", NULL};
static const char *const stmt_minmax_aggregates[] = {"min", "max", NULL};
static const char *const stmt_sum_aggregates[] = {"sum", NULL};
static const char *const stmt_unsupported_aggregates[] = {
	"avg", "string_agg", "array_agg", NULL
};

/*
 * Classify a function name into TeccAggregateClass.
 * Returns TECC_AGG_CLASS_NONE for untracked functions (pass through).
 */
static TeccAggregateClass
stmt_get_aggregate_class(const char *func_name)
{
	int i;

	if (func_name == NULL)
		return TECC_AGG_CLASS_NONE;

	for (i = 0; stmt_count_aggregates[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_count_aggregates[i]) == 0)
			return TECC_AGG_CLASS_COUNT;
	}
	for (i = 0; stmt_minmax_aggregates[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_minmax_aggregates[i]) == 0)
			return TECC_AGG_CLASS_MINMAX;
	}
	for (i = 0; stmt_sum_aggregates[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_sum_aggregates[i]) == 0)
			return TECC_AGG_CLASS_SUM;
	}
	for (i = 0; stmt_unsupported_aggregates[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_unsupported_aggregates[i]) == 0)
			return TECC_AGG_CLASS_UNSUPPORTED;
	}

	return TECC_AGG_CLASS_NONE;
}

/*
 * Validate an aggregate function call against the encryption algorithm
 * of each encrypted column argument.
 *
 * Returns true if the aggregate is valid or has no encrypted column args
 * (pass-through). Returns false if an error is set via
 * tecc_processor_set_error() and the caller should abort.
 *
 * Only validates the 6 tracked aggregate names (count, min, max, sum, avg,
 * string_agg, array_agg). All other functions pass through.
 */
bool
stmt_validate_aggregate_funcall(FuncCall *fc, TeccStatementData *statement_data)
{
	const char *func_name;
	TeccAggregateClass agg_class;
	ListCell *lc;

	if (fc == NULL || statement_data == NULL)
		return true;
	if (fc->funcname == NIL)
		return true;

	/* 1. Extract function name from the LAST element of funcname list
	 *    (any_name_list grammar: [catalog?, schema?, function]). */
	{
		Node *name_node = (Node *) llast(fc->funcname);
		if (name_node == NULL)
			return true;
		func_name = strVal((Value *) name_node);
		if (func_name == NULL)
			return true;
	}

	/* 2. Classify; untracked functions (scalars, user-defined, etc.) pass
	 *    through so existing behaviour is unchanged. */
	agg_class = stmt_get_aggregate_class(func_name);
	if (agg_class == TECC_AGG_CLASS_NONE)
		return true;

	/* 3. count(*) / zero-arg aggregates: no column args — always pass. */
	if (fc->args == NIL)
		return true;

	/* 4. Scan arguments for encrypted column references. */
	foreach(lc, fc->args)
	{
		Node *arg = (Node *) lc->data.ptr_value;

		/* Skip non-ColumnRef args (constants, expressions, etc.)
		 * e.g. string_agg(enc_col, ',') — the ',' separator Constant
		 * is not validated. */
		if (arg == NULL || nodeTag(arg) != FE_T_ColumnRef)
			continue;

		{
			ColumnRef *col_ref = (ColumnRef *) arg;
			char *col_name = NULL;
			char *qualifier = NULL;
			ListCell *elc;
			int nf;

			if (col_ref->fields == NIL)
				continue;

			nf = list_length(col_ref->fields);
			col_name = strVal((Value *) list_nth(col_ref->fields, nf - 1));
			if (nf >= 2)
				qualifier = strVal((Value *) list_nth(col_ref->fields, nf - 2));
			if (col_name == NULL)
				continue;

			/* 5. Match against statement_data->encrypted_columns
			 *    (populated from FROM clause in Pass 1). */
			foreach(elc, statement_data->encrypted_columns)
			{
				TeccColumnRefData *ec = (TeccColumnRefData *) elc->data.ptr_value;

				if (ec == NULL || ec->column_name[0] == '\0')
					continue;
				if (strcmp(ec->column_name, col_name) != 0)
					continue;
				/* If the column ref is qualified (t.col), check the
				 * qualifier against table_alias or table_name. */
				if (qualifier != NULL &&
					strcmp(ec->table_alias, qualifier) != 0 &&
					strcmp(ec->table_name, qualifier) != 0)
					continue;

				/* 6. Resolve full metadata for encryption_type bitmask.
				 *    Same pattern as stmt_process_funcall_constants. */
				{
					TeccColumnCacheItem *col_cache = NULL;

					if (statement_data->conn != NULL)
					{
						TeccState *s = tecc_get_state(statement_data->conn);
						const char *db = PQdb(statement_data->conn);
						const char *schema = (s != NULL) ?
							tecc_cache_get_guc_param(s, "current_schema") : NULL;

						if (s != NULL && db != NULL && schema != NULL &&
							ec->table_name[0] != '\0')
						{
							col_cache = tecc_cache_get_encrypted_column_by_name(
								s, db, schema, ec->table_name, col_name);
						}
					}

					if (col_cache == NULL)
						continue;	/* defensive: no metadata → don't false-error */

					/* ReqID:TECC4.0 - Window functions containing
					 * aggregates over encrypted columns are not
					 * supported.  The server-side window aggregate
					 * executor has no HE accumulation path; reject
					 * early with a clear message instead of a
					 * cryptic server error. */
					if (fc->over != NULL)
					{
						tecc_processor_set_error(statement_data,
							TECC_PROCESSOR_ERROR,
							"aggregate function \"%s\" inside window "
							"function is not supported on encrypted "
							"column \"%s\"",
							func_name, col_name);
						return false;
					}

					/* 7. Algorithm check per aggregate class. */
					switch (agg_class)
					{
						case TECC_AGG_CLASS_MINMAX:
							/* MIN/MAX require OPE for ciphertext comparison */
							if (!(col_cache->encryption_type & TECC_ENC_TYPE_OPE))
							{
								tecc_processor_set_error(statement_data,
									TECC_PROCESSOR_ERROR,
									"aggregate function \"%s\" requires OPE encryption, "
									"but column \"%s\" is not configured with OPE",
									func_name, col_name);
								return false;
							}
							break;

						case TECC_AGG_CLASS_SUM:
							/* SUM: supported on he-add int2/int4/int8
							 * columns via server-side homomorphic
							 * accumulation (设计 F5: EC-ElGamal 聚合
							 * 状态机); rejected on other encrypted
							 * columns. */
							if (!(col_cache->encryption_type & TECC_ENC_TYPE_HE_ADD) ||
								(col_cache->original_type_oid != INT2OID &&
								 col_cache->original_type_oid != INT4OID &&
								 col_cache->original_type_oid != INT8OID))
							{
								tecc_processor_set_error(statement_data,
									TECC_PROCESSOR_ERROR,
									"aggregate function \"%s\" is not supported on "
									"encrypted column \"%s\": SUM requires he-add "
									"encryption on int2/int4/int8 columns",
									func_name, col_name);
								return false;
							}
							break;

						case TECC_AGG_CLASS_UNSUPPORTED:
							/* AVG/string_agg/array_agg: rejected for any
							 * encrypted column (array_agg returns *_encarray
							 * OID which has no decryption mapping). */
							tecc_processor_set_error(statement_data,
								TECC_PROCESSOR_ERROR,
								"aggregate function \"%s\" is not supported on "
								"encrypted column \"%s\"",
								func_name, col_name);
							return false;

						case TECC_AGG_CLASS_COUNT:
						default:
							/* count: always supported */
							break;
					}
				}
				break;	/* column matched — stop scanning encrypted_columns */
			}
		}
	}

	/* No encrypted column args found, or all passed validation. */
	return true;
}

/*
 * ===========================================================================
 * ReqID:TECC4.0 - Scalar function validation
 *
 * Certain scalar functions on encrypted columns require specific algorithms
 * (greatest/least need OPE for ordering; decode needs DET or OPE for equality;
 * power needs HE_MUL on int2/int4/int8 — repeated FF-ElGamal multiplication
 * on the server).
 *
 * Classification tables — case-insensitive matching via strcasecmp.
 * ===========================================================================
 */

/* Scalar functions that require OPE (ordering comparison) */
static const char *const stmt_scalars_need_ope[] = {"greatest", "least", NULL};

/* Scalar functions that require DET or OPE (equality comparison) */
static const char *const stmt_scalars_need_det_or_ope[] = {"decode", NULL};

/* Scalar functions that require HE_MUL (rejected on columns without it) */
static const char *const stmt_scalars_need_he_mul[] = {"power", NULL};

/*
 * Classify a scalar function name for algorithm validation.
 * Returns 0=none (pass through), 1=needs OPE, 2=needs DET or OPE,
 * 3=needs HE_MUL (power).
 */
static int
stmt_get_scalar_class(const char *func_name)
{
	int i;

	if (func_name == NULL)
		return 0;

	for (i = 0; stmt_scalars_need_ope[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_scalars_need_ope[i]) == 0)
			return 1;	/* needs OPE */
	}
	for (i = 0; stmt_scalars_need_det_or_ope[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_scalars_need_det_or_ope[i]) == 0)
			return 2;	/* needs DET or OPE */
	}
	for (i = 0; stmt_scalars_need_he_mul[i] != NULL; i++)
	{
		if (strcasecmp(func_name, stmt_scalars_need_he_mul[i]) == 0)
			return 3;	/* needs HE_MUL (power) */
	}
	return 0;	/* pass through */
}

/*
 * Validate a (non-aggregate) function call against column encryption algorithms.
 *
 * Returns true if valid or pass-through; false if an error is set and the
 * caller should abort.
 */
bool
stmt_validate_scalar_funcall(FuncCall *fc, TeccStatementData *statement_data)
{
	const char *func_name;
	int scalar_class;
	ListCell *lc;

	if (fc == NULL || statement_data == NULL)
		return true;
	if (fc->funcname == NIL)
		return true;

	/* 1. Extract function name from LAST element of funcname list. */
	{
		Node *name_node = (Node *) llast(fc->funcname);
		if (name_node == NULL)
			return true;
		func_name = strVal((Value *) name_node);
		if (func_name == NULL)
			return true;
	}

	/* 2. Classify; untracked functions pass through. */
	scalar_class = stmt_get_scalar_class(func_name);
	if (scalar_class == 0)
		return true;

	/* 3. Zero-arg functions: no column args — always pass. */
	if (fc->args == NIL)
		return true;

	/* 4. Scan arguments for encrypted column references. */
	foreach(lc, fc->args)
	{
		Node *arg = (Node *) lc->data.ptr_value;

		if (arg == NULL || nodeTag(arg) != FE_T_ColumnRef)
			continue;

		{
			ColumnRef *col_ref = (ColumnRef *) arg;
			char *col_name = NULL;
			char *qualifier = NULL;
			ListCell *elc;
			int nf;

			if (col_ref->fields == NIL)
				continue;

			nf = list_length(col_ref->fields);
			col_name = strVal((Value *) list_nth(col_ref->fields, nf - 1));
			if (nf >= 2)
				qualifier = strVal((Value *) list_nth(col_ref->fields, nf - 2));
			if (col_name == NULL)
				continue;

			/* 5. Match against statement_data->encrypted_columns. */
			foreach(elc, statement_data->encrypted_columns)
			{
				TeccColumnRefData *ec = (TeccColumnRefData *) elc->data.ptr_value;

				if (ec == NULL || ec->column_name[0] == '\0')
					continue;
				if (strcmp(ec->column_name, col_name) != 0)
					continue;
				if (qualifier != NULL &&
					strcmp(ec->table_alias, qualifier) != 0 &&
					strcmp(ec->table_name, qualifier) != 0)
					continue;

				/* 6. Resolve full metadata for encryption_type bitmask. */
				{
					TeccColumnCacheItem *col_cache = NULL;

					if (statement_data->conn != NULL)
					{
						TeccState *s = tecc_get_state(statement_data->conn);
						const char *db = PQdb(statement_data->conn);
						const char *schema = (s != NULL) ?
							tecc_cache_get_guc_param(s, "current_schema") : NULL;

						if (s != NULL && db != NULL && schema != NULL &&
							ec->table_name[0] != '\0')
						{
							col_cache = tecc_cache_get_encrypted_column_by_name(
								s, db, schema, ec->table_name, col_name);
						}
					}

					if (col_cache == NULL)
						continue;

					/* needs OPE (greatest/least) */
					if (scalar_class == 1)
					{
						if (!(col_cache->encryption_type & TECC_ENC_TYPE_OPE))
						{
							tecc_processor_set_error(statement_data,
								TECC_PROCESSOR_ERROR,
								"function \"%s\" requires OPE encryption, "
								"but column \"%s\" is not configured with OPE",
								func_name, col_name);
							return false;
						}
					}

					/* needs DET or OPE (decode) */
					if (scalar_class == 2)
					{
						if (!(col_cache->encryption_type &
						      (TECC_ENC_TYPE_DET | TECC_ENC_TYPE_OPE)))
						{
							tecc_processor_set_error(statement_data,
								TECC_PROCESSOR_ERROR,
								"function \"%s\" requires DET or OPE encryption, "
								"but column \"%s\" is not configured with either",
								func_name, col_name);
							return false;
						}
					}

					/* needs HE_MUL (power): supported via repeated
					 * FF-ElGamal multiplication on he-mul int2/int4/
					 * int8 columns. */
					if (scalar_class == 3)
					{
						if (!(col_cache->encryption_type & TECC_ENC_TYPE_HE_MUL) ||
							(col_cache->original_type_oid != INT2OID &&
							 col_cache->original_type_oid != INT4OID &&
							 col_cache->original_type_oid != INT8OID))
						{
							tecc_processor_set_error(statement_data,
								TECC_PROCESSOR_ERROR,
								"function \"%s\" is not supported on "
								"encrypted column \"%s\": power requires "
								"he-mul encryption on int2/int4/int8 columns",
								func_name, col_name);
							return false;
						}
					}
				}
				break;	/* column matched — stop scanning encrypted_columns */
			}
		}
	}

	/* No encrypted column args found, or all passed validation. */
	return true;
}

/* ReqID:TECC4.0 - Bug 9151: 视图定义 ORDER BY 加密列 OPE 校验。
 * 普通 SELECT 的 ORDER BY 由后端 cipherdata_operators 在执行时校验，
 * 但 CREATE VIEW 只存储定义不执行 ORDER BY，后端校验被绕过。
 * 此函数在视图创建时对 ORDER BY 中的加密列做前端 OPE 校验。 */
static TeccStmtProcessorResult
tecc_validate_view_order_by_ope(const SelectStmt *select_stmt,
                                const TeccStatementData *statement_data)
{
    ListCell *lc;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                         "Bug9151: ORDER BY OPE check: sortClause=%p, encrypted_columns=%p",
                         (void *) select_stmt->sortClause,
                         (void *) statement_data->encrypted_columns);

    if (select_stmt == NULL || select_stmt->sortClause == NIL)
    {
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    for (lc = list_head(select_stmt->sortClause); lc != NULL; lc = lnext(lc))
    {
        SortBy *sortby = (SortBy *) lfirst(lc);
        ColumnRef *col_ref;
        TeccColumnCacheItem *col_item = NULL;
        TeccState *state;
        const char *db_name;
        const char *sort_col_name = NULL;
        const char *sort_table_name = NULL;

        if (sortby == NULL || sortby->node == NULL)
            continue;

        /* 仅处理简单列引用（col_det），跳过表达式/函数等复杂排序项 */
        if (nodeTag(sortby->node) != FE_T_ColumnRef)
            continue;

        col_ref = (ColumnRef *) sortby->node;

        /* 用 encrypted_columns 列表查找排序列是否为加密列 */
        if (statement_data->encrypted_columns == NIL)
            continue;

        {
            ListCell *ed_lc;
            int ed_match_count = 0;

            /* 从 ColumnRef 提取列名。前端 parser 的 ColumnRef fields 使用
             * 后端 T_String (Value) 节点，而非 FE_T_String。 */
            if (col_ref->fields != NIL && col_ref->fields->length == 1)
            {
                Node *field = (Node *) lfirst(list_head(col_ref->fields));
                if (field != NULL)
                    sort_col_name = strVal((Value *) field);
            }
            else if (col_ref->fields != NIL && col_ref->fields->length == 2)
            {
                /* table.column 形式 */
                sort_table_name = strVal((Value *) lfirst(list_head(col_ref->fields)));
                sort_col_name = strVal((Value *) lfirst(list_nth_cell(col_ref->fields, 1)));
            }

            if (sort_col_name == NULL)
            {
                continue;
            }

            foreach(ed_lc, statement_data->encrypted_columns)
            {
                TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *) lfirst(ed_lc);
                if (ed == NULL || ed->column_name[0] == '\0')
                    continue;
                if (strcmp(ed->column_name, sort_col_name) != 0)
                    continue;
                if (sort_table_name != NULL &&
                    strcmp(ed->table_alias, sort_table_name) != 0 &&
                    strcmp(ed->table_name, sort_table_name) != 0)
                    continue;
                ed_match_count++;
            }

            if (ed_match_count == 0)
                continue;  /* 非加密列，跳过 */

            /* 查找对应的 TeccProcessorColumnRef 获取表名 */
            foreach(ed_lc, statement_data->encrypted_columns)
            {
                TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *) lfirst(ed_lc);
                if (ed == NULL || ed->column_name[0] == '\0')
                    continue;
                if (strcmp(ed->column_name, sort_col_name) != 0)
                    continue;
                if (sort_table_name != NULL &&
                    strcmp(ed->table_alias, sort_table_name) != 0 &&
                    strcmp(ed->table_name, sort_table_name) != 0)
                    continue;
                sort_table_name = ed->table_name;
                break;
            }
        }

        /* 通过缓存查询加密列元数据，检查 OPE 支持 */
        state = (statement_data->conn != NULL) ? tecc_get_state(statement_data->conn) : NULL;
        if (state == NULL || sort_table_name == NULL)
            continue;

        db_name = PQdb(statement_data->conn);
        if (db_name == NULL || db_name[0] == '\0')
            db_name = "postgres";

        col_item = tecc_cache_get_encrypted_column_by_name(
            state, db_name, "public", sort_table_name, sort_col_name);

        if (col_item != NULL && !(col_item->encryption_type & 0x02))
        {
            /* 0x02 = HG_ENCRYPTION_TYPE_OPE — 排序列不支持 OPE */
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                "ORDER BY column \"%s\" requires OPE (order-preserving encryption) "
                "but the column is not configured with OPE. "
                "Please include 'ope' in support_algo.",
                sort_col_name);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process CREATE VIEW statement - ReqID:TECC4.0 密态视图支持
 *
 * 视图定义查询按 SELECT 处理：复用 tecc_stmt_processor_process_select
 * 识别加密列、加密 WHERE/HAVING/ON 中的密态列常量、校验表达式
 * （报错行为与直接查询基表一致）。
 *
 * 位置机制：scanner 的 location 为相对完整 SQL 的字节偏移
 * （tecc_scanner.l YY_USER_ACTION），因此常量加密后 values_processor
 * 直接在 statement_data->query（完整 CREATE VIEW 语句）中替换，
 * 生成的 rewritten_query 即改写后的完整 CREATE VIEW 语句。
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_view(const ViewStmt *view_stmt,
                                 TeccStatementData *statement_data)
{
    TeccStmtProcessorResult result;

    if (view_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    statement_data->stmt_type = TECC_STMT_TYPE_CREATE_VIEW;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                         "Processing CREATE VIEW statement");

    /* ReqID:TECC4.0 - 临时视图拒绝：与临时加密表策略一致，
     * 临时视图引用加密列时明确报错。临时视图 oid 会话私有，
     * 与数据获取器池连接的跨会话元数据查询不兼容。 */
    if (view_stmt->temp)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                             "CREATE TEMP VIEW detected, checking for encrypted columns");
    }

    if (view_stmt->query != NULL && nodeTag(view_stmt->query) == FE_T_SelectStmt)
    {
        result = tecc_stmt_processor_process_select((SelectStmt *)view_stmt->query,
                                                   statement_data);
    }
    else if (view_stmt->query != NULL && nodeTag(view_stmt->query) == FE_T_SetOp)
    {
        /* ReqID:TECC4.0 - UNION ALL/INTERSECT/EXCEPT 视图定义：
         * 递归处理联合操作的左右分支，每个分支都是 SelectStmt，
         * 分别识别加密列并加密 WHERE 条件中的常量。 */
        SetOperationStmt *setop = (SetOperationStmt *) view_stmt->query;
        TeccStmtProcessorResult lresult = TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS;
        TeccStmtProcessorResult rresult = TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                             "Processing CREATE VIEW with SetOperation (UNION/INTERSECT/EXCEPT)");

        if (setop->larg != NULL && nodeTag(setop->larg) == FE_T_SelectStmt)
        {
            lresult = tecc_stmt_processor_process_select((SelectStmt *)setop->larg,
                                                         statement_data);
        }
        if (setop->rarg != NULL && nodeTag(setop->rarg) == FE_T_SelectStmt)
        {
            rresult = tecc_stmt_processor_process_select((SelectStmt *)setop->rarg,
                                                         statement_data);
        }

        result = (lresult == TECC_STMT_PROCESSOR_SUCCESS ||
                  rresult == TECC_STMT_PROCESSOR_SUCCESS)
                 ? TECC_STMT_PROCESSOR_SUCCESS
                 : TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    }
    else
    {
        return TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    }

    /* ReqID:TECC4.0 - Bug 9151: 视图定义 ORDER BY 加密列 OPE 校验。
     * 普通 SELECT 的 ORDER BY 由后端在执行时校验，但视图创建只存储
     * 定义不执行 ORDER BY，后端校验被绕过。此处前端校验排序列的 OPE。 */
    if (result != TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
        statement_data->encrypted_columns != NIL)
    {
        if (view_stmt->query != NULL && nodeTag(view_stmt->query) == FE_T_SelectStmt)
        {
            TeccStmtProcessorResult oby_result;

            oby_result = tecc_validate_view_order_by_ope(
                (const SelectStmt *) view_stmt->query, statement_data);
            if (oby_result != TECC_STMT_PROCESSOR_SUCCESS)
                return oby_result;
        }
    }

    /* ReqID:TECC4.0 - 临时视图拒绝：SELECT 处理完成后检查是否引用了加密列。
     * 使用 needs_rewrite 判断（仅当 WHERE/表达式中实际加密了密态列常量时置位），
     * 而非 encrypted_columns（它包含 FROM 表的全部加密列，即使 SELECT 未引用）。 */
    if (view_stmt->temp && statement_data->needs_rewrite)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
            "TECC does not support temporary views with encrypted columns");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    return result;
}

/* ReqID:TECC4.0 - 递归收集目标列表表达式（聚合/运算）中的加密列，保存其
 * schema/table/column 名到连接状态（pending_expr_col_*），供结果解密
 * （tecc_processor_run_post_query）恢复列元数据。每个目标项只记录第一个
 * 加密列（结果列对应一个底层列语义）；返回 true 表示已记录。
 *
 * default_table/default_schema/default_alias: FROM 单表名、其 schema 与别名，
 * 用于补全单名列（sum(i4_a)）expand 后缺失的表名/schema。结果为聚合/表达式
 * 时 PG 协议不返回 table_oid，必须按名回查缓存恢复列元数据，schema 错了就
 * 查不到；列引用写的是别名（min(x.c1) FROM s1.m x）时别名也不是真实表名。 */
static bool
tecc_collect_expr_target_column(Node *expr, const TeccStatementData *statement_data,
                                 const char *default_table, const char *default_schema,
                                 const char *default_alias)
{
    TeccState *state;
    bool found = false;

    if (expr == NULL || statement_data == NULL || statement_data->conn == NULL)
        return false;

    state = tecc_get_state(statement_data->conn);
    if (state == NULL || state->pending_expr_col_count >= TECC_MAX_EXPR_RESULT_COLS)
        return false;

    switch (nodeTag(expr))
    {
        case FE_T_ColumnRef:
            {
                ColumnRef *col_ref = (ColumnRef *) expr;
                TeccColumnRefData *tecc_col = NULL;

                if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data) &&
                    tecc_col != NULL)
                {
                    /* 单名列（sum(i4_a)）expand 时 table_name 为空，is_encrypted 不可靠。
                     * 用 FROM 单表名补全后按名查缓存确认加密。 */
                if (tecc_col->table_name[0] == '\0' && default_table != NULL)
                    strlcpy(tecc_col->table_name, default_table, NAMEDATALEN);
                if (tecc_col->table_name[0] != '\0' && tecc_col->column_name[0] != '\0')
                    {
                        /* ReqID:TECC4.0 - schema 解析优先级：
                         *   1. 列引用自带限定名（min(s1.m.c1)）；
                         *   2. FROM 单表项的 schema（min(c1) FROM s1.m）—— 缺失
                         *      时按 search_path 解析表名。原实现硬编码 "public"，
                         *      对非 public schema 的表一律查不到元数据，导致聚合
                         *      结果列拿不到列元数据、解密失败（HSM 下还会因
                         *      hsm_kms_type 残留报 "HSM not available"）。
                         *   3. search_path 解析（未限定 FROM 表名）。 */
                        const char *sname = NULL;
                        const char *db_name = PQdb(statement_data->conn);
                        TeccColumnCacheItem *ci;

                        if (tecc_col->schema_name[0] != '\0')
                            sname = tecc_col->schema_name;
                        else if (default_schema != NULL && default_schema[0] != '\0')
                            sname = default_schema;
                        else
                            sname = tecc_stmt_resolve_table_schema(statement_data, NULL,
                                                                   tecc_col->table_name);

                        if (db_name == NULL || db_name[0] == '\0')
                            db_name = "postgres";
                        ci = tecc_cache_get_encrypted_column_by_name(state, db_name, sname,
                                                                 tecc_col->table_name,
                                                                 tecc_col->column_name);
                        /* ReqID:TECC4.0 - 列引用可能写的是 FROM 表别名
                         * （min(x.c1) FROM s1.m x）：别名不是真实表名，查不到时
                         * 若确认与 FROM 单表别名相同，则按真实表名重试，并把真实
                         * 表名写回，供结果处理阶段恢复列元数据。别名不同（多表
                         * FROM 里引用其它表）不重试，避免把别的表的元数据套上来。 */
                        if (ci == NULL &&
                            default_alias != NULL && default_alias[0] != '\0' &&
                            strcmp(tecc_col->table_name, default_alias) == 0 &&
                            default_table != NULL && default_table[0] != '\0')
                        {
                            const char *real_sname;
                            TeccColumnCacheItem *real_ci;

                            if (default_schema != NULL && default_schema[0] != '\0')
                                real_sname = default_schema;
                            else
                                real_sname = tecc_stmt_resolve_table_schema(statement_data,
                                                                           NULL,
                                                                           default_table);
                            real_ci = tecc_cache_get_encrypted_column_by_name(state, db_name,
                                                                      real_sname, default_table,
                                                                      tecc_col->column_name);
                            if (real_ci != NULL)
                            {
                                ci = real_ci;
                                sname = real_sname;
                                strlcpy(tecc_col->table_name, default_table, NAMEDATALEN);
                            }
                        }
                        if (ci != NULL)
                        {
                            strlcpy(state->pending_expr_col_schemas[state->pending_expr_col_count],
                                    sname, NAMEDATALEN);
                            strlcpy(state->pending_expr_col_tables[state->pending_expr_col_count],
                                    tecc_col->table_name, NAMEDATALEN);
                            strlcpy(state->pending_expr_col_columns[state->pending_expr_col_count],
                                    tecc_col->column_name, NAMEDATALEN);
							state->pending_expr_col_raw_hex[state->pending_expr_col_count] = false;
							state->pending_expr_col_field_indexes[state->pending_expr_col_count] = -1;
							state->pending_expr_col_raw_algorithms[state->pending_expr_col_count][0] = '\0';
                    state->pending_expr_col_count++;
                    found = true;
                }
            }
        }
        if (tecc_col != NULL)
            tecc_column_ref_data_destroy(tecc_col);
    }break;

        case FE_T_FuncCall:
            {
                FuncCall *func_call = (FuncCall *) expr;
                const char *func_name;

                /* ReqID:TECC4.0 - ARRAY[...] 构造器（函数名 'array'）的结果是
                 * *_enc[] 数组类型且元素为多个加密列。数组结果无单列元数据
                 * 可关联，不纳入 pending 单列表达式机制——否则 PG 协议对数组
                 * 结果返回 InvalidOid 会错误匹配 pending 首项。跳过不收集
                 * （结果保持密文输出）。array_agg 有明确的元素列，纳入
                 * pending 收集并在结果处理时逐元素解密（需求 4.4.1.4）。 */
                if (func_call->funcname != NIL)
                {
                    func_name = strVal(linitial(func_call->funcname));
                    if (func_name != NULL && pg_strcasecmp(func_name, "array") == 0)
                        break;
                }

                if (func_call->args != NULL)
                {
                    ListCell *lc;

                    foreach(lc, func_call->args)
                    {
                        if (tecc_collect_expr_target_column((Node *) lc->data.ptr_value,
                                                            statement_data, default_table,
                                                            default_schema, default_alias))
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }
            break;

        case FE_T_A_Expr:
            {
                A_Expr *aexpr = (A_Expr *) expr;

                if (aexpr->lexpr != NULL &&
                    tecc_collect_expr_target_column(aexpr->lexpr, statement_data, default_table,
                                                    default_schema, default_alias))
                    found = true;
                if (!found && aexpr->rexpr != NULL)
                    found = tecc_collect_expr_target_column(aexpr->rexpr, statement_data,
                                                            default_table, default_schema,
                                                            default_alias);
            }
            break;

        case FE_T_TypeCast:
            {
                TypeCast *tc = (TypeCast *) expr;

                if (tc->arg != NULL)
                    found = tecc_collect_expr_target_column(tc->arg, statement_data, default_table,
                                                            default_schema, default_alias);
            }
            break;

        /* ReqID:TECC4.0 - CaseExpr：递归收集 THEN/ELSE 分支中的加密列，
         * 供结果解密恢复列元数据（CASE 结果列在 PG 协议中无 table_oid/attnum，
         * 需按底层分支列名查缓存）。 */
        case FE_T_CaseExpr:
            {
                CaseExpr *ce = (CaseExpr *) expr;
                ListCell *lc;

                if (ce->args != NIL)
                {
                    foreach(lc, ce->args)
                    {
                        CaseWhen *cw = (CaseWhen *) lfirst(lc);
                        if (cw != NULL && cw->result != NULL &&
                            tecc_collect_expr_target_column(cw->result,
                                                            statement_data,
                                                            default_table,
                                                            default_schema,
                                                            default_alias))
                        {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found && ce->defresult != NULL)
                    found = tecc_collect_expr_target_column(ce->defresult,
                                                            statement_data,
                                                            default_table,
                                                            default_schema,
                                                            default_alias);
            }
            break;

        default:
            break;
    }

    return found;
}

/* ReqID:TECC4.0 - 收集子查询输出列的底层加密列到主查询 pending。
 * 子查询场景（SELECT sum(v) FROM (SELECT col AS v FROM t) s）：外层列 v 无
 * table_oid/attnum 且无法用外层 FROM 表名补全（外层 FROM 是子查询）。递归
 * 处理子查询后，遍历其 targetList，把每个输出列的底层加密列（用子查询
 * FROM 单表名补全）记录到主查询 pending，供结果解密恢复列元数据。
 *
 * Bug 9152：当子查询使用 SELECT * 时，解析器将其表示为 ColumnRef 而非
 * val==NULL，无法通过表达式分析提取加密列。此时检测 targetList 处理后
 * 是否有新增加密列，若无则从 FROM 子句的单表加载全部加密列元数据。 */
static void
tecc_collect_subselect_output_columns(const SelectStmt *subselect,
                                      TeccStatementData *statement_data)
{
    const char *sub_from_table = NULL;
    const char *sub_from_schema = NULL;
    const char *sub_from_alias = NULL;
    ListCell *tlc;
    int cols_before;

    if (subselect == NULL || statement_data == NULL)
        return;

    /* 提取子查询 FROM 单表名/schema/别名（用于补全子查询内单名列） */
    if (subselect->fromClause != NULL && subselect->fromClause->head != NULL)
    {
        Node *sub_from = (Node *) subselect->fromClause->head->data.ptr_value;

        if (sub_from != NULL && nodeTag(sub_from) == FE_T_RangeVar)
        {
            RangeVar *sub_rv = (RangeVar *) sub_from;

            sub_from_table = sub_rv->relname;
            sub_from_schema = sub_rv->schemaname;
            if (sub_rv->alias != NULL)
                sub_from_alias = sub_rv->alias->aliasname;
        }
    }

    cols_before = (statement_data->encrypted_columns != NIL)
                  ? list_length(statement_data->encrypted_columns) : 0;

    /* 遍历子查询输出列，收集底层加密列到主查询 pending */
    if (subselect->targetList != NULL)
    {
        foreach(tlc, subselect->targetList)
        {
            ResTarget *sub_rt = (ResTarget *) lfirst(tlc);

            if (sub_rt != NULL && sub_rt->val != NULL)
                tecc_collect_expr_target_column(sub_rt->val, statement_data,
                                                sub_from_table, sub_from_schema,
                                                sub_from_alias);
        }
    }

    /* ReqID:TECC4.0 - Bug 9152: 如果遍历 targetList 没有新增加密列
     * （例如 SELECT * / ColumnRef 通配符），但 FROM 子句有简单单表，
     * 则直接从表元数据加载全部加密列。 */
    if (((statement_data->encrypted_columns != NIL)
         ? list_length(statement_data->encrypted_columns) : 0) == cols_before
        && subselect->fromClause != NULL
        && subselect->fromClause->head != NULL)
    {
        Node *sub_from = (Node *) subselect->fromClause->head->data.ptr_value;

        if (sub_from != NULL && nodeTag(sub_from) == FE_T_RangeVar)
        {
            RangeVar *rv = (RangeVar *) sub_from;
            TeccColumnCacheItem **table_cols = NULL;
            int table_col_count = 0;

            stmt_load_table_metadata(rv, statement_data,
                                     &table_cols, &table_col_count);
            if (table_cols != NULL && table_col_count > 0)
            {
                int ci;

                for (ci = 0; ci < table_col_count; ci++)
                {
                    if (table_cols[ci] != NULL)
                    {
                        TeccColumnRefData *cd = tecc_column_ref_data_create();

                        if (cd != NULL)
                        {
                            ListCell *ec_lc;
                            bool      duplicate = false;

                            tecc_column_ref_data_init(cd, NULL,
                                                      rv->schemaname,
                                                      rv->relname,
                                                      table_cols[ci]->column_name);
                            cd->is_encrypted = true;

                            /* ReqID:TECC4.0 - 该列可能已由子查询传播加入，
                             * 避免重复条目导致外层未限定列名解析歧义。 */
                            foreach(ec_lc, statement_data->encrypted_columns)
                            {
                                if (tecc_column_ref_data_equal(
                                        cd, (TeccColumnRefData *) lfirst(ec_lc)))
                                {
                                    duplicate = true;
                                    break;
                                }
                            }
                            if (duplicate)
                            {
                                tecc_column_ref_data_destroy(cd);
                                continue;
                            }

                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, cd);
                        }
                    }
                }
                pg_free(table_cols);
            }
        }
    }
}

/* Begin - ReqID:TECC4.0 */
/* Register a diagnostic column projection in the same replacement list as
 * WHERE constants.  Replacing only the AST leaves the original function in
 * the SQL sent to the backend, where this frontend helper does not exist. */
static bool
stmt_rewrite_decrypt_raw(FuncCall *call, TeccStatementData *statement_data)
{
    ColumnRef *column;
    ListCell *cell;
    PQExpBufferData sql;
    TeccRawValue *replacement;
    Node *source;

    if (list_length(call->args) != 2 || call->agg_star ||
        call->agg_distinct || call->agg_order != NIL ||
        call->agg_filter != NULL || call->over != NULL || call->func_variadic)
        goto invalid_call;

    source = (Node *) linitial(call->args);
    if (source == NULL || nodeTag(source) != FE_T_ColumnRef)
        goto invalid_call;
    column = (ColumnRef *) source;
    if (column->fields == NIL || statement_data->query == NULL ||
        call->location < 0 || call->end_location <= call->location ||
        (size_t) call->end_location > strlen(statement_data->query))
        goto invalid_call;

    initPQExpBuffer(&sql);
    foreach(cell, column->fields)
    {
        Node *part = (Node *) lfirst(cell);
        char *part_name;
        char *quoted;

        /* Modify - ReqID:TECC4.0
         * ColumnRef fields produced by any_name_list are FE_T_Const nodes;
         * strVal() also keeps compatibility with FE_T_String fields. */
        part_name = strVal((Value *) part);
        if (part_name == NULL)
        {
            termPQExpBuffer(&sql);
            goto invalid_call;
        }
        quoted = PQescapeIdentifier(statement_data->conn,
                                    part_name, strlen(part_name));
        if (quoted == NULL)
        {
            termPQExpBuffer(&sql);
            goto invalid_call;
        }
        if (cell != column->fields->head)
            appendPQExpBufferChar(&sql, '.');
        appendPQExpBufferStr(&sql, quoted);
        PQfreemem(quoted);
    }
    if (PQExpBufferBroken(&sql))
    {
        termPQExpBuffer(&sql);
        goto invalid_call;
    }

    replacement = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
    if (replacement == NULL)
    {
        termPQExpBuffer(&sql);
        goto invalid_call;
    }
    replacement->state = TECC_RAW_VALUE_STATE_ENCRYPTED;
    replacement->is_sql_expression = true;
    replacement->location = (size_t) call->location;
    replacement->data_size = (size_t) (call->end_location - call->location);
    replacement->processed.encrypted_value = pg_strdup(sql.data);
    termPQExpBuffer(&sql);
    if (statement_data->raw_values_list == NULL)
        statement_data->raw_values_list = tecc_raw_values_list_create(4);
    if (statement_data->raw_values_list == NULL ||
        tecc_raw_values_list_add(statement_data->raw_values_list, replacement) < 0)
    {
        tecc_raw_value_destroy(replacement);
        goto invalid_call;
    }
    statement_data->needs_rewrite = 1;
    return true;

invalid_call:
    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
        "tecc_decrypt_raw requires a column and a constant algorithm name");
    return false;
}
/* End - ReqID:TECC4.0 */

/* Begin - Bug 9323 - 嵌套子查询 FROM 项的统一处理 */

/*
 * Bug 9323 - 把子查询/CTE 输出列的重命名传播到父语句。
 *
 * 子查询可以用别的名字把加密列暴露给外层：
 *   SELECT name AS n FROM t_a           -> ResTarget->name
 *   WITH cte(n) AS (SELECT name ...)    -> CommonTableExpr->aliascolnames
 *   FROM (SELECT name FROM t_a) s(n)    -> Alias->colnames
 * 外层只能引用重命名后的名字。若传播到父语句的条目只带底层列名，外层
 * "s.n = 'x'" 匹配不到加密列，常量按明文下发，服务端报 invalid hex string。
 * 这里按输出位置把暴露名记到 column_alias，供外层按引用名匹配。
 */
static void
stmt_apply_sub_output_aliases(TeccStatementData *statement_data,
                              const SelectStmt *subselect,
                              const char *sub_alias,
                              const List *aliascolnames)
{
    ListCell *tlc;
    int       position = 0;

    if (statement_data == NULL || subselect == NULL ||
        subselect->targetList == NIL)
        return;

    foreach(tlc, subselect->targetList)
    {
        ResTarget  *rt = (ResTarget *) lfirst(tlc);
        const char *ref_name = NULL;
        const char *exposed_name = NULL;
        ListCell   *lc;

        position++;

        /* 只处理简单列引用：表达式输出（函数结果等）没有可对齐的加密列 */
        if (rt == NULL || rt->val == NULL ||
            nodeTag(rt->val) != FE_T_ColumnRef)
            continue;

        {
            ColumnRef *cref = (ColumnRef *) rt->val;

            if (cref->fields == NIL)
                continue;
            /* SELECT * 的 "*" 占位由 tecc_collect_subselect_output_columns
             * 的兜底路径处理，这里拿不到列名，跳过。 */
            ref_name = strVal((Value *) llast(cref->fields));
        }
        if (ref_name == NULL)
            continue;

        /* 暴露名：列别名列表优先于目标列 AS 别名，与 PostgreSQL 一致 */
        if (aliascolnames != NIL && position <= list_length(aliascolnames))
            exposed_name = strVal((Value *) list_nth(aliascolnames, position - 1));
        else if (rt->name != NULL)
            exposed_name = rt->name;

        if (exposed_name == NULL || strcmp(exposed_name, ref_name) == 0)
            continue;

        foreach(lc, statement_data->encrypted_columns)
        {
            TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc);
            const char        *exposed_here;

            if (ed == NULL || ed->column_name[0] == '\0')
                continue;
            /* 只改属于本子查询的条目（已按子查询别名打标） */
            if (sub_alias != NULL && sub_alias[0] != '\0' &&
                strcmp(ed->table_alias, sub_alias) != 0)
                continue;

            exposed_here = tecc_column_ref_data_get_exposed_name(ed);
            if (exposed_here == NULL || strcmp(exposed_here, ref_name) != 0)
                continue;

            tecc_column_ref_data_set_column_alias(ed, exposed_name);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                "Bug 9323: expose encrypted column %s.%s as \"%s\" to parent query",
                ed->table_name, ed->column_name, exposed_name);
        }
    }
}

/*
 * Bug 9323 - 处理一个子查询节点（普通 SELECT 或集合操作），把它的改写状态、
 * 加密常量与加密列传播到父语句，并应用输出列重命名。
 * 集合操作（UNION/INTERSECT/EXCEPT）没有自己的 targetList/WHERE，递归处理
 * 左右分支，别名与列别名列表对每个分支同样生效。
 */
static void
stmt_process_subquery_node(const Node *node, const char *sub_alias,
                           const List *aliascolnames,
                           TeccStatementData *statement_data)
{
    if (node == NULL || statement_data == NULL)
        return;

    if (nodeTag(node) == FE_T_SetOp)
    {
        const SetOperationStmt *setop = (const SetOperationStmt *) node;

        stmt_process_subquery_node(setop->larg, sub_alias, aliascolnames,
                                   statement_data);
        stmt_process_subquery_node(setop->rarg, sub_alias, aliascolnames,
                                   statement_data);
        return;
    }

    if (nodeTag(node) != FE_T_SelectStmt)
        return;

    {
        TeccStatementData      sub_data;
        TeccStmtProcessorResult sub_result;

        memset(&sub_data, 0, sizeof(TeccStatementData));
        sub_data.conn = statement_data->conn;

        sub_result = tecc_stmt_processor_process_select((const SelectStmt *) node,
                                                        &sub_data);
        if (sub_result != TECC_STMT_PROCESSOR_SUCCESS)
            tecc_processor_debug(TECC_LOG_ERROR,
                                 "Failed to process FROM RangeSubselect: %d",
                                 sub_result);

        if (sub_data.needs_rewrite)
            statement_data->needs_rewrite = 1;

        /* 子查询内 WHERE 的加密常量必须写到父语句的改写列表，否则随父语句
         * 一起以明文下发。 */
        stmt_propagate_sub_rewrite_state(statement_data, &sub_data, sub_alias);
        /* 外层 FROM 项是子查询而非表，无法按表名加载元数据 */
        tecc_collect_subselect_output_columns((const SelectStmt *) node,
                                              statement_data);
        stmt_apply_sub_output_aliases(statement_data,
                                      (const SelectStmt *) node,
                                      sub_alias, aliascolnames);
    }
}

/*
 * Bug 9323 - 递归处理 FROM 项里出现的子查询：JOIN 的一侧、嵌套 JOIN、
 * 以及 FROM 项本身就是子查询/集合操作的情形。原实现只在 FROM 顶层直接是
 * RangeSubselect 时处理子查询，JOIN 一侧的子查询整棵子树都被跳过——
 * 其中的密态列不传播、外层 "s.col" 的密态常量以明文下发。
 */
static void
stmt_process_from_subselect_item(Node *item, TeccStatementData *statement_data)
{
    if (item == NULL || statement_data == NULL)
        return;

    if (nodeTag(item) == FE_T_RangeSubselect)
    {
        RangeSubselect *rss = (RangeSubselect *) item;
        const char     *sub_alias = (rss->alias != NULL)
                                    ? rss->alias->aliasname : NULL;
        const List     *colnames = (rss->alias != NULL)
                                   ? rss->alias->colnames : NIL;

        stmt_process_subquery_node(rss->subquery, sub_alias, colnames,
                                   statement_data);
    }
    else if (nodeTag(item) == FE_T_JoinExpr)
    {
        JoinExpr *join = (JoinExpr *) item;

        stmt_process_from_subselect_item(join->larg, statement_data);
        stmt_process_from_subselect_item(join->rarg, statement_data);
    }
    else if (nodeTag(item) == FE_T_SelectStmt || nodeTag(item) == FE_T_SetOp)
    {
        stmt_process_subquery_node(item, NULL, NIL, statement_data);
    }
}

/* End - Bug 9323 */

/*
 * Process SELECT statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_select(const SelectStmt *select_stmt,
                                   TeccStatementData *statement_data)
{
    ListCell *lc;
    int target_count = 0;
    int from_count = 0;
    TeccWhereClauseProcessorResult where_result;
    ResTarget *res_target = NULL;
    /* ReqID:TECC4.0 - FROM 单表名/schema：目标列表单名列（sum(i4_a)）expand 时
     * table_name 为空，用 FROM 表名补全以确认加密状态；schema 用于按名回查
     * 列元数据（PG 协议对表达式结果列不返回 table_oid，schema 错了就查不到）。 */
    const char *from_table = NULL;
    const char *from_schema = NULL;
    const char *from_alias = NULL;

    if (select_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - 提取 FROM 单表名/schema/别名（仅 RangeVar 直接来源） */
    if (select_stmt->fromClause != NULL && select_stmt->fromClause->head != NULL)
    {
        Node *from_item = (Node *) select_stmt->fromClause->head->data.ptr_value;

        if (from_item != NULL && nodeTag(from_item) == FE_T_RangeVar)
        {
            RangeVar *from_rv = (RangeVar *) from_item;

            from_table = from_rv->relname;
            from_schema = from_rv->schemaname;
            if (from_rv->alias != NULL)
                from_alias = from_rv->alias->aliasname;
        }
    }

    /* ReqID:TECC4.0 - Set stmt_type for downstream safety checks */
    statement_data->stmt_type = TECC_STMT_TYPE_SELECT;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing SELECT statement");

    /* ReqID:TECC4.0 - Traverse CTE definitions first */
    if (select_stmt->cteList != NIL)
    {
        ListCell *cte_lc;
        List *prev_cte_names = NIL;
        List *prev_cte_cols = NIL;

        foreach(cte_lc, select_stmt->cteList)
        {
            CommonTableExpr *cte = (CommonTableExpr *) lfirst(cte_lc);
            if (cte->ctequery != NULL)
            {
                TeccStatementData cte_data;
                memset(&cte_data, 0, sizeof(TeccStatementData));
                cte_data.conn = statement_data->conn;  /* propagate connection for CTE encryption */

                /* Begin - ReqID:TECC4.0 - 链式 CTE：把上游 CTE 的加密列注入本 CTE */
                /* 本 CTE 的 FROM 若是前面已定义的 CTE，按 CTE 名查表元数据必然落空
                 * （CTE 名不是基表），上游的列映射就断在这里。这里把上游 CTE 的列
                 * 复制进来、别名改成它的引用名，使 `a.col` 形式的引用可解析。 */
                if (nodeTag(cte->ctequery) == FE_T_SelectStmt &&
                    prev_cte_names != NIL)
                {
                    SelectStmt *cte_sel = (SelectStmt *)cte->ctequery;
                    ListCell *from_lc;

                    foreach(from_lc, cte_sel->fromClause)
                    {
                        Node *from_node = (Node *)lfirst(from_lc);
                        const char *from_name;
                        int n_prev;
                        int pi;

                        if (from_node == NULL || nodeTag(from_node) != FE_T_RangeVar)
                            continue;
                        from_name = ((RangeVar *)from_node)->relname;
                        if (from_name == NULL)
                            continue;

                        n_prev = list_length(prev_cte_names);
                        for (pi = 0; pi < n_prev; pi++)
                        {
                            const char *prev_name =
                                (const char *)list_nth(prev_cte_names, pi);
                            List *prev_cols = (List *)list_nth(prev_cte_cols, pi);
                            ListCell *src_lc;

                            if (prev_name == NULL || prev_cols == NIL)
                                continue;
                            if (strcmp(prev_name, from_name) != 0)
                                continue;

                            foreach(src_lc, prev_cols)
                            {
                                TeccColumnRefData *src =
                                    (TeccColumnRefData *)lfirst(src_lc);
                                TeccColumnRefData *cp;

                                if (src == NULL)
                                    continue;
                                cp = tecc_column_ref_data_copy(src);
                                if (cp != NULL)
                                {
                                    tecc_column_ref_data_set_alias(cp, from_name);
                                    cte_data.encrypted_columns =
                                        lappend(cte_data.encrypted_columns, cp);
                                }
                            }
                            break;
                        }
                    }
                }
                /* End - ReqID:TECC4.0 */

                if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                {
                    tecc_stmt_processor_process_select((SelectStmt *)cte->ctequery, &cte_data);
                }
                /* ReqID:TECC4.0 - DML CTE: INSERT/UPDATE/DELETE in WITH clause */
                else if (nodeTag(cte->ctequery) == FE_T_InsertStmt)
                {
                    tecc_stmt_processor_process_insert((InsertStmt *)cte->ctequery, &cte_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_UpdateStmt)
                {
                    tecc_stmt_processor_process_update((UpdateStmt *)cte->ctequery, &cte_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_DeleteStmt)
                {
                    tecc_stmt_processor_process_delete((DeleteStmt *)cte->ctequery, &cte_data);
                }
                /* ReqID:TECC4.0 - Bug 9152 家族：CTE 内的加密常量（如
                 * WITH x AS (SELECT ... WHERE col_det='Alice')）必须把改写
                 * 标志与 raw_values_list 传播到主查询，否则主查询不会产出
                 * 改写后的 SQL，密态常量以明文发往服务端。 */
                stmt_propagate_sub_rewrite_state(statement_data, &cte_data,
                                                 cte->ctename);

                /* Bug 9323 - CTE 可以用别的名字暴露加密列（WITH cte(n) AS
                 * ... 或 CTE 内 SELECT name AS n），主查询只能按新名字引用，
                 * 不跟踪重命名则主查询 WHERE 的密态常量会以明文下发。
                 * 必须在上面传播之后执行：改的是父语句账本里刚收到的那批条目。 */
                if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                {
                    stmt_apply_sub_output_aliases(statement_data,
                                                  (SelectStmt *)cte->ctequery,
                                                  cte->ctename,
                                                  cte->aliascolnames);
                }

                /* ReqID:TECC4.0 - 记录本 CTE，供后续 CTE 引用时取用其列
                 * （见上面的链式 CTE 注入逻辑）。cte_data 是栈变量，但
                 * encrypted_columns 指向的链表由 pg_malloc 分配、此处未释放，
                 * 后续 CTE 使用期间一直有效。 */
                prev_cte_names = lappend(prev_cte_names, (void *)cte->ctename);
                prev_cte_cols = lappend(prev_cte_cols, (void *)cte_data.encrypted_columns);
            }
        }
    }

    /* Process target list */
    if (select_stmt->targetList != NIL) {
        for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next) {
            Node *target_node = (Node *) lc->data.ptr_value;
            if (target_node == NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "target_node is NULL, skipping");
                continue;
            }

            /* FIX: Check if this is a Const node (type FE_T_Const = 211) and skip it */
            if (target_node->type == FE_T_Const) {
                target_count++;
                continue;
            }

            /* Debug: Log the node type */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "target_node type: %d", target_node->type);

            /* Check if this is a ColumnRef node (type 7) */
            if (target_node->type == FE_T_ColumnRef) {
                /* Handle ColumnRef node directly in target list */
                ColumnRef *col_ref = (ColumnRef *) target_node;
                TeccColumnRefData *tecc_col = NULL;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing ColumnRef node directly in target list");

                /* Expand column reference with state context */
                if (statement_data->conn != NULL)
                {
                    /* Get TECC state from connection */
                    TeccState *state = tecc_get_state(statement_data->conn);
                    if (state != NULL)
                    {
                        if (!tecc_expand_column_ref_with_state(col_ref, &tecc_col, state))
                        {
                            tecc_col = NULL;
                        }
                    }
                }

                target_count++;
                continue;
            }

            /* Only cast to ResTarget if it's not a Const or ColumnRef node */
            res_target = (ResTarget *) target_node;
            if (res_target->val == NULL)
                continue;

            target_count++;

            /* Process each target expression */
            if (nodeTag(res_target->val) == FE_T_ColumnRef)
            {
                ColumnRef *col_ref = (ColumnRef *) res_target->val;
                TeccColumnRefData *tecc_col = NULL;

                /* Expand column reference with state context */
                if (statement_data->conn != NULL)
                {
                    /* Get TECC state from connection */
                    TeccState *state = tecc_get_state(statement_data->conn);
                    if (state != NULL)
                    {
                        if (!tecc_expand_column_ref_with_state(col_ref, &tecc_col, state))
                        {
                            tecc_col = NULL;
                        }
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "TECC state not available for column reference expansion");
                        tecc_col = NULL;
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "No connection available for column reference expansion");
                    tecc_col = NULL;
                }
                {
                    if (tecc_col != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Processed target column: %s (encrypted=%s)",
                                           tecc_col->alias_fqdn,
                                           tecc_col->is_encrypted ? "true" : "false");

                        /* ReqID:TECC4.0 - Encrypted column in SELECT target list:
                         * needs post-query result decryption, but no SQL rewrite.
                         * WHERE clause and expression encryption sets needs_rewrite separately. */
                        if (tecc_col->is_encrypted)
                        {
                            
                        }

                        tecc_column_ref_data_destroy(tecc_col);
                    }
                }
            }
            else if (nodeTag(res_target->val) == FE_T_FuncCall)
            {
                FuncCall *func_call = (FuncCall *) res_target->val;
                TeccFuncNameData *func_data = NULL;
				bool is_decrypt_raw_target = false;
				const char *decrypt_raw_algorithm = NULL;

                /* Expand function name */
                if (tecc_expand_function_name(func_call, &func_data))
                {
                    if (func_data != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Processed target function: %s.%s",
                                           func_data->schema_name,
                                                   func_data->function_name);

                        /* ReqID:TECC4.0 - 识别 update_meta()：执行该函数的会话在
                         * 结果处理阶段立即清空本地元数据缓存，不等 60s mark 节流；
                         * 其它会话仍走原有的 mark 检查机制。 */
                        if (func_data->function_name != NULL &&
                            strcmp(func_data->function_name, "update_meta") == 0)
                        {
                                TeccState *um_state = tecc_get_state(statement_data->conn);

                                if (um_state != NULL)
                                        um_state->last_stmt_type = TECC_STMT_TYPE_UPDATE_META;
                        }

						if (func_data->function_name != NULL &&
							pg_strcasecmp(func_data->function_name, "tecc_decrypt_raw") == 0)
						{
							TeccState  *raw_state = tecc_get_state(statement_data->conn);
							Node	   *raw_source_arg = NULL;

							is_decrypt_raw_target = true;
							if (raw_state != NULL)
								raw_state->pending_decrypt_raw_call = true;
							if (func_call->args != NULL &&
								list_length(func_call->args) >= 2)
							{
                                /* ReqID:TECC4.0 - Declare before assignments. */
                                Node *algo_arg = (Node *) list_nth(func_call->args, 1);
								raw_source_arg = (Node *) list_nth(func_call->args, 0);

								if (algo_arg != NULL && nodeTag(algo_arg) == FE_T_String)
									decrypt_raw_algorithm = strVal((Value *) algo_arg);
								else if (algo_arg != NULL && nodeTag(algo_arg) == FE_T_Const)
								{
									Const	   *algo_const = (Const *) algo_arg;

									if (!algo_const->constisnull &&
										algo_const->constvalue.sval != NULL)
										decrypt_raw_algorithm = algo_const->constvalue.sval;
								}
							}

                            /* ReqID:TECC4.0 - Materialize the SQL replacement before
                             * removing the call from the expression tree. */
							if (decrypt_raw_algorithm == NULL)
							{
								tecc_processor_set_error(statement_data,
									TECC_PROCESSOR_ERROR,
									"tecc_decrypt_raw requires a column and a constant algorithm name");
								pg_free(func_data);
								return TECC_STMT_PROCESSOR_ERROR;
							}

							/* ReqID:TECC4.0 - Preserve the former backend
							 * function's fail-closed behavior for an empty
							 * algorithm instead of treating it as RND. */
							if (decrypt_raw_algorithm[0] == '\0')
							{
								tecc_processor_set_error(statement_data,
									TECC_PROCESSOR_ERROR,
									"unsupported TECC ciphertext algorithm \"\"");
								pg_free(func_data);
								return TECC_STMT_PROCESSOR_ERROR;
							}

							if (!stmt_rewrite_decrypt_raw(func_call, statement_data))
                            {
                                tecc_processor_set_error(statement_data,
                                    TECC_PROCESSOR_ERROR,
                                    "tecc_decrypt_raw requires a column and a constant algorithm name");
                                pg_free(func_data);
                                return TECC_STMT_PROCESSOR_ERROR;
                            }

							/* ReqID:TECC4.0 - tecc_decrypt_raw() is a frontend
							 * diagnostic pseudo-function.  Replace the function
							 * call sent to the backend with its encrypted source
							 * expression; post-query processing extracts the
							 * requested segment and decrypts it on the client. */
							if (raw_source_arg != NULL)
							{
								res_target->val = raw_source_arg;
								statement_data->needs_rewrite = 1;
							}
						}

                        /* Process function arguments for encrypted columns */
                        if (func_call->args != NULL)
                        {
                            ListCell *arg_lc;
                            for (arg_lc = func_call->args->head; arg_lc != NULL; arg_lc = arg_lc->next)
                            {
                                Node *arg = (Node *) arg_lc->data.ptr_value;
                                if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
                                {
                                    ColumnRef *col_ref = (ColumnRef *) arg;
                                    TeccColumnRefData *tecc_col = NULL;

                                    if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                                    {
                                        if (tecc_col != NULL && tecc_col->is_encrypted)
                                        {
                                            if (statement_data->stmt_type != TECC_STMT_TYPE_SELECT)
                                            {
                                                statement_data->needs_rewrite = 1;
                                            }
                                            
                                        }
                                        tecc_column_ref_data_destroy(tecc_col);
                                    }
                                }
                            }
                        }

                        pg_free(func_data);
            }
        }
        /* ReqID:TECC4.0 - 表达式结果列（sum/avg 等聚合）：PG 协议不返回
         * table_oid/attnum，记录底层加密列名供结果解密恢复列元数据。 */
		{
			TeccState  *raw_state = tecc_get_state(statement_data->conn);
			int			raw_start = (raw_state != NULL) ? raw_state->pending_expr_col_count : 0;
			int			raw_i;
			bool		raw_column_found;

			raw_column_found = tecc_collect_expr_target_column(
				res_target->val, statement_data, from_table, from_schema, from_alias);

			/* ReqID:TECC4.0 - tecc_decrypt_raw() must never degrade into a
			 * pass-through projection for an ordinary column. */
			if (is_decrypt_raw_target && !raw_column_found)
			{
				tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
					"tecc_decrypt_raw requires a TECC encrypted value");
				return TECC_STMT_PROCESSOR_ERROR;
			}

			if (is_decrypt_raw_target && raw_state != NULL)
			{
				for (raw_i = raw_start; raw_i < raw_state->pending_expr_col_count; raw_i++)
				{
					raw_state->pending_expr_col_raw_hex[raw_i] = true;
					raw_state->pending_expr_col_field_indexes[raw_i] = target_count - 1;
					strlcpy(raw_state->pending_expr_col_raw_algorithms[raw_i],
							decrypt_raw_algorithm != NULL ? decrypt_raw_algorithm : "rnd",
							sizeof(raw_state->pending_expr_col_raw_algorithms[raw_i]));
				}
			}
		}
    }
    else
    {
        /* ReqID:TECC4.0 - Other expression types in target list.
         * Constant encryption is deferred to the second pass
         * (after FROM clause populates encrypted_columns). */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Deferring target expression type: %d to second pass",
                                   nodeTag(res_target->val));
        /* ReqID:TECC4.0 - 表达式结果列（a+b 等列运算）：记录底层加密列名。 */
        tecc_collect_expr_target_column(res_target->val, statement_data, from_table,
                                        from_schema, from_alias);
    }
        }
    }

    /* Process FROM clause */
    if (select_stmt->fromClause != NULL) {
        ListCell *lc;
        for (lc = select_stmt->fromClause->head; lc != NULL; lc = lc->next) {
            Node *from_item = (Node *) lc->data.ptr_value;

            from_count++;

            /* Process each from item */
            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *range_var = (RangeVar *) from_item;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing FROM table: %s.%s",
                                   range_var->schemaname ? range_var->schemaname : "(null)",
                                   range_var->relname ? range_var->relname : "(null)");

                /* Load table metadata and identify encrypted columns */
                {
                    TeccColumnCacheItem **table_cols = NULL;
                    int table_col_count = 0;
                    stmt_load_table_metadata(range_var, statement_data, &table_cols, &table_col_count);
                    if (table_cols != NULL && table_col_count > 0)
                    {
                        int col_i;
                        for (col_i = 0; col_i < table_col_count; col_i++)
                        {
                            if (table_cols[col_i] != NULL)
                            {
                                TeccColumnRefData *col_data = tecc_column_ref_data_create();
                                if (col_data != NULL)
                                {
                                    /* ReqID:TECC4.0 - Use init function to set all fields including table_alias.
                                     * 未限定表名经 tecc_stmt_resolve_table_schema 解析真实 schema
                                     * （与 stmt_load_table_metadata 一致），供后续按 schema 查缓存。 */
                                    tecc_column_ref_data_init(col_data,
                                                             NULL,
                                                             tecc_stmt_resolve_table_schema(statement_data, range_var->schemaname, range_var->relname),
                                                             range_var->relname,
                                                             table_cols[col_i]->column_name);
                                    /* Set table alias if present on RangeVar */
                                    if (range_var->alias != NULL
                                        && range_var->alias->aliasname != NULL)
                                        tecc_column_ref_data_set_alias(col_data,
                                            range_var->alias->aliasname);
                                    col_data->is_encrypted = true;
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, col_data);
                                }
                            }
                        }
                        pg_free(table_cols);
                    }
                }
            }
        else if (nodeTag(from_item) == FE_T_SelectStmt)
        {
            /* Subselect in FROM clause */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing FROM subselect");

            /* Process subselect recursively */
            if (nodeTag(from_item) == FE_T_SelectStmt)
            {
                SelectStmt *subselect = (SelectStmt *) from_item;
                TeccStmtProcessorResult subselect_result;

                /* 创建临时的statement_data用于子查询处理 */
                TeccStatementData subselect_data;
                memset(&subselect_data, 0, sizeof(TeccStatementData));
                subselect_data.conn = statement_data->conn;  /* propagate connection for subquery encryption */

                subselect_result = tecc_stmt_processor_process_select(subselect, &subselect_data);
                if (subselect_result != TECC_STMT_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "Failed to process FROM subselect: %d", subselect_result);
                    return subselect_result;
                }

                /* 如果子查询需要改写，则主查询也需要改写 */
                if (subselect_data.needs_rewrite)
                {
                    statement_data->needs_rewrite = 1;
                }
                /* ReqID:TECC4.0 - Bug 9152: 传播子查询的 WHERE 改写标志和
                 * raw_values_list 到主查询，确保子查询内的加密常量
                 * （如 WHERE col_det='Alice'）也被写入改写列表。 */
                stmt_propagate_sub_rewrite_state(statement_data, &subselect_data,
                                                 NULL);

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Successfully processed FROM subselect");
            /* ReqID:TECC4.0 - 收集子查询输出列底层加密列到主查询 pending */
            tecc_collect_subselect_output_columns(subselect, statement_data);
        }
        }
        else if (nodeTag(from_item) == FE_T_RangeSubselect)
        {
            /* ReqID:TECC4.0 - RangeSubselect in FROM clause
             * Unwrap subquery and process recursively so encrypted
             * columns/constants in subquery are visible to the main query.
             * Bug 9323: 交给统一处理函数，同时覆盖子查询为集合操作
             * （UNION/INTERSECT/EXCEPT）以及输出列被重命名的情形。 */
            stmt_process_from_subselect_item(from_item, statement_data);
        }
        else if (nodeTag(from_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively process JOIN components
                 * so encrypted columns from joined tables are discovered. */
                JoinExpr *join = (JoinExpr *) from_item;
                Node *join_items[2] = { join->larg, join->rarg };
                int ji;
                for (ji = 0; ji < 2; ji++)
                {
                    Node *ji_node = join_items[ji];
                    if (ji_node == NULL) continue;
                    /* Bug 9323 - JOIN 的一侧是子查询时，原先只处理 RangeVar，
                     * 子查询整棵子树被跳过（密态列不传播、外层 "s.col" 的密态
                     * 常量以明文下发）。这里对子查询递归处理。 */
                    if (nodeTag(ji_node) == FE_T_RangeSubselect)
                    {
                        stmt_process_from_subselect_item(ji_node, statement_data);
                        continue;
                    }
                    if (nodeTag(ji_node) == FE_T_RangeVar)
                    {
                        RangeVar *rv = (RangeVar *) ji_node;
                        TeccColumnCacheItem **table_cols = NULL;
                        int table_col_count = 0;
                        stmt_load_table_metadata(rv, statement_data, &table_cols, &table_col_count);
                        if (table_cols != NULL && table_col_count > 0)
                        {
                            int ci;
                            for (ci = 0; ci < table_col_count; ci++)
                            {
                                if (table_cols[ci] != NULL)
                                {
                                    TeccColumnRefData *cd = tecc_column_ref_data_create();
                                    if (cd != NULL)
                                    {
                                        /* ReqID:TECC4.0 - Use init function to set fields including table_alias */
                                        tecc_column_ref_data_init(cd, NULL,
                                                                 rv->schemaname,
                                                                 rv->relname,
                                                                 table_cols[ci]->column_name);
                                        if (rv->alias != NULL && rv->alias->aliasname != NULL)
                                            tecc_column_ref_data_set_alias(cd, rv->alias->aliasname);
                                        cd->is_encrypted = true;
                                        statement_data->encrypted_columns = lappend(
                                            statement_data->encrypted_columns, cd);
                                    }
                                }
                            }
                            pg_free(table_cols);
}
                    }
                    else if (nodeTag(ji_node) == FE_T_JoinExpr)
                    {
                        /* Recurse: nested JOIN. Wrap in a single-item list for processing.
                         * Fall through to recursive type dispatch.
                         * Simply process larg/rarg of the nested join. */
                        JoinExpr *nested = (JoinExpr *) ji_node;
                        Node *nested_items[2] = { nested->larg, nested->rarg };
                        int ni;
                        for (ni = 0; ni < 2; ni++)
                        {
                            Node *n_node = nested_items[ni];
                            /* Bug 9323 - 嵌套 JOIN 的一侧同样可能是子查询 */
                            if (n_node != NULL &&
                                (nodeTag(n_node) == FE_T_RangeSubselect ||
                                 nodeTag(n_node) == FE_T_JoinExpr))
                            {
                                stmt_process_from_subselect_item(n_node, statement_data);
                                continue;
                            }
                            if (n_node != NULL && nodeTag(n_node) == FE_T_RangeVar)
                            {
                                RangeVar *nrv = (RangeVar *) n_node;
                                TeccColumnCacheItem **tcols = NULL;
                                int tcnt = 0;
                                stmt_load_table_metadata(nrv, statement_data, &tcols, &tcnt);
                                if (tcols != NULL && tcnt > 0)
                                {
                                    int cj;
                                    for (cj = 0; cj < tcnt; cj++)
                                    {
                                        if (tcols[cj] != NULL)
                                        {
                                            TeccColumnRefData *cd2 = tecc_column_ref_data_create();
                                            if (cd2 != NULL)
                                            {
                                                strncpy(cd2->column_name, tcols[cj]->column_name, NAMEDATALEN - 1);
                                                cd2->column_name[NAMEDATALEN - 1] = '\0';
                                                if (nrv->relname)
                                                {
                                                    strncpy(cd2->table_name, nrv->relname, NAMEDATALEN - 1);
                                                    cd2->table_name[NAMEDATALEN - 1] = '\0';
                                                }
                                                cd2->is_encrypted = true;
                                                statement_data->encrypted_columns = lappend(
                                                    statement_data->encrypted_columns, cd2);
                                            }
                                        }
                                    }
                                    pg_free(tcols);
                                }
                            }
                        }
                    }
                }

                /* ReqID:TECC4.0 - Process JOIN ON condition (quals).
                 * JOIN quals are not part of select_stmt->whereClause, so
                 * they were never fed to the WHERE clause processor.  Process
                 * them here so comparisons on encrypted columns in ON clauses
                 * get constant encryption, algorithm checks and the
                 * cross-DEK consistency check. */
                if (join->quals != NULL)
                {
                    TeccWhereClauseProcessorResult jq_result;

                    jq_result = tecc_where_clause_processor_process(join->quals, statement_data);
                    if (jq_result != TECC_WHERE_PROCESSOR_SUCCESS)
                    {
                        switch (jq_result)
                        {
                            case TECC_WHERE_PROCESSOR_DEK_MISMATCH:
                                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "Cannot compare encrypted columns with different DEKs. "
                                    "Please ensure the compared columns use the same DEK.");
                                break;
                            case TECC_WHERE_PROCESSOR_LIKE_COLUMN:
                                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "LIKE/NOT LIKE between two encrypted columns is not supported. "
                                    "The LIKE pattern must be a plain string constant.");
                                break;
                            default:
                                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "JOIN clause processing failed");
                                break;
                        }
                        statement_data->needs_rewrite = 0;
                        statement_data->where_needs_rewrite = 0;
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing FROM item type: %d", nodeTag(from_item));
            }
        }
    }

    /* Process WHERE clause if present */
    if (select_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing WHERE clause");

        /* Delegate to WHERE clause processor */
        where_result = tecc_where_clause_processor_process(
            select_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET (deterministic encryption) algorithm, "
                        "but the column is not configured with DET.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires OPE (order-preserving encryption) algorithm, "
                        "but the column is not configured with OPE.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires SEARCH (searchable encryption) algorithm, "
                        "but the column is not configured with SEARCH.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET or OPE algorithm, "
                        "but the column is not configured with either.");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator is not compatible with the encryption algorithm on this column.");
                    break;
                case TECC_WHERE_PROCESSOR_DEK_MISMATCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Cannot compare encrypted columns with different DEKs. "
                        "Please ensure the compared columns use the same DEK.");
                    break;
                case TECC_WHERE_PROCESSOR_LIKE_COLUMN:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "LIKE/NOT LIKE between two encrypted columns is not supported. "
                        "The LIKE pattern must be a plain string constant.");
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_AGGREGATE:
                    /* Error message already set by
                     * stmt_validate_aggregate_funcall() in the walker hook. */
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR:
                    /* Error message already set by
                     * stmt_validate_scalar_funcall() in the walker hook. */
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            /* ReqID:TECC4.0 - Clear rewrite flags to prevent SAFETY CHECK
             * false positive. When WHERE processing fails (e.g. missing OPE),
             * needs_rewrite may have been set by earlier FuncCall constant
             * processing (COALESCE/GREATEST/LEAST). Reset so user gets a
             * clean error message instead of "no rewritten query". */
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE clause processing completed successfully");
    }

    /* ReqID:TECC4.0 - Process FILTER/OVER clauses in target list FuncCall nodes */
    if (select_stmt->targetList != NIL)
    {
        ListCell *tlc2;
        for (tlc2 = select_stmt->targetList->head; tlc2 != NULL; tlc2 = tlc2->next)
        {
            Node *target_node = (Node *)tlc2->data.ptr_value;
            ResTarget *rt;

            if (target_node == NULL)
                continue;
            if (target_node->type == FE_T_Const || target_node->type == FE_T_ColumnRef)
                continue;

            rt = (ResTarget *)target_node;
            if (rt->val == NULL)
                continue;

            if (nodeTag(rt->val) == FE_T_FuncCall)
            {
                FuncCall *fc = (FuncCall *)rt->val;

                /* ReqID:TECC4.0 - Validate aggregate function against
                 * encrypted column algorithms (MIN/MAX need OPE;
                 * SUM/AVG/string_agg rejected). */
                if (!stmt_validate_aggregate_funcall(fc, statement_data))
                {
                    /* Clear rewrite flags to prevent SAFETY CHECK false
                     * positive (mirrors WHERE error path at lines ~1335). */
                    statement_data->needs_rewrite = 0;
                    statement_data->where_needs_rewrite = 0;
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                /* ReqID:TECC4.0 - 需求 4.4.1.4：参数组合统一校验
                 * （coalesce/greatest/least，固定错误格式 F 模板）。 */
                if (!stmt_validate_scalar_arg_combos(fc, statement_data))
                {
                    statement_data->needs_rewrite = 0;
                    statement_data->where_needs_rewrite = 0;
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                /* ReqID:TECC4.0 - Validate scalar function against
                 * encrypted column arguments (power rejected). */
                if (!stmt_validate_scalar_funcall(fc, statement_data))
                {
                    statement_data->needs_rewrite = 0;
                    statement_data->where_needs_rewrite = 0;
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                /* Process FILTER (WHERE) clause - encrypt constants */
                if (fc->agg_filter != NULL)
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1, "Processing FILTER clause in target list");
                    where_result = tecc_where_clause_processor_process(fc->agg_filter, statement_data);
                    if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR, "FILTER clause processing failed");
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
                /* Process OVER clause (WindowDef) */
                if (fc->over != NULL)
                {
                    WindowDef *wd = (WindowDef *)fc->over;
                    ListCell *plc;

                    if (wd->partitionClause != NULL)
                    {
                        for (plc = wd->partitionClause->head; plc != NULL; plc = plc->next)
                        {
                            where_result = tecc_where_clause_processor_process((Node *)plc->data.ptr_value, statement_data);
                            if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                                return TECC_STMT_PROCESSOR_ERROR;
                        }
                    }
                    if (wd->orderClause != NULL)
                    {
                        for (plc = wd->orderClause->head; plc != NULL; plc = plc->next)
                        {
                            SortBy *sortby = (SortBy *)plc->data.ptr_value;
                            if (sortby != NULL && sortby->node != NULL)
                            {
                                where_result = tecc_where_clause_processor_process(sortby->node, statement_data);
                                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                                    return TECC_STMT_PROCESSOR_ERROR;
                            }
                        }
                    }
                }
                /* ReqID:TECC4.0 - Encrypt constants in FuncCall arguments
                 * for functions like COALESCE, NULLIF, GREATEST, LEAST */
                stmt_process_funcall_constants(fc, statement_data);
            }

            /* ReqID:TECC4.0 - Process SubLink in target list */
            if (nodeTag(rt->val) == FE_T_SubLink)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1, "Processing SubLink in target list");
                where_result = tecc_where_clause_processor_process(rt->val, statement_data);
                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_ERROR, "SubLink target processing failed");
                    return TECC_STMT_PROCESSOR_ERROR;
                }
            }
            /* ReqID:TECC4.0 - Process other expression types for constant
             * encryption (CaseExpr, A_Expr, TypeCast, RowExpr, NullTest).
             * This runs in the second pass after FROM clause has populated
             * encrypted_columns, so where_is_encrypted_column() works. */
            else if (nodeTag(rt->val) != FE_T_FuncCall &&
                     nodeTag(rt->val) != FE_T_ColumnRef)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing target expression type %d for constant encryption",
                                   nodeTag(rt->val));
                /* ReqID:TECC4.0 - Check return value. Operators like || are
                 * not compatible with encrypted columns — must reject here
                 * instead of silently passing garbage to the server. */
                where_result = tecc_where_clause_processor_process_expr(
                    rt->val, statement_data);
                if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
                {
                    /* ReqID:TECC4.0 - 下层（如 CASE 分支类型兼容校验、
                     * stmt_validate_aggregate_funcall /
                     * stmt_validate_scalar_funcall）已设置具体错误信息时，
                     * 保留该信息；否则使用通用错误。 */
                    if (statement_data->error_code == TECC_PROCESSOR_SUCCESS &&
                        where_result != TECC_WHERE_PROCESSOR_UNSUPPORTED_AGGREGATE &&
                        where_result != TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_ERROR,
                            "Operator in SELECT expression is not supported "
                            "with encrypted columns");
                    }
                    return TECC_STMT_PROCESSOR_ERROR;
                }
            }
        }
    }

    /* Process HAVING clause if present */
    if (select_stmt->havingClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1, "Processing HAVING clause");
        where_result = tecc_where_clause_processor_process(select_stmt->havingClause, statement_data);
        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR, "HAVING clause processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    

    /* Process TypeCast expressions with encrypted options in SELECT target list */
    if (select_stmt != NULL)
    {
        TeccStmtProcessorResult typecast_result;
        /* 纯粹的 SELECT：没有 INSERT 目标列，只按 cast 声明的长度定型 */
        typecast_result = process_insert_select_typecast(
            (SelectStmt *)select_stmt, statement_data, NULL, NULL, 0);
        if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "SELECT TypeCast processing failed: %d",
                               typecast_result);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* ReqID:TECC4.0 - 元数据加载失败（如 data_fetcher 临时连接失败）时，
     * stmt_load_table_metadata 已通过 tecc_processor_set_error 设置错误码，
     * 此处兜底检查并上抛，避免加密表被误判为普通表导致明文泄漏。 */
    if (statement_data->error_code != TECC_PROCESSOR_SUCCESS)
        return TECC_STMT_PROCESSOR_ERROR;

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Helper: find end position of a TypeCast expression in the original SQL text.
 * The TypeCast expression has the form: <arg>::<type_name> [ENCRYPTED WITH (...)]
 * Returns the byte offset just past the end of the expression.
 */
size_t
find_typecast_expr_end(const char *query, const TypeCast *type_cast)
{
    size_t pos;
    size_t start;
    int depth;

    if (query == NULL || type_cast == NULL)
        return 0;

    /* Start from the TypeName location (the type name token) */
    if (type_cast->typeName == NULL)
        return type_cast->location;

    start = (size_t)type_cast->typeName->location;

    /* If encryptedOpts present, scan for ENCRYPTED WITH (...) */
    if (type_cast->typeName->encryptedOpts == NIL)
    {
        /* Plain type cast without encrypted options - use data_size heuristic */
        /* The type name starts at start. Since we don't have a stored length,
         * just return start + some reasonable guess. For plain type casts
         * (no ENCRYPTED WITH), the expression extends past the type name.
         * Find the end by looking for non-identifier characters. */
        pos = start;
        while (query[pos] != '\0' &&
               (isalnum((unsigned char)query[pos]) || query[pos] == '_'))
            pos++;
        return pos;
    }

    /* With ENCRYPTED WITH, scan from the type name to find encrypted with (...) */
    pos = start;
    while (query[pos] != '\0')
    {
        /* Look for 'encrypted' (case-insensitive) */
        if ((query[pos] == 'e' || query[pos] == 'E') &&
            strncasecmp(query + pos, "encrypted", 9) == 0)
        {
            size_t p = pos + 9;

            /* Skip whitespace and find '(' */
            while (query[p] != '\0' && query[p] != '(' && query[p] != ')')
            {
                if (query[p] == ';' || query[p] == ',')
                    break;
                p++;
            }

            if (query[p] == '(')
            {
                /* Track parenthesis depth to find matching ')' */
                depth = 1;
                p++;
                while (depth > 0 && query[p] != '\0')
                {
                    if (query[p] == '(') depth++;
                    else if (query[p] == ')') depth--;
                    p++;
                }
                return p;  /* Past the closing ')' */
            }
            break;
        }

        /* Stop at delimiters that can't be part of a type name */
        if (query[pos] == ';' || query[pos] == ',')
            break;
        pos++;
    }

    return start; /* fallback */
}

/*
 * Recursively walk an expression tree and collect all TypeCast nodes that have
 * encrypted options. Only collects the outermost TypeCast in each chain — inner
 * TypeCast nodes with encryptedOpts are absorbed into the outer encryption.
 */
static void
collect_typecast_nodes(Node *expr, List **nodes)
{
    if (expr == NULL)
        return;

    switch (expr->type)
    {
        case FE_T_TypeCast:
        {
            TypeCast *tc = (TypeCast *)expr;
            if (tc->typeName != NULL && tc->typeName->encryptedOpts != NIL)
            {
                /* Found a TypeCast with encrypted options — collect it.
                 * Don't recurse into arg since nested TypeCast with
                 * encryptedOpts is absorbed by the outer encryption. */
                *nodes = lappend(*nodes, tc);
            }
            else if (tc->arg != NULL)
            {
                /* No encrypted opts here, keep walking */
                collect_typecast_nodes(tc->arg, nodes);
            }
            break;
        }
        case FE_T_FuncCall:
        {
            FuncCall *fc = (FuncCall *)expr;
            if (fc->args != NIL)
            {
                ListCell *lc;
                foreach (lc, fc->args)
                    collect_typecast_nodes((Node *)lc->data.ptr_value, nodes);
            }
            break;
        }
        /* Begin - ReqID:TECC4.0 - Bug 7857: COLLATE wrapping TypeCast with encryptedOpts */
        case FE_T_CollateClause:
        {
            CollateClause *cc = (CollateClause *)expr;
            if (cc->arg != NULL)
                collect_typecast_nodes(cc->arg, nodes);
            break;
        }
        /* End - ReqID:TECC4.0 */
        /* ReqID:TECC4.0 - Bug 8001: walk A_Expr (WHERE comparison) to find TypeCast */
        case FE_T_A_Expr:
        {
            A_Expr *ae = (A_Expr *)expr;
            if (ae->lexpr != NULL)
                collect_typecast_nodes(ae->lexpr, nodes);
            if (ae->rexpr != NULL)
                collect_typecast_nodes(ae->rexpr, nodes);
            break;
        }
        /* ReqID:TECC4.0 - Bug 8001: walk BoolExpr (AND/OR/NOT) in WHERE */
        case FE_T_BoolExpr:
        {
            struct BoolExpr *be = (struct BoolExpr *)expr;
            ListCell *lc;
            foreach (lc, be->args)
                collect_typecast_nodes((Node *)lc->data.ptr_value, nodes);
            break;
        }
        /* ReqID:TECC4.0 - Bug 8001: walk NullTest/BooleanTest for TypeCast inside */
        case FE_T_NullTest:
        {
            NullTest *nt = (NullTest *)expr;
            if (nt->arg != NULL)
                collect_typecast_nodes(nt->arg, nodes);
            break;
        }
        case FE_T_BooleanTest:
        {
            struct BooleanTest *bt = (struct BooleanTest *)expr;
            if (bt->arg != NULL)
                collect_typecast_nodes(bt->arg, nodes);
            break;
        }
        default:
            break;
    }
}

/*
 * Get the type name string from a TypeCast node as it appears in the original SQL.
 * Returns a palloc'd string (caller must pg_free), or NULL on failure.
 * ReqID:TECC4.0
 */
static char *
get_typecast_typename(const TypeCast *type_cast, const char *orig_query)
{
    const char *start;
    const char *p;
    size_t len;
    char *result;

    if (type_cast == NULL || type_cast->typeName == NULL || orig_query == NULL)
        return NULL;

    start = orig_query + type_cast->typeName->location;
    p = start;

    /* Scan forward through identifier chars (alphanum, _, ., " for qualified names) */
    while (*p != '\0' && (isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '"'))
        p++;

    len = p - start;
    if (len == 0)
        return NULL;

    result = (char *)pg_malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

/*
 * Determine encryption algorithm flags for a TypeCast expression based on the
 * encrypted type name and optional support_algo in the encrypted options.
 * ReqID:TECC4.0
 *
 * 非 static：CTAS 的密态 cast 处理也要按同一套规则决定算法（此前写死只支持
 * DET，声明了 SUPPORT_ALGO='det,search' 的列在 CTAS 建出的表上做 LIKE 会报
 * "not encrypted with SEARCH"）。
 */
void
get_typecast_algo_flags(const TypeCast *type_cast,
                        const char *typename_str,
                        bool *supports_det,
                        bool *supports_ope,
                        bool *supports_search,
                        bool *supports_he_add,
                        bool *supports_he_mul)
{
    ListCell *lc;

    /* Defaults based on encrypted type */
    *supports_det = false;
    *supports_ope = false;
    *supports_search = false;
    *supports_he_add = false;
    *supports_he_mul = false;

    if (typename_str != NULL)
    {
        if (strcasecmp(typename_str, "int2_enc") == 0 ||
            strcasecmp(typename_str, "int4_enc") == 0 ||
            strcasecmp(typename_str, "int8_enc") == 0)
            *supports_det = true;
        else if (strcasecmp(typename_str, "float4_enc") == 0 ||
                 strcasecmp(typename_str, "float8_enc") == 0 ||
                 strcasecmp(typename_str, "numeric_enc") == 0)
            *supports_ope = true;
        else if (strcasecmp(typename_str, "text_enc") == 0 ||
                 strcasecmp(typename_str, "varchar_enc") == 0 ||
                 strcasecmp(typename_str, "bpchar_enc") == 0)
            *supports_det = true;
    }

    /* Check for support_algo override in encrypted options */
    if (type_cast == NULL || type_cast->typeName == NULL)
        return;

    foreach(lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *) lc->data.ptr_value;
        if (elem != NULL && elem->defname != NULL &&
            pg_strcasecmp(elem->defname, "support_algo") == 0)
        {
            char *algo_value;

            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
            {
                algo_value = strVal((const Value *)elem->arg);
                *supports_det = (strstr(algo_value, "det") != NULL);
                *supports_ope = (strstr(algo_value, "ope") != NULL);
                *supports_search = (strstr(algo_value, "search") != NULL);
                *supports_he_add = (strstr(algo_value, "he-add") != NULL);
                *supports_he_mul = (strstr(algo_value, "he-mul") != NULL);
            }
            break;
        }
    }
}

/* Begin - ReqID:TECC4.0 - Bug 7893/7901 */

/* Type family classification for encrypted type conversions */
typedef enum
{
    TECC_TYPE_FAMILY_NONE = 0,
    TECC_TYPE_FAMILY_INT,
    TECC_TYPE_FAMILY_FLOAT,
    TECC_TYPE_FAMILY_NUMERIC,
    TECC_TYPE_FAMILY_STRING
} TeccTypeFamily;

/*
 * Classify an encrypted type name into a type family.
 * Conversions within the same family (e.g., text_enc ↔ varchar_enc) are
 * allowed; cross-family conversions (e.g., text_enc → int2_enc) are not.
 */
static TeccTypeFamily
get_enc_type_family(const char *typename_str)
{
    if (typename_str == NULL)
        return TECC_TYPE_FAMILY_NONE;
    if (strcasecmp(typename_str, "int2_enc") == 0 ||
        strcasecmp(typename_str, "int4_enc") == 0 ||
        strcasecmp(typename_str, "int8_enc") == 0)
        return TECC_TYPE_FAMILY_INT;
    if (strcasecmp(typename_str, "float4_enc") == 0 ||
        strcasecmp(typename_str, "float8_enc") == 0)
        return TECC_TYPE_FAMILY_FLOAT;
    if (strcasecmp(typename_str, "numeric_enc") == 0)
        return TECC_TYPE_FAMILY_NUMERIC;
    if (strcasecmp(typename_str, "text_enc") == 0 ||
        strcasecmp(typename_str, "varchar_enc") == 0 ||
        strcasecmp(typename_str, "bpchar_enc") == 0)
        return TECC_TYPE_FAMILY_STRING;
    return TECC_TYPE_FAMILY_NONE;
}

/*
 * Rank an encrypted type by storage width (larger rank = wider type).
 * Used to reject narrowing casts within the same type family
 * (e.g., int8_enc -> int4_enc).
 */
static int
get_enc_type_rank(const char *typename_str)
{
    if (typename_str == NULL)
        return 0;
    if (strcasecmp(typename_str, "int2_enc") == 0)
        return 1;
    if (strcasecmp(typename_str, "int4_enc") == 0 ||
        strcasecmp(typename_str, "float4_enc") == 0)
        return 2;
    if (strcasecmp(typename_str, "int8_enc") == 0 ||
        strcasecmp(typename_str, "float8_enc") == 0)
        return 3;
    if (strcasecmp(typename_str, "numeric_enc") == 0)
        return 4;
    /* String types all treated equally */
    return 0;
}

/*
 * Extract DEK name from a TypeCast's encrypted options.
 * Returns a pointer into the parsed tree (do not free).
 */
static const char *
get_typecast_dek_name(const TypeCast *type_cast)
{
    ListCell *lc;

    if (type_cast == NULL || type_cast->typeName == NULL)
        return NULL;

    foreach (lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *) lc->data.ptr_value;
        if (elem != NULL && elem->defname != NULL &&
            pg_strcasecmp(elem->defname, "dek") == 0)
        {
            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                return strVal((const Value *)elem->arg);
            break;
        }
    }
    return NULL;
}

/*
 * ReqID:TECC4.0 - 需求 4.4.1.4：不支持的内置函数返回固定格式错误信息。
 * F 模板：<函数名> 不支持当前参数组合：<原因>。
 *
 * 文案 locale 可变（对齐 PG gettext 语义）：按 LC_ALL→LC_MESSAGES→LANG 顺序
 * 检测，zh_* 输出中文，其余（C/C.UTF-8/en_* 等）输出英文。回归测试稳定性
 * 不靠文案固定，而靠 TAP 环境固定 LC_MESSAGES=C（expected 按英文捕获）。
 *
 * 本节实现 coalesce/greatest/least/nullif/decode/power 参数组合统一校验：
 *   - 裸 ::密态类型 cast 普通常量（服务端无明文→密态 cast 行，必败）
 *   - 密态列 × 普通列混用（需求口径不支持）
 *   - 密态列/密态 cast 之间的非拓宽组合（跨族 + 数值格洞）
 *   - 密态列 × 类别不匹配的普通常量
 *   - 无密态列时，ew(dek) 密态 cast 常量 × 普通常量混用
 * 放行：密态列×同类型常量、同族可拓宽组合、ew cast×ew cast 同族、ew cast×NULL、
 *       \x 密文字面量 cast（合法直传）。
 */

/* F 模板文案表（[reason][locale]，locale 0=中文 1=英文；
 * 枚举定义见 stmt_processor.h） */
static const char *const stmt_f_reason_texts[STMT_F_REASON_COUNT][2] =
{
    /* BARE_CAST */
    {
        "普通常量不能直接转换为密态类型，请使用 encrypted with (dek=...) "
        "指定密钥，或改用与密态列同类型的常量",
        "plain constants cannot be cast to encrypted types directly; "
        "use encrypted with (dek=...) or use a constant of the same type "
        "as the encrypted column"
    },
    /* ENC_PLAIN_MIX */
    {
        "密态列与普通列不能混用",
        "encrypted columns and plain columns cannot be mixed"
    },
    /* TYPE_COMBO */
    {
        "密态类型之间不支持该类型组合",
        "this combination of encrypted types is not supported"
    },
    /* CONST_MISMATCH */
    {
        "常量类型与密态列类型不匹配",
        "the constant type does not match the encrypted column type"
    },
    /* ENC_PLAIN_CONST */
    {
        "密态类型常量与普通常量不能混用",
        "encrypted-type constants and plain constants cannot be mixed"
    },
    /* CASE_SWITCH */
    {
        "switch 形式（CASE 表达式 WHEN 常量）暂不支持，请改用 CASE WHEN 条件形式",
        "the switch form (CASE expression WHEN constant) is not supported; "
        "use the CASE WHEN condition form instead"
    },
    /* CASE_BRANCH */
    {
        "分支常量类型无法按密态列 %s 的加密方式加密，请将分支统一为密态列"
        "或与密态列原始类型匹配的普通常量",
        "the branch constant type cannot be encrypted like encrypted column %s; "
        "unify branches to encrypted columns or plain constants matching the "
        "column's original type"
    }
};

/* 当前 locale 是否为中文（LC_ALL→LC_MESSAGES→LANG 顺序检测） */
bool
stmt_msg_locale_is_zh(void)
{
    const char *loc;

    loc = getenv("LC_ALL");
    if (loc == NULL || loc[0] == '\0')
        loc = getenv("LC_MESSAGES");
    if (loc == NULL || loc[0] == '\0')
        loc = getenv("LANG");
    if (loc == NULL || loc[0] == '\0')
        return false;

    return (loc[0] == 'z' || loc[0] == 'Z') &&
           (loc[1] == 'h' || loc[1] == 'H');
}

/* F 模板前缀格式串（含函数名与原因两个 %s 占位） */
const char *
stmt_f_template(void)
{
    return stmt_msg_locale_is_zh() ?
        "%s 不支持当前参数组合：%s" :
        "%s does not support this argument combination: %s";
}

/* F 模板原因文案（CASE_BRANCH 返回的格式串含 %s 占位） */
const char *
stmt_f_reason_text(int reason_id)
{
    int loc;

    if (reason_id < 0 || reason_id >= STMT_F_REASON_COUNT)
        reason_id = STMT_F_REASON_TYPE_COMBO;
    loc = stmt_msg_locale_is_zh() ? 0 : 1;
    return stmt_f_reason_texts[reason_id][loc];
}

/* 普通基类型名 → 类型族与宽度秩（对齐密态拓宽格） */
static bool
stmt_plain_type_family_and_rank(const char *typename_str,
                                TeccTypeFamily *family, int *rank)
{
    if (typename_str == NULL || family == NULL || rank == NULL)
        return false;
    if (strcasecmp(typename_str, "int2") == 0 ||
        strcasecmp(typename_str, "smallint") == 0)
    { *family = TECC_TYPE_FAMILY_INT; *rank = 1; return true; }
    if (strcasecmp(typename_str, "int4") == 0 ||
        strcasecmp(typename_str, "integer") == 0)
    { *family = TECC_TYPE_FAMILY_INT; *rank = 2; return true; }
    if (strcasecmp(typename_str, "int8") == 0 ||
        strcasecmp(typename_str, "bigint") == 0)
    { *family = TECC_TYPE_FAMILY_INT; *rank = 3; return true; }
    if (strcasecmp(typename_str, "float4") == 0 ||
        strcasecmp(typename_str, "real") == 0)
    { *family = TECC_TYPE_FAMILY_FLOAT; *rank = 2; return true; }
    if (strcasecmp(typename_str, "float8") == 0 ||
        strcasecmp(typename_str, "double precision") == 0)
    { *family = TECC_TYPE_FAMILY_FLOAT; *rank = 3; return true; }
    if (strcasecmp(typename_str, "numeric") == 0 ||
        strcasecmp(typename_str, "decimal") == 0)
    { *family = TECC_TYPE_FAMILY_NUMERIC; *rank = 4; return true; }
    if (strcasecmp(typename_str, "text") == 0 ||
        strcasecmp(typename_str, "varchar") == 0 ||
        strcasecmp(typename_str, "character varying") == 0 ||
        strcasecmp(typename_str, "bpchar") == 0 ||
        strcasecmp(typename_str, "char") == 0)
    { *family = TECC_TYPE_FAMILY_STRING; *rank = 0; return true; }
    return false;
}

/* 基类型 OID → 类型族 */
static TeccTypeFamily
stmt_oid_to_type_family(Oid oid)
{
    switch (oid)
    {
        case INT2OID: case INT4OID: case INT8OID:
            return TECC_TYPE_FAMILY_INT;
        case FLOAT4OID: case FLOAT8OID:
            return TECC_TYPE_FAMILY_FLOAT;
        case NUMERICOID:
            return TECC_TYPE_FAMILY_NUMERIC;
        case TEXTOID: case VARCHAROID: case BPCHAROID:
            return TECC_TYPE_FAMILY_STRING;
        default:
            return TECC_TYPE_FAMILY_NONE;
    }
}

/* 基类型 OID → 宽度秩 */
static int
stmt_oid_to_type_rank(Oid oid)
{
    switch (oid)
    {
        case INT2OID: return 1;
        case INT4OID: case FLOAT4OID: return 2;
        case INT8OID: case FLOAT8OID: return 3;
        case NUMERICOID: return 4;
        default: return 0;
    }
}

/*
 * 密态类型组合是否可拓宽（对齐 pg_cast.dat 323-361 隐式拓宽格）。
 * 洞：int4×float4、int8×float4、int8×float8；字符串族与数值族不可互转。
 */
static bool
stmt_enc_families_widenable(TeccTypeFamily fa, int ra,
                            TeccTypeFamily fb, int rb)
{
    if (fa == fb)
        return true;
    if (fa == TECC_TYPE_FAMILY_STRING || fb == TECC_TYPE_FAMILY_STRING)
        return false;
    if ((fa == TECC_TYPE_FAMILY_INT && fb == TECC_TYPE_FAMILY_FLOAT) ||
        (fb == TECC_TYPE_FAMILY_INT && fa == TECC_TYPE_FAMILY_FLOAT))
    {
        int ir = (fa == TECC_TYPE_FAMILY_INT) ? ra : rb;
        int fr = (fa == TECC_TYPE_FAMILY_FLOAT) ? ra : rb;

        /* float4×int4/int8、float8×int8 无拓宽路径 */
        if (ir >= 2 && fr <= ir)
            return false;
    }
    return true;
}

/*
 * 判断 TypeCast 是否为"裸 ::密态类型 cast"：
 * 目标类型是密态类型、未带 encrypted with (dek=...)、且最内层常量不是
 * NULL 或 \x 密文字面量。此类 cast 服务端无明文→密态 cast 行，必报错。
 */
static bool
stmt_type_cast_is_bare_enc(const TypeCast *tc, TeccStatementData *statement_data)
{
    char *typename_str;
    TeccTypeFamily family;
    const Node *inner;

    if (tc == NULL || tc->typeName == NULL || statement_data == NULL)
        return false;

    /* 带 encrypted with (dek=...) 的 cast 由加密重写路径处理 */
    if (get_typecast_dek_name(tc) != NULL)
        return false;

    typename_str = get_typecast_typename(tc, statement_data->query);
    if (typename_str == NULL)
        return false;
    family = get_enc_type_family(typename_str);
    pg_free(typename_str);
    if (family == TECC_TYPE_FAMILY_NONE)
        return false;

    /* 最内层常量：NULL cast 与 \x 密文字面量 cast 不算裸 cast */
    inner = (const Node *) tc->arg;
    while (inner != NULL && nodeTag(inner) == FE_T_TypeCast)
        inner = (const Node *) ((const TypeCast *) inner)->arg;
    if (inner != NULL && nodeTag(inner) == FE_T_Const)
    {
        const Const *c = (const Const *) inner;

        if (c->constisnull)
            return false;
        if (c->consttype == TEXTOID && c->constvalue.sval != NULL &&
            c->constvalue.sval[0] == '\\' && c->constvalue.sval[1] == 'x')
            return false;
    }
    return true;
}

/*
 * 递归检测节点树中是否含"裸 ::密态类型 cast"（覆盖 CASE 分支/switch 表达式）。
 */
bool
stmt_node_has_bare_enc_cast(Node *node, TeccStatementData *statement_data)
{
    if (node == NULL)
        return false;

    switch (nodeTag(node))
    {
        case FE_T_TypeCast:
            return stmt_type_cast_is_bare_enc((TypeCast *) node, statement_data);

        case FE_T_CaseExpr:
            {
                CaseExpr *ce = (CaseExpr *) node;
                ListCell *lc;

                if (stmt_node_has_bare_enc_cast((Node *) ce->arg, statement_data))
                    return true;
                if (stmt_node_has_bare_enc_cast((Node *) ce->defresult, statement_data))
                    return true;
                if (ce->args != NIL)
                {
                    foreach(lc, ce->args)
                    {
                        CaseWhen *cw = (CaseWhen *) lfirst(lc);

                        if (cw != NULL &&
                            stmt_node_has_bare_enc_cast((Node *) cw->result, statement_data))
                            return true;
                    }
                }
                return false;
            }

        default:
            return false;
    }
}

/*
 * 按列名/别名在 statement_data->encrypted_columns 中解析加密列，
 * 命中后取列缓存项（借用指针）。未命中返回 NULL。
 */
static TeccColumnCacheItem *
stmt_lookup_enc_column_cache(Node *col_node, TeccStatementData *statement_data)
{
    ColumnRef *col_ref;
    char *col_name;
    char *qualifier;
    int nf;
    ListCell *lc;

    if (col_node == NULL || nodeTag(col_node) != FE_T_ColumnRef ||
        statement_data == NULL)
        return NULL;

    col_ref = (ColumnRef *) col_node;
    if (col_ref->fields == NIL)
        return NULL;

    nf = list_length(col_ref->fields);
    col_name = strVal((Value *) list_nth(col_ref->fields, nf - 1));
    qualifier = (nf >= 2) ? strVal((Value *) list_nth(col_ref->fields, nf - 2)) : NULL;

    foreach(lc, statement_data->encrypted_columns)
    {
        TeccColumnRefData *ec = (TeccColumnRefData *) lc->data.ptr_value;

        if (ec == NULL || ec->column_name[0] == '\0')
            continue;
        if (strcmp(ec->column_name, col_name) != 0)
            continue;
        if (qualifier != NULL &&
            strcmp(ec->table_alias, qualifier) != 0 &&
            strcmp(ec->table_name, qualifier) != 0)
            continue;

        {
            TeccState *s = tecc_get_state(statement_data->conn);
            const char *db = (statement_data->conn != NULL) ?
                PQdb(statement_data->conn) : NULL;
            const char *schema = (s != NULL) ?
                tecc_cache_get_guc_param(s, "current_schema") : NULL;

            if (s != NULL && db != NULL && schema != NULL &&
                ec->table_name[0] != '\0')
            {
                return tecc_cache_get_encrypted_column_by_name(
                    s, db, schema, ec->table_name, col_name);
            }
        }
        break;
    }
    return NULL;
}

/*
 * 参数节点可归类的"常量类型族"（普通常量 / 普通类型 cast / UMINUS 包装）：
 * 用于与密态列的类别匹配校验。无法归类（NULL、密态 cast、嵌套函数等）返回 false。
 */
static bool
stmt_arg_const_family(Node *arg, TeccStatementData *statement_data,
                      TeccTypeFamily *family, int *rank)
{
    if (arg == NULL || family == NULL || rank == NULL)
        return false;

    if (nodeTag(arg) == FE_T_Const)
    {
        Const *c = (Const *) arg;

        if (c->constisnull)
            return false;

        /*
         * ReqID:TECC4.0 - 浮点字面量常量按数值族归类。
         *
         * 前端为保精度把 FLOAT 字面量原样保留为 TEXTOID 字符串常量
         * （见 tecc_grammar.y 的 FLOAT 规则 / Bug8178），其 consttype 是
         * TEXTOID。若此处一律按 OID 归类，会得到 STRING 族，导致
         * `GREATEST(numeric_enc 列, 500.00)` 被误判为"常量类型不匹配"。
         *
         * 这里用 text_numeric_literal 标记区分"数值文本形态"与真正的字符串
         * 字面量：前者按 NUMERIC 族处理（与显式 ::numeric cast 语义一致，
         * 可经拓宽格与 int/numeric/float 列比较）；后者（如 '123'）不受影响，
         * 仍按 STRING 族处理。
         */
        if (c->consttype == TEXTOID && c->text_numeric_literal)
        {
            *family = stmt_oid_to_type_family(NUMERICOID);
            *rank = stmt_oid_to_type_rank(NUMERICOID);
            return (*family != TECC_TYPE_FAMILY_NONE);
        }

        *family = stmt_oid_to_type_family(c->consttype);
        *rank = stmt_oid_to_type_rank(c->consttype);
        return (*family != TECC_TYPE_FAMILY_NONE);
    }

    if (nodeTag(arg) == FE_T_TypeCast)
    {
        TypeCast *tc = (TypeCast *) arg;
        char *tn = get_typecast_typename(tc, statement_data->query);
        bool ok = false;

        if (tn != NULL)
        {
            /* 密态类型 cast 由裸 cast / ew cast 分支处理，此处只归类普通类型 */
            if (get_enc_type_family(tn) == TECC_TYPE_FAMILY_NONE)
                ok = stmt_plain_type_family_and_rank(tn, family, rank);
            pg_free(tn);
        }
        return ok;
    }

    if (nodeTag(arg) == FE_T_A_Expr)
    {
        A_Expr *a = (A_Expr *) arg;

        /* UMINUS(常量) / UMINUS(TypeCast)：负号不影响类别 */
        if (a->lexpr == NULL && a->rexpr != NULL)
            return stmt_arg_const_family(a->rexpr, statement_data, family, rank);
    }

    return false;
}

/*
 * ReqID:TECC4.0 - 列存在性探测结论
 */
typedef enum StmtColProbeResult
{
    STMT_COL_PROBE_UNKNOWN = 0, /* 表不存在 / 无权限 / 探测失败 —— 不可判定 */
    STMT_COL_PROBE_EXISTS,      /* 表存在，且存在该列 */
    STMT_COL_PROBE_ABSENT       /* 表存在，但没有该列 */
} StmtColProbeResult;

/*
 * ReqID:TECC4.0 - 探测 <schema>.<table> 上是否存在名为 column_name 的列。
 *
 * 前端只有密态列元数据（hg_encrypted_columns_info），没有普通列清单，无法
 * 本地判断"某列不存在"。这里经数据获取器的独立临时连接查询 pg_attribute
 * （该连接 enable_tecc=false，不会递归触发 TECC 处理）。
 *
 * 返回 STMT_COL_PROBE_UNKNOWN 时调用方必须回退原有行为：表不存在、无权限、
 * 探测失败都不足以断定"列不存在"。
 */
static StmtColProbeResult
stmt_probe_column_exists(TeccStatementData *statement_data,
                         const char *schema, const char *table,
                         const char *column_name)
{
    TeccState *state;
    TeccCursorInterface *cursor;
    char *esc_schema;
    char *esc_table;
    char query[600];
    bool table_seen = false;
    bool column_seen = false;

    if (statement_data == NULL || statement_data->conn == NULL ||
        schema == NULL || schema[0] == '\0' ||
        table == NULL || table[0] == '\0' ||
        column_name == NULL || column_name[0] == '\0')
        return STMT_COL_PROBE_UNKNOWN;

    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
        return STMT_COL_PROBE_UNKNOWN;

    /* 注意：nspname/relname 比较的是字符串字面量，须用 PQescapeLiteral
     * （PQescapeIdentifier 产出的是双引号标识符，在这里会被当成列名）。 */
    esc_schema = PQescapeLiteral(statement_data->conn, schema, strlen(schema));
    esc_table = PQescapeLiteral(statement_data->conn, table, strlen(table));
    if (esc_schema == NULL || esc_table == NULL)
    {
        if (esc_schema != NULL)
            PQfreemem(esc_schema);
        if (esc_table != NULL)
            PQfreemem(esc_table);
        return STMT_COL_PROBE_UNKNOWN;
    }

    snprintf(query, sizeof(query),
             "SELECT a.attname FROM pg_catalog.pg_attribute a "
             "JOIN pg_catalog.pg_class c ON c.oid = a.attrelid "
             "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
             "WHERE n.nspname = %s AND c.relname = %s "
             "AND a.attnum > 0 AND NOT a.attisdropped",
             esc_schema, esc_table);

    PQfreemem(esc_schema);
    PQfreemem(esc_table);

    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
        return STMT_COL_PROBE_UNKNOWN;

    while (cursor->fetch_next(cursor) == TECC_FETCHER_OK)
    {
        const char *attname = cursor->get_column(cursor, 0);

        table_seen = true; /* 有行即表明该表存在 */
        if (attname != NULL && strcmp(attname, column_name) == 0)
        {
            column_seen = true;
            break;
        }
    }

    cursor->close(cursor);
    cursor->destroy(cursor);

    if (column_seen)
        return STMT_COL_PROBE_EXISTS;
    if (table_seen)
        return STMT_COL_PROBE_ABSENT;
    return STMT_COL_PROBE_UNKNOWN;
}

/*
 * ReqID:TECC4.0 - FUNCALL 参数中是否引用了"确认不存在"的列。
 *
 * 仅在即将报 ENC_PLAIN_MIX（密态列 × 普通列不能混用）时调用：查不到密态列
 * 元数据的列，既可能是真实存在的普通列（应报混用），也可能是根本不存在的列
 * （应让服务端报 column "..." does not exist）。
 *
 * 探测到任一"确认不存在"的列即返回 true，由调用方跳过本次参数组合校验、把语句
 * 交还服务端；探测不确定时一律返回 false，保持原有行为不变。
 */
static bool
stmt_args_reference_missing_column(FuncCall *fc,
                                   TeccStatementData *statement_data)
{
    ListCell *lc;

    if (fc == NULL || statement_data == NULL)
        return false;

    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        ColumnRef *col_ref;
        const char *col_name;
        const char *qualifier = NULL;
        ListCell *lc2;
        int nf;
        bool any_exists = false;
        bool any_absent = false;

        if (arg == NULL || nodeTag(arg) != FE_T_ColumnRef)
            continue;

        /* 能解析到密态列元数据的参数不是"普通列"候选 */
        if (stmt_lookup_enc_column_cache(arg, statement_data) != NULL)
            continue;

        col_ref = (ColumnRef *) arg;
        if (col_ref->fields == NIL)
            continue;

        nf = list_length(col_ref->fields);
        col_name = strVal((Value *) list_nth(col_ref->fields, nf - 1));
        if (col_name == NULL || col_name[0] == '\0')
            continue;
        if (nf >= 2)
            qualifier = strVal((Value *) list_nth(col_ref->fields, nf - 2));

        /* 在语句涉及的表中定位候选（限定符同时比对表别名与表名） */
        foreach(lc2, statement_data->encrypted_columns)
        {
            TeccColumnRefData *ec = (TeccColumnRefData *) lc2->data.ptr_value;
            const char *schema;
            StmtColProbeResult probe;

            if (ec == NULL || ec->table_name[0] == '\0')
                continue;
            if (qualifier != NULL &&
                strcmp(ec->table_alias, qualifier) != 0 &&
                strcmp(ec->table_name, qualifier) != 0)
                continue;

            schema = tecc_stmt_resolve_table_schema(statement_data,
                                                    ec->schema_name,
                                                    ec->table_name);
            probe = stmt_probe_column_exists(statement_data, schema,
                                             ec->table_name, col_name);
            if (probe == STMT_COL_PROBE_EXISTS)
            {
                any_exists = true;
                break;
            }
            if (probe == STMT_COL_PROBE_ABSENT)
                any_absent = true;
        }

        /* 命中任一表即视为存在；全部探测成功且都不含该列才判定"不存在" */
        if (!any_exists && any_absent)
            return true;
    }

    return false;
}

/*
 * F 模板统一校验：coalesce/greatest/least 的参数组合。
 * 返回 false 表示已按固定格式设置错误信息，调用方须中止。
 */
bool
stmt_validate_scalar_arg_combos(FuncCall *fc, TeccStatementData *statement_data)
{
    const char *func_name;
    ListCell *lc;
    bool has_enc_col = false;
    bool has_plain_col = false;
    bool has_bare_enc_cast = false;
    bool has_ew_enc_cast = false;
    bool has_plain_const = false;
    bool enc_fam_set = false;
    TeccTypeFamily enc_fam = TECC_TYPE_FAMILY_NONE;
    int enc_rank = 0;
    bool fam_conflict = false;

    if (fc == NULL || statement_data == NULL ||
        fc->funcname == NIL || fc->args == NIL)
        return true;

    func_name = strVal((Value *) llast(fc->funcname));
    if (func_name == NULL)
        return true;
    if (strcasecmp(func_name, "coalesce") != 0 &&
        strcasecmp(func_name, "greatest") != 0 &&
        strcasecmp(func_name, "least") != 0 &&
        strcasecmp(func_name, "nullif") != 0 &&
        strcasecmp(func_name, "decode") != 0 &&
        strcasecmp(func_name, "power") != 0)
        return true;

    /*
     * 按函数定制校验档位（需求 4.4.1.4）：
     * - coalesce/greatest/least/nullif：全档（R1 裸 cast、R2 密态×普通列、
     *   R3 类型组合、R4 常量类别、R5 密态常量×普通常量）
     * - decode：R1/R5 + R2，常量类别仅校验搜索位置（奇数位，末位默认值为结果不查）
     * - power：仅 R1/R5（列相关拒绝由 power 既有校验器统一处理，保持其固定文案）
     */
    {
        bool is_decode = (strcasecmp(func_name, "decode") == 0);
        bool is_power = (strcasecmp(func_name, "power") == 0);
        bool full_combo = !is_decode && !is_power;

    /* 第一遍：参数形态收集 */
    foreach(lc, fc->args)
    {
        Node *arg = (Node *) lc->data.ptr_value;
        TeccTypeFamily fam;
        int rank;

        if (arg == NULL)
            continue;

        if (nodeTag(arg) == FE_T_TypeCast)
        {
            TypeCast *tc = (TypeCast *) arg;
            char *tn = get_typecast_typename(tc, statement_data->query);

            if (tn != NULL)
            {
                fam = get_enc_type_family(tn);
                if (fam != TECC_TYPE_FAMILY_NONE)
                {
                    rank = get_enc_type_rank(tn);
                    if (get_typecast_dek_name(tc) != NULL)
                    {
                        has_ew_enc_cast = true;
                        if (enc_fam_set)
                        {
                            if (!stmt_enc_families_widenable(enc_fam, enc_rank, fam, rank))
                                fam_conflict = true;
                        }
                        else
                        {
                            enc_fam = fam;
                            enc_rank = rank;
                            enc_fam_set = true;
                        }
                    }
                    else if (stmt_type_cast_is_bare_enc(tc, statement_data))
                    {
                        has_bare_enc_cast = true;
                    }
                }
                pg_free(tn);
            }
            continue;
        }

        if (nodeTag(arg) == FE_T_ColumnRef)
        {
            TeccColumnCacheItem *ci =
                stmt_lookup_enc_column_cache(arg, statement_data);

            if (ci != NULL)
            {
                has_enc_col = true;
                fam = stmt_oid_to_type_family(ci->original_type_oid);
                rank = stmt_oid_to_type_rank(ci->original_type_oid);
                if (enc_fam_set)
                {
                    if (!stmt_enc_families_widenable(enc_fam, enc_rank, fam, rank))
                        fam_conflict = true;
                }
                else
                {
                    enc_fam = fam;
                    enc_rank = rank;
                    enc_fam_set = true;
                }
            }
            else
            {
                has_plain_col = true;
            }
            continue;
        }

        if (nodeTag(arg) == FE_T_Const)
        {
            if (!((Const *) arg)->constisnull)
                has_plain_const = true;
            continue;
        }

        if (nodeTag(arg) == FE_T_A_Expr)
        {
            A_Expr *a = (A_Expr *) arg;

            if (a->lexpr == NULL && a->rexpr != NULL)
            {
                if (nodeTag(a->rexpr) == FE_T_Const)
                {
                    if (!((Const *) a->rexpr)->constisnull)
                        has_plain_const = true;
                }
                continue;
            }
            /* 其他运算表达式：跳过，由其自身路径处理 */
        }
        /* 嵌套 FuncCall 等：由其自身校验路径处理 */
    }

    /* 第二遍：判定（固定错误格式 F 模板） */
    if (has_bare_enc_cast)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
            stmt_f_template(), func_name,
            stmt_f_reason_text(STMT_F_REASON_BARE_CAST));
        return false;
    }

    if (has_enc_col)
    {
        if (has_plain_col && (full_combo || is_decode))
        {
            /*
             * ReqID:TECC4.0 - 查不到密态列元数据的列，可能是"根本不存在的列"，
             * 此时报"密态列与普通列不能混用"属误导。先向服务端确认一次：
             * 确认存在 → 维持下面的混用报错；确认不存在 → 跳过本次校验，
             * 交还服务端报 column "..." does not exist。探测不确定则维持原行为。
             */
            if (stmt_args_reference_missing_column(fc, statement_data))
                return true;

            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), func_name,
                stmt_f_reason_text(STMT_F_REASON_ENC_PLAIN_MIX));
            return false;
        }
        if (fam_conflict && full_combo)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), func_name,
                stmt_f_reason_text(STMT_F_REASON_TYPE_COMBO));
            return false;
        }

        /* 密态列 × 常量类别匹配校验（含普通类型 cast 常量）。
         * decode 仅校验搜索位置（奇数位参数，末位默认值为结果不查）；
         * power 不校验（列相关拒绝由 power 既有校验器统一处理）。 */
        if (full_combo || is_decode)
        {
            int arg_idx = 0;
            int n_args = list_length(fc->args);

            foreach(lc, fc->args)
            {
                Node *arg = (Node *) lc->data.ptr_value;
                TeccTypeFamily cfam;
                int crank;

                if (is_decode &&
                    (arg_idx % 2 == 0 || arg_idx == n_args - 1))
                {
                    arg_idx++;
                    continue;
                }
                arg_idx++;

                if (!stmt_arg_const_family(arg, statement_data, &cfam, &crank))
                    continue;
                if (cfam == TECC_TYPE_FAMILY_STRING)
                {
                    if (enc_fam != TECC_TYPE_FAMILY_STRING)
                    {
                        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                            stmt_f_template(), func_name,
                            stmt_f_reason_text(STMT_F_REASON_CONST_MISMATCH));
                        return false;
                    }
                }
                else
                {
                    if (enc_fam == TECC_TYPE_FAMILY_STRING)
                    {
                        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                            stmt_f_template(), func_name,
                            stmt_f_reason_text(STMT_F_REASON_CONST_MISMATCH));
                        return false;
                    }
                    if (!stmt_enc_families_widenable(enc_fam, enc_rank, cfam, crank))
                    {
                        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                            stmt_f_template(), func_name,
                            stmt_f_reason_text(STMT_F_REASON_TYPE_COMBO));
                        return false;
                    }
                }
            }
        }
        return true;
    }

    if (has_ew_enc_cast)
    {
        if (has_plain_const)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), func_name,
                stmt_f_reason_text(STMT_F_REASON_ENC_PLAIN_CONST));
            return false;
        }
        if (fam_conflict && full_combo)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), func_name,
                stmt_f_reason_text(STMT_F_REASON_TYPE_COMBO));
            return false;
        }
    }

    return true;
    }
}

/*
 * Check that nested TypeCast nodes with encryptedOpts don't have conflicting
 * encrypted type families or DEK names. Cross-family casts (text_enc→int2_enc)
 * are rejected per Bug 7893/7894. Same-family casts with different DEKs
 * (text_enc→varchar_enc with dek_t1→dek_t2) are rejected per Bug 7901.
 * ReqID:TECC4.0 - Bug 7893/7901
 */
static bool
check_typecast_chain_nested_types(const TypeCast *type_cast,
                                  const char *orig_query,
                                  TeccStatementData *statement_data)
{
    char *outer_type;
    Node *arg_node;

    if (type_cast == NULL || type_cast->arg == NULL)
        return true;

    outer_type = get_typecast_typename(type_cast, orig_query);
    if (outer_type == NULL)
        return true;

    arg_node = type_cast->arg;

    while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
    {
        TypeCast *inner = (TypeCast *)arg_node;

        if (inner->typeName != NULL &&
            inner->typeName->encryptedOpts != NIL)
        {
            char *inner_type = get_typecast_typename(inner, orig_query);

            if (inner_type != NULL)
            {
                TeccTypeFamily inner_family;
                TeccTypeFamily outer_family;

                inner_family = get_enc_type_family(inner_type);
                outer_family = get_enc_type_family(outer_type);

                                /*
                 * Cross-family conversion check.
                 * Numeric families (INTEGER, FLOAT, NUMERIC) are inter-convertible
                 * with rank-based narrowing protection (e.g., int4_enc -> float8_enc
                 * is widening → allowed; float8_enc -> int4_enc is narrowing → rejected).
                 * Other cross-family casts (e.g., text_enc -> int2_enc) are rejected.
                 * ReqID:TECC4.0 - Bug 7893/7993
                 */
                if (inner_family != TECC_TYPE_FAMILY_NONE &&
                    outer_family != TECC_TYPE_FAMILY_NONE &&
                    inner_family != outer_family)
                {
                    bool inner_is_numeric = (inner_family == TECC_TYPE_FAMILY_INT ||
                                             inner_family == TECC_TYPE_FAMILY_FLOAT ||
                                             inner_family == TECC_TYPE_FAMILY_NUMERIC);
                    bool outer_is_numeric = (outer_family == TECC_TYPE_FAMILY_INT ||
                                             outer_family == TECC_TYPE_FAMILY_FLOAT ||
                                             outer_family == TECC_TYPE_FAMILY_NUMERIC);

                    if (!inner_is_numeric || !outer_is_numeric)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_ERROR,
                            "Cannot cast encrypted type %s to %s: "
                            "incompatible encrypted types",
                            inner_type, outer_type);
                        if (inner_type != NULL)
                            pg_free(inner_type);
                        if (outer_type != NULL)
                            pg_free(outer_type);
                        return false;
                    }

                    /* Cross-numeric-family narrowing check: reject if inner type is
                     * wider than outer type (e.g., float8_enc -> int4_enc). */
                    {
                        int inner_rank = get_enc_type_rank(inner_type);
                        int outer_rank = get_enc_type_rank(outer_type);
                        if (inner_rank > 0 && outer_rank > 0 &&
                            inner_rank > outer_rank)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "Cannot cast encrypted type %s to %s: "
                                "narrowing conversion would lose data",
                                inner_type, outer_type);
                            if (inner_type != NULL)
                                pg_free(inner_type);
                            if (outer_type != NULL)
                                pg_free(outer_type);
                            return false;
                        }
                    }
                }

                /* Same-family with different DEK → reject */
                if (inner_family != TECC_TYPE_FAMILY_NONE &&
                    outer_family != TECC_TYPE_FAMILY_NONE &&
                    inner_family == outer_family)
                {
                    const char *inner_dek;
                    const char *outer_dek;

                    inner_dek = get_typecast_dek_name(inner);
                    outer_dek = get_typecast_dek_name(type_cast);

                    if (inner_dek != NULL && outer_dek != NULL &&
                        strcasecmp(inner_dek, outer_dek) != 0)
                    {
                        tecc_processor_set_error(statement_data,
                            TECC_PROCESSOR_ERROR,
                            "Cannot cast encrypted type %s to %s "
                            "with different DEKs (%s vs %s)",
                            inner_type, outer_type,
                            inner_dek, outer_dek);
                        if (inner_type != NULL)
                            pg_free(inner_type);
                        if (outer_type != NULL)
                            pg_free(outer_type);
                        return false;
                    }

                    /* Same-family narrowing check: reject if inner type is
                     * wider than outer type (e.g., int4_enc -> int2_enc).
                     * ReqID:TECC4.0 - Bug 7992 */
                    {
                        int inner_rank = get_enc_type_rank(inner_type);
                        int outer_rank = get_enc_type_rank(outer_type);
                        if (inner_rank > 0 && outer_rank > 0 &&
                            inner_rank > outer_rank)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "Cannot cast encrypted type %s to %s: "
                                "narrowing conversion would lose data",
                                inner_type, outer_type);
                            if (inner_type != NULL)
                                pg_free(inner_type);
                            if (outer_type != NULL)
                                pg_free(outer_type);
                            return false;
                        }
                    }
                }
            }
            if (inner_type != NULL)
                pg_free(inner_type);
        }
        arg_node = inner->arg;
    }

    if (outer_type != NULL)
        pg_free(outer_type);
    return true;
}
/* End - ReqID:TECC4.0 */

/*
 * Find the comparison context (column reference and operator) for a TypeCast
 * node within a WHERE clause tree. Walks the expression tree looking for the
 * parent A_Expr that contains this TypeCast on one side and a ColumnRef on
 * the other. Returns true and sets col_ref/operator_name if found.
 * ReqID:TECC4.0 - Bug 8001
 */
static bool
find_where_comparison_context(Node *where_clause, TypeCast *type_cast,
                              ColumnRef **col_ref, const char **operator_name)
{
    if (where_clause == NULL || type_cast == NULL || col_ref == NULL || operator_name == NULL)
        return false;

    switch (nodeTag(where_clause))
    {
        case FE_T_A_Expr:
        {
            A_Expr *ae = (A_Expr *)where_clause;

            if (ae->lexpr == (Node *)type_cast || ae->rexpr == (Node *)type_cast)
            {
                Node *other = (ae->lexpr == (Node *)type_cast)
                    ? ae->rexpr : ae->lexpr;

                if (other == NULL)
                    return false;

                /* Unwrap CollateClause (e.g., name COLLATE "C") */
                if (nodeTag(other) == FE_T_CollateClause)
                    other = ((CollateClause *)other)->arg;

                if (nodeTag(other) != FE_T_ColumnRef)
                    return false;

                *col_ref = (ColumnRef *)other;
                *operator_name = (ae->name != NIL)
                    ? strVal((Value *)llast(ae->name)) : NULL;
                return true;
            }

            /* Recurse */
            if (ae->lexpr != NULL &&
                find_where_comparison_context(ae->lexpr, type_cast, col_ref, operator_name))
                return true;
            if (ae->rexpr != NULL &&
                find_where_comparison_context(ae->rexpr, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        case FE_T_BoolExpr:
        {
            struct BoolExpr *be = (struct BoolExpr *)where_clause;
            ListCell *lc;
            foreach (lc, be->args)
            {
                if (find_where_comparison_context((Node *)lc->data.ptr_value,
                                                  type_cast, col_ref, operator_name))
                    return true;
            }
            break;
        }
        case FE_T_NullTest:
        {
            NullTest *nt = (NullTest *)where_clause;
            if (nt->arg != NULL &&
                find_where_comparison_context(nt->arg, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        case FE_T_BooleanTest:
        {
            struct BooleanTest *bt = (struct BooleanTest *)where_clause;
            if (bt->arg != NULL &&
                find_where_comparison_context(bt->arg, type_cast, col_ref, operator_name))
                return true;
            break;
        }
        default:
            break;
    }
    return false;
}

/*
 * Map a SQL operator name string to TeccOperatorType enum.
 * ReqID:TECC4.0 - Bug 8001
 */
static TeccOperatorType
map_operator_name_to_type(const char *operator_name)
{
    if (operator_name == NULL) return TECC_OP_UNKNOWN;
    if (strcmp(operator_name, "=") == 0) return TECC_OP_EQUAL;
    if (strcmp(operator_name, "<>") == 0 || strcmp(operator_name, "!=") == 0)
        return TECC_OP_NOT_EQUAL;
    if (strcmp(operator_name, "<") == 0) return TECC_OP_LESS_THAN;
    if (strcmp(operator_name, ">") == 0) return TECC_OP_GREATER_THAN;
    if (strcmp(operator_name, "<=") == 0) return TECC_OP_LESS_EQUAL;
    if (strcmp(operator_name, ">=") == 0) return TECC_OP_GREATER_EQUAL;
    if (pg_strcasecmp(operator_name, "like") == 0) return TECC_OP_LIKE;
    if (pg_strcasecmp(operator_name, "not like") == 0) return TECC_OP_NOT_LIKE;
    if (pg_strcasecmp(operator_name, "ilike") == 0) return TECC_OP_ILIKE;
    if (pg_strcasecmp(operator_name, "not ilike") == 0) return TECC_OP_NOT_ILIKE;
    return TECC_OP_UNKNOWN;
}

/*
 * ReqID:TECC4.0 - Bug 9327
 * 取连接上的客户端编码，供字符计数使用；无连接时按 UTF-8（前端文本管线默认）。
 */
static int
stmt_client_encoding(TeccStatementData *statement_data)
{
    if (statement_data != NULL && statement_data->conn != NULL)
        return PQclientEncoding(statement_data->conn);
    return PG_UTF8;
}

/*
 * ReqID:TECC4.0 - Bug 9327
 * 把加密失败的原因带到语句级错误上。
 *
 * 调度器按目标列声明的长度拒收时返回 NULL，原因只记在连接的错误状态里；直接返回
 * 失败码会让用户只看到"处理失败"，看不出是长度超限。仅在语句尚无更具体错误时写入。
 */
static void
stmt_report_encrypt_failure(TeccStatementData *statement_data,
                            const TeccUnifiedEncryptResult *encrypt_result)
{
    const char *reason = NULL;

    if (statement_data == NULL || statement_data->error_message[0] != '\0')
        return;

    if (encrypt_result != NULL && encrypt_result->error_msg[0] != '\0')
        reason = encrypt_result->error_msg;
    else if (statement_data->conn != NULL)
        reason = tecc_get_last_error(statement_data->conn);

    if (reason == NULL || reason[0] == '\0')
        return;

    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                             "Encryption failed: %s", reason);
}

/*
 * ReqID:TECC4.0 - Bug 9327 写入侧长度校验（一）：cast 声明的长度。
 *
 * "值::varchar_enc(N) ENCRYPTED WITH (...)" 是【显式 cast】，原生 varchar(N) 在
 * 这里静默截断（超出部分是普通字符也截），服务端却只拿到密文、无从判断明文长度，
 * 所以必须在前端把这次截断做掉（判据见 tecc_char_cast_len_clip）。
 *
 * 嵌套 cast 由内往外逐层定型（原生也是每个 cast 各截一次），故先递归处理内层。
 */
static void
stmt_apply_cast_len_clip_node(Node *node, Oid base_oid,
                              char *plaintext, int encoding)
{
    TypeCast *type_cast;

    if (node == NULL || plaintext == NULL || nodeTag(node) != FE_T_TypeCast)
        return;

    type_cast = (TypeCast *) node;
    stmt_apply_cast_len_clip_node(type_cast->arg, base_oid, plaintext, encoding);

    if (type_cast->typeName != NULL)
        tecc_char_cast_len_clip(type_cast->typeName->typemod, base_oid,
                                plaintext, encoding);
}

static void
stmt_apply_cast_len_clip(TypeCast *type_cast, Oid base_oid,
                         char *plaintext, TeccStatementData *statement_data)
{
    if (type_cast == NULL || plaintext == NULL)
        return;

    stmt_apply_cast_len_clip_node((Node *) type_cast, base_oid, plaintext,
                                  stmt_client_encoding(statement_data));
}

/*
 * ReqID:TECC4.0 - Bug 9327
 * 取节点在列表中的下标（不存在返回 -1）。用于把 SELECT/INSERT 目标列表里的位置映射
 * 到 INSERT 的列位置 —— 目标列表可能含不参与映射的节点，按下标遍历容易与 continue
 * 相互作用出错，故按指针回查。
 */
static int
stmt_node_index_in_list(List *list, const Node *node)
{
    ListCell *lc;
    int         index = 0;

    if (list == NIL || node == NULL)
        return -1;

    foreach (lc, list)
    {
        if (lc->data.ptr_value == (void *) node)
            return index;
        index++;
    }

    return -1;
}

/*
 * ReqID:TECC4.0 - Bug 9327 写入侧长度校验（二）：目标列声明的长度。
 *
 * 按 INSERT 的列位置解析出目标密态列。有显式列表时用列表里第 position 个列名匹配，
 * 没有列表时按列的 attnum（position + 1）匹配 —— 与 INSERT 的列位置语义一致。
 * 与目标列关联的（类型）长度校验由调度器按返回元数据里的 typmod 完成：把
 * original_type_mod 作为 type_mod 传给加密入口，即得到与服务端一致的报错。
 *
 * out_column_name（可为 NULL）返回解析出的列名，供报错信息使用。
 */
static TeccColumnCacheItem *
stmt_resolve_insert_target_column(const InsertStmt *insert_stmt,
                                  TeccColumnCacheItem **table_columns,
                                  int table_column_count,
                                  int position,
                                  const char **out_column_name)
{
    const char *col_name = NULL;
    int         i;

    if (out_column_name != NULL)
        *out_column_name = NULL;

    if (insert_stmt == NULL || table_columns == NULL || position < 0)
        return NULL;

    if (insert_stmt->cols != NIL && position < list_length(insert_stmt->cols))
    {
        Node *col_node = (Node *) list_nth(insert_stmt->cols, position);

        if (col_node != NULL && col_node->type == FE_T_ResTarget)
            col_name = ((ResTarget *) col_node)->name;
        else if (col_node != NULL && col_node->type == FE_T_String)
            col_name = (const char *) col_node;
    }

    for (i = 0; i < table_column_count; i++)
    {
        TeccColumnCacheItem *item = table_columns[i];

        if (item == NULL)
            continue;

        if (col_name != NULL && item->column_name != NULL)
        {
            if (strcmp(item->column_name, col_name) != 0)
                continue;
        }
        else if (item->column_attnum != position + 1)
            continue;

        if (out_column_name != NULL)
            *out_column_name = item->column_name;
        return item;
    }

    return NULL;
}

/*
 * ReqID:TECC4.0
 * 把一条已加密的带 ENCRYPTED WITH 的 cast 登记为 raw_values_list 里的
 * is_sql_expression 替换项（location/data_size 指向原 SQL 里整段 cast 文本）。
 *
 * 与普通常量走同一次从左到右的 SQL 文本改写，避免"cast 改写成功就整体跳过
 * values 处理器"导致同句里写进其它加密列的普通常量不被加密。
 */
static bool
stmt_add_typecast_raw_value(TeccStatementData *statement_data,
                            const TypeCast *type_cast,
                            const char *orig_query,
                            size_t orig_len,
                            const TeccUnifiedEncryptResult *encrypt_result)
{
    size_t expr_start;
    size_t expr_end;
    char *typename_str;
    const char *bare_hex;
    size_t bare_len;
    size_t repl_cap;
    size_t repl_len;
    char *replacement;
    TeccRawValue *rv;

    if (statement_data == NULL || type_cast == NULL ||
        orig_query == NULL || encrypt_result == NULL ||
        encrypt_result->hex_string == NULL)
        return false;

    expr_start = (size_t) type_cast->location;
    expr_end = find_typecast_expr_end(orig_query, type_cast);
    if (expr_start >= orig_len || expr_end > orig_len ||
        expr_end <= expr_start)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid TypeCast expression positions: start=%zu end=%zu",
                           expr_start, expr_end);
        return false;
    }

    bare_hex = encrypt_result->hex_string;
    bare_len = encrypt_result->hex_len;
    if (bare_len >= 2 && bare_hex[0] == '\\' && bare_hex[1] == 'x')
    {
        bare_hex += 2;
        bare_len -= 2;
    }

    /* 替换文本：E'\\x<hex>'::typename —— 与原 cast 整段等价的密文表达式 */
    typename_str = get_typecast_typename(type_cast, orig_query);
    repl_cap = bare_len + 16 +
               ((typename_str != NULL) ? strlen(typename_str) : 0);
    replacement = (char *) pg_malloc(repl_cap);
    if (replacement == NULL)
    {
        if (typename_str != NULL)
            pg_free(typename_str);
        return false;
    }
    repl_len = 0;
    replacement[repl_len++] = 'E';
    replacement[repl_len++] = '\'';
    replacement[repl_len++] = '\\';
    replacement[repl_len++] = '\\';
    replacement[repl_len++] = 'x';
    memcpy(replacement + repl_len, bare_hex, bare_len);
    repl_len += bare_len;
    replacement[repl_len++] = '\'';
    if (typename_str != NULL && typename_str[0] != '\0')
    {
        size_t tlen = strlen(typename_str);
        replacement[repl_len++] = ':';
        replacement[repl_len++] = ':';
        memcpy(replacement + repl_len, typename_str, tlen);
        repl_len += tlen;
        pg_free(typename_str);
        typename_str = NULL;
    }

    /* ReqID:TECC4.0 - PREPARE 体改写时把原样的 ENCRYPTED WITH (...) 拼回去。
     *
     * 上面算出的 expr_start..expr_end 覆盖的是**整段** cast（含 ENCRYPTED WITH，
     * 见 find_typecast_expr_end），而替换文本只写了 E'\x..'::type，于是子句随
     * 替换被删掉。子句是服务端在 CREATE TABLE ... AS EXECUTE 时派生结果表列加密
     * 元数据的唯一来源（transformCreateTableAsStmt 从原始解析树里取），丢了就再也
     * 拿不回来。这里把原文本里从 ENCRYPTED 关键字到 expr_end 的那一段原样接上。
     *
     * 定位方式：在 cast 区间内跳过单引号字符串后找第一个 ENCRYPTED 单词，
     * 这样常量或选项值里出现 "encrypted" 字样也不会误判。 */
    if (statement_data->keep_encrypted_with_clause)
    {
        size_t  clause_start = 0;
        size_t  si;
        bool    in_string = false;
        bool    found = false;

        for (si = expr_start; si + 9 <= expr_end; si++)
        {
            if (orig_query[si] == '\'')
            {
                /* '' 是字符串里的转义引号，不改变状态 */
                if (in_string && si + 1 < expr_end && orig_query[si + 1] == '\'')
                {
                    si++;
                    continue;
                }
                in_string = !in_string;
                continue;
            }
            if (in_string)
                continue;
            if (pg_strncasecmp(orig_query + si, "encrypted", 9) == 0)
            {
                clause_start = si;
                found = true;
                break;
            }
        }

        if (found)
        {
            size_t  clause_len = expr_end - clause_start;

            if (clause_len > 0)
            {
                replacement = (char *) pg_realloc(
                    replacement, repl_len + 1 + clause_len + 1);
                replacement[repl_len++] = ' ';
                memcpy(replacement + repl_len, orig_query + clause_start,
                       clause_len);
                repl_len += clause_len;
            }
        }
    }

    replacement[repl_len] = '\0';

    /* ReqID:TECC4.0 - 同一条 cast 可能被登记两次：INSERT ... SELECT 先由
     * tecc_stmt_processor_process_select 以"无目标列"上下文跑一遍本处理，随后
     * INSERT 分支再带目标列上下文跑一遍（目标列长度校验与排序规则只在后一遍有）。
     * 旧实现靠"后一遍的 rewritten_query 覆盖前一遍"隐式去重，这里保持同一语义：
     * 命中同一区间就换成后一遍的加密产物，避免同一段文本被替换两次。
     * 顺序固定为"先无目标列、后有目标列"，故后者胜出即是所需结果。 */
    if (statement_data->raw_values_list != NULL)
    {
        int prev_n = tecc_raw_values_list_count(statement_data->raw_values_list);
        int prev_i;

        for (prev_i = 0; prev_i < prev_n; prev_i++)
        {
            TeccRawValue *prev_rv = tecc_raw_values_list_get(
                statement_data->raw_values_list, prev_i);

            if (prev_rv == NULL || !prev_rv->is_sql_expression)
                continue;
            if (prev_rv->location != expr_start ||
                prev_rv->data_size != expr_end - expr_start)
                continue;

            if (prev_rv->state == TECC_RAW_VALUE_STATE_ENCRYPTED &&
                prev_rv->processed.encrypted_value != NULL)
                pg_free(prev_rv->processed.encrypted_value);
            prev_rv->state = TECC_RAW_VALUE_STATE_ENCRYPTED;
            prev_rv->processed.encrypted_value = replacement;
            statement_data->needs_rewrite = 1;
            return true;
        }
    }

    rv = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
    if (rv == NULL)
    {
        pg_free(replacement);
        return false;
    }
    rv->state = TECC_RAW_VALUE_STATE_ENCRYPTED;
    rv->is_sql_expression = true;
    rv->location = expr_start;
    rv->data_size = expr_end - expr_start;
    rv->processed.encrypted_value = replacement;

    if (statement_data->raw_values_list == NULL)
        statement_data->raw_values_list = tecc_raw_values_list_create(8);
    if (statement_data->raw_values_list == NULL ||
        tecc_raw_values_list_add(statement_data->raw_values_list, rv) < 0)
    {
        tecc_raw_value_destroy(rv);
        return false;
    }
    statement_data->needs_rewrite = 1;
    return true;
}

/*
 * Process a single TypeCast node with encryptedOpts: extract the constant,
 * encrypt it, and append the encrypted value to the rewritten query.
 *
 * dest_col 是本位置对应的 INSERT 目标列元数据（语义见
 * stmt_resolve_insert_target_column）；目标列不可知时传 NULL，表示只做 cast
 * 自带长度的定型。
 *
 * 目标列已知时还要把【真实的表名/列名】传给加密入口，而不是占位的
 * "insert_select"/类型名 —— 调度器据此从元数据取列排序规则，DET/OPE 才能算出与
 * 列自身路径一致的 ICU 排序键。否则非 C/POSIX 排序规则的库里，cast 写进去的值与
 * 该列的等值比较密文不相等，等值查询会静默查不到（详见
 * tecc_unified_encrypt_string_with_options 里同一处查找的注释）。
 *
 * 排序规则只在本语句【没有目标表】时才按"新列继承库默认排序规则"解析
 * （statement_data->has_insert_target 为假）。INSERT ... VALUES / INSERT ...
 * SELECT 有目标表：这时列真实存在，排序规则由列自己决定，按库默认去猜会让
 * 显式 COLLATE "C"/"POSIX" 的列从"恰好一致"变成不一致。
 */
static TeccStmtProcessorResult
process_one_typecast(TeccStatementData *statement_data,
                     TypeCast *type_cast,
                     const char *orig_query,
                     size_t orig_len,
                     int *typecast_processed,
                     TeccColumnCacheItem *dest_col)
{
    char *dek_name = NULL;
    const char *he_add_dek_name = NULL;
    const char *he_mul_dek_name = NULL;
    char plaintext[256];
    ListCell *opt_lc;
    TeccUnifiedEncryptResult *encrypt_result = NULL;

    /* Find DEK name from encrypted options */
    foreach (opt_lc, type_cast->typeName->encryptedOpts)
    {
        DefElem *elem = (DefElem *)opt_lc->data.ptr_value;
        if (elem != NULL &&
            (elem->defname != NULL && pg_strcasecmp(elem->defname, "dek") == 0))
        {
            if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                dek_name = strVal((const Value *)elem->arg);
            break;
        }
    }

    /* ReqID:TECC4.0 - 同态 DEK 名由同一条 ENCRYPTED WITH 给出：目标列还不存在时
     * （视图/物化视图定义里的 cast、纯 SELECT 的 cast）加密入口拿不到列元数据，
     * 同态 DEK 只能靠这里传下去（见 tecc_unified_encrypt_string_with_options_and_collation）。*/
    tecc_typename_get_he_dek_names(type_cast->typeName,
                                   &he_add_dek_name, &he_mul_dek_name);

    /* ReqID:TECC4.0 - 纯同态列没有对称 DEK（只写 HE_ADD_DEK/HE_MUL_DEK），与列路径
     * 同一处理：用同态 DEK 名当 dek_name，否则这类 cast 会被直接跳过。 */
    if (dek_name == NULL && he_add_dek_name != NULL)
        dek_name = (char *) he_add_dek_name;
    else if (dek_name == NULL && he_mul_dek_name != NULL)
        dek_name = (char *) he_mul_dek_name;

    if (dek_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "No DEK found in encrypted options for TypeCast");
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    /* Begin - ReqID:TECC4.0 - Bug 7893: reject nested TypeCasts with conflicting encrypted types */
    if (!check_typecast_chain_nested_types(type_cast, orig_query, statement_data))
        return TECC_STMT_PROCESSOR_ERROR;
    /* End - ReqID:TECC4.0 */

    /* Extract plaintext from the Const argument */
    {
        /* Walk through nested TypeCast chain to find innermost Const */
        Node *arg_node = type_cast->arg;
        Const *cnst;

        while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
            arg_node = ((TypeCast *)arg_node)->arg;

        cnst = (Const *)arg_node;
        if (cnst == NULL || cnst->type != FE_T_Const)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "TypeCast arg is not a Const, skipping");
            return TECC_STMT_PROCESSOR_SUCCESS;
        }

        /* Convert Const value to string based on type */
        if (cnst->constbyval && (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
        {
            /* Integer constant (int2 or int4) */
            snprintf(plaintext, sizeof(plaintext), "%d",
                     (int)cnst->constvalue.ival);
        }
        else if (cnst->constbyval && cnst->constlen == sizeof(float8))
        {
            /* Float constant */
            snprintf(plaintext, sizeof(plaintext), "%g",
                     cnst->constvalue.dval);
        }
        else if (cnst->constvalue.sval != NULL && !cnst->constisnull)
        {
            /* String constant */
            snprintf(plaintext, sizeof(plaintext), "%s",
                     cnst->constvalue.sval);
        }
        else if (cnst->constisnull)
        {
            /* NULL constant - just pass through */
            return TECC_STMT_PROCESSOR_SUCCESS;
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Unsupported Const type for encryption");
            return TECC_STMT_PROCESSOR_SUCCESS;
        }
    }

    /* Encrypt the value. 加密参数（算法/目标长度/排序规则来源的表列）见下：
     * 算法标志取自密态类型名与 ENCRYPTED WITH 里的 support_algo；目标列未知时用
     * with_options 的占位表/列名（TypeCast 表达式本身不在列元数据缓存里）。 */
    {
        char *typename_str;
        Oid base_oid;
        bool supports_det, supports_ope, supports_search, supports_he_add, supports_he_mul;
        const char *enc_table;
        const char *enc_column;
        int32 dest_typmod;
        char *collation_locale = NULL;

        typename_str = get_typecast_typename(type_cast, orig_query);
        base_oid = (typename_str != NULL) ?
            map_enc_type_name_to_base_oid(typename_str) : InvalidOid;
        get_typecast_algo_flags(type_cast, typename_str,
                                &supports_det, &supports_ope, &supports_search,
                                &supports_he_add, &supports_he_mul);

        /* ReqID:TECC4.0 - Bug 9327: 目标列已知时，按【真实表列】加密 —— 调度器
         * 据此取列的排序规则，DET/OPE 才能算出与列自身写入路径一致的 ICU 排序键；
         * 同时把列声明的长度交给调度器做赋值语义的严格校验。 */
        enc_table = (dest_col != NULL) ? dest_col->table_name : "insert_select";
        enc_column = (dest_col != NULL) ? dest_col->column_name
                                        : (typename_str ? typename_str : "typecast");
        dest_typmod = (dest_col != NULL) ? dest_col->original_type_mod : -1;

        /* ReqID:TECC4.0 - 语句没有目标表（纯 SELECT、CREATE VIEW / CREATE
         * MATERIALIZED VIEW 定义里的 cast）：这些 cast 声明出来的列还不存在，
         * 由 cast 类型建出、没有 COLLATE，继承的就是库默认排序规则 —— 显式解析
         * 出来交给加密入口，否则 DET/OPE 不做 ICU 排序键，密文与列自身路径不相等。
         * INSERT ... SELECT / VALUES 有目标表，排序规则以目标列自己为准，不在这里
         * 假设（见函数头注释）。 */
        if (dest_col == NULL && !statement_data->has_insert_target)
            collation_locale = resolve_new_column_collation_locale(
                statement_data->conn, base_oid, NULL);

        /* ReqID:TECC4.0 - Bug 9327: 先按 cast 声明的长度做显式 cast 的静默截断，
         * 目标列声明的长度则交给调度器按 dest_typmod 做赋值语义的严格校验
         * （超长且超出部分含非空格才拒绝），与原生 varchar/bpchar 的分工一致。 */
        stmt_apply_cast_len_clip(type_cast, base_oid, plaintext, statement_data);

        if (OidIsValid(base_oid))
        {
            encrypt_result = tecc_unified_encrypt_string_with_options_and_collation(
                statement_data->conn,
                plaintext,
                enc_table, enc_column,
                NULL, dek_name,
                base_oid, dest_typmod,
                supports_det, supports_ope, supports_search, supports_he_add, supports_he_mul,
                collation_locale, he_add_dek_name, he_mul_dek_name, NULL);
        }
        else
        {
            encrypt_result = tecc_unified_encrypt_string(
                statement_data->conn, plaintext,
                enc_table, enc_column,
                NULL, dek_name, NULL);
        }

        if (typename_str != NULL)
            pg_free(typename_str);
        if (collation_locale != NULL)
            pg_free(collation_locale);
    }

    if (encrypt_result == NULL ||
        encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        /* ReqID:TECC4.0 - Bug 9327: 按目标列长度拒收时调度器返回 NULL，具体原因
         * （如 value too long for type character varying(N)）在其错误记录里；
         * 取出来作为语句错误上抛，否则用户只看到"处理失败"、不知为何。 */
        stmt_report_encrypt_failure(statement_data, encrypt_result);

        tecc_processor_debug(TECC_LOG_ERROR,
                           "Encryption failed for TypeCast value: %s",
                           encrypt_result ? encrypt_result->error_msg : "unknown error");
        if (encrypt_result)
            tecc_unified_free_encrypt_result(encrypt_result);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* ReqID:TECC4.0 - 登记为 is_sql_expression raw value，与普通常量在
     * tecc_values_processor_rewrite_values 里一次从左到右改写完成。
     * 不再就地拼 new_query / 设置 rewritten_query，否则同句里写进其它
     * 加密列的普通常量会被整体跳过 values 处理器而不加密。 */
    if (!stmt_add_typecast_raw_value(statement_data, type_cast, orig_query,
                                     orig_len, encrypt_result))
    {
        tecc_unified_free_encrypt_result(encrypt_result);
        return TECC_STMT_PROCESSOR_ERROR;
    }
    tecc_unified_free_encrypt_result(encrypt_result);

    (*typecast_processed)++;
    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process INSERT...SELECT encrypted TypeCast expressions.
 * For statements like: INSERT INTO enc_table SELECT 1::int2_enc ENCRYPTED WITH (dek=...)
 * This function encrypts the constant values and rewrites the query.
 *
 * insert_stmt / table_columns（可为 NULL）是 INSERT 的目标表上下文，用于把密态 cast
 * 所在的位置映射到目标列，从而按目标列声明的长度做校验（Bug 9327）；非 INSERT 场景
 * 传 NULL，此时只按 cast 自身声明的长度定型。
 */
static TeccStmtProcessorResult
process_insert_select_typecast(SelectStmt *select_stmt,
                              TeccStatementData *statement_data,
                              const InsertStmt *insert_stmt,
                              TeccColumnCacheItem **table_columns,
                              int table_column_count)
{
    ListCell *lc;
    const char *orig_query;
    size_t orig_len;
    int typecast_processed = 0;

    if (select_stmt == NULL ||
        (select_stmt->targetList == NIL && select_stmt->valuesLists == NIL))
        return TECC_STMT_PROCESSOR_SUCCESS;

    orig_query = statement_data->query;
    if (orig_query == NULL)
    {
        /* ReqID:TECC4.0 - Subquery (e.g., RangeSubselect in FROM) has no original query
         * string; skip TypeCast rewriting since there is nothing to rewrite. */
        return TECC_STMT_PROCESSOR_SUCCESS;
    }

    orig_len = strlen(orig_query);

    /* Build rewritten query by scanning from left to right */
    if (select_stmt->targetList != NIL)
    {
    for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next)
    {
        Node *target_node = (Node *)lc->data.ptr_value;
        ResTarget *res_target;

        if (target_node == NULL || target_node->type != FE_T_ResTarget)
            continue;

        res_target = (ResTarget *)target_node;
        if (res_target->val == NULL)
            continue;

        /* Collect all TypeCast nodes with encryptedOpts from this target,
         * including those nested inside function arguments, subexpressions, etc. */
        {
            List *typecast_nodes = NIL;
            ListCell *tc_lc;

            collect_typecast_nodes(res_target->val, &typecast_nodes);

            if (typecast_nodes == NIL)
                continue;

            foreach (tc_lc, typecast_nodes)
            {
                TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;

                /* Found a TypeCast with encrypted options */
                {
                char *dek_name = NULL;
                const char *he_add_dek_name = NULL;
                const char *he_mul_dek_name = NULL;
                char plaintext[256];
                ListCell *opt_lc;
                TeccUnifiedEncryptResult *encrypt_result = NULL;

                /* Find DEK name from encrypted options */
            foreach (opt_lc, type_cast->typeName->encryptedOpts)
            {
                DefElem *elem = (DefElem *)opt_lc->data.ptr_value;
                if (elem != NULL &&
                    (elem->defname != NULL && pg_strcasecmp(elem->defname, "dek") == 0))
                {
                    if (elem->arg != NULL && nodeTag(elem->arg) == FE_T_String)
                        dek_name = strVal((const Value *)elem->arg);
                    break;
                }
            }

            /* ReqID:TECC4.0 - 同态 DEK 名同 process_one_typecast：目标列还不存在时
             * 只能由这里从 ENCRYPTED WITH 解析后传给加密入口。 */
            tecc_typename_get_he_dek_names(type_cast->typeName,
                                           &he_add_dek_name, &he_mul_dek_name);
            if (dek_name == NULL && he_add_dek_name != NULL)
                dek_name = (char *) he_add_dek_name;
            else if (dek_name == NULL && he_mul_dek_name != NULL)
                dek_name = (char *) he_mul_dek_name;

            if (dek_name == NULL)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "No DEK found in encrypted options for TypeCast");
                continue;
            }

            /* Begin - ReqID:TECC4.0 - Bug 7893: reject nested TypeCasts with conflicting encrypted types */
            if (!check_typecast_chain_nested_types(type_cast, orig_query, statement_data))
            {
                return TECC_STMT_PROCESSOR_ERROR;
            }
            /* End - ReqID:TECC4.0 */

            /* Extract plaintext from the Const argument */
            {
                /* Walk through nested TypeCast chain to find innermost Const */
                Node *arg_node = type_cast->arg;
                Const *cnst;

                while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
                    arg_node = ((TypeCast *)arg_node)->arg;

                cnst = (Const *)arg_node;
                if (cnst == NULL || cnst->type != FE_T_Const)
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "TypeCast arg is not a Const, skipping");
                    continue;
                }

                /* Convert Const value to string based on type */
                if (cnst->constbyval && (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
                {
                    /* Integer constant */
                    snprintf(plaintext, sizeof(plaintext), "%d",
                             (int)cnst->constvalue.ival);
                }
                else if (cnst->constbyval && cnst->constlen == sizeof(float8))
                {
                    /* Float constant */
                    snprintf(plaintext, sizeof(plaintext), "%g",
                             cnst->constvalue.dval);
                }
                else if (cnst->constvalue.sval != NULL && !cnst->constisnull)
                {
                    /* String constant */
                    snprintf(plaintext, sizeof(plaintext), "%s",
                             cnst->constvalue.sval);
                }
                else if (cnst->constisnull)
                {
                    /* NULL constant - just pass through */
                    continue;
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Unsupported Const type for encryption");
                    continue;
                }
            }

            /* Encrypt the value */
            {
                char *typename_str = get_typecast_typename(type_cast, orig_query);
                Oid base_oid = (typename_str != NULL) ?
                    map_enc_type_name_to_base_oid(typename_str) : InvalidOid;
                bool supports_det, supports_ope, supports_search, supports_he_add, supports_he_mul;
                /* ReqID:TECC4.0 - Bug 9327: 本位置对应的 INSERT 目标列元数据 ——
                 * 长度校验与排序规则（密文一致性）都取自它 */
                TeccColumnCacheItem *dest_col = NULL;
                const char *enc_table;
                const char *enc_column;
                int32 dest_typmod;
                char *collation_locale = NULL;

                get_typecast_algo_flags(type_cast, typename_str,
                                        &supports_det, &supports_ope, &supports_search,
                                        &supports_he_add, &supports_he_mul);

                dest_col = stmt_resolve_insert_target_column(
                    insert_stmt, table_columns, table_column_count,
                    stmt_node_index_in_list(select_stmt->targetList, target_node), NULL);

                enc_table = (dest_col != NULL) ? dest_col->table_name : "insert_select";
                enc_column = (dest_col != NULL) ? dest_col->column_name
                                                : (typename_str ? typename_str : "typecast");
                dest_typmod = (dest_col != NULL) ? dest_col->original_type_mod : -1;

                /* ReqID:TECC4.0 - 语句没有目标表（纯 SELECT、视图/物化视图定义里的
                 * cast）：这些列由 cast 声明类型建出、没有 COLLATE，继承的是库默认
                 * 排序规则，显式解析出来避免 DET/OPE 密文与列自身路径不等。
                 * INSERT ... SELECT 有目标表则排序规则以目标列自己为准，不在这里
                 * 假设（与 process_one_typecast 同一判据）。 */
                if (dest_col == NULL && !statement_data->has_insert_target)
                    collation_locale = resolve_new_column_collation_locale(
                        statement_data->conn, base_oid, NULL);

                /* ReqID:TECC4.0 - Bug 9327: 先按 cast 声明的长度静默截断（显式 cast
                 * 语义），再由调度器按目标列长度做赋值语义的严格校验。 */
                stmt_apply_cast_len_clip(type_cast, base_oid, plaintext, statement_data);

                if (OidIsValid(base_oid))
                {
                    encrypt_result =
                        tecc_unified_encrypt_string_with_options_and_collation(
                            statement_data->conn, plaintext,
                            enc_table, enc_column,
                            NULL, dek_name,
                            base_oid, dest_typmod,
                            supports_det, supports_ope, supports_search, supports_he_add, supports_he_mul,
                            collation_locale, he_add_dek_name, he_mul_dek_name, NULL);
                }
                else
                {
                    encrypt_result = tecc_unified_encrypt_string(
                        statement_data->conn, plaintext,
                        enc_table, enc_column,
                        NULL, dek_name, NULL);
                }

                if (typename_str != NULL)
                    pg_free(typename_str);
                if (collation_locale != NULL)
                    pg_free(collation_locale);
            }

            if (encrypt_result == NULL ||
                encrypt_result->error_code != TECC_CRYPTO_SUCCESS)
            {
                /* ReqID:TECC4.0 - Bug 9327: 同 process_one_typecast，
                 * 把调度器记录的原因（长度超限等）带到语句错误上 */
                stmt_report_encrypt_failure(statement_data, encrypt_result);

                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Encryption failed for TypeCast value: %s",
                                   encrypt_result ? encrypt_result->error_msg : "unknown error");
                if (encrypt_result)
                    tecc_unified_free_encrypt_result(encrypt_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            /* ReqID:TECC4.0 - 登记为 is_sql_expression raw value（location/data_size
             * 指向整段 cast 原文），由 phase-2 与同句里的普通常量在一次从左到右的
             * 改写里一并完成。不再就地拼 new_query：本函数已不产出 rewritten_query
             * （见函数尾注释），就地改写的结果会被丢弃，等于这条 cast 没被加密。 */
            if (!stmt_add_typecast_raw_value(statement_data, type_cast, orig_query,
                                             orig_len, encrypt_result))
            {
                tecc_unified_free_encrypt_result(encrypt_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            tecc_unified_free_encrypt_result(encrypt_result);

            typecast_processed++;
            }
        }
    }
    }
    }

    /* Process valuesLists (INSERT...VALUES) — each row has a list of value expressions */
    if (select_stmt->valuesLists != NIL)
    {
        ListCell *row_lc;
        foreach (row_lc, select_stmt->valuesLists)
        {
            List *row = (List *)row_lc->data.ptr_value;
            ListCell *val_lc;
            if (row == NIL) continue;

            foreach (val_lc, row)
            {
                Node *val = (Node *)val_lc->data.ptr_value;
                if (val == NULL) continue;

                /* Values in INSERT...VALUES are direct expression nodes,
                 * not wrapped in ResTarget like targetList entries */
                {
                    List *typecast_nodes = NIL;
                    ListCell *tc_lc;
                    /* ReqID:TECC4.0 - Bug 9327: 本值所在位置对应的 INSERT 列 */
                    TeccColumnCacheItem *dest_col;

                    collect_typecast_nodes(val, &typecast_nodes);

                    if (typecast_nodes == NIL)
                        continue;

                    dest_col = stmt_resolve_insert_target_column(
                        insert_stmt, table_columns, table_column_count,
                        stmt_node_index_in_list(row, val), NULL);

                    foreach (tc_lc, typecast_nodes)
                    {
                        TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;

                        TeccStmtProcessorResult res = process_one_typecast(
                            statement_data, type_cast, orig_query, orig_len,
                            &typecast_processed, dest_col);
                        if (res != TECC_STMT_PROCESSOR_SUCCESS)
                            return res;
                    }
                }
            }
        }
    }

    /* Process whereClause — TypeCast expressions with encrypted options
     * in WHERE/Having clause (e.g., col = 'val'::text_enc encrypted with (dek=...)).
     *
     * For WHERE clause comparisons, we must use tecc_unified_encrypt_for_where()
     * instead of the generic process_one_typecast() encryption, because:
     *   1. The operator determines which algorithm to use (DET for =, OPE for < >)
     *   2. The column's DEK/KEK from the cache must be used, not the TypeCast's DEK
     *   3. The TypeCast's support_algo may not include the algorithm needed
     * ReqID:TECC4.0 - Bug 8001 */
    if (select_stmt->whereClause != NULL)
    {
        List *typecast_nodes = NIL;
        collect_typecast_nodes(select_stmt->whereClause, &typecast_nodes);
        if (typecast_nodes != NIL)
        {
            ListCell *tc_lc;
            foreach (tc_lc, typecast_nodes)
            {
                TypeCast *type_cast = (TypeCast *)tc_lc->data.ptr_value;
                ColumnRef *col_ref = NULL;
                const char *op_name = NULL;
                const char *column_name = NULL;
                const char *table_name = NULL;
                const char *schema_name = NULL;
                const char *dek_name = NULL;
                const char *kek_name = NULL;
                TeccOperatorType op_type;
                Node *arg_node;
                Const *cnst;
                char plaintext[256];
                TeccUnifiedEncryptResult *encrypt_result = NULL;
                bool where_encrypted = false;

                /* Find the comparison column and operator for this TypeCast */
                if (find_where_comparison_context(select_stmt->whereClause,
                                                  type_cast, &col_ref, &op_name))
                {
                    /* Extract column name from ColumnRef */
                    if (col_ref->fields != NIL)
                    {
                        int nf = list_length(col_ref->fields);
                        column_name = strVal((Value *)llast(col_ref->fields));
                        if (nf >= 2)
                            table_name = strVal((Value *)list_nth(col_ref->fields, nf - 2));
                        if (nf >= 3)
                            schema_name = strVal((Value *)linitial(col_ref->fields));
                    }

                    if (column_name != NULL)
                    {
                        /* Resolve real table name from encrypted_columns */
                        ListCell *elc;
                        foreach (elc, statement_data->encrypted_columns)
                        {
                            TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *)lfirst(elc);
                            if (ed == NULL || ed->column_name[0] == '\0')
                                continue;
                            if (strcmp(ed->column_name, column_name) != 0)
                                continue;
                            if (table_name != NULL)
                            {
                                if (strcmp(ed->table_alias, table_name) == 0 ||
                                    strcmp(ed->table_name, table_name) == 0)
                                {
                                    table_name = ed->table_name;
                                    if (ed->schema_name[0] != '\0')
                                        schema_name = ed->schema_name;
                                    where_encrypted = true;
                                    break;
                                }
                            }
                            else
                            {
                                /* No table qualifier — single table, use it */
                                table_name = ed->table_name;
                                if (ed->schema_name[0] != '\0')
                                    schema_name = ed->schema_name;
                                where_encrypted = true;
                                break;
                            }
                        }
                    }
                }

                if (where_encrypted && column_name != NULL && table_name != NULL)
                {
                    /* Look up DEK from cache */
                    dek_name = tecc_get_column_dek_name(statement_data,
                                                         schema_name, table_name, column_name);
                    if (dek_name != NULL)
                    {
                        /* Look up KEK from DEK cache */
                        if (statement_data->conn != NULL)
                        {
                            TeccState *state = tecc_get_state(statement_data->conn);
                            if (state != NULL)
                            {
                                TeccDekCacheItem *dek_item =
                                    tecc_cache_get_dek_by_name_search_path(
                                        state, dek_name);
                                if (dek_item != NULL)
                                    kek_name = dek_item->kek_name;
                            }
                        }
                    }

                    if (dek_name != NULL && kek_name != NULL)
                    {
                        /* Map operator string to TeccOperatorType */
                        op_type = map_operator_name_to_type(op_name);

                        if (op_type != TECC_OP_UNKNOWN)
                        {
                            /* Extract plaintext from innermost Const */
                            arg_node = type_cast->arg;
                            while (arg_node != NULL && arg_node->type == FE_T_TypeCast)
                                arg_node = ((TypeCast *)arg_node)->arg;

                            cnst = (Const *)arg_node;
                            if (cnst != NULL && cnst->type == FE_T_Const && !cnst->constisnull)
                            {
                                if (cnst->constbyval &&
                                    (cnst->constlen == sizeof(int32_t) || cnst->constlen == 2))
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%d",
                                             (int)cnst->constvalue.ival);
                                }
                                else if (cnst->constbyval && cnst->constlen == sizeof(float8))
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%g",
                                             cnst->constvalue.dval);
                                }
                                else if (cnst->constvalue.sval != NULL)
                                {
                                    snprintf(plaintext, sizeof(plaintext), "%s",
                                             cnst->constvalue.sval);
                                }
                                else
                                {
                                    where_encrypted = false; /* Unsupported Const type */
                                }
                            }
                            else
                            {
                                where_encrypted = false; /* Not a Const or IS NULL */
                            }
                        }
                        else
                        {
                            where_encrypted = false; /* Unknown operator */
                        }
                    }
                    else
                    {
                        where_encrypted = false; /* Could not resolve DEK/KEK */
                    }
                }

                if (where_encrypted)
                {
                    /* ReqID:TECC4.0 - Bug 9327: 比较值先按 cast 声明的长度做显式 cast
                     * 截断（原生 'x'::varchar(N) 就是这样）。不截的话密文是按完整值加密
                     * 的，本应命中的行会静默比不到 —— 比较用的是 cast 出来的值，与左侧
                     * 列声明的长度无关（原生比较也不套用列的 typmod）。 */
                    {
                        char *clip_typename = get_typecast_typename(type_cast, orig_query);
                        Oid clip_base_oid = (clip_typename != NULL) ?
                            map_enc_type_name_to_base_oid(clip_typename) : InvalidOid;

                        stmt_apply_cast_len_clip(type_cast, clip_base_oid, plaintext,
                                                 statement_data);
                        if (clip_typename != NULL)
                            pg_free(clip_typename);
                    }

                    /* Encrypt using WHERE-specific function */
                    encrypt_result = tecc_unified_encrypt_for_where(
                        statement_data->conn, plaintext,
                        table_name, column_name, schema_name,
                        kek_name, dek_name, op_type, 0, NULL);

                    if (encrypt_result != NULL &&
                        encrypt_result->error_code == TECC_CRYPTO_SUCCESS)
                    {
                        /* ReqID:TECC4.0 - 登记为 is_sql_expression raw value，由 phase-2
                         * 与同句普通常量在一次从左到右的改写里一并完成（不再就地拼
                         * new_query：本函数已不产出 rewritten_query，就地改写会被丢弃）。 */
                        if (!stmt_add_typecast_raw_value(statement_data, type_cast,
                                                         orig_query, orig_len,
                                                         encrypt_result))
                        {
                            tecc_unified_free_encrypt_result(encrypt_result);
                            return TECC_STMT_PROCESSOR_ERROR;
                        }

                        tecc_unified_free_encrypt_result(encrypt_result);
                        encrypt_result = NULL;
                        continue; /* Skip to next TypeCast */
                    }

                    /* Encryption failed — fall through to fallback */
                    if (encrypt_result)
                    {
                        tecc_unified_free_encrypt_result(encrypt_result);
                        encrypt_result = NULL;
                    }
                }

                /* Fallback: use generic process_one_typecast */
                {
                    /* WHERE 场景没有"目标列"，只按 cast 声明的长度定型。
                     * 排序规则判据仍按本语句有无目标表（见 process_one_typecast）：
                     * 比较值不是"新列"，不能假设它继承库默认排序规则。 */
                    TeccStmtProcessorResult res = process_one_typecast(
                        statement_data, type_cast, orig_query, orig_len,
                        &typecast_processed, NULL);
                    if (res != TECC_STMT_PROCESSOR_SUCCESS)
                        return res;
                }
            }
        }
    }

    /* ReqID:TECC4.0 - 本函数不自建 rewritten_query：每条带 ENCRYPTED WITH 的 cast
     * 经 stmt_add_typecast_raw_value 登记进 raw_values_list，由 phase-2
     * （tecc_values_processor_rewrite_values）与同句里的普通常量在一次从左到右的
     * 改写中一并替换。若在这里另拼一份 rewritten_query，会让 INSERT ... VALUES 分支
     * 以为语句已被完整改写而跳过 phase-1，同句的普通常量永远不被加密。 */
    if (typecast_processed > 0)
        statement_data->needs_rewrite = 1;

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * ReqID:TECC4.0 - 将子语句（FROM 子查询 / CTE）处理得到的改写状态传播到外层：
 * needs_rewrite / where_needs_rewrite 标志，以及 raw_values_list 中的加密常量
 * （深拷贝，避免子语句 statement_data 出作用域后指针悬垂）。
 * 缺少该传播时外层不会产出改写后的 SQL，命中 SAFETY CHECK 报
 * "TECC processor detected encrypted columns but failed to produce rewritten query"。
 */
static void
stmt_propagate_sub_rewrite_state(TeccStatementData *statement_data,
                                 const TeccStatementData *sub_data,
                                 const char *sub_alias)
{
    int rv_idx;
    int rv_count;

    if (statement_data == NULL || sub_data == NULL)
        return;

    if (sub_data->needs_rewrite)
        statement_data->needs_rewrite = 1;

    if (sub_data->where_needs_rewrite)
        statement_data->where_needs_rewrite = 1;

    stmt_propagate_sub_encrypted_columns(statement_data, sub_data, sub_alias);

    if (sub_data->raw_values_list == NULL)
        return;

    rv_count = tecc_raw_values_list_count(sub_data->raw_values_list);
    if (rv_count <= 0)
        return;

    if (statement_data->raw_values_list == NULL)
        statement_data->raw_values_list = tecc_raw_values_list_create(16);
    if (statement_data->raw_values_list == NULL)
        return;

    for (rv_idx = 0; rv_idx < rv_count; rv_idx++)
    {
        TeccRawValue *rv = tecc_raw_values_list_get(sub_data->raw_values_list, rv_idx);
        TeccRawValue *copy;

        if (rv == NULL)
            continue;
        copy = tecc_raw_value_copy(rv);
        if (copy != NULL)
        {
            if (tecc_raw_values_list_add(statement_data->raw_values_list, copy) < 0)
                tecc_raw_value_destroy(copy);
        }
    }
}

/*
 * ReqID:TECC4.0 - 把子语句（FROM 子查询 / CTE）暴露的加密列传播到父语句。
 * 多层嵌套时父语句的 FROM 项是子查询而非表，无法按表名加载元数据；缺少
 * 这些列信息会导致外层 WHERE 的密态常量不被加密（明文发往服务端）。
 */
static void
stmt_propagate_sub_encrypted_columns(TeccStatementData *statement_data,
                                     const TeccStatementData *sub_data,
                                     const char *sub_alias)
{
    ListCell *lc;

    if (statement_data == NULL || sub_data == NULL ||
        sub_data->encrypted_columns == NIL)
        return;

    foreach(lc, sub_data->encrypted_columns)
    {
        TeccColumnRefData *col = (TeccColumnRefData *) lfirst(lc);
        TeccColumnRefData *copy;
        ListCell          *plc;
        bool               exists = false;

        if (col == NULL)
            continue;

        foreach(plc, statement_data->encrypted_columns)
        {
            TeccColumnRefData *exist = (TeccColumnRefData *) lfirst(plc);
            const char        *want_alias;

            if (exist == NULL)
                continue;

            /*
             * ReqID:TECC4.0 - 去重必须带上目标别名：同一张基表被两条 CTE
             * 各自引用时，会传播出「同表同列、但外层要用各自 CTE 名引用」的
             * 记录。只按 (表,列) 判重会让后一条 CTE 的记录被丢弃，外层
             * `b.col` 解析不到，于是多 CTE 时只有第一个 CTE 生效。
             */
            want_alias = (sub_alias != NULL) ? sub_alias : "";
            if (tecc_column_ref_data_equal(col, exist) &&
                strcmp(exist->alias_fqdn, want_alias) == 0)
            {
                exists = true;
                break;
            }
        }
        if (exists)
            continue;

        copy = tecc_column_ref_data_copy(col);
        if (copy != NULL)
        {
            /* ReqID:TECC4.0 - 外层只能通过 FROM 项的别名（子查询别名/CTE 名）
             * 引用这些列，故用别名覆盖 table_alias，使 "sub.col" 形式也能
             * 解析到加密列。 */
            if (sub_alias != NULL && sub_alias[0] != '\0')
                tecc_column_ref_data_set_alias(copy, sub_alias);
            statement_data->encrypted_columns =
                lappend(statement_data->encrypted_columns, copy);
        }
    }
}

/*
 * ReqID:TECC4.0 - 归一化 INSERT...SELECT 目标位置上的"常量"形态。
 *
 * 识别下列形态（其余返回 false，由调用方保持原样发出）：
 *   裸 Const             SELECT 1, 2
 *   裸 cast 常量         SELECT 8, 80::int4_enc
 *   带符号常量           SELECT 9, -40
 *   带符号 cast 常量     SELECT 9, -40::int4_enc
 *
 * 注：本前端语法表把 UMINUS 声明在 TYPECAST 之后，因而 UMINUS 优先级更高
 * （与 PG 主源码的顺序相反），所以 -40::int4_enc 解析为
 * TypeCast(A_Expr(负号, Const)) —— 负号落在 cast 内侧。
 *
 * 输出：*out_const 内层常量；*out_sign 一元符号（'\0' 表示没有）；*out_loc /
 * *out_dsize 为需要被密文替换掉的原文区间 —— 带 cast 时覆盖整段 cast 表达式
 * （含 ::类型名[(typmod)]），否则只覆盖常量本体（含符号）。漏掉 cast 尾巴会在
 * 密文常量之后残留 "::原始类型" 后缀。
 */
static bool
stmt_normalize_insert_select_const(Node *val, const char *query,
								   Const **out_const, char *out_sign,
								   size_t *out_loc, size_t *out_dsize)
{
	TypeCast   *tc = NULL;
	A_Expr	   *ae = NULL;
	Const	   *cnst;
	Node	   *inner;
	size_t		sign_pos;
	size_t		value_loc;
	size_t		value_end;
	size_t		extent;
	char		sign;

	*out_const = NULL;
	*out_sign = '\0';
	*out_loc = 0;
	*out_dsize = 0;

	if (val == NULL || query == NULL)
		return false;

	inner = val;
	if (nodeTag(val) == FE_T_TypeCast)
	{
		/* 带 ENCRYPTED WITH 的 cast 由 process_one_typecast() 依 cast 自带的
		 * DEK 处理，不在这里接管。 */
		tc = (TypeCast *) val;
		if (tc->typeName == NULL || tc->typeName->encryptedOpts != NIL)
			return false;
		inner = tc->arg;
	}

	if (inner == NULL)
		return false;

	if (nodeTag(inner) == FE_T_Const)
		cnst = (Const *) inner;
	else if (nodeTag(inner) == FE_T_A_Expr)
	{
		/* 一元正负号：A_Expr(运算符, lexpr == NULL, rexpr = Const)。
		 * 二元表达式（如 1 + 2）不是常量，不在支持范围内。 */
		ae = (A_Expr *) inner;
		if (ae->kind != AEXPR_OP || ae->lexpr != NULL
			|| ae->rexpr == NULL || nodeTag(ae->rexpr) != FE_T_Const)
			return false;
		cnst = (Const *) ae->rexpr;
	}
	else
		return false;

	if (cnst->constisnull || cnst->location < 0 || cnst->data_size <= 0)
		return false;

	/* 符号直接从前一个非空白字符读，不依赖 A_Expr 的 location 语义。 */
	sign = '\0';
	sign_pos = (size_t) cnst->location;
	if (ae != NULL)
	{
		while (sign_pos > 0
			   && (query[sign_pos - 1] == ' ' || query[sign_pos - 1] == '\t'))
			sign_pos--;
		if (sign_pos > 0
			&& (query[sign_pos - 1] == '-' || query[sign_pos - 1] == '+'))
		{
			sign = query[sign_pos - 1];
			sign_pos--;
		}
	}

	/* 一元符号只对数值常量有意义。字符串常量带符号（-'' 之类）不做支持，
	 * 免得把符号混进字符串明文、写出语义错误的密文。 */
	if (sign != '\0'
		&& !(cnst->consttype == 20 ||	/* INT8OID */
			 cnst->consttype == 21 ||	/* INT2OID */
			 cnst->consttype == 23 ||	/* INT4OID */
			 cnst->consttype == 700 ||	/* FLOAT4OID */
			 cnst->consttype == 701 ||	/* FLOAT8OID */
			 cnst->text_numeric_literal))
		return false;

	value_loc = sign_pos;
	value_end = (size_t) cnst->location + (size_t) cnst->data_size;

	if (tc != NULL)
	{
		extent = stmt_scan_type_cast_extent(query, value_end);
		if (extent > value_end)
			value_end = extent;
	}

	if (value_end <= value_loc)
		return false;

	*out_const = cnst;
	*out_sign = sign;
	*out_loc = value_loc;
	*out_dsize = value_end - value_loc;
	return true;
}

/*
 * Process INSERT statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_insert(const InsertStmt *insert_stmt,
                                  TeccStatementData *statement_data)
{
    int encrypted_column_count = 0;
    int values_column_count = 0;
    TeccStmtProcessorResult select_result;
    TeccValuesProcessorResult values_result;
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccColumnRefData *col_ref = NULL;
    char *column_name = NULL;

    if (insert_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_INSERT;

    /* ReqID:TECC4.0 - 本语句写入已存在的目标表。INSERT 的 SELECT 部分稍后会交给
     * SELECT 处理器跑一遍（那一路看不到目标列），用它把"写入已有列"与"新建列"
     * 分开，避免按库默认排序规则去猜已有列的排序规则。 */
    statement_data->has_insert_target = true;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing INSERT statement");

    /* 添加详细调试日志 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "INSERT statement details: relation=%s, cols=%p, selectStmt=%p",
                        insert_stmt->relation ? insert_stmt->relation->relname : "NULL",
                        insert_stmt->cols,
                        insert_stmt->selectStmt);

    /* Process target table */
    if (insert_stmt->relation != NULL)
    {
        RangeVar *range_var = insert_stmt->relation;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "INSERT target table: %s.%s",
                           range_var->schemaname ? range_var->schemaname : "(null)",
                           range_var->relname ? range_var->relname : "(null)");

        /* Load table metadata and identify encrypted columns */
        stmt_load_table_metadata(range_var, statement_data, &table_encrypted_columns, &table_encrypted_column_count);
        

        /* Check if column list is explicitly specified */
        if (insert_stmt->cols != NULL)
        {
            ListCell *lc;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "INSERT column list exists, checking %d columns",
                               insert_stmt->cols->length);

            for (lc = insert_stmt->cols->head; lc != NULL; lc = lc->next)
            {
                Node *target_node = (Node *) lc->data.ptr_value;
                bool is_encrypted;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT column target, node=%p, type=%d (0x%x)",
                                   target_node, target_node ? target_node->type : -1, target_node ? target_node->type : 0);

                /* Check if this is a Const node */
                if (target_node == NULL || target_node->type == FE_T_Const) {
                    /* It's a constant value or NULL, not a column reference */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Skipping Const node or NULL");
                    continue;
                }

                /* 检查是否是ResTarget节点 */
                if (target_node->type == FE_T_ResTarget) {
                    ResTarget *res_target = (ResTarget *) target_node;
                    if (res_target->name != NULL)
                    {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "INSERT target column: %s", res_target->name);
                        values_column_count++;

                        /* Check if this column is encrypted using cached column list */
                        is_encrypted = stmt_is_column_in_encrypted_list(res_target->name, table_encrypted_columns, table_encrypted_column_count);
                        if (is_encrypted) {
                            statement_data->needs_rewrite = 1;
                            encrypted_column_count++;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Column %s is encrypted, marking statement for rewrite",
                                               res_target->name);

                            /* Create column reference and add to encrypted columns list */
                            col_ref = tecc_column_ref_data_create();
                            if (col_ref != NULL) {
                                tecc_column_ref_data_init(col_ref,
                                                         NULL,
                                                         range_var->schemaname,
                                                         range_var->relname,
                                                         res_target->name);
                                statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Added encrypted column %s.%s to encrypted_columns list",
                                                   range_var->relname ? range_var->relname : "(null)",
                                                   res_target->name);
                            }
                        }
                    } else {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "INSERT target column is NULL");
                    }
                }
                /* 检查是否是字符串节点（直接列名） */
                else if (target_node->type == FE_T_String) {
                    column_name = (char *)target_node;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target column (String): %s", column_name);
                    values_column_count++;

                    /* Check if this column is encrypted using cached column list */
                    is_encrypted = stmt_is_column_in_encrypted_list(column_name, table_encrypted_columns, table_encrypted_column_count);
                    if (is_encrypted) {
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Column %s is encrypted, marking statement for rewrite",
                                           column_name);

                        /* Create column reference and add to encrypted columns list */
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL) {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     range_var->schemaname,
                                                     range_var->relname,
                                                     column_name);
                            statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s.%s to encrypted_columns list",
                                               range_var->relname ? range_var->relname : "(null)",
                                               column_name);
                        }
                    }
                }
                /* 特殊处理：如果type字段看起来像字符串内容（检查前4个字节是否是ASCII字符） */
                else if (target_node->type > 0x20 && target_node->type < 0x7F000000) {
                    /* 这可能是一个字符串指针被错误地解释为Node */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target appears to be string: type=0x%x, as string: '%c%c%c%c'",
                                       target_node->type,
                                       (target_node->type >> 0) & 0xFF,
                                       (target_node->type >> 8) & 0xFF,
                                       (target_node->type >> 16) & 0xFF,
                                       (target_node->type >> 24) & 0xFF);

                    /* 尝试从ListCell数据中直接获取字符串 */
                    column_name = (char *)lc->data.ptr_value;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target column from ListCell data: %s", column_name);
                    values_column_count++;

                    /* Check if this column is encrypted using cached column list */
                    is_encrypted = stmt_is_column_in_encrypted_list(column_name, table_encrypted_columns, table_encrypted_column_count);
                    if (is_encrypted) {
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Column %s is encrypted, marking statement for rewrite",
                                           column_name);
                    }
                }
                else {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "INSERT target is not ResTarget or String, type=%d (0x%x)",
                                       target_node->type, target_node->type);
                    continue;  /* 跳过非ResTarget节点 */
                }
            }
        }
        else
        {
            /* No explicit column list - use positional mapping from table metadata */
            if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "INSERT has no explicit column list, using table metadata for %d encrypted columns",
                                   table_encrypted_column_count);

                for (int i = 0; i < table_encrypted_column_count; i++)
                {
                    TeccColumnCacheItem *col_cache = table_encrypted_columns[i];
                    if (col_cache != NULL && col_cache->column_name != NULL)
                    {
                        TeccColumnRefData *col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     NULL,
                                                     insert_stmt->relation->relname,
                                                     col_cache->column_name);
                            statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s to encrypted_columns list (positional)",
                                               col_cache->column_name);
                        }
                    }
                }

                statement_data->needs_rewrite = 1;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Table has encrypted columns, marking for rewrite");
            }
        }
    }

    /* Process VALUES clause or SELECT subquery */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "INSERT statement selectStmt pointer: %p", insert_stmt->selectStmt);

    if (insert_stmt->selectStmt != NULL)
    {
        Node *select_node = insert_stmt->selectStmt;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "INSERT select node type: %d", nodeTag(select_node));

        if (nodeTag(select_node) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) select_node;

            /* Check if this is a VALUES clause or SELECT statement */
            /* In PostgreSQL, VALUES clause is represented as a SelectStmt with valuesLists != NULL */
            if (select_stmt->defaultValues)
            {
                /* This is a DEFAULT VALUES clause — no values to encrypt, send SQL as-is. */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT DEFAULT VALUES clause (no values to process)");
                /* ReqID:TECC4.0 - Bug 7972: DEFAULT VALUES needs no rewrite.
                 * Set rewritten_query to original query to pass the safety check. */
                if (statement_data->rewritten_query != NULL)
                    pg_free(statement_data->rewritten_query);
                if (statement_data->query != NULL)
                    statement_data->rewritten_query = pg_strdup(statement_data->query);
                statement_data->needs_rewrite = false;
                statement_data->where_needs_rewrite = false;
                /* Skip values processing — no values to encrypt. */
                return TECC_STMT_PROCESSOR_SUCCESS;
            }
            else if (select_stmt->valuesLists != NULL)
            {
                /* This is a VALUES clause */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT VALUES clause");

                /* First, process TypeCast expressions with encrypted options
                 * (e.g., 'test'::varchar_enc encrypted with (dek = dek_t1)) */
                {
                    TeccStmtProcessorResult typecast_result;
                    typecast_result = process_insert_select_typecast(
                        select_stmt, statement_data, insert_stmt,
                        table_encrypted_columns, table_encrypted_column_count);
                    if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "INSERT...VALUES TypeCast processing failed: %d",
                                           typecast_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }

                /* Begin - ReqID:TECC4.0 - Bug 7904: Check DEK consistency for VALUES TypeCasts */
                if (select_stmt->valuesLists != NIL
                    && table_encrypted_columns != NULL
                    && table_encrypted_column_count > 0)
                {
                    List *first_row;
                    ListCell *col_lc;
                    int col_idx;

                    first_row = (List *) list_nth(select_stmt->valuesLists, 0);
                    col_idx = 0;

                    foreach (col_lc, first_row)
                    {
                        Node *val;
                        TypeCast *tc;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;
                        ListCell *opt_lc;
                        const char *type_dek;

                        val = (Node *) col_lc->data.ptr_value;
                        tc = NULL;
                        ins_col_name = NULL;
                        col_item = NULL;
                        type_dek = NULL;

                        if (val == NULL || val->type != FE_T_TypeCast)
                        {
                            col_idx++;
                            continue;
                        }
                        tc = (TypeCast *) val;

                        if (tc->typeName == NULL
                            || tc->typeName->encryptedOpts == NIL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Map position to INSERT column */
                        if (insert_stmt->cols != NIL
                            && col_idx < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, col_idx);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == col_idx + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }

                        if (ins_col_name == NULL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Find column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }

                        if (col_item == NULL)
                        {
                            col_idx++;
                            continue;
                        }

                        /* Extract DEK name from TypeCast */
                        foreach (opt_lc, tc->typeName->encryptedOpts)
                        {
                            DefElem *elem = (DefElem *) opt_lc->data.ptr_value;
                            if (elem != NULL && elem->defname != NULL
                                && pg_strcasecmp(elem->defname, "dek") == 0)
                            {
                                if (elem->arg != NULL
                                    && nodeTag(elem->arg) == FE_T_String)
                                    type_dek = strVal((const Value *) elem->arg);
                                break;
                            }
                        }

                        if (type_dek != NULL
                            && col_item->dek_name != NULL
                            && pg_strcasecmp(type_dek, col_item->dek_name) != 0)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "cannot assign encrypted value with DEK %s "
                                "to column %s which uses DEK %s",
                                type_dek, ins_col_name, col_item->dek_name);
                            return TECC_STMT_PROCESSOR_ERROR;
                        }

                        col_idx++;
                    }
                }
                /* End - ReqID:TECC4.0 */

                /* ReqID:TECC4.0 - 始终跑 values 处理器（phase-1）：带 ENCRYPTED WITH
                 * 的 cast 由 process_insert_select_typecast 登记为 raw value，普通
                 * 常量必须由 phase-1 收集进同一 raw_values_list，phase-2 才能在一次
                 * 从左到右改写里同时替换两者。旧判据 rewritten_query == NULL 会在
                 * cast 路径下整体跳过 phase-1，导致同句普通常量明文外发。 */
                values_result = tecc_values_processor_process(statement_data);

                if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "VALUES clause processing failed: %d", values_result);
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "VALUES clause processing completed successfully");
            }
            else
            {
                /* This is a SELECT statement */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing INSERT...SELECT statement");

                /* Process the SELECT statement recursively */
                select_result = tecc_stmt_processor_process_select(
                    select_stmt, statement_data);

                if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "INSERT...SELECT processing failed: %d", select_result);
                    return TECC_STMT_PROCESSOR_ERROR;
                }

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "INSERT...SELECT processing completed successfully");

                /* ReqID:TECC4.0 - Handle constants in SELECT target list that map
                 * to encrypted INSERT columns. When a SELECT target at a position that
                 * corresponds to an encrypted INSERT column is a constant — a bare
                 * Const, a bare cast of one (e.g. SELECT 8, 80::int4_enc), or either
                 * of those with a leading sign (e.g. SELECT 9, -40 / -40::int4_enc) —
                 * encrypt it with that column's DEK/algorithms and add it to
                 * raw_values_list for subsequent SQL text rewrite via
                 * tecc_processor_rewrite_query(). See
                 * stmt_normalize_insert_select_const() for the accepted shapes. */
                if (statement_data->needs_rewrite
                    && select_stmt->targetList != NIL
                    && table_encrypted_columns != NULL
                    && table_encrypted_column_count > 0)
                {
                    int num_targets = list_length(select_stmt->targetList);
                    int tp;

                    for (tp = 0; tp < num_targets; tp++)
                    {
                        Node *target_node;
                        ResTarget *res_target;
                        Node *val;
                        Const *cnst;
                        char const_sign;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;
                        /* ReqID:TECC4.0 - 待替换区间：不带 cast 时为常量本体（含
                         * 一元符号），带 cast 时为整段 cast 表达式 */
                        size_t data_loc;
                        size_t data_dsize;

                        ins_col_name = NULL;
                        col_item = NULL;

                        /* Get INSERT column name at this position */
                        if (insert_stmt->cols != NIL
                            && tp < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, tp);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            /* No explicit column list — use column_attnum mapping */
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == tp + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (ins_col_name == NULL)
                            continue;

                        /* Find encrypted column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (col_item == NULL)
                            continue;   /* Not an encrypted column */

                        /* Check SELECT target at this position */
                        target_node = (Node *) list_nth(
                            select_stmt->targetList, tp);
                        if (target_node == NULL
                            || target_node->type != FE_T_ResTarget)
                            continue;
                        res_target = (ResTarget *) target_node;
                        val = res_target->val;

                        /* ReqID:TECC4.0 - 该位置对应密态列时，把"常量"在客户端加密。
                         * 可识别的形态（裸常量 / 裸 cast 常量 / 带符号的这两种）与
                         * 替换区间的取法见 stmt_normalize_insert_select_const()。
                         * 密态字面量的完整 TECC 写法是
                         * "值::enc类型 ENCRYPTED WITH (DEK = ..)"，那类 cast 已由
                         * process_one_typecast 依自身 DEK 处理；这里只接管不带
                         * ENCRYPTED WITH 的常量，按目标列的 DEK 与算法加密。
                         * 其余形态（列引用、二元表达式、函数调用等）不在
                         * INSERT...SELECT 常量加密范围内，保持原样发出。 */
                        if (!stmt_normalize_insert_select_const(
                                val, statement_data->query, &cnst, &const_sign,
                                &data_loc, &data_dsize))
                        {
                            /* ReqID:TECC4.0 - 只补信息、不改判定：这里**只**写
                             * error_message，不动 error_code，故任何语句的成败
                             * 判断都与改动前完全一致。error_message 仅在外层
                             * SAFETY CHECK 本来就要报错时才被取用
                             * （fe-tecc-integration.c 的 "failed to produce
                             * rewritten query" 那条兜底），因此成功路径不会看到
                             * 这句话 —— 包括"源是别的密态表密文列、走服务端密文
                             * 拷贝"这类同样会在此跳过的合法写法。
                             * 仅在为空时写，避免盖掉更具体的既有错误。 */
                            if (statement_data->error_message[0] == '\0')
                                snprintf(statement_data->error_message,
                                         sizeof(statement_data->error_message),
                                         "INSERT ... SELECT into encrypted column "
                                         "\"%s\" (position %d) requires a literal "
                                         "constant (optionally signed, optionally a cast "
                                         "with ENCRYPTED WITH (DEK = ...)); pre-compute "
                                         "the value and insert it as a literal",
                                         ins_col_name, tp + 1);
                            continue;
                        }

                        /* Extract plaintext from Const */
                        {
                            char plaintext[256];
                            bool has_value;

                            has_value = false;
                            /* ReqID:TECC4.0 - Extract plaintext by consttype.
                             * INT2OID(21)/INT4OID(23)/INT8OID(20): constbyval,
                             *   extract from ival.
                             * FLOAT4OID(700)/FLOAT8OID(701): constbyval,
                             *   extract from dval.
                             * VARCHAR/BPCHAR/TEXT: !constbyval, sval pointer.
                             * 一元负号是独立的 A_Expr，取值/取 dval/sval 都会丢
                             * 符号，故按 const_sign 补回（'+' 视为无符号）。 */
                            if (cnst->consttype == 20 ||  /* INT8OID */
                                cnst->consttype == 21 ||  /* INT2OID */
                                cnst->consttype == 23)    /* INT4OID */
                            {
                                snprintf(plaintext, sizeof(plaintext), "%s%lld",
                                         (const_sign == '-') ? "-" : "",
                                         (long long) cnst->constvalue.ival);
                                has_value = true;
                            }
                            else if (cnst->consttype == 700 ||  /* FLOAT4OID */
                                     cnst->consttype == 701)    /* FLOAT8OID */
                            {
                                snprintf(plaintext, sizeof(plaintext), "%s%g",
                                         (const_sign == '-') ? "-" : "",
                                         cnst->constvalue.dval);
                                has_value = true;
                            }
                            else if (cnst->constvalue.sval != NULL)
                            {
                                snprintf(plaintext, sizeof(plaintext), "%s%s",
                                         (const_sign == '-') ? "-" : "",
                                         cnst->constvalue.sval);
                                has_value = true;
                            }

                            if (!has_value)
                                continue;

                            /* Encrypt using column metadata */
                            {
                                TeccUnifiedEncryptResult *encrypt_result;
                                bool supports_det;
                                bool supports_ope;
                                bool supports_search;
                                bool supports_he_add;
                                bool supports_he_mul;
                                TeccRawValue *rv;
                                const char *type_name;
                                const char *effective_dek_name;

                                /* ReqID:TECC4.0 - 纯 HE 列（HE_ADD_DEK/HE_MUL_DEK）
                                 * 没有对称 DEK，col_item->dek_name 为 NULL。此时必须
                                 * 与 VALUES 路径（tecc_get_column_dek_name）一致地改用
                                 * 列上的 HE DEK 名：调度器只有拿到非空 DEK 名才会去加载
                                 * EC/FF 公钥（HE 加密走公钥，不需要 SM4 DEK 材料）。
                                 * 传 NULL 会跳过公钥加载，HE_ADD/HE_MUL 加密失败。 */
                                effective_dek_name = col_item->dek_name;
                                if (effective_dek_name == NULL
                                    || effective_dek_name[0] == '\0')
                                {
                                    if (col_item->he_add_dek_name != NULL
                                        && col_item->he_add_dek_name[0] != '\0')
                                        effective_dek_name =
                                            col_item->he_add_dek_name;
                                    else
                                        effective_dek_name =
                                            col_item->he_mul_dek_name;
                                }

                                supports_det =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_DET) != 0;
                                supports_ope =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_OPE) != 0;
                                supports_search =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_SEARCH) != 0;
                                supports_he_add =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_HE_ADD) != 0;
                                supports_he_mul =
                                    (col_item->encryption_type
                                     & TECC_ENC_TYPE_HE_MUL) != 0;

                                encrypt_result =
                                    tecc_unified_encrypt_string_with_options(
                                        statement_data->conn,
                                        plaintext,
                                        col_item->table_name,
                                        col_item->column_name,
                                        NULL,  /* kek_name */
                                        effective_dek_name,
                                        col_item->original_type_oid,
                                        col_item->original_type_mod,
                                        supports_det,
                                        supports_ope,
                                        supports_search,
                                        supports_he_add,
                                        supports_he_mul,
                                        NULL);

                                if (encrypt_result == NULL
                                    || encrypt_result->error_code
                                       != TECC_CRYPTO_SUCCESS)
                                {
                                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "Failed to encrypt bare constant "
                                        "for column %s: %s",
                                        col_item->column_name,
                                        encrypt_result
                                        ? encrypt_result->error_msg
                                        : "unknown error");
                                    if (encrypt_result)
                                        tecc_unified_free_encrypt_result(
                                            encrypt_result);
                                    continue;
                                }

                                if (data_dsize == 0)
                                {
                                    tecc_unified_free_encrypt_result(
                                        encrypt_result);
                                    continue;
                                }

                                /* Create raw value, add to raw_values_list */
                                if (statement_data->raw_values_list == NULL)
                                {
                                    statement_data->raw_values_list =
                                        tecc_raw_values_list_create(
                                            num_targets);
                                }

                                rv = tecc_raw_value_create_from_string(
                                    plaintext, strlen(plaintext));
                                if (rv != NULL)
                                {
                                    rv->location = data_loc;
                                    rv->data_size = data_dsize;

                                    /* Set type name for ::typename suffix */
                                    type_name =
                                        get_enc_type_name_from_base_oid(
                                            col_item->original_type_oid);
                                    if (type_name != NULL)
                                        rv->column_type = pg_strdup(type_name);

                                    rv->processed.encrypted_value =
                                        pg_strdup(
                                            encrypt_result->hex_string);
                                    rv->state =
                                        TECC_RAW_VALUE_STATE_ENCRYPTED;

                                    tecc_raw_values_list_add(
                                        statement_data->raw_values_list, rv);

                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Encrypted constant "
                                        "for column %s at position %d",
                                        col_item->column_name,
                                        (int) rv->location);
                                }

                                tecc_unified_free_encrypt_result(
                                    encrypt_result);
                            }
                        }
                    }
                }

                /* Begin - ReqID:TECC4.0 - Bug 7904: Check DEK consistency for TypeCast targets */
                {
                    int num_tp;
                    int tp2;

                    num_tp = list_length(select_stmt->targetList);

                    for (tp2 = 0; tp2 < num_tp; tp2++)
                    {
                        Node *target_node;
                        ResTarget *res_target;
                        Node *val;
                        const char *ins_col_name;
                        int tci;
                        TeccColumnCacheItem *col_item;
                        TypeCast *tc;
                        ListCell *opt_lc;
                        const char *type_dek;

                        ins_col_name = NULL;
                        col_item = NULL;
                        tc = NULL;
                        type_dek = NULL;

                        /* Get INSERT column name at this position */
                        if (insert_stmt->cols != NIL
                            && tp2 < list_length(insert_stmt->cols))
                        {
                            Node *col_node = (Node *) list_nth(
                                insert_stmt->cols, tp2);
                            if (col_node->type == FE_T_ResTarget)
                                ins_col_name = ((ResTarget *) col_node)->name;
                            else if (col_node->type == FE_T_String)
                                ins_col_name = (const char *) col_node;
                        }
                        else
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_attnum
                                       == tp2 + 1)
                                {
                                    ins_col_name =
                                        table_encrypted_columns[tci]->column_name;
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (ins_col_name == NULL)
                            continue;

                        /* Find encrypted column metadata if not set above */
                        if (col_item == NULL)
                        {
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                if (table_encrypted_columns[tci] != NULL
                                    && table_encrypted_columns[tci]->column_name != NULL
                                    && strcmp(table_encrypted_columns[tci]->column_name,
                                              ins_col_name) == 0)
                                {
                                    col_item = table_encrypted_columns[tci];
                                    break;
                                }
                            }
                        }
                        if (col_item == NULL)
                            continue;

                        /* Check SELECT target at this position */
                        target_node = (Node *) list_nth(
                            select_stmt->targetList, tp2);
                        if (target_node == NULL
                            || target_node->type != FE_T_ResTarget)
                            continue;
                        res_target = (ResTarget *) target_node;
                        val = res_target->val;
                        if (val == NULL || val->type != FE_T_TypeCast)
                            continue;
                        tc = (TypeCast *) val;

                        /* Only check TypeCasts with encrypted options */
                        if (tc->typeName == NULL
                            || tc->typeName->encryptedOpts == NIL)
                            continue;

                        /* Extract DEK name from encrypted options */
                        foreach (opt_lc, tc->typeName->encryptedOpts)
                        {
                            DefElem *elem = (DefElem *) opt_lc->data.ptr_value;
                            if (elem != NULL && elem->defname != NULL
                                && pg_strcasecmp(elem->defname, "dek") == 0)
                            {
                                if (elem->arg != NULL
                                    && nodeTag(elem->arg) == FE_T_String)
                                    type_dek = strVal((const Value *) elem->arg);
                                break;
                            }
                        }

                        if (type_dek != NULL
                            && col_item->dek_name != NULL
                            && pg_strcasecmp(type_dek, col_item->dek_name) != 0)
                        {
                            tecc_processor_set_error(statement_data,
                                TECC_PROCESSOR_ERROR,
                                "cannot assign encrypted value with DEK %s "
                                "to column %s which uses DEK %s",
                                type_dek, ins_col_name, col_item->dek_name);
                            return TECC_STMT_PROCESSOR_ERROR;
                        }
                    }
                }
                /* End - ReqID:TECC4.0 */

                /* ReqID:TECC4.0 - Handle VALUES inside RangeSubselect in INSERT...SELECT.
                 * Walk the SELECT's FROM clause for RangeSubselect nodes containing VALUES.
                 * For each, map VALUES columns to encrypted INSERT target columns and encrypt. */
                if (select_stmt->fromClause != NIL
                    && statement_data->encrypted_columns != NIL)
                {
                    int num_target_positions = list_length(select_stmt->targetList);
                    bool *target_is_encrypted;
                    char **target_enc_col_name;
                    int tp;

                    target_is_encrypted = (bool *) palloc(num_target_positions * sizeof(bool));
                    target_enc_col_name = (char **) palloc(num_target_positions * sizeof(char *));
                    memset(target_is_encrypted, 0, num_target_positions * sizeof(bool));
                    memset(target_enc_col_name, 0, num_target_positions * sizeof(char *));

                    /* Build INSERT position → encrypted column name mapping */
                    for (tp = 0; tp < num_target_positions; tp++)
                    {
                        const char *ins_col_name = NULL;
                        bool is_enc = false;

                        if (insert_stmt->cols != NIL && tp < list_length(insert_stmt->cols))
                        {
                            ResTarget *ins_rt = (ResTarget *) list_nth(insert_stmt->cols, tp);
                            ins_col_name = ins_rt->name;
                        }
                        else if (table_encrypted_column_count > 0 && table_encrypted_columns != NULL)
                        {
                            int tci;
                            for (tci = 0; tci < table_encrypted_column_count; tci++)
                            {
                                TeccColumnCacheItem *ci = table_encrypted_columns[tci];
                                if (ci != NULL && ci->column_attnum == tp + 1)
                                {
                                    ins_col_name = ci->column_name;
                                    break;
                                }
                            }
                        }

                        if (ins_col_name != NULL)
                        {
                            ListCell *eclc;
                            foreach(eclc, statement_data->encrypted_columns)
                            {
                                TeccColumnRefData *cd = (TeccColumnRefData *) lfirst(eclc);
                                if (strcmp(ins_col_name, cd->column_name) == 0)
                                {
                                    is_enc = true;
                                    break;
                                }
                            }
                        }
                        target_is_encrypted[tp] = is_enc;
                        if (is_enc)
                            target_enc_col_name[tp] = (char *) ins_col_name;
                    }

                    /* Walk FROM clause for RangeSubselect with VALUES */
                    {
                        ListCell *frm_lc;
                        foreach(frm_lc, select_stmt->fromClause)
                        {
                            Node *from_item = (Node *) lfirst(frm_lc);
                            if (nodeTag(from_item) != FE_T_RangeSubselect)
                                continue;
                            {
                                RangeSubselect *rss = (RangeSubselect *) from_item;
                                if (rss->subquery == NULL
                                    || nodeTag(rss->subquery) != FE_T_SelectStmt)
                                    continue;
                                {
                                    SelectStmt *subselect = (SelectStmt *) rss->subquery;
                                    if (subselect->valuesLists == NIL || rss->alias == NULL
                                        || rss->alias->colnames == NIL)
                                        continue;
                                    {
                                        Alias *alias = rss->alias;
                                        List *rss_colnames = alias->colnames;
                                        int num_rss_cols = list_length(rss_colnames);
                                        List *target_columns = NIL;
                                        int vcol;

                                        for (vcol = 0; vcol < num_rss_cols; vcol++)
                                        {
                                            char *rss_col_name = (char *) list_nth(rss_colnames, vcol);
                                            bool found_encrypted = false;
                                            char *insert_col_name = NULL;
                                            ListCell *slc;
                                            int sel_pos = 0;

                                            if (rss_col_name == NULL) continue;

                                            /* Search SELECT targetList for ColumnRef
                                             * matching this RangeSubselect alias + column */
                                            foreach(slc, select_stmt->targetList)
                                            {
                                                ResTarget *sel_rt = (ResTarget *) lfirst(slc);
                                                if (sel_rt->val != NULL
                                                    && nodeTag(sel_rt->val) == FE_T_ColumnRef)
                                                {
                                                    ColumnRef *cref = (ColumnRef *) sel_rt->val;
                                                    if (list_length(cref->fields) == 2)
                                                    {
                                                        Node *f1 = (Node *) list_nth(cref->fields, 0);
                                                        Node *f2 = (Node *) list_nth(cref->fields, 1);
                                                        const char *f1_str = NULL;
                                                        const char *f2_str = NULL;

                                                        if (nodeTag(f1) == FE_T_Const)
                                                            f1_str = ((struct Const *)f1)->constvalue.sval;
                                                        if (nodeTag(f2) == FE_T_Const)
                                                            f2_str = ((struct Const *)f2)->constvalue.sval;

                                                        if (f1_str != NULL && f2_str != NULL
                                                            && alias->aliasname != NULL
                                                            && strcmp(f1_str, alias->aliasname) == 0
                                                            && strcmp(f2_str, rss_col_name) == 0)
                                                        {
                                                            if (sel_pos < num_target_positions
                                                                && target_is_encrypted[sel_pos])
                                                            {
                                                                insert_col_name = target_enc_col_name[sel_pos];
                                                                found_encrypted = true;
                                                            }
                                                            break;
                                                        }
                                                    }
                                                }
                                                sel_pos++;
                                            }

                                            {
                                                ResTarget *rt = makeResTarget(
                                                    found_encrypted ? insert_col_name : NULL,
                                                    NULL, 0);
                                                target_columns = lappend(target_columns, (Node *) rt);
                                            }
                                        }

                                        /* Process VALUES encryption */
                                        {
                                            TeccValuesProcessorResult vp_result;
                                            vp_result = tecc_values_processor_process_insert_values(
                                                subselect->valuesLists,
                                                target_columns,
                                                /* ReqID:TECC4.0 - 未限定表名按 search_path 解析真实 schema */
                                                insert_stmt->relation ?
                                                    tecc_stmt_resolve_table_schema(statement_data,
                                                        insert_stmt->relation->schemaname,
                                                        insert_stmt->relation->relname) : NULL,
                                                insert_stmt->relation ? insert_stmt->relation->relname : NULL,
                                                statement_data);

                                            if (vp_result == TECC_VALUES_PROCESSOR_SUCCESS
                                                && statement_data->raw_values_list != NULL)
                                            {
                                                /* Fix column_type for subquery VALUES:
                                                 * the explicit target_columns path sets it NULL,
                                                 * but subquery VALUES need ::type_name cast. */
                                                TeccRawValuesList *rvlist = statement_data->raw_values_list;
                                                int rvcount = tecc_raw_values_list_count(rvlist);
                                                int rvi;
                                                for (rvi = 0; rvi < rvcount; rvi++)
                                                {
                                                    TeccRawValue *rv = tecc_raw_values_list_get(rvlist, rvi);
                                                    if (rv != NULL && rv->needs_encryption
                                                        && (rv->column_type == NULL || rv->column_type[0] == '\0')
                                                        && rv->column_name != NULL)
                                                    {
                                                        int tci;
                                                        for (tci = 0; tci < table_encrypted_column_count; tci++)
                                                        {
                                                            TeccColumnCacheItem *ci = table_encrypted_columns[tci];
                                                            if (ci != NULL && ci->column_name != NULL
                                                                && strcmp(ci->column_name, rv->column_name) == 0)
                                                            {
                                                                const char *enc_type = get_enc_type_name_from_base_oid(
                                                                    ci->original_type_oid);
                                                                if (enc_type != NULL)
                                                                    tecc_raw_value_set_column_info(rv, NULL, enc_type, InvalidOid);
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        /* Free target_columns */
                                        {
                                            ListCell *tc_lc;
                                            foreach(tc_lc, target_columns)
                                            {
                                                ResTarget *rt = (ResTarget *) lfirst(tc_lc);
                                                if (rt != NULL) pfree(rt);
                                            }
                                            list_free(target_columns);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pfree(target_is_encrypted);
                    pfree(target_enc_col_name);
                }

                /* Process TypeCast expressions with encrypted options in the SELECT list */
                {
                    TeccStmtProcessorResult typecast_result;
                    typecast_result = process_insert_select_typecast(
                        select_stmt, statement_data, insert_stmt,
                        table_encrypted_columns, table_encrypted_column_count);
                    if (typecast_result != TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "INSERT...SELECT TypeCast processing failed: %d",
                                           typecast_result);
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
            }
        }
        else
        {
            /* Handle other node types that might represent VALUES clause */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Processing INSERT VALUES clause (node type: %d)",
                               nodeTag(select_node));

            /* Delegate to values processor */
            values_result = tecc_values_processor_process(statement_data);

            if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "VALUES clause processing failed: %d", values_result);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "VALUES clause processing completed successfully");
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "INSERT statement has no VALUES or SELECT clause");
    }

    /* ReqID:TECC4.0 - Process ON CONFLICT DO UPDATE SET clause.
     * Traverse SET assignments in ON CONFLICT DO UPDATE to identify and
     * encrypt values for encrypted columns.
     * Must run BEFORE table_encrypted_columns is freed and AFTER
     * raw_values_list is populated by INSERT values processing. */
    if (insert_stmt->onConflictClause != NIL)
    {
        ListCell *occ_lc;
        foreach(occ_lc, insert_stmt->onConflictClause)
        {
            OnConflictClause *occ = (OnConflictClause *) lfirst(occ_lc);
            if (occ != NULL && occ->action == ONCONFLICT_UPDATE
                && occ->targetList != NIL)
            {
                ListCell *set_lc;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing ON CONFLICT DO UPDATE SET clause");
                for (set_lc = occ->targetList->head; set_lc != NULL; set_lc = set_lc->next)
                {
                    Node *set_node = (Node *) set_lc->data.ptr_value;
                    if (set_node == NULL || nodeTag(set_node) != FE_T_ResTarget)
                        continue;
                    {
                        ResTarget *set_target = (ResTarget *) set_node;
                        bool is_enc = false;
                        if (set_target->name == NULL)
                            continue;

                        /* Check if column is encrypted */
                        if (table_encrypted_columns != NULL)
                            is_enc = stmt_is_column_in_encrypted_list(
                                set_target->name, table_encrypted_columns,
                                table_encrypted_column_count);
                        else if (insert_stmt->relation != NULL)
                            is_enc = stmt_check_column_encrypted(
                                statement_data,
                                insert_stmt->relation->schemaname,
                                insert_stmt->relation->relname,
                                set_target->name);

                        if (is_enc)
                        {
                            statement_data->needs_rewrite = 1;
                            col_ref = tecc_column_ref_data_create();
                            if (col_ref != NULL && insert_stmt->relation != NULL)
                            {
                                tecc_column_ref_data_init(col_ref,
                                                         NULL,
                                                         insert_stmt->relation->schemaname,
                                                         insert_stmt->relation->relname,
                                                         set_target->name);
                                statement_data->encrypted_columns = lappend(
                                    statement_data->encrypted_columns, col_ref);
                            }
                            encrypted_column_count++;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "ON CONFLICT DO UPDATE SET column %s is encrypted",
                                               set_target->name);
                        }
                    }
                }

                /* ReqID:TECC4.0 - Encrypt ON CONFLICT DO UPDATE SET values.
                 * Build a separate raw_values_list for ON CONFLICT SET entries,
                 * process and encrypt, then merge into main raw_values_list.
                 * This avoids re-processing already-encrypted INSERT values. */
                if (statement_data->needs_rewrite)
                {
                    ListCell *enc_lc;
                    int occ_added = 0;
                    TeccRawValuesList *occ_list = NULL;

                    for (enc_lc = occ->targetList->head; enc_lc != NULL; enc_lc = enc_lc->next)
                    {
                        Node *n = (Node *) enc_lc->data.ptr_value;
                        ResTarget *rt;
                        bool is_enc;
                        TeccRawValue *rv;
                        Const *cv;

                        if (n == NULL || nodeTag(n) != FE_T_ResTarget)
                            continue;
                        rt = (ResTarget *) n;
                        if (rt->name == NULL || rt->val == NULL)
                            continue;

                        /* Check if this SET column is encrypted */
                        is_enc = false;
                        if (table_encrypted_columns != NULL)
                            is_enc = stmt_is_column_in_encrypted_list(
                                rt->name, table_encrypted_columns,
                                table_encrypted_column_count);
                        else if (insert_stmt->relation != NULL)
                            is_enc = stmt_check_column_encrypted(statement_data,
                                insert_stmt->relation->schemaname,
                                insert_stmt->relation->relname, rt->name);
                        if (!is_enc)
                            continue;

                        /* Only process Const values that scanner tagged */
                        if (nodeTag(rt->val) != FE_T_Const)
                        {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ON CONFLICT SET column %s value is not Const, skipping",
                                rt->name);
                            continue;
                        }
                        cv = (Const *) rt->val;

                        /* Lazy-init the ON CONFLICT raw_values_list */
                        if (occ_list == NULL)
                        {
                            occ_list = tecc_raw_values_list_create(
                                list_length(occ->targetList));
                            if (occ_list == NULL)
                                break;
                        }

                        rv = tecc_create_raw_value_from_node(rt->val);
                        if (rv == NULL)
                            continue;

                        rv->location = (size_t) cv->location;
                        rv->data_size = (size_t) cv->data_size;
                        rv->needs_encryption = true;
                        rv->table_name = insert_stmt->relation->relname
                            ? pg_strdup(insert_stmt->relation->relname) : NULL;
                        rv->column_name = pg_strdup(rt->name);

                        tecc_raw_values_list_add(occ_list, rv);
                        occ_added++;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ON CONFLICT SET: raw_value for %s.%s (loc=%zu, size=%zu)",
                            rv->table_name ? rv->table_name : "?",
                            rv->column_name ? rv->column_name : "?",
                            rv->location, rv->data_size);
                    }

                    if (occ_added > 0 && occ_list != NULL)
                    {
                        TeccValuesProcessorResult occ_proc_result;
                        int k;

                        occ_proc_result = tecc_process_values_list(
                            occ_list, statement_data);
                        if (occ_proc_result != TECC_VALUES_PROCESSOR_SUCCESS)
                        {
                            tecc_processor_debug(TECC_LOG_ERROR,
                                "ON CONFLICT SET value encryption failed: %d",
                                occ_proc_result);
                            tecc_raw_values_list_destroy(occ_list);
                        }
                        else
                        {
                            /* Merge encrypted ON CONFLICT values into main list */
                            int occ_count = tecc_raw_values_list_count(occ_list);
                            for (k = 0; k < occ_count; k++)
                            {
                                TeccRawValue *rv = tecc_raw_values_list_get(occ_list, k);
                                if (rv != NULL
                                    && statement_data->raw_values_list != NULL)
                                {
                                    tecc_raw_values_list_add(
                                        statement_data->raw_values_list, rv);
                                }
                            }
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ON CONFLICT SET: encrypted+merged %d values",
                                occ_count);
                            /* Free list container (values now owned by main list) */
                            if (occ_list->values != NULL)
                                pg_free(occ_list->values);
                            pg_free(occ_list);
                        }
                    }
                }
            }
        }
    }

    /* Free encrypted columns list — must come after ON CONFLICT block
     * which needs table_encrypted_columns for column encryption check. */
    if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
    {
        pg_free(table_encrypted_columns);
    }

    

    /* ReqID:TECC4.0 - Detect INSERT...SELECT with WITH clause (CTE) into encrypted table.
     * When the INSERT source is a SELECT from a CTE, the CTE values are computed
     * server-side and cannot be encrypted by the client at SQL parse time.
     * Report a clear error so the user knows this feature combination is unsupported. */
    if (statement_data->needs_rewrite
        && statement_data->rewritten_query == NULL
        && insert_stmt != NULL && insert_stmt->selectStmt != NULL
        && nodeTag(insert_stmt->selectStmt) == FE_T_SelectStmt
        && insert_stmt->withClause != NIL
        && statement_data->encrypted_columns != NIL)
    {
        SelectStmt *sel_stmt = (SelectStmt *) insert_stmt->selectStmt;

        /* Only when the source is a SELECT (not direct VALUES) */
        if (sel_stmt->valuesLists == NIL && sel_stmt->fromClause != NIL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "WITH clause (CTE) combined with INSERT...SELECT into encrypted table "
                               "is not supported in TECC mode. "
                               "The CTE values are computed on the server side and cannot be "
                               "pre-encrypted by the client. "
                               "Consider: 1) rewriting as INSERT...VALUES with pre-computed values, "
                               "or 2) avoiding the WITH clause for INSERTs into encrypted tables.");
            tecc_record_error(statement_data->conn
                              ? tecc_get_state(statement_data->conn) : NULL,
                            TECC_ERR_PROCESSOR,
                            "WITH ... INSERT ... SELECT into encrypted table is not supported "
                            "because CTE values are server-side computed and cannot be "
                            "client-side encrypted",
                            __func__, __LINE__, false);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* ReqID:TECC4.0 - 元数据加载失败时上抛，避免加密表被误判为普通表导致明文泄漏。 */
    if (statement_data->error_code != TECC_PROCESSOR_SUCCESS)
        return TECC_STMT_PROCESSOR_ERROR;

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/* ReqID:TECC4.0 - 列是否带声明长度的字符型密态列（varchar_enc/bpchar_enc） */
static bool
stmt_char_column_typmod(const TeccColumnCacheItem *item)
{
    if (item == NULL)
        return false;
    if (item->original_type_oid != 1043 /* VARCHAROID */ &&
        item->original_type_oid != 1042 /* BPCHAROID */)
        return false;
    return item->original_type_mod >= (int) sizeof(int32);
}

/* ReqID:TECC4.0 - 按列名在加密列元数据数组里找列 */
static TeccColumnCacheItem *
stmt_find_column_item(TeccColumnCacheItem **items, int count, const char *column_name)
{
    int i;

    if (items == NULL || column_name == NULL)
        return NULL;

    for (i = 0; i < count; i++)
    {
        if (items[i] != NULL && items[i]->column_name != NULL &&
            strcmp(items[i]->column_name, column_name) == 0)
            return items[i];
    }

    return NULL;
}

/* ReqID:TECC4.0 - 列限定名（表名或别名）是不是 UPDATE 的目标表 */
static bool
stmt_qualifier_is_update_target(const UpdateStmt *update_stmt, const char *qualifier)
{
    if (update_stmt == NULL || qualifier == NULL || update_stmt->relation == NULL)
        return false;
    if (update_stmt->relation->relname != NULL &&
        strcmp(update_stmt->relation->relname, qualifier) == 0)
        return true;
    if (update_stmt->relation->alias != NULL &&
        update_stmt->relation->alias->aliasname != NULL &&
        strcmp(update_stmt->relation->alias->aliasname, qualifier) == 0)
        return true;
    return false;
}

/* ReqID:TECC4.0 - 在 UPDATE 的 FROM 子句里按表名/别名找 RangeVar */
static RangeVar *
stmt_find_from_rangevar(const UpdateStmt *update_stmt, const char *qualifier)
{
    ListCell *lc;

    if (update_stmt == NULL || qualifier == NULL)
        return NULL;

    foreach(lc, update_stmt->fromClause)
    {
        Node *item = (Node *) lfirst(lc);
        RangeVar *rv;

        if (item == NULL || nodeTag(item) != FE_T_RangeVar)
            continue;

        rv = (RangeVar *) item;
        if (rv->relname != NULL && strcmp(rv->relname, qualifier) == 0)
            return rv;
        if (rv->alias != NULL && rv->alias->aliasname != NULL &&
            strcmp(rv->alias->aliasname, qualifier) == 0)
            return rv;
    }

    return NULL;
}

/*
 * ReqID:TECC4.0 - 密文拷贝（SET 密态列 = 另一个密态列）的声明长度校验（保守版）。
 *
 * 这类赋值由服务端搬运密文完成（同 DEK 同算法可直接搬），前端拿不到源值的明文
 * 长度：长度只存在于密文里，服务端也没有解密能力、密态类型的 typmod 强制转换是
 * 空操作，所以做不到原生那样"按值"判断。这里退一步按【声明长度】判断：源列声明
 * 长度大于目标列声明长度时无法保证值放得下，直接拒绝；源列声明不超过目标列时，
 * 写入侧已按声明长度校验过该列的值，拷贝不会超长，放行。
 * 代价：原生允许的"值恰好放得下"也会被拒（如 varchar(20) 里的 'abc' 拷进
 * varchar(5)），报错文案里写明了原因。
 */
static bool
stmt_check_ciphertext_copies(const UpdateStmt *update_stmt,
                             TeccStatementData *statement_data,
                             TeccColumnCacheItem **table_columns,
                             int table_column_count)
{
    ListCell *lc;

    if (update_stmt == NULL || update_stmt->targetList == NIL)
        return true;

    foreach(lc, update_stmt->targetList)
    {
        ResTarget *rt = (ResTarget *) lfirst(lc);
        ColumnRef *cref;
        const char *src_name = NULL;
        const char *qualifier = NULL;
        int nfields;
        TeccColumnCacheItem *tgt_item;
        TeccColumnCacheItem *src_item = NULL;

        if (rt == NULL || rt->name == NULL || rt->val == NULL)
            continue;
        if (nodeTag(rt->val) != FE_T_ColumnRef)
            continue;                     /* 只处理整列拷贝；表达式保持原行为 */

        tgt_item = stmt_find_column_item(table_columns, table_column_count, rt->name);
        if (!stmt_char_column_typmod(tgt_item))
            continue;                     /* 目标不是带声明长度的字符型密态列 */

        cref = (ColumnRef *) rt->val;
        nfields = list_length(cref->fields);
        if (nfields < 1)
            continue;
        src_name = strVal((Value *) llast(cref->fields));
        if (nfields >= 2)
            qualifier = strVal((Value *) list_nth(cref->fields, nfields - 2));
        if (src_name == NULL)
            continue;

        if (qualifier == NULL || stmt_qualifier_is_update_target(update_stmt, qualifier))
        {
            /* 同表内拷贝：列元数据就在手上的目标表里 */
            src_item = stmt_find_column_item(table_columns, table_column_count, src_name);
        }
        else
        {
            RangeVar *src_rv = stmt_find_from_rangevar(update_stmt, qualifier);
            TeccColumnCacheItem **src_cols = NULL;
            int src_count = 0;

            if (src_rv != NULL &&
                stmt_load_table_metadata(src_rv, statement_data, &src_cols, &src_count))
                src_item = stmt_find_column_item(src_cols, src_count, src_name);
            if (src_cols != NULL)
                pg_free(src_cols);
        }

        if (!stmt_char_column_typmod(src_item))
            continue;                     /* 源列长度未知（或非字符型）：保持原行为 */
        if (src_item->original_type_mod <= tgt_item->original_type_mod)
            continue;                     /* 声明不宽于目标列：拷贝不会超长 */

        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
            "value too long for type %s(%d) (copying from column \"%s\" declared %s(%d); "
            "a ciphertext copy cannot verify the value's actual length)",
            (tgt_item->original_type_oid == 1042) ? "character" : "character varying",
            tgt_item->original_type_mod - (int) sizeof(int32),
            src_item->column_name != NULL ? src_item->column_name : "?",
            (src_item->original_type_oid == 1042) ? "character" : "character varying",
            src_item->original_type_mod - (int) sizeof(int32));
        return false;
    }

    return true;
}

/*
 * Process UPDATE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_update(const UpdateStmt *update_stmt,
                                  TeccStatementData *statement_data)
{
    ListCell *lc;
    int set_clause_count = 0;
    int encrypted_column_count = 0;
    TeccValuesProcessorResult values_result;
    TeccWhereClauseProcessorResult where_result;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    Node *target_node = NULL;
    ResTarget *res_target = NULL;
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    TeccColumnRefData *col_ref = NULL;

    if (update_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_UPDATE;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing UPDATE statement");

    /* Process target table */
    if (update_stmt->relation != NULL)
    {
        RangeVar *range_var = update_stmt->relation;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "UPDATE target table: %s.%s",
                           range_var->schemaname ? range_var->schemaname : "(null)",
                           range_var->relname ? range_var->relname : "(null)");

        /* Load table metadata and identify encrypted columns */
        stmt_load_table_metadata(range_var, statement_data, &table_encrypted_columns, &table_encrypted_column_count);
    }

    /* Process SET clause */
    if (update_stmt->targetList != NIL) {
        for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            bool is_encrypted = false;

            target_node = (Node *) lc->data.ptr_value;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            res_target = (ResTarget *) target_node;
            if (res_target->name == NULL)
                continue;

            set_clause_count++;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "UPDATE SET clause: %s", res_target->name);

            /* Check if this column is encrypted using cached column list */
            if (table_encrypted_columns != NULL)
                is_encrypted = stmt_is_column_in_encrypted_list(res_target->name, table_encrypted_columns, table_encrypted_column_count);
            else
                is_encrypted = stmt_check_column_encrypted(statement_data,
                    update_stmt->relation ? update_stmt->relation->schemaname : NULL,
                    update_stmt->relation ? update_stmt->relation->relname : NULL,
                    res_target->name);

            if (is_encrypted) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "UPDATE SET column %s is encrypted",
                                   res_target->name);

                /* ReqID:TECC4.0 - For encrypted SET columns, only encrypt constant
                 * values client-side. ColumnRef/A_Expr values (e.g. SET c_f4 = c_i2)
                 * pass through unchanged — the server handles ciphertext copy
                 * between compatible encrypted numeric types directly. */
                if (res_target->val != NULL)
                {
                    int val_type = nodeTag(res_target->val);
                    bool is_const_val = (val_type == FE_T_Const
                                         || val_type == FE_T_Integer
                                         || val_type == FE_T_Float
                                         || val_type == FE_T_String
                                         || val_type == FE_T_RowExpr);
                    bool is_unary_minus = false;

                    if (val_type == FE_T_A_Expr)
                    {
                        A_Expr *aexpr = (A_Expr *)res_target->val;
                        /* ReqID:TECC4.0 - 仅当一元负号作用于常量（如 -5、-32768）
                         * 才按"常量值"加密；作用于列引用/表达式（如 -i4_a）时
                         * 是列运算，透传给服务端同态一元操作符处理。 */
                        is_unary_minus = (aexpr->kind == AEXPR_OP
                                          && aexpr->lexpr == NULL
                                          && aexpr->name != NIL
                                          && aexpr->rexpr != NULL
                                          && nodeTag(aexpr->rexpr) == FE_T_Const);
                    }

                    /* ReqID:TECC4.0 - UPDATE SET 密态列 = 常量::密态类型(N)
                     * ENCRYPTED WITH (...)：按显式 cast 语义先把常量截断到 cast
                     * 声明的长度（与 INSERT 的 cast 路径一致，见
                     * stmt_apply_cast_len_clip），然后按常量走通用加密改写。
                     * 加密参数（DEK/算法/排序规则）取【目标列】——值最终写进这一列，
                     * 密文必须与列自身路径一致；这与 INSERT 用 cast 声明的 DEK/算法
                     * 略有不同，已在台账登记。不这样做时这条 cast 既不加密也不报错，
                     * 原样发给服务端，服务端按密态类型输入函数解析明文报
                     * invalid hex string。
                     * 只处理最内层是常量的 cast；其它形态（如 f(x)::密态类型）
                     * 保持原有"交给服务端"的行为。 */
                    if (!is_const_val && !is_unary_minus &&
                        val_type == FE_T_TypeCast)
                    {
                        TypeCast *set_cast = (TypeCast *)res_target->val;
                        Node *inner = set_cast->arg;

                        while (inner != NULL && nodeTag(inner) == FE_T_TypeCast)
                            inner = ((TypeCast *)inner)->arg;

                        if (set_cast->typeName != NULL &&
                            set_cast->typeName->encryptedOpts != NIL &&
                            inner != NULL && nodeTag(inner) == FE_T_Const &&
                            !((Const *)inner)->constisnull)
                        {
                            Const *set_cv = (Const *)inner;

                            if (set_cv->constvalue.sval != NULL)
                            {
                                const char *set_type_name =
                                    strVal((Value *) llast(set_cast->typeName->names));
                                Oid set_base_oid =
                                    map_enc_type_name_to_base_oid(set_type_name);

                                if (OidIsValid(set_base_oid))
                                    tecc_char_cast_len_clip(set_cast->typeName->typemod,
                                                            set_base_oid,
                                                            set_cv->constvalue.sval,
                                                            statement_data->conn != NULL ?
                                                            PQclientEncoding(statement_data->conn) :
                                                            PG_UTF8);
                            }
                            is_const_val = true;
                        }
                    }

                    if (is_const_val || is_unary_minus)
                    {
                        /* Constant value — client can encrypt */
                        statement_data->needs_rewrite = 1;
                        encrypted_column_count++;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL && update_stmt->relation != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     update_stmt->relation->schemaname,
                                                     update_stmt->relation->relname,
                                                     res_target->name);
                            if (update_stmt->relation->alias != NULL
                                && update_stmt->relation->alias->aliasname != NULL)
                                tecc_column_ref_data_set_alias(col_ref,
                                    update_stmt->relation->alias->aliasname);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Added encrypted column %s.%s to encrypted_columns list",
                                update_stmt->relation->relname,
                                res_target->name);
                        }
                    }
                    else
                    {
                        /* ReqID:TECC4.0 - P2-3 同态运算结果赋值校验：
                         * 同态运算（二元 +/- *）结果密文仅含 he-add/he-mul 段，
                         * 只允许赋值给纯 he-add/he-mul 列（需求 V4.0.1-01-3 约束）；
                         * 赋值给含对称算法（DET/OPE/SEARCH）的列报错，
                         * 防止静默写入不可解密数据。 */
                        if (val_type == FE_T_A_Expr)
                        {
                            A_Expr *aexpr = (A_Expr *) res_target->val;

                            if (aexpr->kind == AEXPR_OP &&
                                aexpr->lexpr != NULL && aexpr->rexpr != NULL &&
                                aexpr->name != NIL)
                            {
                                Node *op_node = (Node *) llast(aexpr->name);
                                const char *op_name = NULL;

                                if (op_node != NULL && nodeTag(op_node) == FE_T_String)
                                    op_name = strVal((const Value *) op_node);

                                if (op_name != NULL &&
                                    (strcmp(op_name, "+") == 0 ||
                                     strcmp(op_name, "-") == 0 ||
                                     strcmp(op_name, "*") == 0 ||
                                     strcmp(op_name, "^") == 0))
                                {
                                    int target_algo = 0;
                                    int ti;

                                    for (ti = 0; ti < table_encrypted_column_count; ti++)
                                    {
                                        if (table_encrypted_columns[ti] != NULL &&
                                            table_encrypted_columns[ti]->column_name != NULL &&
                                            strcmp(table_encrypted_columns[ti]->column_name,
                                                   res_target->name) == 0)
                                        {
                                            target_algo =
                                                table_encrypted_columns[ti]->encryption_type;
                                            break;
                                        }
                                    }

                                    /* 对称算法位：0x01 DET / 0x02 OPE / 0x04 SEARCH */
                                    if (target_algo & (0x01 | 0x02 | 0x04))
                                    {
                                        tecc_processor_set_error(statement_data,
                                            TECC_PROCESSOR_INVALID_INPUT,
                                            "cannot assign HE-only ciphertext to column '%s' "
                                            "which uses symmetric algorithms (DET/OPE/SEARCH)",
                                            res_target->name);
                                        return TECC_STMT_PROCESSOR_ERROR;
                                    }
                                }
                            }
                        }
                        /* ColumnRef or expression — server handles ciphertext copy.
                         * Don't set needs_rewrite, don't add to encrypted_columns. */
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "UPDATE SET column %s: non-Const value, passing through for server-side copy",
                            res_target->name);
                    }
                }
            }

            /* Process the value expression if present */
            if (res_target->val != NULL)
            {
                /* Extract column references from the value expression */
                List *columns = NIL;
                stmt_extract_columns_from_expression(res_target->val, &columns, statement_data);

                /* Check if any referenced columns are encrypted */
                if (columns != NIL)
                {
                    ListCell *col_lc;
                    for (col_lc = columns->head; col_lc != NULL; col_lc = col_lc->next)
                    {
                        TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                        if (tecc_col != NULL && tecc_col->is_encrypted)
                        {
                            
                        }
                        tecc_column_ref_data_destroy(tecc_col);
                    }
                    list_free(columns);
                }
            }
        }

        /* ReqID:TECC4.0 - Expand RowExpr in multi-column SET.
         * When (col1, col2) = ROW(val1, val2), the grammar creates
         * a ResTarget per column with the same RowExpr val.
         * Expand: assign n'th RowExpr arg to n'th ResTarget. */
        {
            ListCell *mlc;
            for (mlc = update_stmt->targetList->head; mlc != NULL; mlc = mlc->next)
            {
                ResTarget *rt = (ResTarget *) mlc->data.ptr_value;
                Node *shared_val;
                if (rt == NULL || rt->val == NULL) continue;
                if (nodeTag(rt->val) != FE_T_RowExpr) continue;

                shared_val = rt->val;
                {
                    RowExpr *rx = (RowExpr *) shared_val;
                    int nargs = list_length(rx->args);
                    ListCell *mc2;
                    int idx;

                    /* Count consecutive targets sharing this RowExpr */
                    idx = 0;
                    for (mc2 = mlc; mc2 != NULL; mc2 = mc2->next)
                    {
                        ResTarget *rt2 = (ResTarget *) mc2->data.ptr_value;
                        if (rt2 == NULL || rt2->val != shared_val) break;
                        idx++;
                    }

                    if (idx > 1 && idx <= nargs)
                    {
                        /* Assign n'th arg to each target */
                        ListCell *assign_lc;
                        int ai;
                        for (ai = 0, assign_lc = mlc;
                             assign_lc != NULL && ai < nargs;
                             assign_lc = assign_lc->next, ai++)
                        {
                            ResTarget *rt2 = (ResTarget *) assign_lc->data.ptr_value;
                            if (rt2 == NULL || rt2->val != shared_val) break;
                            rt2->val = (Node *) list_nth(rx->args, ai);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Multi-col SET: column %s <- RowExpr arg[%d]",
                                rt2->name ? rt2->name : "?", ai);
                        }
                        /* Skip processed targets (minus 1 for loop incr) */
                        {
                            int skip;
                            for (skip = 0; skip < idx - 1 && mlc != NULL; skip++)
                                mlc = mlc->next;
                        }
                    }
                }
            }
        }

        /* Delegate to values processor for actual value processing */
        values_result = tecc_values_processor_process(statement_data);

        if (values_result != TECC_VALUES_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "SET clause processing failed: %d", values_result);
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "SET clause processing completed: %d columns", set_clause_count);

        /* ReqID:TECC4.0 - HE 标量运算 SET 赋值常量加密（第二遍，必须在
         * tecc_values_processor_process 之后：values 处理器会整体重建
         * raw_values_list，若在此前添加 HE 常量 raw value 会被覆盖丢失）。
         *
         * SET enc_col = enc_col + k / - k / * k 中的整数常量 k 在前端同态
         * 加密，改写为 `enc_col + 'hex'::intN_enc`，服务端走全密文同态
         * 操作符（he_add_generic / he_sub_generic / he_mul_generic），
         * 常量明文不出前端。^ 幂指数保持明文（服务端重复乘法循环次数）。
         *
         * 实现：临时把 SET 列加入 encrypted_columns 使表达式处理器能识别
         * 加密列，处理完立即还原（values 处理器对二元 A_Expr 值保持跳过，
         * 避免占位值损坏 SQL）。 */
        if (update_stmt->targetList != NIL)
        {
            ListCell *he_lc;

            for (he_lc = update_stmt->targetList->head; he_lc != NULL; he_lc = he_lc->next)
            {
                ResTarget *he_rt = (ResTarget *) he_lc->data.ptr_value;
                A_Expr *he_ae;
                Node *he_op_node;
                const char *he_op_name = NULL;

                if (he_rt == NULL || he_rt->val == NULL)
                    continue;
                if (nodeTag(he_rt->val) != FE_T_A_Expr)
                    continue;

                he_ae = (A_Expr *) he_rt->val;
                if (he_ae->kind != AEXPR_OP ||
                    he_ae->lexpr == NULL || he_ae->rexpr == NULL ||
                    he_ae->name == NIL)
                    continue;

                he_op_node = (Node *) llast(he_ae->name);
                if (he_op_node != NULL && nodeTag(he_op_node) == FE_T_String)
                    he_op_name = strVal((const Value *) he_op_node);

                /* ^ 幂指数保持明文，仅 + - * 走常量加密 */
                if (he_op_name == NULL ||
                    (strcmp(he_op_name, "+") != 0 &&
                     strcmp(he_op_name, "-") != 0 &&
                     strcmp(he_op_name, "*") != 0))
                    continue;

                /* ReqID:TECC4.0 - Skip non-encrypted columns:
                 * Only attempt HE constant encryption when the column
                 * actually has encryption metadata (DEK/KEK) in the cache.
                 * Non-encrypted columns (e.g. plain float4) should pass
                 * through without modification. */
                {
                    const char *he_schema = update_stmt->relation ?
                        update_stmt->relation->schemaname : NULL;
                    const char *he_table = update_stmt->relation ?
                        update_stmt->relation->relname : NULL;
                    char *he_dek_check = tecc_get_column_dek_name(
                        statement_data, he_schema, he_table, he_rt->name);
                    if (he_dek_check == NULL)
                    {
                        /* Column not encrypted — skip HE constant encryption */
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "UPDATE SET target column %s is not encrypted, skipping HE constant encryption",
                            he_rt->name ? he_rt->name : "?");
                        continue;
                    }
                    pfree(he_dek_check);
                }

                {
                    List *he_saved_cols;
                    List *he_tmp_cols = NIL;
                    TeccColumnRefData *he_tmp_ref = NULL;
                    TeccWhereClauseProcessorResult he_expr_result;

                    he_saved_cols = statement_data->encrypted_columns;

                    he_tmp_ref = tecc_column_ref_data_create();
                    if (he_tmp_ref != NULL)
                    {
                        tecc_column_ref_data_init(he_tmp_ref, NULL,
                            update_stmt->relation ? update_stmt->relation->schemaname : NULL,
                            update_stmt->relation ? update_stmt->relation->relname : NULL,
                            he_rt->name);
                        if (update_stmt->relation != NULL &&
                            update_stmt->relation->alias != NULL &&
                            update_stmt->relation->alias->aliasname != NULL)
                            tecc_column_ref_data_set_alias(he_tmp_ref,
                                update_stmt->relation->alias->aliasname);
                        he_tmp_cols = lappend(he_tmp_cols, he_tmp_ref);
                    }

                    statement_data->encrypted_columns = he_tmp_cols;
                    he_expr_result = tecc_where_clause_processor_process_expr(
                        he_rt->val, statement_data);
                    statement_data->encrypted_columns = he_saved_cols;

                    /* 释放临时 encrypted_columns（含 he_tmp_ref） */
                    if (he_tmp_ref != NULL)
                        tecc_column_ref_data_destroy(he_tmp_ref);
                    if (he_tmp_cols != NULL)
                        list_free(he_tmp_cols);

                    if (he_expr_result != TECC_WHERE_PROCESSOR_SUCCESS)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                            "UPDATE SET HE constant encryption failed for column %s",
                            he_rt->name ? he_rt->name : "?");
                        return TECC_STMT_PROCESSOR_ERROR;
                    }
                }
            }
        }
    }

    /* ReqID:TECC4.0 - Process UPDATE FROM clause for multi-table UPDATE.
     * Load metadata for FROM tables so encrypted columns in WHERE conditions
     * referencing FROM tables are properly recognized and encrypted. */
    if (update_stmt->fromClause != NIL)
    {
        ListCell *frm_lc;
        statement_data->multi_table_context = 1;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing UPDATE FROM clause (%d items)",
                           list_length(update_stmt->fromClause));
        for (frm_lc = update_stmt->fromClause->head; frm_lc != NULL; frm_lc = frm_lc->next)
        {
            Node *from_item = (Node *) frm_lc->data.ptr_value;
            if (from_item == NULL)
                continue;

            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *rv = (RangeVar *) from_item;
                TeccColumnCacheItem **from_cols = NULL;
                int from_col_count = 0;
                const char *alias_name = NULL;

                /* Extract alias if present */
                if (rv->alias != NULL && rv->alias->aliasname != NULL)
                    alias_name = rv->alias->aliasname;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "UPDATE FROM table: %s.%s (alias=%s)",
                                   rv->schemaname ? rv->schemaname : "public",
                                   rv->relname ? rv->relname : "(null)",
                                   alias_name ? alias_name : "(none)");

                stmt_load_table_metadata(rv, statement_data, &from_cols, &from_col_count);
                if (from_cols != NULL && from_col_count > 0)
                {
                    int ci;
                    for (ci = 0; ci < from_col_count; ci++)
                    {
                        if (from_cols[ci] == NULL) continue;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL)
                        {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     rv->schemaname,
                                                     rv->relname,
                                                     from_cols[ci]->column_name);
                            if (alias_name != NULL)
                                tecc_column_ref_data_set_alias(col_ref, alias_name);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added FROM encrypted column: %s.%s (alias=%s)",
                                               rv->relname, from_cols[ci]->column_name,
                                               alias_name ? alias_name : rv->relname);
                        }
                    }
                    pg_free(from_cols);
                }
            }
            else if (nodeTag(from_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively traverse JOIN arg tables */
                JoinExpr *j = (JoinExpr *) from_item;
                ListCell *jlc;
                List *join_items = NIL;
                join_items = lappend(join_items, j->larg);
                join_items = lappend(join_items, j->rarg);
                for (jlc = join_items->head; jlc != NULL; jlc = jlc->next)
                {
                    Node *ji = (Node *) jlc->data.ptr_value;
                    if (ji != NULL && nodeTag(ji) == FE_T_RangeVar)
                    {
                        RangeVar *jrv = (RangeVar *) ji;
                        TeccColumnCacheItem **jcols = NULL;
                        int jcol_count = 0;
                        const char *jalias = NULL;
                        if (jrv->alias != NULL && jrv->alias->aliasname != NULL)
                            jalias = jrv->alias->aliasname;
                        stmt_load_table_metadata(jrv, statement_data, &jcols, &jcol_count);
                        if (jcols != NULL && jcol_count > 0)
                        {
                            int ji2;
                            for (ji2 = 0; ji2 < jcol_count; ji2++)
                            {
                                if (jcols[ji2] == NULL) continue;
                                col_ref = tecc_column_ref_data_create();
                                if (col_ref != NULL)
                                {
                                    tecc_column_ref_data_init(col_ref, NULL,
                                                             jrv->schemaname,
                                                             jrv->relname,
                                                             jcols[ji2]->column_name);
                                    if (jalias != NULL)
                                        tecc_column_ref_data_set_alias(col_ref, jalias);
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, col_ref);
                                }
                            }
                            pg_free(jcols);
                        }
                    }
                }
                list_free(join_items);
            }
            else if (nodeTag(from_item) == FE_T_RangeSubselect)
            {
                /* ReqID:TECC4.0 - RangeSubselect in FROM clause */
                RangeSubselect *rss = (RangeSubselect *) from_item;
                if (rss->subquery != NULL && nodeTag(rss->subquery) == FE_T_SelectStmt)
                {
                    SelectStmt *subq = (SelectStmt *) rss->subquery;
                    TeccStatementData sub_data;
                    memset(&sub_data, 0, sizeof(TeccStatementData));
                    sub_data.conn = statement_data->conn;
                    tecc_stmt_processor_process_select(subq, &sub_data);
                    if (sub_data.needs_rewrite)
                        statement_data->needs_rewrite = 1;
                }
            }
        }
    }

    /* Ensure all encrypted columns from table are in encrypted_columns list
     * BEFORE WHERE clause processing. WHERE columns (e.g. 'c_int2') may not
     * be in SET clause and need to be in the list for where_is_encrypted_column()
     * to find them during the process phase. ReqID:TECC4.0 */
    if (table_encrypted_columns != NULL && update_stmt->relation != NULL)
    {
        int kk;
        for (kk = 0; kk < table_encrypted_column_count; kk++)
        {
            bool found_already = false;
            ListCell *lc2;
            if (table_encrypted_columns[kk] == NULL) continue;
            foreach(lc2, statement_data->encrypted_columns)
            {
                TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc2);
                if (ed != NULL)
                {
                    const char *cn = ed->column_name;
                    if (cn[0] == '\0') cn = ed->table_name;
                    if (strcmp(cn, table_encrypted_columns[kk]->column_name) == 0)
                    { found_already = true; break; }
                }
            }
            if (!found_already)
            {
                col_ref = tecc_column_ref_data_create();
                if (col_ref != NULL) {
                    tecc_column_ref_data_init(col_ref,
                        NULL,
                        update_stmt->relation->schemaname,
                        update_stmt->relation->relname,
                        table_encrypted_columns[kk]->column_name);
                    /* ReqID:TECC4.0 - Set table alias if target table has one */
                    if (update_stmt->relation->alias != NULL
                        && update_stmt->relation->alias->aliasname != NULL)
                        tecc_column_ref_data_set_alias(col_ref,
                            update_stmt->relation->alias->aliasname);
                    statement_data->encrypted_columns = lappend(
                        statement_data->encrypted_columns, col_ref);
                }
            }
        }
    }

    /* Process WHERE clause if present */
    if (update_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing UPDATE WHERE clause");

        /* Delegate to WHERE clause processor */
        where_result = tecc_where_clause_processor_process(
            update_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET (deterministic encryption) algorithm, "
                        "but the column is not configured with DET.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires OPE (order-preserving encryption) algorithm, "
                        "but the column is not configured with OPE.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires SEARCH (searchable encryption) algorithm, "
                        "but the column is not configured with SEARCH.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET or OPE algorithm, "
                        "but the column is not configured with either.");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator is not compatible with the encryption algorithm on this column.");
                    break;
                case TECC_WHERE_PROCESSOR_DEK_MISMATCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Cannot compare encrypted columns with different DEKs. "
                        "Please ensure the compared columns use the same DEK.");
                    break;
                case TECC_WHERE_PROCESSOR_LIKE_COLUMN:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "LIKE/NOT LIKE between two encrypted columns is not supported. "
                        "The LIKE pattern must be a plain string constant.");
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_AGGREGATE:
                    /* Error message already set by
                     * stmt_validate_aggregate_funcall() in the walker hook. */
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR:
                    /* Error message already set by
                     * stmt_validate_scalar_funcall() in the walker hook. */
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE clause processing completed successfully");
    }

    /* ReqID:TECC4.0 - 密文拷贝的声明长度校验（见 stmt_check_ciphertext_copies）：
     * 放在 FROM/WHERE 处理之后，此时目标表与 FROM 表的列元数据都已就绪。 */
    if (!stmt_check_ciphertext_copies(update_stmt, statement_data,
                                      table_encrypted_columns,
                                      table_encrypted_column_count))
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - 元数据加载失败时上抛，避免加密表被误判为普通表导致明文泄漏。 */
    if (statement_data->error_code != TECC_PROCESSOR_SUCCESS)
        return TECC_STMT_PROCESSOR_ERROR;

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process DELETE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_delete(const DeleteStmt *delete_stmt,
                                  TeccStatementData *statement_data)
{
    TeccColumnCacheItem **table_encrypted_columns = NULL;
    int table_encrypted_column_count = 0;
    int i;

    if (delete_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* ReqID:TECC4.0 - Set stmt_type */
    statement_data->stmt_type = TECC_STMT_TYPE_DELETE;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing DELETE statement");

    /* Process target table */
    if (delete_stmt->relation != NULL)
    {
        /* Extract table information */
        const char *table_name = delete_stmt->relation->relname;
        const char *schema_name = delete_stmt->relation->schemaname;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE target table: %s.%s",
                           schema_name ? schema_name : "public",
                           table_name ? table_name : "(null)");

        /* Check if table contains encrypted columns */
        /* Query metadata cache to check for encrypted columns in this table */
        if (table_name != NULL) {
            /* Load table metadata and get encrypted columns list */
            stmt_load_table_metadata(delete_stmt->relation, statement_data,
                                   &table_encrypted_columns, &table_encrypted_column_count);

            /* Populate encrypted_columns list for WHERE clause processing */
            /* 填充encrypted_columns供where_is_encrypted_column判断，但不强制needs_rewrite
             * 只有当WHERE中确实有加密列时，后续WHERE处理才会设needs_rewrite */
            if (table_encrypted_columns != NULL && table_encrypted_column_count > 0)
            {
                for (i = 0; i < table_encrypted_column_count; i++)
                {
                    TeccColumnCacheItem *cache_item = table_encrypted_columns[i];
                    if (cache_item != NULL && cache_item->column_name[0] != '\0')
                    {
                        TeccColumnRefData *col_ref;
                        col_ref = tecc_column_ref_data_create();
                        if (col_ref != NULL) {
                            tecc_column_ref_data_init(col_ref,
                                                     NULL,
                                                     schema_name ? schema_name : "public",
                                                     table_name,
                                                     cache_item->column_name);
                            /* ReqID:TECC4.0 - Set table alias if target table has one */
                            if (delete_stmt->relation->alias != NULL
                                && delete_stmt->relation->alias->aliasname != NULL)
                                tecc_column_ref_data_set_alias(col_ref,
                                    delete_stmt->relation->alias->aliasname);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added encrypted column %s.%s to encrypted_columns list",
                                               table_name, cache_item->column_name);
                        }
                    }
                }
                /* 不在这里设needs_rewrite=1，由WHERE处理阶段按需设置 */
            }
        }
    }

    /* ReqID:TECC4.0 - Process DELETE USING clause for multi-table DELETE.
     * Load metadata for USING tables so encrypted columns in WHERE conditions
     * referencing USING tables are properly recognized and encrypted. */
    if (delete_stmt->usingClause != NIL)
    {
        ListCell *us_lc;
        statement_data->multi_table_context = 1;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE USING clause (%d items)",
                           list_length(delete_stmt->usingClause));
        for (us_lc = delete_stmt->usingClause->head; us_lc != NULL; us_lc = us_lc->next)
        {
            Node *using_item = (Node *) us_lc->data.ptr_value;
            if (using_item == NULL)
                continue;

            if (nodeTag(using_item) == FE_T_RangeVar)
            {
                RangeVar *rv = (RangeVar *) using_item;
                TeccColumnCacheItem **us_cols = NULL;
                int us_col_count = 0;
                const char *alias_name = NULL;

                if (rv->alias != NULL && rv->alias->aliasname != NULL)
                    alias_name = rv->alias->aliasname;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "DELETE USING table: %s.%s (alias=%s)",
                                   rv->schemaname ? rv->schemaname : "public",
                                   rv->relname ? rv->relname : "(null)",
                                   alias_name ? alias_name : "(none)");

                stmt_load_table_metadata(rv, statement_data, &us_cols, &us_col_count);
                if (us_cols != NULL && us_col_count > 0)
                {
                    int ci;
                    for (ci = 0; ci < us_col_count; ci++)
                    {
                        TeccColumnRefData *us_col_ref;
                        if (us_cols[ci] == NULL) continue;
                        us_col_ref = tecc_column_ref_data_create();
                        if (us_col_ref != NULL)
                        {
                            tecc_column_ref_data_init(us_col_ref,
                                                     NULL,
                                                     rv->schemaname,
                                                     rv->relname,
                                                     us_cols[ci]->column_name);
                            if (alias_name != NULL)
                                tecc_column_ref_data_set_alias(us_col_ref, alias_name);
                            statement_data->encrypted_columns = lappend(
                                statement_data->encrypted_columns, us_col_ref);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added USING encrypted column: %s.%s (alias=%s)",
                                               rv->relname, us_cols[ci]->column_name,
                                               alias_name ? alias_name : rv->relname);
                        }
                    }
                    pg_free(us_cols);
                }
            }
            else if (nodeTag(using_item) == FE_T_JoinExpr)
            {
                /* ReqID:TECC4.0 - Recursively traverse JOIN arg tables */
                JoinExpr *j = (JoinExpr *) using_item;
                ListCell *jlc;
                List *join_items = NIL;
                join_items = lappend(join_items, j->larg);
                join_items = lappend(join_items, j->rarg);
                for (jlc = join_items->head; jlc != NULL; jlc = jlc->next)
                {
                    Node *ji = (Node *) jlc->data.ptr_value;
                    if (ji != NULL && nodeTag(ji) == FE_T_RangeVar)
                    {
                        RangeVar *jrv = (RangeVar *) ji;
                        TeccColumnCacheItem **jcols = NULL;
                        int jcol_count = 0;
                        const char *jalias = NULL;
                        if (jrv->alias != NULL && jrv->alias->aliasname != NULL)
                            jalias = jrv->alias->aliasname;
                        stmt_load_table_metadata(jrv, statement_data, &jcols, &jcol_count);
                        if (jcols != NULL && jcol_count > 0)
                        {
                            int ji2;
                            for (ji2 = 0; ji2 < jcol_count; ji2++)
                            {
                                TeccColumnRefData *jcol_ref;
                                if (jcols[ji2] == NULL) continue;
                                jcol_ref = tecc_column_ref_data_create();
                                if (jcol_ref != NULL)
                                {
                                    tecc_column_ref_data_init(jcol_ref, NULL,
                                                             jrv->schemaname,
                                                             jrv->relname,
                                                             jcols[ji2]->column_name);
                                    if (jalias != NULL)
                                        tecc_column_ref_data_set_alias(jcol_ref, jalias);
                                    statement_data->encrypted_columns = lappend(
                                        statement_data->encrypted_columns, jcol_ref);
                                }
                            }
                            pg_free(jcols);
                        }
                    }
                }
                list_free(join_items);
            }
        }
    }

    /* Process WHERE clause if present */
    if (delete_stmt->whereClause != NULL)
    {
        List *where_columns;
        ListCell *lc;
        bool has_encrypted_columns;
        TeccWhereClauseProcessorResult where_result;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing DELETE WHERE clause");

        /* First, extract columns from WHERE clause to check for encrypted columns */
        where_columns = NIL;
        stmt_extract_columns_from_expression(delete_stmt->whereClause, &where_columns, statement_data);

        /* Check if any extracted columns are encrypted */
        if (where_columns != NULL)
        {
            has_encrypted_columns = false;

            for (lc = where_columns->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL && tecc_col->is_encrypted)
                {
                    has_encrypted_columns = true;
                    
                }
            }

            /* If WHERE clause contains encrypted columns, mark statement as needing rewrite */
            if (has_encrypted_columns)
            {
                statement_data->needs_rewrite = 1;
                
            }

            /* Clean up column list */
            if (where_columns != NULL)
            {
                for (lc = where_columns->head; lc != NULL; lc = lc->next)
                {
                    TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                    if (tecc_col != NULL)
                        tecc_column_ref_data_destroy(tecc_col);
                }
                list_free(where_columns);
            }
        }

        /* Delegate to WHERE clause processor for actual processing */
        where_result = tecc_where_clause_processor_process(
            delete_stmt->whereClause, statement_data);

        if (where_result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            switch (where_result)
            {
                case TECC_WHERE_PROCESSOR_MISSING_DET:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET (deterministic encryption) algorithm, "
                        "but the column is not configured with DET.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires OPE (order-preserving encryption) algorithm, "
                        "but the column is not configured with OPE.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_SEARCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires SEARCH (searchable encryption) algorithm, "
                        "but the column is not configured with SEARCH.");
                    break;
                case TECC_WHERE_PROCESSOR_MISSING_DET_OPE:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator requires DET or OPE algorithm, "
                        "but the column is not configured with either.");
                    break;
                case TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Operator is not compatible with the encryption algorithm on this column.");
                    break;
                case TECC_WHERE_PROCESSOR_DEK_MISMATCH:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "Cannot compare encrypted columns with different DEKs. "
                        "Please ensure the compared columns use the same DEK.");
                    break;
                case TECC_WHERE_PROCESSOR_LIKE_COLUMN:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "LIKE/NOT LIKE between two encrypted columns is not supported. "
                        "The LIKE pattern must be a plain string constant.");
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_AGGREGATE:
                    /* Error message already set by
                     * stmt_validate_aggregate_funcall() in the walker hook. */
                    break;
                case TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR:
                    /* Error message already set by
                     * stmt_validate_scalar_funcall() in the walker hook. */
                    break;
                default:
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                        "WHERE clause processing failed");
                    break;
            }
            statement_data->needs_rewrite = 0;
            statement_data->where_needs_rewrite = 0;
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DELETE statement has no WHERE clause");
    }



    /* ReqID:TECC4.0 - 元数据加载失败时上抛，避免加密表被误判为普通表导致明文泄漏。 */
    if (statement_data->error_code != TECC_PROCESSOR_SUCCESS)
        return TECC_STMT_PROCESSOR_ERROR;

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/* Forward declarations */
static char *binary_to_hex_string(const unsigned char *data, size_t data_len);
static const char *find_matching_close_paren(const char *open_paren);

/*
 * Helper function to extract option value from CREATE KEK options
 */
static const char *
get_kek_option_value(const List *options, const char *opt_name)
{
    ListCell *lc;

    if (options == NULL)
        return NULL;

    foreach(lc, options)
    {
        DefElem *def = (DefElem *) lfirst(lc);
        /* ReqID:TECC4.0 - 选项名大小写不敏感：TECC 前端解析器保留 SQL 标识符
         * 原始大小写（不象标准 PG 词法那样折叠小写），用户按 tecc_ktool 模板
         * 写大写 KEY_STORE/KEY_ALGO/KEY_PATH 时必须能匹配到 'key_store' 等
         * 小写键名，否则 sm2 KEK 的 PUBLIC_KEY 不会注入 hg_kek_args。 */
        if (def->defname && strcasecmp(def->defname, opt_name) == 0)
        {
            if (def->arg)
            {
                if (nodeTag(def->arg) == FE_T_String)
                {
                    Value *val = (Value *) def->arg;
                    return val->val.str;
                }
                else if (nodeTag(def->arg) == FE_T_Const)
                {
                    Const *c = (Const *) def->arg;
                    if (c->consttype == 25 || c->consttype == 227)
                        return c->constvalue.sval;
                }
            }
        }
    }
    return NULL;
}

/*
 * basename of a key path (last '/' or '\' segment) -- mirrors the backend's
 * tecc_key_path_basename used by the createkek.c duplicate-file checks.
 */
static const char *
tecc_kek_path_basename_fr(const char *path)
{
    const char *last;

    if (path == NULL)
        return "";

    last = strrchr(path, '/');
#ifdef _WIN32
    {
        const char *bwd = strrchr(path, '\\');
        if (bwd != NULL && (last == NULL || bwd > last))
            last = bwd;
    }
#endif
    return (last != NULL) ? last + 1 : path;
}

/*
 * tecc_create_kek_check_duplicate - frontend duplicate pre-check for CREATE KEK.
 *
 * QA 报修: 用一个已登记 key 文件再建第二个 KEK 时，后端 createkek.c 的查重报错
 * 在 lc_messages=zh_CN 服务器上显示中文本地化前缀（"错误:"/"描述:"），而同会话
 * 里其余 TECC CREATE KEK 校验（缺文件等）都是前端硬编码英文（"ERROR:"）。
 * 为让 tecc 客户端路径语言统一，在发 SQL 前用 SQL 镜像后端的权威查重规则
 * （createkek.c tecc_kek_key_path_check + get_kek_oid_ext）并以前端英文风格中止。
 * 后端查重保留作兜底（非 -k/纯 SQL 客户端仍会触发）。
 *
 * 该目录查询是 best-effort：任何查询失败都放行让后端决定，可读目录绝不阻塞
 * CREATE KEK。
 *
 * Returns 0 on no conflict (or check could not run), -1 on conflict after
 * printing the English ERROR/HINT diagnostic to stderr.
 */
static int
tecc_create_kek_check_duplicate(TeccState *state,
                                TeccStatementData *statement_data,
                                const char *kek_name,
                                const char *key_store,
                                const char *key_path,
                                const char *key_namespace,
                                bool if_not_exists)
{
    static const char query[] =
        "SELECT k.kek_name, n.nspname, "
        "  coalesce((SELECT encode(ka.value, 'escape') FROM hg_kek_args ka "
        "             WHERE ka.kek_id = k.oid AND ka.function_name = 'kek_config' "
        "               AND upper(ka.key) = 'KEY_PATH'), ''), "
        "  coalesce((SELECT encode(ka.value, 'escape') FROM hg_kek_args ka "
        "             WHERE ka.kek_id = k.oid AND ka.function_name = 'kek_config' "
        "               AND upper(ka.key) = 'KEY_STORE'), '') "
        "FROM hg_kek k "
        "JOIN pg_namespace n ON n.oid = k.key_namespace";
    const char *new_basename;
    PGconn *conn;
    PGresult *res;
    int i;
    int nrows;

    if (statement_data == NULL || statement_data->conn == NULL ||
        kek_name == NULL || key_store == NULL || key_path == NULL)
        return 0;

    conn = statement_data->conn;
    new_basename = tecc_kek_path_basename_fr(key_path);

    /* The nested catalog query must not itself be TECC-preprocessed. */
    if (state != NULL)
        state->disable_once = true;
    res = PQexec(conn, query);
    if (state != NULL)
        state->disable_once = false;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        if (res != NULL)
            PQclear(res);
        return 0;
    }

    nrows = PQntuples(res);

    /*
     * Pass 1: duplicate KEK name in the same namespace (mirrors backend's
     * get_kek_oid_ext check, which runs before the KEY_PATH scan).  A name
     * conflict under IF NOT EXISTS is NOT an error -- the backend emits a
     * NOTICE and skips, so pass through and let it do so.
     */
    if (key_namespace != NULL)
    {
        for (i = 0; i < nrows; i++)
        {
            const char *ex_name = PQgetvalue(res, i, 0);
            const char *ex_nsp = PQgetvalue(res, i, 1);

            if (strcmp(ex_name, kek_name) == 0 &&
                strcmp(ex_nsp, key_namespace) == 0)
            {
                PQclear(res);
                if (if_not_exists)
                    return 0;	/* backend will NOTICE + skip */
                fprintf(stderr, "ERROR: KEK \"%s\" already exists\n", kek_name);
                return -1;
            }
        }
    }

    /*
     * Pass 2: another KEK already uses the same key file.  Mirrors backend
     * tecc_kek_key_path_check:
     *   - identical full path is a conflict only within the same KEY_STORE
     *     (different hardware may share a slot string like "hsm:id=1");
     *   - a different path with the same file name (basename) is a conflict
     *     even across directories / key stores.
     */
    for (i = 0; i < nrows; i++)
    {
        const char *ex_path = PQgetvalue(res, i, 2);
        const char *ex_store = PQgetvalue(res, i, 3);

        if (ex_path[0] == '\0')
            continue;

        if (strcmp(ex_path, key_path) == 0)
        {
            bool same_store = (ex_store[0] == '\0' ||
                               strcmp(ex_store, key_store) == 0);

            if (!same_store)
                continue;	/* different key_store, same slot: OK */

            fprintf(stderr,
                    "ERROR: KEY_PATH \"%s\" is already used by another KEK\n",
                    key_path);
            fprintf(stderr, "HINT: Each KEK must use a unique key file.\n");
            PQclear(res);
            return -1;
        }

        if (strcmp(tecc_kek_path_basename_fr(ex_path), new_basename) == 0)
        {
            fprintf(stderr,
                    "ERROR: key file name \"%s\" is already used by another KEK at \"%s\"\n",
                    new_basename, ex_path);
            fprintf(stderr,
                    "HINT: Each KEK must have a unique file name, even across different directories.\n");
            PQclear(res);
            return -1;
        }
    }

    PQclear(res);
    return 0;
}

/*
 * Process CREATE KEK statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_kek(const CreateKekStmt *create_kek_stmt,
                                      TeccStatementData *statement_data)
{
    TeccCreateKekProcessorResult result;

    if (create_kek_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /*
     * 前端重复预检：同名 KEK / 同一 key 文件。后端 createkek.c 的查重报错在
     * lc_messages=zh_CN 服务器上会带中文本地化标签（"错误:"/"描述:"），与其余
     * 前端英文校验不一致（QA 报修）；这里发 SQL 前先用英文前端风格拦截，后端
     * 查重仍保留作权威兜底。best-effort，查重查询失败则放行。
     */
    {
        const char *kek_name;
        const char *key_store;
        const char *key_path;
        const char *key_namespace = NULL;
        TeccState *state;

        kek_name = (create_kek_stmt->names != NIL) ?
            strVal(llast(create_kek_stmt->names)) : NULL;
        key_store = get_kek_option_value(create_kek_stmt->options, "key_store");
        key_path = get_kek_option_value(create_kek_stmt->options, "key_path");

        state = tecc_get_state(statement_data->conn);
        if (kek_name != NULL && key_store != NULL && key_path != NULL &&
            state != NULL)
        {
            /* 目标 namespace：限定名取 schema 段，否则取 current_schema */
            if (list_length(create_kek_stmt->names) > 1)
                key_namespace = strVal(linitial(create_kek_stmt->names));
            else
                key_namespace = tecc_cache_get_guc_param(state, "current_schema");

            if (tecc_create_kek_check_duplicate(state, statement_data,
                                                kek_name, key_store, key_path,
                                                key_namespace,
                                                create_kek_stmt->if_not_exists) != 0)
                return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* Delegate to CREATE KEK processor */
    result = tecc_create_kek_processor_run(create_kek_stmt, statement_data);

    /* Convert result */
    switch (result)
    {
        case TECC_CREATE_KEK_PROCESSOR_SUCCESS:
        {
            TeccState *state = tecc_get_state(statement_data->conn);
            if (state != NULL)
            {
                /* Store key name for post-SQL PENDING cleanup */
                const char *kek_name;
                kek_name = (create_kek_stmt->names != NIL) ?
                    strVal(llast(create_kek_stmt->names)) : NULL;
                if (kek_name != NULL)
                {
                    const char *key_namespace;
                    TeccKekCacheItem *pending;

                    state->last_stmt_type = TECC_STMT_TYPE_CREATE_KEK;

                    strncpy(state->last_key_name, kek_name, NAMEDATALEN - 1);
                    state->last_key_name[NAMEDATALEN - 1] = '\0';

                    /* 从限定名中提取namespace（schema），无限定名则使用current_schema */
                    if (list_length(create_kek_stmt->names) > 1)
                    {
                        key_namespace = strVal(linitial(create_kek_stmt->names));
                    }
                    else
                    {
                        key_namespace = tecc_cache_get_guc_param(state, "current_schema");
                    }
                    if (key_namespace != NULL)
                    {
                        strncpy(state->last_key_namespace, key_namespace, NAMEDATALEN - 1);
                        state->last_key_namespace[NAMEDATALEN - 1] = '\0';
                    }
                    else
                    {
                        state->last_key_namespace[0] = '\0';
                    }

                    /* Insert PENDING cache item */
                    pending = (TeccKekCacheItem *)pg_malloc0(sizeof(TeccKekCacheItem));
                    pending->key_name = pg_strdup(kek_name);
                    pending->key_namespace = pg_strdup(key_namespace ? key_namespace : "");
                    pending->kek_oid = InvalidOid;
                    pending->cache_status = TECC_CACHE_STATUS_PENDING;
                    if (tecc_cache_insert_kek_item(state, pending))
                    {
                        pg_free(pending->key_name);
                        pg_free(pending->key_namespace);
                        pg_free(pending);
                    }
                }
            }

            /* ReqID:TECC4.0 - HE 硬加密：sm2 KEK 时前端获取公钥（65 字节未压缩点），
             * 改写 SQL 注入 public_key，替代后端 createkek.c 读 .pub 文件。 */
            {
                const char *algo = get_kek_option_value(create_kek_stmt->options, "key_algo");
                const char *key_store;
                const char *key_path;
                unsigned char *pub_point = NULL;
                size_t pub_len = 0;

                key_store = get_kek_option_value(create_kek_stmt->options, "key_store");
                key_path = get_kek_option_value(create_kek_stmt->options, "key_path");

                if (algo != NULL &&
                    strcmp(algo, "sm2") == 0 &&
                    key_store != NULL && key_path != NULL)
                {
				if (strcmp(key_store, "localkms") == 0)
				{
					/* 读 .pub（65 字节未压缩点 04||x||y，tecc_ktool 直接输出裸点） */
					const char *ext = strrchr(key_path, '.');
					char pub_path[1024];
					FILE *fp;

					if (ext != NULL && strcmp(ext, ".priv") == 0 &&
						(size_t)(ext - key_path) < sizeof(pub_path) - 5)
					{
						size_t base_len = (size_t)(ext - key_path);
						memcpy(pub_path, key_path, base_len);
						memcpy(pub_path + base_len, ".pub", 5);
					}
					else
					{
						snprintf(pub_path, sizeof(pub_path), "%s.pub", key_path);
					}

					fp = fopen(pub_path, "rb");
					if (fp != NULL)
					{
						unsigned char buf[65];

						if (fread(buf, 1, 65, fp) == 65 && buf[0] == 0x04)
						{
							pub_point = (unsigned char *)OPENSSL_malloc(65);
							if (pub_point != NULL)
							{
								memcpy(pub_point, buf, 65);
								pub_len = 65;
							}
						}
						fclose(fp);
					}
				}
                    else if (strcmp(key_store, "swxakms") == 0 ||
                             strcmp(key_store, "tzwykms") == 0 ||
                             strcmp(key_store, "hsmkms") == 0)
                    {
                        /* HSM export_public_key → raw 点(bits+x+y) → 65 字节 04||x||y */
                        HsmAdapterInterface *adapter =
                            (strcmp(key_store, "swxakms") == 0) ? get_swxa_adapter() : get_tzhsm_adapter();
                        unsigned int key_index = 0;
                        HsmDevice *device = NULL;

                        if (adapter != NULL && parse_hsm_key_path(key_path, &key_index) &&
                            adapter->init(&device, NULL) == HSM_SUCCESS)
                        {
                            if (adapter->connect(device) == HSM_SUCCESS)
                            {
                                void *raw = NULL;
                                size_t raw_len = 0;

                                if (adapter->export_public_key(device, key_index, &raw, &raw_len) == HSM_SUCCESS &&
                                    raw != NULL && raw_len == 68)
                                {
                                    pub_point = (unsigned char *)OPENSSL_malloc(65);
                                    if (pub_point != NULL)
                                    {
                                        pub_point[0] = 0x04;
                                        memcpy(pub_point + 1, (unsigned char *)raw + 4, 64);
                                        pub_len = 65;
                                    }
                                }
                                if (raw != NULL)
                                    free(raw);
                                adapter->disconnect(device);
                            }
                            adapter->cleanup(device);
                        }
                    }

                    if (pub_point != NULL && pub_len == 65)
                    {
                        statement_data->kek_public_key_hex = binary_to_hex_string(pub_point, pub_len);
                        OPENSSL_free(pub_point);
                    }
                }
            }

            /* ReqID:TECC4.0 - SQL 改写：注入 public_key 参数到 CREATE KEK 的 WITH 括号 */
            if (statement_data->kek_public_key_hex != NULL)
            {
                const char *q = statement_data->query;
                const char *open_paren = (q != NULL) ? strchr(q, '(') : NULL;
                const char *close_paren = (open_paren != NULL) ? find_matching_close_paren(open_paren) : NULL;

                if (q != NULL && open_paren != NULL && close_paren != NULL)
                {
                    size_t prefix_len = (size_t)(close_paren - q);
                    size_t suffix_len = strlen(q) - prefix_len - 1;
					const char *head = ", public_key = '";
					const char *mid = "', _tecc_processed = 'yes'";
					size_t hexlen = strlen(statement_data->kek_public_key_hex);
					size_t new_len = prefix_len + strlen(head) + hexlen + strlen(mid) + 1 + suffix_len + 1;
					char *new_query = (char *)pg_malloc(new_len);
					size_t pos = 0;

					memcpy(new_query + pos, q, prefix_len);
					pos += prefix_len;
					memcpy(new_query + pos, head, strlen(head));
					pos += strlen(head);
					memcpy(new_query + pos, statement_data->kek_public_key_hex, hexlen);
					pos += hexlen;
					memcpy(new_query + pos, mid, strlen(mid));
					pos += strlen(mid);
					new_query[pos++] = ')';
                    memcpy(new_query + pos, close_paren + 1, suffix_len);
                    pos += suffix_len;
                    new_query[pos] = '\0';

                    if (statement_data->rewritten_query != NULL)
                        pg_free(statement_data->rewritten_query);
                    statement_data->rewritten_query = new_query;
                    tecc_stmt_set_needs_rewrite(statement_data, true);
                }
            }

            return TECC_STMT_PROCESSOR_SUCCESS;
        }
        case TECC_CREATE_KEK_PROCESSOR_INVALID_INPUT:
        case TECC_CREATE_KEK_PROCESSOR_VALIDATION_ERROR:
        case TECC_CREATE_KEK_PROCESSOR_UNSUPPORTED_KMS:
            return TECC_STMT_PROCESSOR_ERROR;
        case TECC_CREATE_KEK_PROCESSOR_HSM_ERROR:
            tecc_processor_debug(TECC_LOG_ERROR,
                                "HSM error in CREATE KEK processing");
            return TECC_STMT_PROCESSOR_ERROR;
        case TECC_CREATE_KEK_PROCESSOR_INTERNAL_ERROR:
        default:
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/*
 * tecc_stmt_processor_process_alter_kek - Process ALTER KEK statement
 *
 * Performs DEK re-encryption: loads old/new KEK files, decrypts each
 * associated DEK with the old KEK, re-encrypts with the new KEK, and
 * builds a rewritten SQL that updates hg_dek_args with the new values.
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_alter_kek(const AlterKekStmt *alter_kek_stmt,
                                      TeccStatementData *statement_data)
{
    const char *kek_name;
    const char *kek_namespace = NULL;    /* 限定名里的模式；未限定为 NULL */
    const char *kek_lookup_name;         /* 前端查找用名（限定名带点号） */
    char        kek_qname[2 * NAMEDATALEN + 2];
    const char *kek_eff_namespace;       /* 3a 查询实际过滤的模式（见下文的身份统一） */
    char        kek_eff_nsp_buf[NAMEDATALEN];
    TeccKekCacheItem *kek_item = NULL;   /* 元数据缓存项：同路径去重 + 模式回填 */
    const char *new_key_path = NULL;
    ListCell   *lc;
    TeccState  *state;
    PGconn     *conn;
    unsigned char *old_kek = NULL;
    size_t       old_kek_len = 0;
    unsigned char *new_kek = NULL;
    size_t       new_kek_len = 0;
    int          ret;
    const char *q;
    size_t      qlen;
    const char *close_paren;
    size_t      prefix;
    char       *rewritten;
    PQExpBufferData update_buf = {NULL, 0, 0};
    PGresult   *res;
    int         n_deks;
    int         i;
    Oid         kek_oid = InvalidOid;
    bool        is_asym = false;
    bool        tecc_was_disabled = false;
    /* ReqID:TECC4.0 - 非对称 KEK 重加密：新 KEK 公钥（由私钥 DER 派生一次，
     * 供对称 DEK encrypted_value 信封与 HE 私钥信封复用） */
    unsigned char *new_pub = NULL;
    size_t       new_pub_len = 0;

    if (alter_kek_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    kek_name = (alter_kek_stmt->names != NIL) ?
        strVal(llast(alter_kek_stmt->names)) : NULL;
    if (kek_name == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /*
     * ReqID:TECC4.0 - Bug 9330: 保留限定名里的模式。
     *
     * ALTER KEK 的名字来自 any_name，可以写成 sch1.kek_sm4（跨模式轮换）。
     * 但前端后续所有 KEK 查找都只接受"一个名称字符串"（明文缓存、元数据缓存、
     * key_manager、按 KEK 名查 hg_kek），限定名只能靠 "schema.name" 点号形式
     * 表达 —— CREATE DEK 的 KEK = 'sch1.kek_sm4' 走的就是这条路。原实现只取
     * names 的末元素（裸名），模式被整个丢掉，查找随之退回 search_path；当 KEK
     * 所在模式不在 search_path 上时，必然报 "Failed to load old KEK"
     * （日志里先有 "KEK 'x' not found in cache"），把"跨模式轮换"伪装成
     * "KEK 不存在"，用户无从下手。
     *
     * 元素取法与 PG DeconstructQualifiedName 一致：末元素为名、次末元素为模式
     * （any_name 允许 catalog.schema.name，此时模式是倒数第二个元素）。
     *
     * 注意这里不能写 lsecond()：前端的列表 API 是 tecc_frontend_list.h 里那套
     * 按函数实现的 shim（lfirst/llast/linitial/list_nth_cell），没有 lsecond，
     * 而本编译单元也不再包含 pg_list.h 的宏版本 —— 写了会编出对 lsecond 的
     * 外部调用，链接期不报错（共享库默认允许未定义符号），加载时才炸
     * "symbol lookup error: undefined symbol: lsecond"。取次末元素统用
     * lfirst(list_nth_cell(l, n))（同本文件其它调用点）。
     */
    {
        int nparts = list_length(alter_kek_stmt->names);

        if (nparts >= 3)
            kek_namespace = strVal((Value *) lfirst(list_nth_cell(alter_kek_stmt->names, 1)));
        else if (nparts == 2)
            kek_namespace = strVal(linitial(alter_kek_stmt->names));
    }

    if (kek_namespace != NULL)
    {
        snprintf(kek_qname, sizeof(kek_qname), "%s.%s", kek_namespace, kek_name);
        kek_lookup_name = kek_qname;
    }
    else
        kek_lookup_name = kek_name;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing ALTER KEK '%s'", kek_lookup_name);

    /* Extract new key_path from ALTER KEK options */
    foreach(lc, alter_kek_stmt->options)
    {
        DefElem *opt = (DefElem *) lfirst(lc);
        if (opt != NULL && opt->defname != NULL &&
            strcasecmp(opt->defname, "key_path") == 0)
        {
            if (opt->arg != NULL)
            {
                if (nodeTag(opt->arg) == FE_T_String)
                    new_key_path = strVal((Value *)opt->arg);
                else if (nodeTag(opt->arg) == FE_T_Const)
                    new_key_path = ((Const *)opt->arg)->constvalue.sval;
            }
            break;
        }
    }

    if (new_key_path == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "ALTER KEK missing KEY_PATH option");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    conn = statement_data->conn;
    if (conn == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    state = tecc_get_state(conn);
    if (state == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                "Failed to get TECC state");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /*
     * ReqID:TECC4.0 - Bug 9255: 前端 USAGE 权限预检，且必须先于旧 KEK 加载。
     *
     * 无 USAGE 的 KEK 会被服务端 hg_kek_info 视图
     * （... WHERE has_kek_privilege(k.oid, 'USAGE')）过滤掉，根本进不了本地
     * KEK 缓存；于是"加载旧 KEK"必然失败，用户看到的是
     * "Failed to load old KEK"（伴随 "KEK 'x' not found in cache" 日志），
     * 把权限不足伪装成 KEK 不存在，用户无从知道要 GRANT。
     *
     * 这里给出与后端 aclcheck_error() 同文案的明确报错。只拦"确无权限"，
     * 权限查询自身的连接/内存类失败不阻断，仍由后端 AlterKekCmd 做权威校验。
     */
    {
        TeccFetcherErrorCode perm_ret;

        perm_ret = tecc_check_kek_permission((const TeccState *)state, kek_name,
                                             kek_namespace);
        if (perm_ret == TECC_FETCHER_ERROR_PERMISSION)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK '%s': user lacks USAGE privilege", kek_name);
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "permission denied for key encryption key %s",
                                    kek_name);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* ReqID:TECC4.0 - 同路径 ALTER 报错：新 KEY_PATH 与当前完全相同是
     * no-op 轮换（DEK 用同一把密钥重加密，密钥材料未变）。在重加密
     * DEK 前直接拒绝，避免误导用户以为轮换成功，也避免无谓的客户端
     * 重加密工作（后端 AlterKekCmd 还有权威校验兜底）。 */
    {
        kek_item =
            tecc_cache_get_kek_by_name_search_path((const TeccState *)state, kek_lookup_name);
        if (kek_item != NULL && kek_item->kms_path != NULL &&
            strcmp(kek_item->kms_path, new_key_path) == 0)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                    "ALTER KEK: new KEY_PATH is identical to the current key path; "
                                    "specify a different KEY_PATH to rotate the KEK");
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /*
     * ReqID:TECC4.0 - Bug 9330（未限定名一侧）：统一"这把 KEK 是谁"的判定。
     *
     * 旧 KEK 明文来自 tecc_key_get_kek_with_state() → key_manager，未限定名时它
     * 走的是 tecc_cache_get_kek_by_name_search_path()（current_schema → search_path
     * 逐个模式），身份是"search_path 解析到的那把"。而下面 3a 若只按名字查 hg_kek，
     * 在多模式同名 KEK 存在时会另选一把（无 ORDER BY，取堆序第一行）——明文来自 A
     * 模式、OID 与 DEK 列表来自 B 模式。实测后果（同材料同名 KEK，search_path 指向
     * B）：dek（归属 A）的信封被 B 的新密钥重加密，而 hg_dek.kek_id 仍是 A，该 DEK
     * 之后永久解不开，此前的 ALTER 却报成功 —— 静默且不可逆。
     *
     * 因此未限定名时把 3a 的过滤模式回填为"上一步缓存实际解析到的模式"，让明文加载、
     * 元数据缓存、3a 三处用同一身份。kek_item 与 old_kek 加载用的是同一个查找函数，
     * 裸名能走到这里就说明它非空，故这条回填是完备的；限定名则已有精确模式，不受影响。
     */
    kek_eff_namespace = kek_namespace;
    if (kek_eff_namespace == NULL && kek_item != NULL && kek_item->key_namespace != NULL)
    {
        strncpy(kek_eff_nsp_buf, kek_item->key_namespace, NAMEDATALEN - 1);
        kek_eff_nsp_buf[NAMEDATALEN - 1] = '\0';
        kek_eff_namespace = kek_eff_nsp_buf;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "ALTER KEK '%s': new KEY_PATH = '%s'",
                        kek_name, new_key_path);

    /*
     * Step 1: Load the OLD KEK plaintext.
     * Use tecc_key_get_kek_with_state which handles cache + file loading.
     * 用限定名（如有），否则缓存/元数据查找会退回 search_path —— Bug 9330。
     */
    old_kek = tecc_key_get_kek_with_state((void *)state, kek_lookup_name,
                                          &old_kek_len);
    if (old_kek == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                "Failed to load old KEK");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "ALTER KEK: loaded old KEK (len=%zu)",
                        old_kek_len);

    /*
     * Step 2: Load the NEW KEK.
     * - HSM KEK（旧 KEK 为 16 字节 HSM placeholder）→ 新 KEK 也在密码机内，
     *   按新 key_path 解析 key_index，构造新 placeholder（复用 HSM/SWXA 标记）。
     * - localkms KEK → 用 local_kms_load_key 读取并解密新密钥文件。
     */
    {
        bool old_is_hsm = (old_kek_len == 16 &&
                           (memcmp(old_kek + 4, "HSM_KEY", 7) == 0 ||
                            memcmp(old_kek + 4, "SWXAKEY", 7) == 0));

        if (old_is_hsm)
        {
            unsigned int new_key_index = 0;

            if (!parse_hsm_key_path(new_key_path, &new_key_index))
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "ALTER KEK: invalid HSM key path for new KEK: %s",
                                        new_key_path);
                tecc_key_free_key_data(old_kek, old_kek_len);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            new_kek = (unsigned char *)pg_malloc(16);
            if (new_kek == NULL)
            {
                tecc_key_free_key_data(old_kek, old_kek_len);
                return TECC_STMT_PROCESSOR_ERROR;
            }
            new_kek_len = 16;
            memset(new_kek, 0, new_kek_len);
            new_kek[0] = (new_key_index >> 0) & 0xFF;
            new_kek[1] = (new_key_index >> 8) & 0xFF;
            new_kek[2] = (new_key_index >> 16) & 0xFF;
            new_kek[3] = (new_key_index >> 24) & 0xFF;
            memcpy(new_kek + 4, old_kek + 4, 7);   /* 复用 HSM_KEY/SWXAKEY 标记 */

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK: built HSM placeholder for new KEK (key_index=%u)",
                                new_key_index);
        }
        else
        {
            LocalKmsContext ctx;
            char *last_slash;
            char *new_key_path_dup;

            memset(&ctx, 0, sizeof(ctx));
            ctx.encrypt_files = true;
            ctx.file_mode = 0600;

            new_key_path_dup = pg_strdup(new_key_path);
            if (new_key_path_dup == NULL)
            {
                tecc_key_free_key_data(old_kek, old_kek_len);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            last_slash = strrchr(new_key_path_dup, '/');
            if (last_slash != NULL)
            {
                *last_slash = '\0';
                ctx.base_path = new_key_path_dup;
                ctx.key_filename = last_slash + 1;
            }
            else
            {
                ctx.base_path = pg_strdup(".");
                ctx.key_filename = new_key_path_dup;
            }

            /* ReqID:TECC4.0 - Honor TECC_KEK_PATH like CREATE KEK does.
             * The C layer overrides the base dir so the new KEK file is located
             * in the cross-platform KEK directory, not the stored absolute path. */
            {
                const char *env_path = getenv("TECC_KEK_PATH");
                if (env_path != NULL && env_path[0] != '\0')
                {
                    if (last_slash == NULL && ctx.base_path)
                        pg_free(ctx.base_path);
                    ctx.base_path = pg_strdup(env_path);
                    /* last_slash no longer refers into ctx.base_path */
                    last_slash = NULL;
                }
            }

            ret = local_kms_load_key(&ctx, ctx.key_filename, &new_kek, &new_kek_len);
            if (ret != 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "Failed to load new KEK file. "
                                        "Check that the file exists and was created by tecc_ktool.");
                tecc_key_free_key_data(old_kek, old_kek_len);
                if (last_slash == NULL && ctx.base_path)
                    pg_free(ctx.base_path);
                pg_free(new_key_path_dup);
                return TECC_STMT_PROCESSOR_ERROR;
            }

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK: loaded new KEK from '%s' (len=%zu)",
                                new_key_path, new_kek_len);

            /* ctx.base_path may point into new_key_path_dup; only free if it
             * was separately allocated (the "else" branch with pg_strdup(".")). */
            if (last_slash == NULL && ctx.base_path)
                pg_free(ctx.base_path);
            pg_free(new_key_path_dup);
        }
    }

    /*
     * Step 3: Query the server for KEK metadata and DEK list.
     * Temporarily disable TECC to avoid recursive processing.
     */
    state->disable_once = true;
    tecc_was_disabled = true;

    /* 3a. Get KEK OID and key_algo from server */
    {
        char kek_escaped[NAMEDATALEN * 2 + 2];
        char nsp_escaped[NAMEDATALEN * 2 + 2];
        char kek_query[1024];
        int  qwritten;

        /*
         * ReqID:TECC4.0 - Bug 9330: 取名必须连模式一起限定。
         *
         * hg_kek 是基表（不是带 has_kek_privilege 过滤的 hg_kek_info 视图），
         * 只按 kek_name 匹配会命中"任意一个"同名 KEK。多个模式里有同名 KEK 时，
         * 跨模式轮换会拿到别的模式那个 KEK 的 OID，接着按该 OID 重加密一批
         * 本不相干的 DEK —— 比直接报错更危险的静默错绑。限定名给出模式时，
         * 一并按 pg_namespace.nspname 过滤。
         *
         * 用 kek_eff_namespace 而不是 kek_namespace：未限定名时它已被回填为
         * search_path 实际解析到的模式（见上文身份统一），保证本查询取到的
         * OID/DEK 列表与旧 KEK 明文来自同一把 KEK。两者在限定名场景下等价。
         */
        PQescapeStringConn(conn, kek_escaped, kek_name,
                          strlen(kek_name), NULL);

        if (kek_eff_namespace != NULL)
        {
            PQescapeStringConn(conn, nsp_escaped, kek_eff_namespace,
                              strlen(kek_eff_namespace), NULL);
            qwritten = snprintf(kek_query, sizeof(kek_query),
                "SELECT k.oid, "
                "  coalesce((SELECT encode(ka.value, 'escape') FROM hg_kek_args ka "
                "             WHERE ka.kek_id = k.oid AND upper(ka.key) = 'KEY_ALGO'), "
                "           'sm4') "
                "FROM hg_kek k JOIN pg_namespace n ON k.key_namespace = n.oid "
                "WHERE k.kek_name = '%s' AND n.nspname = '%s'",
                kek_escaped, nsp_escaped);
        }
        else
        {
            qwritten = snprintf(kek_query, sizeof(kek_query),
                "SELECT k.oid, "
                "  coalesce((SELECT encode(ka.value, 'escape') FROM hg_kek_args ka "
                "             WHERE ka.kek_id = k.oid AND upper(ka.key) = 'KEY_ALGO'), "
                "           'sm4') "
                "FROM hg_kek k WHERE k.kek_name = '%s'",
                kek_escaped);
        }

        if (qwritten < 0 || qwritten >= (int) sizeof(kek_query))
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "ALTER KEK: failed to build KEK lookup query");
            goto cleanup;
        }

        res = PQexec(conn, kek_query);
        if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK ||
            PQntuples(res) == 0)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "ALTER KEK: KEK not found in database");
            if (res) PQclear(res);
            goto cleanup;
        }

        if (kek_oid == InvalidOid)
            kek_oid = atooid(PQgetvalue(res, 0, 0));
        {
            const char *kek_type_val = PQgetvalue(res, 0, 1);
            is_asym = (kek_type_val != NULL &&
                       strcmp(kek_type_val, "sm2") == 0);
        }
        PQclear(res);
        res = NULL;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ALTER KEK: KEK OID=%u, is_asym=%d",
                            kek_oid, is_asym);
    }

    /* 3b. Query list of DEKs associated with this KEK */
    {
        char dek_query[512];

        /* ReqID:TECC4.0 - Mark HE DEKs (those with he_ec_privkey or
         * he_eg_privkey) so Step 4 skips their encrypted_value (placeholder
         * for HE DEKs; decrypting it with the KEK would fail). */
        snprintf(dek_query, sizeof(dek_query),
            "SELECT d.oid, d.dek_name, "
            "  coalesce(encode(da.value, 'hex'), ''), "
            "  EXISTS(SELECT 1 FROM hg_dek_args he "
            "         WHERE he.dek_id = d.oid AND upper(he.key) IN "
            "           ('HE_EC_PRIVKEY','HE_EG_PRIVKEY')) AS is_he "
            "FROM hg_dek d "
            "JOIN hg_dek_args da ON d.oid = da.dek_id AND da.key = 'encrypted_value' "
            "WHERE d.kek_id = %u",
            kek_oid);

        res = PQexec(conn, dek_query);
        if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "ALTER KEK: failed to query DEK list");
            if (res) PQclear(res);
            goto cleanup;
        }

        n_deks = PQntuples(res);
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ALTER KEK: found %d DEKs", n_deks);
    }

    /*
     * Step 4: For each DEK, decrypt with old KEK and re-encrypt with new KEK.
     * Build UPDATE statements for all DEKs.
     */
    initPQExpBuffer(&update_buf);

    /* ReqID:TECC4.0 - 非对称 KEK：派生/导出新 KEK 公钥。
     * - HSM 托管（new_kek 为 16 字节 placeholder）：从密码机导出 SM2 公钥
     *   （export_public_key → raw 点 → 65 字节 04||x||y）；
     * - localkms：从新 KEK 私钥 DER 派生。
     * 任一 DEK 需要信封重加密，派生失败整体中止（与解密失败同语义）。 */
    if (is_asym && n_deks > 0)
    {
        bool new_is_hsm = (new_kek_len == 16 &&
                           (memcmp(new_kek + 4, "HSM_KEY", 7) == 0 ||
                            memcmp(new_kek + 4, "SWXAKEY", 7) == 0));

        if (new_is_hsm)
        {
            bool is_swxa = (memcmp(new_kek + 4, "SWXAKEY", 7) == 0);
            unsigned int new_key_index = (unsigned int)new_kek[0] |
                                         ((unsigned int)new_kek[1] << 8) |
                                         ((unsigned int)new_kek[2] << 16) |
                                         ((unsigned int)new_kek[3] << 24);
            HsmAdapterInterface *hsm_adapter =
                is_swxa ? get_swxa_adapter() : get_tzhsm_adapter();
            HsmDevice *device = NULL;
            void *raw = NULL;
            size_t raw_len = 0;

            if (hsm_adapter != NULL && hsm_adapter->export_public_key != NULL &&
                hsm_adapter->init(&device, NULL) == HSM_SUCCESS)
            {
                if (hsm_adapter->connect(device) == HSM_SUCCESS)
                {
                    if (hsm_adapter->export_public_key(device, new_key_index,
                                                       &raw, &raw_len) == HSM_SUCCESS &&
                        raw != NULL && raw_len == 68)
                    {
                        new_pub = (unsigned char *)OPENSSL_malloc(65);
                        if (new_pub != NULL)
                        {
                            new_pub[0] = 0x04;
                            memcpy(new_pub + 1, (unsigned char *)raw + 4, 64);
                            new_pub_len = 65;
                        }
                    }
                    if (raw != NULL)
                        free(raw);
                    hsm_adapter->disconnect(device);
                }
                hsm_adapter->cleanup(device);
            }

            if (new_pub == NULL || new_pub_len == 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                         "ALTER KEK aborted: failed to derive new KEK "
                                         "public key. No key rotation was applied.");
                if (new_pub)
                    OPENSSL_free(new_pub);
                termPQExpBuffer(&update_buf);
                goto cleanup;
            }
        }
        else
        {
            if (tecc_sm2_derive_pubkey(new_kek, new_kek_len,
                                       &new_pub, &new_pub_len) != TECC_SM2_SUCCESS ||
                new_pub == NULL || new_pub_len == 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                         "ALTER KEK aborted: failed to derive new KEK "
                                         "public key. No key rotation was applied.");
                if (new_pub)
                    OPENSSL_free(new_pub);
                termPQExpBuffer(&update_buf);
                goto cleanup;
            }
        }
        tecc_processor_debug(TECC_LOG_DEBUG1,
                             "ALTER KEK: derived new KEK public key (len=%zu)",
                             new_pub_len);
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "ALTER KEK: processing %d DEKs with old KEK len=%zu, new KEK len=%zu",
                        n_deks, old_kek_len, new_kek_len);

    for (i = 0; i < n_deks; i++)
    {
        const char *dek_oid_str;
        const char *dek_name_str;
        const char *enc_hex;
        size_t      enc_bin_len;
        unsigned char *enc_bin = NULL;
        unsigned char *plain_dek = NULL;
        size_t       plain_dek_len = 0;
        unsigned char *new_enc = NULL;
        size_t       new_enc_len = 0;
        char        *new_enc_hex = NULL;
        bool         is_he_dek = false;
        int          j;

        dek_oid_str = PQgetvalue(res, i, 0);
        dek_name_str = PQgetvalue(res, i, 1);
        enc_hex = PQgetvalue(res, i, 2);
        is_he_dek = (PQgetvalue(res, i, 3) != NULL &&
                     strcmp(PQgetvalue(res, i, 3), "t") == 0);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ALTER KEK: DEK[%d] oid=%s name='%s' enc_hex_len=%zu is_he=%d",
                            i, dek_oid_str, dek_name_str,
                            enc_hex ? strlen(enc_hex) : 0, is_he_dek);

        /* ReqID:TECC4.0 - HE DEKs have a placeholder encrypted_value; skip
         * the traditional SM4 re-encryption for them (decrypting with the
         * SM2 KEK would fail).  Their HE private key envelopes are handled
         * separately below. */
        if (is_he_dek)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK: DEK '%s' is HE, skipping encrypted_value re-encryption",
                                dek_name_str);
            continue;
        }

        if (enc_hex == NULL || enc_hex[0] == '\0')
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK: DEK '%s' has no encrypted_value, skipping",
                                dek_name_str);
            continue;
        }

        /*
         * Convert hex to binary.
         * NOTE: The encrypted_value in hg_dek_args was stored as the ASCII
         * representation of the hex string (byteain escape format without \x
         * prefix). So encode(value, 'hex') returns the hex encoding of ASCII
         * hex chars. We need TWO hex decodes: first to get the ASCII hex
         * string, second to get the actual binary encrypted DEK.
         */
        enc_bin_len = strlen(enc_hex) / 2;
        {
            unsigned char *ascii_hex;
            size_t       actual_len;

            /* First decode: hex → ASCII (the original hex string) */
            ascii_hex = (unsigned char *)pg_malloc(enc_bin_len);
            if (ascii_hex == NULL)
                continue;

            for (j = 0; j < (int)enc_bin_len; j++)
            {
                char c1 = enc_hex[j * 2];
                char c2 = enc_hex[j * 2 + 1];
                unsigned char nib1, nib2;

                nib1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                       (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                       (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
                nib2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                       (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                       (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
                ascii_hex[j] = (nib1 << 4) | nib2;
            }

            /* Second decode: ASCII hex string → encrypted DEK binary */
            actual_len = enc_bin_len / 2;
            enc_bin = (unsigned char *)pg_malloc(actual_len);
            if (enc_bin == NULL)
            {
                pg_free(ascii_hex);
                continue;
            }

            for (j = 0; j < (int)actual_len; j++)
            {
                char c1 = (char)ascii_hex[j * 2];
                char c2 = (char)ascii_hex[j * 2 + 1];
                unsigned char nib1, nib2;

                nib1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                       (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                       (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
                nib2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                       (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                       (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
                enc_bin[j] = (nib1 << 4) | nib2;
            }
            enc_bin_len = actual_len;

            pg_free(ascii_hex);
        }

        /* Decrypt DEK with old KEK.
         * A failure here is fatal: abort the whole rotation so that
         * NO DEK is re-encrypted and KEY_PATH is NOT updated. Otherwise
         * some DEKs would be re-encrypted with the new KEK while others
         * remain under the old KEK — later reads would fail HMAC.
         * ReqID:TECC4.0 - 用 tecc_key_decrypt_dek 统一入口：HSM KEK（16 字节
         * placeholder）走密码机解密，本地 KEK 走 security_decrypt_dek_auto
         * （按 blob 结构自动区分 SM2 信封与 SM4-CBC+HMAC）。 */
        ret = tecc_key_decrypt_dek(enc_bin, enc_bin_len,
                                   old_kek, old_kek_len,
                                   &plain_dek, &plain_dek_len);
        if (ret != SECURITY_SUCCESS)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "ALTER KEK aborted: failed to decrypt DEK "
                                    "with old KEK. No key rotation was applied.");
            tecc_processor_debug(TECC_LOG_WARNING,
                                "ALTER KEK: failed to decrypt DEK '%s' (ret=%d)",
                                dek_name_str, ret);
            pg_free(enc_bin);
            goto cleanup;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ALTER KEK: decrypted DEK '%s' (len=%zu)",
                            dek_name_str, plain_dek_len);

        /* Re-encrypt with new KEK.
         * Same fatal semantics as decryption above: abort everything.
         * Modify - ReqID:TECC4.0 - 必须与 CREATE DEK 走**同一套**分派
         * （encrypt_dek_based_on_kek_type）：DEK 的封装方式由 KEK 的**托管方式**
         * 决定，而不只是"是否非对称"——
         *   HSM 托管（非对称也算）→ 设备 SM4 封装；
         *   本地 KEK → 非对称走 SM2 信封、对称走 SM4-CBC+HMAC。
         * 这里原先只按 is_asym 分派，非对称 HSM KEK 的 DEK 被写成 SM2 信封，
         * 而读取侧对 HSM placeholder 一律走设备 SM4 解密（tecc_key_decrypt_dek
         * 的 HSM 分支），于是轮换后该 KEK 下所有对称 DEK 立即失效
         * （SDF_Decrypt 失败 + "key file no longer matches this DEK"）。 */
        ret = encrypt_dek_based_on_kek_type(plain_dek, plain_dek_len,
                                            new_kek, new_kek_len, is_asym,
                                            &new_enc, &new_enc_len);
        if (ret != SECURITY_SUCCESS)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "ALTER KEK aborted: failed to re-encrypt DEK "
                                    "with new KEK. No key rotation was applied.");
            tecc_processor_debug(TECC_LOG_WARNING,
                                "ALTER KEK: failed to encrypt DEK '%s' with new KEK (ret=%d)",
                                dek_name_str, ret);
            security_secure_erase(plain_dek, plain_dek_len);
            pg_free(plain_dek);
            pg_free(enc_bin);
            goto cleanup;
        }

        /* Convert new encrypted DEK to hex */
        new_enc_hex = binary_to_hex_string(new_enc, new_enc_len);

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "ALTER KEK: re-encrypted DEK '%s' (hex_len=%zu)",
                            dek_name_str, new_enc_hex ? strlen(new_enc_hex) : 0);

        /* Build the metadata write-back.
         * NOTE: The original format stores the hex string as ASCII bytes
         * (via byteain escape format); hg_kek_rotate_dek_arg() does the
         * convert_to() internally.
         *
         * ReqID:TECC4.0 - Bug 9255: 走 SECURITY DEFINER 的受控入口，而不是在用户
         * 会话里裸发 UPDATE hg_dek_args —— 该表的 UPDATE 权限不开放给普通用户，
         * 裸 UPDATE 会让"已 GRANT USAGE ON KEK"的用户依旧被
         * permission denied for table hg_dek_args 挡住。函数内部按
         * has_kek_privilege(session_user, kek_id, 'USAGE') 校验，口径与后端
         * AlterKekCmd 一致。
         */
        if (new_enc_hex != NULL)
        {
            appendPQExpBuffer(&update_buf,
                "SELECT hg_kek_rotate_dek_arg(%s, 'encrypted_value', '%s');\n",
                dek_oid_str, new_enc_hex);

            pg_free(new_enc_hex);
        }

        /* Cleanup per-DEK buffers.
         * Modify - ReqID:TECC4.0 - new_enc 恒为 pg_malloc 缓冲区：
         * encrypt_dek_based_on_kek_type 的非对称分支已把
         * tecc_sm2_envelope_encrypt（OPENSSL_malloc）的结果拷进 pg_malloc，
         * HSM 与本地对称分支同样返回可 pg_free 的缓冲区。 */
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        security_secure_erase(new_enc, new_enc_len);
        pg_free(new_enc);
        pg_free(enc_bin);
    }

    /* ReqID:TECC4.0 - Non-asymmetric KEK rotation must also re-envelope the
     * HE private keys (he_ec_privkey/he_eg_privkey) of associated HE DEKs
     * with the NEW KEK's public key.  The old KEK (SM2 private key DER)
     * decrypts the old envelope; the new KEK public key re-encrypts it.
     * NOTE: res (DEK list) is still valid here; freed after this block. */
    if (is_asym && n_deks > 0 && new_pub != NULL)
    {
        /* ReqID:TECC4.0 - new_pub 已在 DEK 重加密循环前由 new_kek 私钥 DER
         * 派生一次（与对称 DEK 信封重加密共享）。 */
        /* ReqID:TECC4.0 - 新 KEK 是否托管在密码机：决定重封装 HE 私钥信封
         * 时是否需要分块（设备 ECCCipher.C 上限 136 字节）。判定口径与
         * CREATE DEK、运行时解封侧一致。 */
        {
            bool        new_kek_is_device = (new_kek_len == 16 &&
                                             new_kek != NULL &&
                                             (memcmp(new_kek + 4, "HSM_KEY", 7) == 0 ||
                                              memcmp(new_kek + 4, "SWXAKEY", 7) == 0));
            /* For each HE DEK, re-envelope its private keys */
            for (i = 0; i < n_deks; i++)
            {
                const char *dek_oid_str = PQgetvalue(res, i, 0);
                const char *he_priv_keys[] = {"he_ec_privkey", "he_eg_privkey"};
                int         hi;

                for (hi = 0; hi < 2; hi++)
                {
                    char        he_key_q[600];
                    PGresult   *he_res;
                    const char *he_key = he_priv_keys[hi];
                    const char *env_hex;
                    char       *env_hex_own = NULL;
                    unsigned char *plain_acc = NULL;
                    size_t      plain_acc_len = 0;
                    char       *new_env_hex = NULL;
                    unsigned char *plain_he = NULL;
                    size_t      plain_he_len = 0;
                    int         k;
                    int         nchunks;
                    int         ci;

                    /* Load the old envelope (hex) for this HE private key */
                    snprintf(he_key_q, sizeof(he_key_q),
                        "SELECT encode(value, 'escape') FROM hg_dek_args "
                        "WHERE dek_id = %s AND upper(key) = upper('%s')",
                        dek_oid_str, he_key);
                    he_res = PQexec(conn, he_key_q);
                    if (he_res == NULL ||
                        PQresultStatus(he_res) != PGRES_TUPLES_OK ||
                        PQntuples(he_res) == 0)
                    {
                        if (he_res) PQclear(he_res);
                        continue;   /* DEK has no this HE private key */
                    }
                    env_hex = PQgetvalue(he_res, 0, 0);
                    if (env_hex == NULL || env_hex[0] == '\0')
                    {
                        PQclear(he_res);
                        continue;
                    }
                    /* 信封 hex 在分块循环中一路要用，先拷出再释放结果集 */
                    env_hex_own = pg_strdup(env_hex);
                    PQclear(he_res);
                    he_res = NULL;
                    if (env_hex_own == NULL)
                        continue;
                    env_hex = env_hex_own;

                    /* ReqID:TECC4.0 - 逐块解封后拼接。KEK 托管在密码机时
                     * he-mul 的 256 字节私钥被拆成 ≤128 字节的多块各自封装
                     * （设备 ECCCipher.C 上限 136 字节）；legacy 单信封
                     * （无 'v2:' 前缀）按 1 块处理，行为与原来完全一致。 */
                    nchunks = tecc_sm2_env_chunk_count(env_hex);
                    if (nchunks <= 0)
                    {
                        pg_free(env_hex_own);
                        continue;
                    }

                    for (ci = 0; ci < nchunks; ci++)
                    {
                        char       *chunk_hex = NULL;
                        unsigned char *chunk_bin;
                        size_t      chunk_bin_len;
                        unsigned char *plain_one = NULL;
                        size_t      plain_one_len = 0;
                        bool        unwrap_failed = false;

                        if (tecc_sm2_env_chunk_hex(env_hex, ci, &chunk_hex) != TECC_SM2_SUCCESS)
                            break;

                        /* Decode hex (ASCII hex chars stored as bytea) */
                        chunk_bin_len = strlen(chunk_hex) / 2;
                        chunk_bin = (unsigned char *)pg_malloc(chunk_bin_len);
                        if (chunk_bin == NULL)
                        {
                            pg_free(chunk_hex);
                            unwrap_failed = true;
                        }
                        else
                        {
                            for (k = 0; k < (int)chunk_bin_len; k++)
                            {
                                char c1 = chunk_hex[k * 2];
                                char c2 = chunk_hex[k * 2 + 1];
                                unsigned char n1, n2;
                                n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                                     (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                                     (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
                                n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                                     (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                                     (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
                                chunk_bin[k] = (n1 << 4) | n2;
                            }
                            pg_free(chunk_hex);
                            chunk_hex = NULL;

                            /* Decrypt one chunk with old KEK private key.
                             * HSM-hosted SM2 KEK（16 字节 placeholder）：走设备
                             * sm2_decrypt，SM2 私钥不出密码机；localkms：本地
                             * tecc_sm2_envelope_decrypt。 */
                            {
                                bool            old_is_hsm = (old_kek_len == 16 &&
                                                              (memcmp(old_kek + 4, "HSM_KEY", 7) == 0 ||
                                                               memcmp(old_kek + 4, "SWXAKEY", 7) == 0));

                                if (old_is_hsm)
                                {
                                    bool            is_swxa = (memcmp(old_kek + 4, "SWXAKEY", 7) == 0);
                                    unsigned int    key_index = (unsigned int)old_kek[0] |
                                                                ((unsigned int)old_kek[1] << 8) |
                                                                ((unsigned int)old_kek[2] << 16) |
                                                                ((unsigned int)old_kek[3] << 24);
                                    HsmAdapterInterface *hsm_adapter =
                                        is_swxa ? get_swxa_adapter() : get_tzhsm_adapter();
                                    HsmDevice       *device = NULL;

                                    if (hsm_adapter != NULL && hsm_adapter->sm2_decrypt != NULL &&
                                        hsm_adapter->init(&device, NULL) == HSM_SUCCESS)
                                    {
                                        if (hsm_adapter->connect(device) == HSM_SUCCESS)
                                        {
                                            if (hsm_adapter->sm2_decrypt(device, key_index,
                                                                         chunk_bin, chunk_bin_len,
                                                                         (void **)&plain_one,
                                                                         &plain_one_len) == HSM_SUCCESS)
                                            {
                                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                                     "ALTER KEK: unwrapped HE private key chunk %d/%d via HSM sm2_decrypt (len=%zu)",
                                                                     ci + 1, nchunks, plain_one_len);
                                            }
                                            hsm_adapter->disconnect(device);
                                        }
                                        hsm_adapter->cleanup(device);
                                    }
                                }
                                else
                                {
                                    if (tecc_sm2_envelope_decrypt(old_kek, old_kek_len,
                                                                  chunk_bin, chunk_bin_len,
                                                                  &plain_one, &plain_one_len) != TECC_SM2_SUCCESS)
                                    {
                                        plain_one = NULL;
                                        plain_one_len = 0;
                                    }
                                }
                            }
                            pg_free(chunk_bin);

                            if (plain_one == NULL || plain_one_len == 0)
                                unwrap_failed = true;
                        }

                        if (unwrap_failed)
                        {
                            if (plain_one != NULL)
                                OPENSSL_free(plain_one);
                            break;
                        }

                        /* 追加本块明文 */
                        plain_acc = (unsigned char *)pg_realloc(plain_acc,
                                                                plain_acc_len + plain_one_len);
                        if (plain_acc == NULL)
                        {
                            OPENSSL_free(plain_one);
                            plain_acc_len = 0;
                            break;
                        }
                        memcpy(plain_acc + plain_acc_len, plain_one, plain_one_len);
                        plain_acc_len += plain_one_len;
                        OPENSSL_free(plain_one);
                        plain_one = NULL;
                    }

                    plain_he = plain_acc;
                    plain_he_len = plain_acc_len;
                    if (plain_he == NULL || plain_he_len == 0)
                    {
                        if (plain_he != NULL)
                        {
                            pg_free(plain_he);
                            plain_he = NULL;
                        }
                        pg_free(env_hex_own);
                        continue;
                    }

                    /* ReqID:TECC4.0 - 重新封装给新 KEK：新 KEK 托管在密码机时
                     * 同样要分块（设备 ECCCipher.C 上限 136 字节），否则轮换后
                     * he-mul 私钥又变成设备解不开的信封。 */
                    if (tecc_sm2_envelope_encrypt_hex(new_pub, new_pub_len,
                                                      plain_he, plain_he_len,
                                                      new_kek_is_device ? TECC_HE_ENV_CHUNK_MAX : 0,
                                                      &new_env_hex) != TECC_SM2_SUCCESS ||
                        new_env_hex == NULL)
                    {
                        pg_free(plain_he);
                        pg_free(env_hex_own);
                        continue;
                    }

                    /* ReqID:TECC4.0 - Bug 9255: 同上，HE 私钥信封的写回
                     * 也走 SECURITY DEFINER 受控入口。 */
                    appendPQExpBuffer(&update_buf,
                        "SELECT hg_kek_rotate_dek_arg(%s, '%s', '%s');\n",
                        dek_oid_str, he_key, new_env_hex);

                    pg_free(new_env_hex);
                    pg_free(plain_he);
                    pg_free(env_hex_own);
                }
            }
        }
    }

    /* ReqID:TECC4.0 - 非对称 KEK 轮换：更新 hg_kek_args 的 PUBLIC_KEY 为
     * 新 KEK 公钥（65 字节解压点，hex）。否则轮换后新建 HE DEK 时前端
     * 读到旧公钥，用旧公钥信封加密 HE 私钥、而运行时用新私钥解密会失败。
     * 存储格式与 CREATE KEK（createkek.c: byteain("\x<hex>") 解码为二进制
     * 65 字节点）一致，读取方用 encode(value,'hex') 取回。 */
    if (is_asym && new_pub != NULL)
    {
        char *new_pub_hex = binary_to_hex_string(new_pub, new_pub_len);
        if (new_pub_hex != NULL)
        {
            /* ReqID:TECC4.0 - Bug 9255: 与 DEK 参数同理，走 SECURITY DEFINER
             * 受控入口，避免普通用户在会话里裸 UPDATE hg_kek_args。 */
            appendPQExpBuffer(&update_buf,
                "SELECT hg_kek_rotate_kek_arg(%u, 'public_key', E'\\\\x%s');\n",
                kek_oid, new_pub_hex);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "ALTER KEK: updating PUBLIC_KEY (hex_len=%zu)",
                                strlen(new_pub_hex));
            pg_free(new_pub_hex);
        }
    }

    PQclear(res);
    res = NULL;

    /* Re-enable TECC before sending the rewritten query */
    state->disable_once = false;
    tecc_was_disabled = false;

    /*
     * Step 5: Build the final rewritten query.
     * The original ALTER KEK (with _tecc_processed) is kept so the backend
     * performs permission checks and updates key_path. We append UPDATE
     * statements for DEK re-encryption.
     */
    q = statement_data->query;
    if (q == NULL)
    {
        termPQExpBuffer(&update_buf);
        goto cleanup;
    }

    qlen = strlen(q);

    /* Find the closing paren to inject _tecc_processed */
    close_paren = q + qlen - 1;
    while (close_paren > q && *close_paren != ')')
        close_paren--;

    if (*close_paren != ')')
    {
        termPQExpBuffer(&update_buf);
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                "ALTER KEK: cannot parse SQL for rewrite");
        goto cleanup;
    }

    prefix = close_paren - q;

    /*
     * rewritten = prefix + ", _tecc_processed = 'yes')" + suffix
     *             + ";\n" + update_statements
     * where suffix is everything after ')' (typically ";" or ";\n")
     */
    {
        size_t suffix_len = qlen - prefix - 1;
        size_t marker_len = strlen(", _tecc_processed = 'yes'");
        size_t update_len = update_buf.len;
        size_t total;
        size_t pos;
        size_t i;
        bool need_sep = false;

        /* ReqID:TECC4.0 - suffix 里没有语句结束符时必须自己补一个 ';'。
         * 原实现假定 ')' 之后一定有 ";"（注释原文：typically ";" or ";\n"），
         * 而 psql -c "ALTER KEK ..."（不带分号）的 suffix 为空，改写结果会变成
         * "ALTER KEK ...)\nSELECT hg_kek_rotate_dek_arg(...);"，被服务端当成
         * 一条语句解析而报 syntax error at or near "SELECT"。 */
        for (i = 0; i < suffix_len; i++)
        {
            if (q[prefix + 1 + i] == ';')
                break;
        }
        need_sep = (update_len > 0 && i >= suffix_len);

        total = prefix + marker_len + 1 + suffix_len + (need_sep ? 1 : 0) + 1 +
                update_len + 1;
        rewritten = (char *)pg_malloc(total);
        if (rewritten == NULL)
        {
            termPQExpBuffer(&update_buf);
            goto cleanup;
        }

        /* Copy prefix before ')' */
        memcpy(rewritten, q, prefix);

        /* Insert _tecc_processed marker */
        memcpy(rewritten + prefix, ", _tecc_processed = 'yes'", marker_len);
        rewritten[prefix + marker_len] = ')';

        /* Copy suffix after ')' */
        memcpy(rewritten + prefix + marker_len + 1, close_paren + 1, suffix_len);

        /* Add delimiter before UPDATEs（原语句未带分号时先补一个） */
        pos = prefix + marker_len + 1 + suffix_len;
        if (need_sep)
            rewritten[pos++] = ';';
        rewritten[pos++] = '\n';

        /* Append UPDATE statements */
        if (update_len > 0)
        {
            memcpy(rewritten + pos, update_buf.data, update_len);
            pos += update_len;
        }
        rewritten[pos] = '\0';
    }

    termPQExpBuffer(&update_buf);

    if (statement_data->rewritten_query != NULL)
        pg_free(statement_data->rewritten_query);
    statement_data->rewritten_query = rewritten;
    tecc_stmt_set_needs_rewrite(statement_data, true);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "ALTER KEK: rewritten query generated (len=%zu)",
                        strlen(rewritten));

    /* Record statement type so post-exec can clear KEK/DEK caches.
     * The KEK was rotated and all DEKs re-encrypted; cached plaintext
     * KEK/DEK values are now stale and must be reloaded. */
    state->last_stmt_type = TECC_STMT_TYPE_ALTER_KEK;
    strncpy(state->last_key_name, kek_name, NAMEDATALEN - 1);
    state->last_key_name[NAMEDATALEN - 1] = '\0';
    {
        const char *key_namespace = kek_eff_namespace;

        if (key_namespace == NULL)
            key_namespace = tecc_cache_get_guc_param(state, "current_schema");
        if (key_namespace != NULL)
        {
            strncpy(state->last_key_namespace, key_namespace, NAMEDATALEN - 1);
            state->last_key_namespace[NAMEDATALEN - 1] = '\0';
        }
        else
            state->last_key_namespace[0] = '\0';
    }

    /* Clean up KEK data */
    tecc_key_free_key_data(old_kek, old_kek_len);
    old_kek = NULL;
    if (new_kek != NULL)
    {
        security_secure_erase(new_kek, new_kek_len);
        pg_free(new_kek);
        new_kek = NULL;
    }
    if (new_pub)
        OPENSSL_free(new_pub);
    new_pub = NULL;

    return TECC_STMT_PROCESSOR_SUCCESS;

cleanup:
    if (tecc_was_disabled && state != NULL)
        state->disable_once = false;
    if (old_kek)
        tecc_key_free_key_data(old_kek, old_kek_len);
    if (new_kek)
    {
        security_secure_erase(new_kek, new_kek_len);
        pg_free(new_kek);
    }
    if (new_pub)
        OPENSSL_free(new_pub);
    termPQExpBuffer(&update_buf);
    return TECC_STMT_PROCESSOR_ERROR;
}

/*
 * Helper function to skip whitespace
 */
static const char *
skip_whitespace(const char *str)
{
    while (*str && isspace((unsigned char)*str))
    {
        str++;
    }
    return str;
}

/*
 * Case-insensitive string comparison
 */
static int
ci_strncmp(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0)
    {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0)
            return diff;
        if (*s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return 0;
}

/*
 * Find "WITH VALUES" case-insensitively, skipping whitespace
 */
static const char *
find_with_values(const char *str)
{
    const char *p = str;

    while (*p)
    {
        /* Skip whitespace */
        p = skip_whitespace(p);

        /* Check if we have "WITH" */
        if (ci_strncmp(p, "WITH", 4) == 0)
        {
            const char *after_with = p + 4;
            after_with = skip_whitespace(after_with);

            /* Check if we have "VALUES" */
            if (ci_strncmp(after_with, "VALUES", 6) == 0)
            {
                return p;
            }
        }

        if (*p)
            p++;
    }

    return NULL;
}

/*
 * Find matching closing parenthesis
 */
static const char *
find_matching_close_paren(const char *open_paren)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *p;
    int paren_count;

    if (*open_paren != '(')
    {
        return NULL;
    }

    p = open_paren + 1;
    paren_count = 1;

    while (*p && paren_count > 0)
    {
        if (*p == '(')
        {
            paren_count++;
        }
        else if (*p == ')')
        {
            paren_count--;
        }
        p++;
    }

    if (paren_count == 0)
    {
        return p - 1; /* p is now after the closing parenthesis */
    }

    return NULL; /* Unbalanced parentheses */
}

/*
 * Helper function to extract KEK name from CREATE DEK options
 */
static const char *
get_kek_name_from_options(const List *options)
{
    ListCell *lc;

    if (options == NULL)
        return NULL;

    foreach(lc, options)
    {
        DefElem *def = (DefElem *) lfirst(lc);
        if (def->defname && strcmp(def->defname, "kek") == 0)
        {
            if (def->arg)
            {
                if (nodeTag(def->arg) == FE_T_String)
                {
                    Value *val = (Value *) def->arg;
                    return val->val.str;
                }
                else if (nodeTag(def->arg) == FE_T_Const)
                {
                    Const *c = (Const *) def->arg;
                    if (c->consttype == 25 || c->consttype == 227)  /* TEXTOID or CSTRINGOID */
                    {
                        return c->constvalue.sval;
                    }
                }
            }
        }
    }

    return NULL;
}

/*
 * Helper function to extract key_algo from CREATE DEK options.
 * Returns the key_algo string, or NULL if not specified.
 */
static const char *
get_key_algo_from_options(const List *options)
{
    ListCell *lc;

    if (options == NULL)
        return NULL;

    foreach(lc, options)
    {
        DefElem *def = (DefElem *) lfirst(lc);
        if (def->defname &&
            pg_strcasecmp(def->defname, "key_algo") == 0)
        {
            if (def->arg)
            {
                const char *raw = NULL;

                if (nodeTag(def->arg) == FE_T_String)
                {
                    Value *val = (Value *) def->arg;
                    raw = val->val.str;
                }
                else if (nodeTag(def->arg) == FE_T_Const)
                {
                    Const *c = (Const *) def->arg;
                    if (c->consttype == 25 || c->consttype == 227)
                    {
                        raw = c->constvalue.sval;
                    }
                }
                return raw;
            }
        }
    }

    return NULL;
}

/*
 * Helper function to encrypt DEK with appropriate method based on KEK type
 */
static int
encrypt_dek_based_on_kek_type(const unsigned char *plain_dek, size_t plain_dek_len,
                             const unsigned char *kek, size_t kek_len,
                             bool is_asym,
                             unsigned char **encrypted_dek, size_t *encrypted_dek_len)
{
    /* Check if this is an HSM placeholder key */
    bool is_hsm_placeholder = (kek_len == 16 && kek_len >= 11 &&
        (memcmp(kek + 4, "HSM_KEY", 7) == 0 || memcmp(kek + 4, "SWXAKEY", 7) == 0));
    if (is_hsm_placeholder)
    {
        /* HSM KEK: use HSM adapter for encryption */
        bool is_swxa = (memcmp(kek + 4, "SWXAKEY", 7) == 0);
        unsigned int key_index;
        HsmAdapterInterface *hsm_adapter = NULL;
        HsmDevice *device = NULL;
        HsmErrorCode error;

        /* Extract key index from placeholder */
        key_index = (unsigned int)kek[0] |
                   ((unsigned int)kek[1] << 8) |
                   ((unsigned int)kek[2] << 16) |
                   ((unsigned int)kek[3] << 24);

        /* Get HSM adapter based on marker */
        if (is_swxa)
            hsm_adapter = get_swxa_adapter();
        else
            hsm_adapter = get_tzhsm_adapter();
        if (!hsm_adapter)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "Failed to get %s adapter for DEK encryption",
                                is_swxa ? "SWXA" : "HSM");
            return -1;
        }

        /* Initialize HSM device */
        error = hsm_adapter->init(&device, NULL);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s initialization failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            return -1;
        }

        /* Connect to HSM */
        error = hsm_adapter->connect(device);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s connection failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Encrypt DEK using HSM */
        error = hsm_adapter->encrypt(device, key_index,
                                    plain_dek, plain_dek_len,
                                    (void **)encrypted_dek, encrypted_dek_len);
        if (error != HSM_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "%s encryption failed: %s",
                                is_swxa ? "SWXA" : "HSM",
                                hsm_adapter->error_string(error));
            hsm_adapter->disconnect(device);
            hsm_adapter->cleanup(device);
            return -1;
        }

        /* Clean up HSM connection */
        hsm_adapter->disconnect(device);
        hsm_adapter->cleanup(device);

        return 0;
    }
    else if (is_asym)
    {
        /* ReqID:TECC4.0 - 对称 DEK 绑非对称 KEK：SM2 信封加密。
         * KEK 明文即 SM2 私钥 DER，派生公钥后信封加密 DEK
         * （与 HE 私钥信封同一 primitive）。 */
        unsigned char *pub = NULL;
        size_t         pub_len = 0;
        int            sm2_ret;

        *encrypted_dek = NULL;
        *encrypted_dek_len = 0;

        if (tecc_sm2_derive_pubkey(kek, kek_len,
                                   &pub, &pub_len) != TECC_SM2_SUCCESS ||
            pub == NULL || pub_len == 0)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "Failed to derive SM2 public key for DEK "
                                "envelope encryption");
            if (pub)
                OPENSSL_free(pub);
            return -1;
        }

        sm2_ret = tecc_sm2_envelope_encrypt(pub, pub_len,
                                           plain_dek, plain_dek_len,
                                           encrypted_dek, encrypted_dek_len);
        OPENSSL_free(pub);
        if (sm2_ret != TECC_SM2_SUCCESS || *encrypted_dek == NULL || *encrypted_dek_len == 0)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "SM2 envelope DEK encryption failed (ret=%d)", sm2_ret);
            if (*encrypted_dek)
                OPENSSL_free(*encrypted_dek);
            return -1;
        }

        /* tecc_sm2_envelope_encrypt 用 OPENSSL_malloc；转为 pg_malloc 缓冲区，
         * 以匹配调用方（create_dek）统一的 pg_free + secure_erase 清理。 */
        {
            unsigned char *pg_enc;

            pg_enc = (unsigned char *)pg_malloc(*encrypted_dek_len);
            if (pg_enc == NULL)
            {
                OPENSSL_free(*encrypted_dek);
                *encrypted_dek = NULL;
                *encrypted_dek_len = 0;
                return -1;
            }
            memcpy(pg_enc, *encrypted_dek, *encrypted_dek_len);
            OPENSSL_free(*encrypted_dek);
            *encrypted_dek = pg_enc;
        }
        return 0;
    }
    else
    {
        /* Normal KEK: use security layer for encryption */
        return security_encrypt_dek_with_hmac(plain_dek, plain_dek_len,
                                             kek, kek_len,
                                             encrypted_dek, encrypted_dek_len);
    }
}

/*
 * Helper function to convert binary data to hex string
 */
static char *
binary_to_hex_string(const unsigned char *data, size_t data_len)
{
    char *hex_str;
    size_t i;

    if (data == NULL || data_len == 0)
        return NULL;

    hex_str = (char *)pg_malloc(data_len * 2 + 1);
    if (hex_str == NULL)
        return NULL;

    for (i = 0; i < data_len; i++)
    {
        /* Use snprintf to avoid buffer overrun */
        snprintf(hex_str + i * 2, 3, "%02x", data[i]);
    }
    hex_str[data_len * 2] = '\0';

    return hex_str;
}

/*
 * ReqID:TECC4.0 - Generate an EC-ElGamal key pair (P-256) for an HE DEK.
 * Returns 0 on success: *pub_hex = public key Q hex (uncompressed 65B),
 * *priv_hex = private key x (32B) encrypted with KEK via
 * security_encrypt_dek_with_hmac. Caller frees both.
 */
static int
tecc_he_gen_ec_keypair(const unsigned char *kek_pub, size_t kek_pub_len,
                       char **pub_hex, char **priv_hex)
{
    EC_GROUP *he_group;
    BIGNUM *ec_priv = NULL;
    EC_POINT *ec_pub = NULL;
    BN_CTX *bn_ctx = NULL;
    unsigned char pub_buf[TECC_EC_POINT_SIZE * 2];
    size_t pub_len;
    unsigned char priv_buf[32];
    int priv_len;
    unsigned char *enc_priv = NULL;
    size_t enc_priv_len = 0;
    int ret;

    *pub_hex = NULL;
    *priv_hex = NULL;

    he_group = tecc_ec_elgamal_create_group();
    if (he_group == NULL)
        return -1;
    bn_ctx = BN_CTX_new();
    if (bn_ctx == NULL)
    {
        EC_GROUP_free(he_group);
        return -1;
    }

    ret = tecc_ec_elgamal_generate_keypair(he_group, &ec_priv, &ec_pub);
    if (ret != TECC_EC_ELGAMAL_SUCCESS)
        goto fail;

    /* Serialize public key Q (uncompressed point) */
    pub_len = EC_POINT_point2oct(he_group, ec_pub, POINT_CONVERSION_UNCOMPRESSED,
                                 pub_buf, sizeof(pub_buf), bn_ctx);
    if (pub_len == 0)
        goto fail;

    /* Serialize private key x (32 bytes) */
    priv_len = tecc_bn_to_fixed(ec_priv, priv_buf, sizeof(priv_buf));
    if (priv_len <= 0)
        goto fail;

    /* ReqID:TECC4.0 - Envelope-encrypt HE private key with the KEK's SM2
     * public key (scheme B). Falls back to nothing on failure. */
    ret = tecc_sm2_envelope_encrypt(kek_pub, kek_pub_len,
                                    priv_buf, (size_t) priv_len,
                                    &enc_priv, &enc_priv_len);
    if (ret != TECC_SM2_SUCCESS)
        goto fail;

    *pub_hex = binary_to_hex_string(pub_buf, pub_len);
    *priv_hex = binary_to_hex_string(enc_priv, enc_priv_len);

    OPENSSL_free(enc_priv);
    enc_priv = NULL;
    security_secure_erase(priv_buf, sizeof(priv_buf));
    EC_POINT_free(ec_pub);
    BN_free(ec_priv);
    BN_CTX_free(bn_ctx);
    EC_GROUP_free(he_group);

    return (*pub_hex != NULL && *priv_hex != NULL) ? 0 : -1;

fail:
    if (enc_priv)
    {
        OPENSSL_free(enc_priv);
    }
    if (ec_pub)
        EC_POINT_free(ec_pub);
    if (ec_priv)
        BN_free(ec_priv);
    if (bn_ctx)
        BN_CTX_free(bn_ctx);
    if (he_group)
        EC_GROUP_free(he_group);
    return -1;
}

/*
 * ReqID:TECC4.0 - Generate an FF ElGamal key pair (2048-bit) for an HE DEK.
 * Uses the default parameters (same as client encrypt/decrypt path).
 * Returns 0 on success: *pub_hex = public key h hex (256B),
 * *priv_hex = private key x encrypted with KEK. Caller frees both.
 */
static int
tecc_he_gen_ff_keypair(const unsigned char *kek_pub, size_t kek_pub_len,
                       bool kek_is_device,
                       char **pub_hex, char **priv_hex)
{
    BIGNUM *ff_p = NULL;
    BIGNUM *ff_q = NULL;
    BIGNUM *ff_g = NULL;
    BIGNUM *ff_priv = NULL;
    BIGNUM *ff_pub = NULL;
    unsigned char pub_buf[256];
    int pub_len;
    unsigned char priv_buf[256];
    int priv_len;
    int ret;

    *pub_hex = NULL;
    *priv_hex = NULL;

    /* Default params (RFC 5114 2048-bit) — must match client encrypt/decrypt */
    ret = tecc_elgamal_get_default_params(&ff_p, &ff_q, &ff_g);
    if (ret != TECC_ELGAMAL_SUCCESS)
        return -1;

    ret = tecc_elgamal_generate_keypair(ff_p, ff_q, ff_g, &ff_priv, &ff_pub);
    if (ret != TECC_ELGAMAL_SUCCESS)
        goto fail;

    pub_len = tecc_bn_to_fixed(ff_pub, pub_buf, sizeof(pub_buf));
    if (pub_len <= 0)
        goto fail;
    priv_len = tecc_bn_to_fixed(ff_priv, priv_buf, sizeof(priv_buf));
    if (priv_len <= 0)
        goto fail;

    /* ReqID:TECC4.0 - Envelope-encrypt HE private key with the KEK's SM2
     * public key (scheme B).  KEK 托管在密码机时必须分块：设备 ECCCipher.C
     * 上限 136 字节（真机实测），而本函数封装的 256 字节 FF-ElGamal 私钥
     * 放不进单个信封；localkms 传 0 保持单信封原格式不变。
     * Falls back to nothing on failure. */
    ret = tecc_sm2_envelope_encrypt_hex(kek_pub, kek_pub_len,
                                        priv_buf, (size_t) priv_len,
                                        kek_is_device ? TECC_HE_ENV_CHUNK_MAX : 0,
                                        priv_hex);
    if (ret != TECC_SM2_SUCCESS)
        goto fail;

    *pub_hex = binary_to_hex_string(pub_buf, (size_t) pub_len);

    security_secure_erase(priv_buf, sizeof(priv_buf));
    BN_free(ff_priv);
    BN_free(ff_pub);
    if (ff_q) BN_free(ff_q);
    BN_free(ff_p);
    BN_free(ff_g);

    return (*pub_hex != NULL && *priv_hex != NULL) ? 0 : -1;

fail:
    if (ff_priv)
        BN_free(ff_priv);
    if (ff_pub)
        BN_free(ff_pub);
    if (ff_q)
        BN_free(ff_q);
    if (ff_p)
        BN_free(ff_p);
    if (ff_g)
        BN_free(ff_g);
    return -1;
}

/*
 * tecc_kek_is_asym - 判断 KEK 是否为非对称（SM2）密钥
 *
 * ReqID:TECC4.0 - 从 hg_kek_args 读 key_algo，判断是否为 sm2（非对称）。
 * 无 key_algo 的对称 KEK 返回 false。
 */
static bool
tecc_kek_is_asym(void *state, const char *kek_name)
{
    char       *hex = NULL;
    char        buf[16];
    int         i;
    size_t      len;

    if (tecc_he_load_kek_arg_hex(state, kek_name, "key_algo", &hex) != 0 ||
        hex == NULL || strlen(hex) / 2 >= sizeof(buf))
    {
        if (hex)
            pg_free(hex);
        return false;
    }

    len = strlen(hex) / 2;
    for (i = 0; i < (int) len; i++)
    {
        char c1 = hex[i * 2];
        char c2 = hex[i * 2 + 1];
        unsigned char n1, n2;
        n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char) (c1 - '0') :
             (c1 >= 'a' && c1 <= 'f') ? (unsigned char) (c1 - 'a' + 10) :
             (c1 >= 'A' && c1 <= 'F') ? (unsigned char) (c1 - 'A' + 10) : 0;
        n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char) (c2 - '0') :
             (c2 >= 'a' && c2 <= 'f') ? (unsigned char) (c2 - 'a' + 10) :
             (c2 >= 'A' && c2 <= 'F') ? (unsigned char) (c2 - 'A' + 10) : 0;
        buf[i] = (n1 << 4) | n2;
    }
    buf[len] = '\0';
    pg_free(hex);

    return (strcmp(buf, "sm2") == 0);
}

/*
 * tecc_hex_to_string - decode a hex string into a NUL-terminated C string.
 *
 * Matches the hg_kek_args convention: option values are hex of the raw string
 * bytes (createkek.c hex-encodes before byteain).  Returns bytes decoded, or
 * -1 on invalid/oversized input.
 */
static int
tecc_hex_to_string(const char *hex, char *buf, size_t out_cap)
{
    size_t hlen;
    size_t i;

    if (hex == NULL || buf == NULL || out_cap == 0)
        return -1;

    hlen = strlen(hex);
    if (hlen % 2 != 0 || hlen / 2 >= out_cap)
        return -1;

    for (i = 0; i < hlen / 2; i++)
    {
        char c1 = hex[i * 2];
        char c2 = hex[i * 2 + 1];
        int n1 = (c1 >= '0' && c1 <= '9') ? (c1 - '0') :
                 (c1 >= 'a' && c1 <= 'f') ? (c1 - 'a' + 10) :
                 (c1 >= 'A' && c1 <= 'F') ? (c1 - 'A' + 10) : -1;
        int n2 = (c2 >= '0' && c2 <= '9') ? (c2 - '0') :
                 (c2 >= 'a' && c2 <= 'f') ? (c2 - 'a' + 10) :
                 (c2 >= 'A' && c2 <= 'F') ? (c2 - 'A' + 10) : -1;
        if (n1 < 0 || n2 < 0)
            return -1;
        buf[i] = (char)((n1 << 4) | n2);
    }
    buf[hlen / 2] = '\0';
    return (int)(hlen / 2);
}

/*
 * ReqID:TECC4.0 - localkms 下非对称 KEK 建 DEK 前校验公钥文件 .pub 存在。
 *
 * sm2 KEK 的密钥对（.priv + .pub）由 tecc_ktool generate 生成。.priv 缺失由 KEK
 * 加载路径报错拦截；.pub 缺失在此显式校验：按『密钥对文件完整性』策略，公钥
 * 文件缺失视为密钥对不完整，禁止继续建 DEK——信封加密虽可仅凭 .priv 派生公钥
 * 完成，但 .pub 是密钥对的组成文件，被 tecc_ktool delete 删除后应阻止新建依赖
 * 它的 DEK。tzwykms/swxakms（HSM 密码机）无本地 .pub 文件，跳过；非 .priv 结尾
 * 的 KEY_PATH 跳过。
 *
 * 返回 0 通过或不适配；-1 表示 .pub 缺失（错误已写入 statement_data）。
 */
static int
tecc_verify_kek_pub_file(TeccState *state, const char *kek_name,
                         TeccStatementData *statement_data)
{
    char *key_store_hex = NULL;
    char *key_path_hex = NULL;
    char key_store[64];
    char key_path[1024];
    char pub_path[1024];
    struct stat pub_stat;
    const char *ext;
    size_t base_len;

    key_store[0] = '\0';
    key_path[0] = '\0';

    /* KEY_STORE：localkms 才做本地文件校验，HSM 密钥在密码机内无 .pub */
    if (tecc_he_load_kek_arg_hex(state, kek_name, "key_store", &key_store_hex) != 0 ||
        key_store_hex == NULL ||
        tecc_hex_to_string(key_store_hex, key_store, sizeof(key_store)) < 0)
        goto done;
    pg_free(key_store_hex);
    key_store_hex = NULL;

    if (strcmp(key_store, "localkms") != 0)
        goto done;

    /* KEY_PATH：sm2 KEK 指向 .priv，兄弟 .pub 必须存在 */
    if (tecc_he_load_kek_arg_hex(state, kek_name, "key_path", &key_path_hex) != 0 ||
        key_path_hex == NULL ||
        tecc_hex_to_string(key_path_hex, key_path, sizeof(key_path)) < 0)
        goto done;
    pg_free(key_path_hex);
    key_path_hex = NULL;

    ext = strrchr(key_path, '.');
    if (ext == NULL || strcmp(ext, ".priv") != 0)
        goto done;

    base_len = (size_t)(ext - key_path);
    if (base_len + 5 >= sizeof(pub_path))
        goto done;
    memcpy(pub_path, key_path, base_len);
    memcpy(pub_path + base_len, ".pub", 5);

    if (stat(pub_path, &pub_stat) != 0)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                "KEK '%s' public key file is unavailable "
                                "(localkms: %s missing), cannot create DEK",
                                kek_name, pub_path);
        return -1;
    }

done:
    if (key_store_hex != NULL)
        pg_free(key_store_hex);
    if (key_path_hex != NULL)
        pg_free(key_path_hex);
    return 0;
}

/*
 * Process CREATE DEK statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_dek(const CreateDekStmt *create_dek_stmt,
                                      TeccStatementData *statement_data)
{
    const char *dek_name;
    const char *kek_name;
    const char *key_algo = NULL;
    int is_he_dek = 0;
    unsigned char *plain_dek = NULL;
    size_t plain_dek_len = 0;
    unsigned char *kek = NULL;
    size_t kek_len = 0;
    unsigned char *encrypted_dek = NULL;
    size_t encrypted_dek_len = 0;
    char *encrypted_dek_hex = NULL;
    int ret;
    bool is_kek_asym = false;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccState *state = NULL;
    const char *original_query = NULL;
    size_t query_len = 0;
    const char *with_values = NULL;
    const char *open_paren = NULL;
    const char *close_paren = NULL;
    const char *p = NULL;
    size_t prefix_len = 0;
    size_t suffix_len = 0;
    size_t new_query_len = 0;
    char *new_query = NULL;
    /* ReqID:TECC4.0 - HE 密钥对生成（问题2）
     * A DEK may back both he-add and he-mul columns, so always generate
     * BOTH EC-ElGamal and FF ElGamal key pairs. */
    char *he_ec_pubkey_hex = NULL;
    char *he_ec_privkey_hex = NULL;
    char *he_eg_pubkey_hex = NULL;
    char *he_eg_privkey_hex = NULL;
    size_t he_fragment_len = 0;
    /* ReqID:TECC4.0 - KEK 是否托管在密码机（决定 HE 私钥信封是否分块） */
    bool kek_is_device = false;

    if (create_dek_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing CREATE DEK statement");

    /* Extract DEK name from qualified name list */
    dek_name = (create_dek_stmt->names != NIL) ?
        strVal(llast(create_dek_stmt->names)) : NULL;
    if (dek_name == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "CREATE DEK statement missing DEK name");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Extract KEK name from options */
    kek_name = get_kek_name_from_options(create_dek_stmt->options);
    if (kek_name == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "CREATE DEK statement missing KEK parameter");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Reject empty or whitespace-only KEK names to avoid stacked errors */
    if (kek_name[0] == '\0' ||
        kek_name[strspn(kek_name, " \t\r\n")] == '\0')
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "KEK name is empty or invalid");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Extract key_algo from options (optional, NULL = traditional SM4-DEK).
     * ReqID:TECC4.0 - DEK 的 key_algo 域为 sm4（默认，对称）/he-add/he-mul
     * （V4.0.1 概要设计 §7.1）：显式 'sm4' 等价于省略，is_he_dek 保持 0，
     * 与 createdek.c 存储注释及 create_stmt_processor 的列校验口径一致。 */
    key_algo = get_key_algo_from_options(create_dek_stmt->options);
    if (key_algo != NULL)
    {
        if (strcmp(key_algo, "he-add") == 0 ||
            strcmp(key_algo, "he-mul") == 0)
        {
            is_he_dek = 1;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "CREATE DEK with HE key_algo: %s", key_algo);
        }
        else if (strcmp(key_algo, "sm4") != 0)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                    "Invalid KEY_ALGO for DEK");
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* Step 1: Get KEK with TeccState for cache access */
    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to get TECC state for connection, cannot encrypt DEK");
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /*
     * ReqID:TECC4.0 - For HE DEKs, the KEK is a non-asymmetric SM2 key pair.
     * We do NOT load the KEK plaintext here (it is an SM2 private key DER,
     * not usable for SM4-DEK encryption). The HE private key is protected by
     * SM2 envelope encryption using the KEK public key (scheme B). The
     * ENCRYPTED_VALUE is a placeholder for HE DEKs.
     */
    if (!is_he_dek)
    {
        /* 强制重读存储（绕过连接级明文缓存）：同一连接内删除 .priv/.key 后
         * CREATE DEK 也必报错，避免产出之后无法解密的 DEK。 */
        kek = tecc_key_get_kek_with_state_refresh(state, kek_name, &kek_len);
        if (kek == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "KEK '%s' cache miss, cannot encrypt DEK", kek_name);
            return TECC_STMT_PROCESSOR_ERROR;
        }

        /* ReqID:TECC4.0 - 对称 DEK 绑非对称 KEK 时走 SM2 信封加密
         * （encrypted_value 信封），绑对称 KEK 保持 SM4-CBC+HMAC。 */
        is_kek_asym = tecc_kek_is_asym(state, kek_name);

        /* DEBUG: 打印加密时使用的KEK */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "[CREATE DEK] Got KEK '%s', len=%zu, is_kek_asym=%d "
                            "for DEK encryption",
                            kek_name, kek_len, is_kek_asym);

        /* ReqID:TECC4.0 - 对称 DEK 绑非对称 KEK 时校验 .pub 公钥文件完整。
         * .priv 缺失已由 KEK 加载报错拦截；.pub 缺失在此显式拦截（密钥对
         * 文件完整性策略），保证 tecc_ktool delete .pub 后 CREATE DEK 报错。 */
        if (is_kek_asym &&
            tecc_verify_kek_pub_file(state, kek_name, statement_data) != 0)
        {
            if (kek)
                tecc_key_free_key_data(kek, kek_len);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* Step 2: Generate random DEK (32 bytes for SM4) */
    ret = security_generate_dek_key(32, &plain_dek, &plain_dek_len);
    if (ret != SECURITY_SUCCESS)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to generate DEK: error %d", ret);
        if (!is_he_dek)
            if (kek)
                tecc_key_free_key_data(kek, kek_len);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /*
     * Step 2.5: ReqID:TECC4.0 - For HE DEKs, generate the HE key pair
     * (EC-ElGamal or FF ElGamal). The HE private key is envelope-encrypted
     * with the KEK's SM2 public key (scheme B: non-asymmetric KEK rejected).
     * These are injected into the rewritten SQL and stored in hg_dek_args.
     */
    if (is_he_dek)
    {
        /* Scheme B: HE DEK must be bound to a non-asymmetric KEK.
         * Verify the KEK's key_algo='sm2' and load its SM2 public key
         * (PUBLIC_KEY = 65-byte uncompressed point) from hg_kek_args. */
        char *kek_pub_hex = NULL;
        unsigned char kek_pub[65];
        size_t kek_pub_len = 0;
        int i;

        /* Read KEY_ALGO of the KEK (stored as bytea; hex-encoded by query). */
        {
            char *kek_type_hex = NULL;
            char kek_type_buf[16];
            size_t kek_type_len;

            if (tecc_he_load_kek_arg_hex(state, kek_name, "key_algo", &kek_type_hex) != 0 ||
                kek_type_hex == NULL || strlen(kek_type_hex) / 2 >= sizeof(kek_type_buf))
            {
                if (kek_type_hex)
                    pg_free(kek_type_hex);
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "HE DEK requires a non-asymmetric KEK "
                                        "(KEY_ALGO='sm2') for SM2 envelope encryption");
                goto he_keygen_fail;
            }
            kek_type_len = strlen(kek_type_hex) / 2;
            for (i = 0; i < (int)kek_type_len; i++)
            {
                char c1 = kek_type_hex[i * 2];
                char c2 = kek_type_hex[i * 2 + 1];
                unsigned char n1, n2;
                n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                     (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                     (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
                n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                     (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                     (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
                kek_type_buf[i] = (n1 << 4) | n2;
            }
            kek_type_buf[kek_type_len] = '\0';
            pg_free(kek_type_hex);

            /* Accept stored 'sm2'. */
            if (strcmp(kek_type_buf, "sm2") != 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "HE DEK requires a non-asymmetric KEK "
                                        "(KEY_ALGO='sm2') for SM2 envelope encryption");
                goto he_keygen_fail;
            }
        }

        /* ReqID:TECC4.0 - HE DEK 同受『密钥对文件完整性』约束：sm2 KEK 的
         * .pub 公钥文件缺失时同样禁止建 DEK（与 .priv 可用性校验同级）。 */
        if (tecc_verify_kek_pub_file(state, kek_name, statement_data) != 0)
            goto he_keygen_fail;

        /* Read PUBLIC_KEY (hex of the 65-byte SM2 public point) */
        if (tecc_he_load_kek_arg_hex(state, kek_name, "public_key", &kek_pub_hex) != 0 ||
            kek_pub_hex == NULL || strlen(kek_pub_hex) / 2 != 65)
        {
            if (kek_pub_hex)
                pg_free(kek_pub_hex);
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "Non-asymmetric KEK missing PUBLIC_KEY for HE DEK");
            goto he_keygen_fail;
        }
        for (i = 0; i < 65; i++)
        {
            char c1 = kek_pub_hex[i * 2];
            char c2 = kek_pub_hex[i * 2 + 1];
            unsigned char n1, n2;
            n1 = (c1 >= '0' && c1 <= '9') ? (unsigned char)(c1 - '0') :
                 (c1 >= 'a' && c1 <= 'f') ? (unsigned char)(c1 - 'a' + 10) :
                 (c1 >= 'A' && c1 <= 'F') ? (unsigned char)(c1 - 'A' + 10) : 0;
            n2 = (c2 >= '0' && c2 <= '9') ? (unsigned char)(c2 - '0') :
                 (c2 >= 'a' && c2 <= 'f') ? (unsigned char)(c2 - 'a' + 10) :
                 (c2 >= 'A' && c2 <= 'F') ? (unsigned char)(c2 - 'A' + 10) : 0;
            kek_pub[i] = (n1 << 4) | n2;
        }
        kek_pub_len = 65;
        pg_free(kek_pub_hex);

        /*
         * ReqID:TECC4.0 - 私钥可用性校验：任何 CREATE DEK（含 HE DEK）均要求
         * 所绑非对称 KEK 私钥可用。软加密 localkms 下 .priv 文件缺失/不可读
         * （含 TECC_KEK_PATH 解析失败、文件解密失败）时 tecc_key_get_kek_with_state
         * 返回 NULL → 报错，禁止创建绑定到私钥不可用 KEK 的 DEK；硬加密
         * tzwykms/swxakms 下返回 HSM 占位密钥，视为私钥在密码机内可用。
         * 用 refresh 变体绕过连接级明文缓存，同一连接内删除 .priv 后
         * 探测也稳定读文件报错。 */
        kek = tecc_key_get_kek_with_state_refresh(state, kek_name, &kek_len);
        if (kek == NULL)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "KEK '%s' private key is unavailable "
                                    "(localkms: .priv file missing or unreadable), "
                                    "cannot create DEK", kek_name);
            goto he_keygen_fail;
        }
        /* ReqID:TECC4.0 - 判定 KEK 是否托管在密码机：口径与运行时解封侧一致
         * （16 字节 placeholder + HSM_KEY/SWXAKEY 标记）。托管时必须分块封装
         * HE 私钥信封——设备 ECCCipher.C 上限 136 字节，he-mul 的 256 字节
         * FF-ElGamal 私钥放不进单个信封（真机实测）。 */
        kek_is_device = (kek_len == 16 &&
                         (memcmp(kek + 4, "HSM_KEY", 7) == 0 ||
                          memcmp(kek + 4, "SWXAKEY", 7) == 0));
        tecc_key_free_key_data(kek, kek_len);
        kek = NULL;
        kek_len = 0;

        /* Generate HE key pair(s) based on key_algo.  Each key_algo has its
         * own independent key pair:
         *   'he-add' -> EC-ElGamal (he_ec_pubkey/he_ec_privkey)
         *   'he-mul' -> FF ElGamal  (he_eg_pubkey/he_eg_privkey)
         * A single HE DEK does NOT carry both pairs. */
        if (strcmp(key_algo, "he-add") == 0)
        {
            if (tecc_he_gen_ec_keypair(kek_pub, kek_pub_len,
                                       &he_ec_pubkey_hex, &he_ec_privkey_hex) != 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "Failed to generate EC-ElGamal key pair");
                goto he_keygen_fail;
            }
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "CREATE DEK: generated EC-ElGamal key pair for '%s' (key_algo=he-add)",
                                dek_name);
        }
        else if (strcmp(key_algo, "he-mul") == 0)
        {
            if (tecc_he_gen_ff_keypair(kek_pub, kek_pub_len, kek_is_device,
                                       &he_eg_pubkey_hex, &he_eg_privkey_hex) != 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                        "Failed to generate FF ElGamal key pair");
                goto he_keygen_fail;
            }
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "CREATE DEK: generated FF ElGamal key pair for '%s' (key_algo=he-mul)",
                                dek_name);
        }
        else
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                    "Invalid HE key_algo for DEK: %s", key_algo);
            goto he_keygen_fail;
        }
    }

he_keygen_fail:
    if (he_ec_pubkey_hex == NULL && he_ec_privkey_hex == NULL &&
        he_eg_pubkey_hex == NULL && he_eg_privkey_hex == NULL && is_he_dek)
    {
        /* Error already set by the specific failure path */
        if (!is_he_dek)
            if (kek)
                tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /*
     * Step 3: For HE DEKs, ENCRYPTED_VALUE is a placeholder (the HE key
     * pair drives encryption, not SM4-DEK). For traditional DEKs, encrypt
     * the SM4 DEK with the KEK as before.
     */
    if (is_he_dek)
    {
        encrypted_dek_len = plain_dek_len;
        encrypted_dek = (unsigned char *)pg_malloc(encrypted_dek_len);
        if (encrypted_dek == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR, "Failed to allocate encrypted DEK");
            security_secure_erase(plain_dek, plain_dek_len);
            pg_free(plain_dek);
            return TECC_STMT_PROCESSOR_ERROR;
        }
        memcpy(encrypted_dek, plain_dek, encrypted_dek_len);
    }
    else
    {
        ret = encrypt_dek_based_on_kek_type(plain_dek, plain_dek_len,
                                           kek, kek_len,
                                           is_kek_asym,
                                           &encrypted_dek, &encrypted_dek_len);
        if (ret != 0)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                                "Failed to encrypt DEK with KEK '%s'", kek_name);
            security_secure_erase(plain_dek, plain_dek_len);
            pg_free(plain_dek);
            if (kek)
                tecc_key_free_key_data(kek, kek_len);
            return TECC_STMT_PROCESSOR_ERROR;
        }
    }

    /* Step 4: Convert encrypted DEK to hex string */
    encrypted_dek_hex = binary_to_hex_string(encrypted_dek, encrypted_dek_len);
    if (encrypted_dek_hex == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to convert encrypted DEK to hex string");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        if (kek)
            tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEK encrypted successfully, hex length: %zu",
                        strlen(encrypted_dek_hex));

    /* Step 5: Rewrite SQL to add ENCRYPTED_VALUE parameter */
    /* We need to modify: CREATE DEK dek_name WITH VALUES(KEK = kek_name) */
    /* To become: CREATE DEK dek_name WITH VALUES(KEK = kek_name, ENCRYPTED_VALUE = 'hex_string') */

    /* Find the position of the closing parenthesis in WITH VALUES(...) */
    original_query = statement_data->query;
    query_len = strlen(original_query);

    /* Find "WITH VALUES" case-insensitively */
    with_values = find_with_values(original_query);
    tecc_processor_debug(TECC_LOG_DEBUG1,
        "SQL REWRITE: with_values=%p, original_query=%p, offset=%td",
        with_values, original_query,
        with_values ? with_values - original_query : -1);
    if (with_values != NULL)
    {
        /* Find opening parenthesis after WITH VALUES */
        p = with_values;
        while (*p && *p != '(')
        {
            p++;
        }

        if (*p == '(')
        {
            open_paren = p;
            /* Find matching closing parenthesis */
            close_paren = find_matching_close_paren(open_paren);
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "SQL REWRITE: close_paren offset=%td, prefix_len will be %td",
            close_paren ? close_paren - original_query : -1,
            close_paren ? close_paren - original_query : -1);
        }
    }

    if (with_values == NULL || open_paren == NULL || close_paren == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to parse CREATE DEK SQL for rewriting");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        if (kek)
            tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        pg_free(encrypted_dek_hex);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Calculate positions */
    /* prefix includes everything before the closing paren, suffix starts after it */
    prefix_len = close_paren - original_query;  /* up to but not including ')' */
    suffix_len = query_len - prefix_len - 1; /* skip the ')' */

    /* Build new query: prefix + ", ENCRYPTED_VALUE = 'hex'" + HE keys + ", _TECC_PROCESSED = 'yes'" + ")" + suffix.
     * KEY_ALGO is NOT injected (already in user's original SQL). */
    /* ReqID:TECC4.0 - HE 密钥对注入片段长度（EC + FF 两套） */
    he_fragment_len = 0;
    if (he_ec_pubkey_hex != NULL)
        he_fragment_len += strlen(", HE_EC_PUBKEY = ''") + strlen(he_ec_pubkey_hex);
    if (he_ec_privkey_hex != NULL)
        he_fragment_len += strlen(", HE_EC_PRIVKEY = ''") + strlen(he_ec_privkey_hex);
    if (he_eg_pubkey_hex != NULL)
        he_fragment_len += strlen(", HE_EG_PUBKEY = ''") + strlen(he_eg_pubkey_hex);
    if (he_eg_privkey_hex != NULL)
        he_fragment_len += strlen(", HE_EG_PRIVKEY = ''") + strlen(he_eg_privkey_hex);

    new_query_len = prefix_len + strlen(", ENCRYPTED_VALUE = '") +
                   strlen(encrypted_dek_hex) + strlen("'") +
                   he_fragment_len +
                   strlen(", _TECC_PROCESSED = 'yes'") + 1 + suffix_len + 1;
    new_query = (char *)pg_malloc(new_query_len);

    if (new_query == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                            "Failed to allocate memory for rewritten query");
        security_secure_erase(plain_dek, plain_dek_len);
        pg_free(plain_dek);
        if (kek)
            tecc_key_free_key_data(kek, kek_len);
        security_secure_erase(encrypted_dek, encrypted_dek_len);
        pg_free(encrypted_dek);
        pg_free(encrypted_dek_hex);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* Construct new query */
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        size_t pos = 0;

    /* Copy original query prefix (everything before the closing paren) */
    memcpy(new_query + pos, original_query, prefix_len);
    pos += prefix_len;

    /* Insert ENCRYPTED_VALUE inside the parens, just before the closing paren */
    memcpy(new_query + pos, ", ENCRYPTED_VALUE = '", strlen(", ENCRYPTED_VALUE = '"));
    pos += strlen(", ENCRYPTED_VALUE = '");

    memcpy(new_query + pos, encrypted_dek_hex, strlen(encrypted_dek_hex));
    pos += strlen(encrypted_dek_hex);

    memcpy(new_query + pos, "'", 1);
    pos += 1;

    /* ReqID:TECC4.0 - HE key pair injection.
     * NOTE: KEY_ALGO is NOT injected here — it already exists in the user's
     * original SQL (is_he_dek only becomes true when key_algo was specified).
     * Injecting it again would create a duplicate hg_dek_args key. The backend
     * parses key_algo from the original statement. */
    if (he_ec_pubkey_hex != NULL)
    {
        const char prefix[] = ", HE_EC_PUBKEY = '";
        memcpy(new_query + pos, prefix, sizeof(prefix) - 1);
        pos += sizeof(prefix) - 1;
        memcpy(new_query + pos, he_ec_pubkey_hex, strlen(he_ec_pubkey_hex));
        pos += strlen(he_ec_pubkey_hex);
        new_query[pos++] = '\'';
    }
    if (he_ec_privkey_hex != NULL)
    {
        const char prefix[] = ", HE_EC_PRIVKEY = '";
        memcpy(new_query + pos, prefix, sizeof(prefix) - 1);
        pos += sizeof(prefix) - 1;
        memcpy(new_query + pos, he_ec_privkey_hex, strlen(he_ec_privkey_hex));
        pos += strlen(he_ec_privkey_hex);
        new_query[pos++] = '\'';
    }
    if (he_eg_pubkey_hex != NULL)
    {
        const char prefix[] = ", HE_EG_PUBKEY = '";
        memcpy(new_query + pos, prefix, sizeof(prefix) - 1);
        pos += sizeof(prefix) - 1;
        memcpy(new_query + pos, he_eg_pubkey_hex, strlen(he_eg_pubkey_hex));
        pos += strlen(he_eg_pubkey_hex);
        new_query[pos++] = '\'';
    }
    if (he_eg_privkey_hex != NULL)
    {
        const char prefix[] = ", HE_EG_PRIVKEY = '";
        memcpy(new_query + pos, prefix, sizeof(prefix) - 1);
        pos += sizeof(prefix) - 1;
        memcpy(new_query + pos, he_eg_privkey_hex, strlen(he_eg_privkey_hex));
        pos += strlen(he_eg_privkey_hex);
        new_query[pos++] = '\'';
    }

    /* ReqID:TECC4.0 - Inject _TECC_PROCESSED marker so backend recognizes this as TECC-processed */
    memcpy(new_query + pos, ", _TECC_PROCESSED = 'yes'", strlen(", _TECC_PROCESSED = 'yes'"));
    pos += strlen(", _TECC_PROCESSED = 'yes'");

    /* Re-insert the closing paren and the suffix (e.g., ";") */
    new_query[pos++] = ')';
    memcpy(new_query + pos, close_paren + 1, suffix_len);
    new_query[new_query_len - 1] = '\0';

    }

    /* Store rewritten query in statement data */
    if (statement_data->rewritten_query != NULL)
    {
        pg_free(statement_data->rewritten_query);
    }
    statement_data->rewritten_query = new_query;
    /* Set needs_rewrite so the caller uses the rewritten query */
    tecc_stmt_set_needs_rewrite(statement_data, true);

    /* Clean up */
    security_secure_erase(plain_dek, plain_dek_len);
    pg_free(plain_dek);
    if (kek)
        tecc_key_free_key_data(kek, kek_len);
    security_secure_erase(encrypted_dek, encrypted_dek_len);
    pg_free(encrypted_dek);
    pg_free(encrypted_dek_hex);
    if (he_ec_pubkey_hex)
        pg_free(he_ec_pubkey_hex);
    if (he_ec_privkey_hex)
        pg_free(he_ec_privkey_hex);
    if (he_eg_pubkey_hex)
        pg_free(he_eg_pubkey_hex);
    if (he_eg_privkey_hex)
        pg_free(he_eg_privkey_hex);

    /* Mark DEK cache for refresh and insert PENDING cache item */
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
        {
            const char *key_namespace;
            TeccDekCacheItem *pending;

            state->last_stmt_type = TECC_STMT_TYPE_CREATE_DEK;

            /* Store key name for post-SQL PENDING cleanup */
            strncpy(state->last_key_name, dek_name, NAMEDATALEN - 1);
            state->last_key_name[NAMEDATALEN - 1] = '\0';

            /* 从限定名中提取namespace（schema），无限定名则使用current_schema */
            if (list_length(create_dek_stmt->names) > 1)
            {
                key_namespace = strVal(linitial(create_dek_stmt->names));
            }
            else
            {
                key_namespace = tecc_cache_get_guc_param(state, "current_schema");
            }
            if (key_namespace != NULL)
            {
                strncpy(state->last_key_namespace, key_namespace, NAMEDATALEN - 1);
                state->last_key_namespace[NAMEDATALEN - 1] = '\0';
            }
            else
            {
                state->last_key_namespace[0] = '\0';
            }

            /* Insert PENDING cache item with known metadata */
            pending = (TeccDekCacheItem *)pg_malloc0(sizeof(TeccDekCacheItem));
            pending->key_name = pg_strdup(dek_name);
            pending->key_namespace = pg_strdup(key_namespace ? key_namespace : "");
            pending->kek_name = pg_strdup(kek_name);
            pending->dek_oid = InvalidOid;
            pending->cache_status = TECC_CACHE_STATUS_PENDING;
            if (tecc_cache_insert_dek_item(state, pending) != 0)
            {
                pg_free(pending->key_name);
                pg_free(pending->key_namespace);
                pg_free(pending->kek_name);
                pg_free(pending);
            }

        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Process CREATE TABLE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_create_table(const CreateStmt *create_stmt,
                                        TeccStatementData *statement_data)
{
    TeccCreateStmtProcessorResult create_result;

    if (create_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing CREATE TABLE statement");

    /* Delegate to CREATE statement processor */
    create_result = tecc_create_stmt_processor_run(create_stmt, statement_data);

    /* Convert CREATE statement processor result to statement processor result */
    switch (create_result)
    {
        case TECC_CREATE_PROCESSOR_SUCCESS:
            return TECC_STMT_PROCESSOR_SUCCESS;

        case TECC_CREATE_PROCESSOR_DUPLICATE_COLUMN:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "CREATE TABLE duplicate column name");
            return TECC_STMT_PROCESSOR_ERROR;

        case TECC_CREATE_PROCESSOR_ERROR:
        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "CREATE TABLE processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/*
 * Process ALTER TABLE statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_alter_table(const AlterTableStmt *alter_stmt,
                                        TeccStatementData *statement_data)
{
    TeccAlterStmtProcessorResult alter_result;

    if (alter_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing ALTER TABLE statement");

    /* Delegate to ALTER statement processor */
    alter_result = tecc_alter_stmt_processor_run(alter_stmt, statement_data);

    switch (alter_result)
    {
        case TECC_ALTER_PROCESSOR_SUCCESS:
            return TECC_STMT_PROCESSOR_SUCCESS;

        case TECC_ALTER_PROCESSOR_ERROR:
        default:
            tecc_processor_debug(TECC_LOG_ERROR,
                               "ALTER TABLE processing failed");
            return TECC_STMT_PROCESSOR_ERROR;
    }
}

/* ReqID:TECC4.0 - Forward declaration for ALTER TABLE dispatch end marker */

/*
 * Process DROP statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_drop(const DropStmt *drop_stmt,
                                TeccStatementData *statement_data)
{
    if (drop_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing DROP statement (removeType=%d)", drop_stmt->removeType);

    /* ReqID:TECC4.0 - Save drop object type, behaviour and key OID
     * for selective adapter cache invalidation. */
    statement_data->drop_remove_type = (int)drop_stmt->removeType;
    statement_data->drop_behavior = (int)drop_stmt->behavior;
    statement_data->drop_key_oid = InvalidOid;

    /* ReqID:TECC4.0 - DROP TABLE：从会话级临时表清单移除。
     * PG 语义下 DROP TABLE 未限定名同样优先解析临时表，清单命中即移除；
     * 不命中（普通表）时无副作用。 */
    if (drop_stmt->removeType == OBJECT_TABLE && drop_stmt->objects != NIL)
    {
        ListCell *lc;
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
        {
            foreach(lc, drop_stmt->objects)
            {
                Node *obj = (Node *) lfirst(lc);
                if (nodeTag(obj) == FE_T_Const)
                {
                    Const *c = (Const *) obj;
                    if (!c->constisnull && c->constvalue.sval != NULL)
                        tecc_state_remove_temp_table(state, c->constvalue.sval);
                }
            }
        }
    }

    /* Snapshot OID from metadata cache before server drops the key.
     * This runs pre-query, so the key still exists on the server. */
    if ((drop_stmt->removeType == OBJECT_KEK || drop_stmt->removeType == OBJECT_DEK) &&
        drop_stmt->objects != NIL)
    {
        const char *kname = strVal(linitial(drop_stmt->objects));
        TeccState *state = tecc_get_state(statement_data->conn);
        if (drop_stmt->removeType == OBJECT_KEK && state)
        {
            TeccKekCacheItem *kek = tecc_cache_get_kek_by_name_search_path(
                (const TeccState *)state, kname);
            if (kek) {
                statement_data->drop_key_oid = kek->kek_oid;
                /* ReqID:TECC4.0 - 密钥文件生命周期由 tecc_ktool 管理（generate/delete）：
                 * DROP KEK 只做元数据缓存失效，不再捕获 kms_path 去 unlink 本地文件。 */
            } else {
            }
        }
        else if (drop_stmt->removeType == OBJECT_DEK && state)
        {
            TeccDekCacheItem *dek = tecc_cache_get_dek_by_name_search_path(
                (const TeccState *)state, kname);
            if (dek) {
                statement_data->drop_key_oid = dek->dek_oid;
            } else {
            }
        }
    }

    /* Invalidate cache when KEK or DEK is dropped */
    if (drop_stmt->removeType == OBJECT_KEK)
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
            tecc_cache_mark_kek_refresh(state);
    }
    else if (drop_stmt->removeType == OBJECT_DEK)
    {
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
            tecc_cache_mark_dek_refresh(state);
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * ReqID:TECC4.0 - Bug 907：取节点里的字符串值（String / Const 两种编码都有）。
 */
static char *
stmt_node_string_value(const Node *node)
{
    if (node == NULL)
        return NULL;
    if (nodeTag(node) == FE_T_String)
        return strVal((const Value *)node);
    if (nodeTag(node) == FE_T_Const)
        return ((Const *)node)->constvalue.sval;
    return NULL;
}

/*
 * ReqID:TECC4.0 - Bug 907：拆解一个目标项的列引用。
 *
 * 前端有两种"星号"编码：SELECT * 是 ColumnRef(fields=[String("*")])，
 * tab.* 是 ColumnRef(fields=[..., A_Star])（本 bug 补的语法产生式）。
 * 两种情况都取不出真实列，必须交给星号展开处理；否则会被当成"解析不出
 * 归属的普通列"，多表时漏掉解密。
 *
 * 输出的 *qual_out 是紧邻列名的那一段限定名（tab.* 的 tab、schema.tab.col
 * 的 tab），用于把列/星号定位到具体的 FROM 基表。
 */
static void
stmt_target_ref_info(Node *val, char **col_out, char **qual_out, bool *is_star)
{
    ColumnRef *cref;
    Node *prev_field = NULL;
    Node *cur_field = NULL;
    ListCell *lc;

    if (col_out != NULL)
        *col_out = NULL;
    if (qual_out != NULL)
        *qual_out = NULL;
    if (is_star != NULL)
        *is_star = false;

    if (val == NULL)
        return;
    if (nodeTag(val) == FE_T_A_Star)
    {
        if (is_star != NULL)
            *is_star = true;
        return;
    }
    if (nodeTag(val) != FE_T_ColumnRef)
        return;

    cref = (ColumnRef *)val;
    if (cref->fields == NULL || list_length(cref->fields) <= 0)
        return;

    /* 走一遍取"列名"和它的前一段：a.b.c 里只有紧邻列名的 b 是表名/别名，
     * a 是 schema/db 前缀。 */
    foreach(lc, cref->fields)
    {
        prev_field = cur_field;
        cur_field = (Node *)lfirst(lc);
    }

    if (nodeTag(cur_field) == FE_T_A_Star)
    {
        if (is_star != NULL)
            *is_star = true;
        if (qual_out != NULL)
            *qual_out = stmt_node_string_value(prev_field);
        return;
    }

    {
        char *col_name = stmt_node_string_value(cur_field);

        if (col_out != NULL)
            *col_out = col_name;
        if (qual_out != NULL)
            *qual_out = stmt_node_string_value(prev_field);
        /* SELECT * 被语法编码成 String("*")，同样按星号处理；判定不能依赖
         * col_out（调用方可能只问 is_star） */
        if (is_star != NULL && col_name != NULL && strcmp(col_name, "*") == 0)
            *is_star = true;
    }
}

/*
 * ReqID:TECC4.0 - Bug 907：判断目标项是否是"整行"展开（* / t.*）。
 */
static bool
stmt_target_is_star(Node *val)
{
    bool is_star = false;

    stmt_target_ref_info(val, NULL, NULL, &is_star);
    return is_star;
}

/*
 * ReqID:TECC4.0 - Bug 907：星号展开所需的 FROM 基表信息。
 *
 * 展开要把 * 变成"按 FROM 顺序、逐表依次列出全部列"，所以必须知道每张
 * 基表的有序全列清单（含真实 attnum，以跨过已 DROP 的列）。加密列缓存里
 * 只有密态列，没有普通列清单，故经数据获取器的独立临时连接查 pg_attribute
 * （与 stmt_probe_column_exists 同一路子；该连接 enable_tecc=false，不会
 * 递归触发 TECC 处理）。
 */
typedef struct StmtCopyFromTable
{
    const char *relname;    /* 指向解析树，仅在本次语句处理期间有效 */
    const char *alias;
    const char *schemaname;
    int        *attnums;    /* 真实 attnum，ORDER BY attnum */
    char      **attnames;
    int         col_count;
    bool        cols_loaded;
} StmtCopyFromTable;

/* 展开上限：超过就回退（对应 FROM 基表数，正常语句远达不到） */
#define STMT_COPY_MAX_FROM_TABLES 32

/*
 * ReqID:TECC4.0 - Bug 907：递归收集 FROM 项里的基表（含 JOIN 树）。
 *
 * 返回累计表数；遇到"无法按表展开"的情形返回 -1，调用方据此回退：
 *   - USING / NATURAL：PG 会把 join 列提前且只输出一次，列序不是"各表顺序拼接"；
 *   - 非 RangeVar（子查询/函数/VALUES/集合操作）：没有 attnum 语义；
 *   - 超出上限。
 */
static int
stmt_copy_collect_from_tables(const Node *item, StmtCopyFromTable *tables,
                              int *count, int max_tables)
{
    if (item == NULL)
        return *count;

    if (nodeTag(item) == FE_T_JoinExpr)
    {
        const JoinExpr *join = (const JoinExpr *) item;

        if (join->isNatural ||
            (join->usingClause != NULL && join->usingClause->length > 0))
            return -1;

        /* 两侧都写入调用方的 count（不要用局部计数器：调用方以 *count 为准，
         * 用局部会让 JOIN 右表"收进来但计数不到"） */
        if (stmt_copy_collect_from_tables(join->larg, tables, count,
                                          max_tables) < 0)
            return -1;
        return stmt_copy_collect_from_tables(join->rarg, tables, count,
                                            max_tables);
    }

    if (nodeTag(item) != FE_T_RangeVar)
        return -1;

    if (*count >= max_tables)
        return -1;

    tables[*count].relname = ((const RangeVar *)item)->relname;
    tables[*count].alias = (((const RangeVar *)item)->alias != NULL)
                           ? ((const RangeVar *)item)->alias->aliasname : NULL;
    tables[*count].schemaname = ((const RangeVar *)item)->schemaname;
    (*count)++;
    return *count;
}

/* 限定名是否指向这张基表。pass=0 只比别名、pass=1 只比表名 ——
 * PG 里别名会遮蔽表名（FROM t a 之后 t.* 不再指向该表），故别名优先。 */
static bool
stmt_copy_table_matches(const StmtCopyFromTable *table, const char *qualifier,
                        int pass)
{
    if (qualifier == NULL)
        return true;    /* 裸 * ：展开所有 FROM 表 */
    if (pass == 0)
        return (table->alias != NULL &&
                pg_strcasecmp(table->alias, qualifier) == 0);
    return (table->relname != NULL &&
            pg_strcasecmp(table->relname, qualifier) == 0);
}

/*
 * ReqID:TECC4.0 - Bug 907：取 <schema>.<table> 的有序非丢弃列清单。
 * 返回 0 表示探测成功（col_count 可能为 0，即查不到该表），-1 表示失败。
 */
static int
stmt_fetch_table_columns(TeccStatementData *statement_data,
                         const char *schema, const char *table,
                         int **attnums, char ***attnames, int *count)
{
    TeccState *state;
    TeccCursorInterface *cursor;
    char *esc_schema;
    char *esc_table;
    char query[800];
    int *nums = NULL;
    char **names = NULL;
    int n = 0;
    int capacity = 0;

    *attnums = NULL;
    *attnames = NULL;
    *count = 0;

    if (statement_data == NULL || statement_data->conn == NULL ||
        schema == NULL || schema[0] == '\0' ||
        table == NULL || table[0] == '\0')
        return -1;

    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
        return -1;

    /* nspname/relname 比较的是字符串字面量，须用 PQescapeLiteral
     * （PQescapeIdentifier 产出的是双引号标识符，会被当成列名）。 */
    esc_schema = PQescapeLiteral(statement_data->conn, schema, strlen(schema));
    esc_table = PQescapeLiteral(statement_data->conn, table, strlen(table));
    if (esc_schema == NULL || esc_table == NULL)
    {
        if (esc_schema != NULL)
            PQfreemem(esc_schema);
        if (esc_table != NULL)
            PQfreemem(esc_table);
        return -1;
    }

    snprintf(query, sizeof(query),
             "SELECT a.attnum, a.attname FROM pg_catalog.pg_attribute a "
             "JOIN pg_catalog.pg_class c ON c.oid = a.attrelid "
             "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
             "WHERE n.nspname = %s AND c.relname = %s "
             "AND a.attnum > 0 AND NOT a.attisdropped "
             "ORDER BY a.attnum",
             esc_schema, esc_table);

    PQfreemem(esc_schema);
    PQfreemem(esc_table);

    cursor = tecc_fetcher_execute_query(state, query);
    if (cursor == NULL)
        return -1;

    while (cursor->fetch_next(cursor) == TECC_FETCHER_OK)
    {
        const char *attnum_str = cursor->get_column(cursor, 0);
        const char *attname_str = cursor->get_column(cursor, 1);
        int attnum;

        if (attnum_str == NULL || attname_str == NULL)
            continue;
        attnum = atoi(attnum_str);
        if (attnum <= 0)
            continue;

        if (n >= capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            nums = (int *)pg_realloc(nums, capacity * sizeof(int));
            names = (char **)pg_realloc(names, capacity * sizeof(char *));
        }
        nums[n] = attnum;
        names[n] = pg_strdup(attname_str);
        n++;
    }

    cursor->close(cursor);
    cursor->destroy(cursor);

    *attnums = nums;
    *attnames = names;
    *count = n;
    return 0;
}

/* 惰性取某张基表的列清单（同一张表只查一次，自连接也不会重复查） */
static int
stmt_copy_ensure_table_columns(TeccStatementData *statement_data,
                               StmtCopyFromTable *table)
{
    const char *schema;

    if (table->cols_loaded)
        return 0;

    schema = tecc_stmt_resolve_table_schema(statement_data, table->schemaname,
                                            table->relname);
    if (stmt_fetch_table_columns(statement_data, schema, table->relname,
                                 &table->attnums, &table->attnames,
                                 &table->col_count) != 0)
        return -1;

    /* 查不到列（表不存在 / 无权限 / 视图无输出列）：不可展开 */
    if (table->col_count <= 0)
        return -1;

    table->cols_loaded = true;
    return 0;
}

/* 释放收集阶段取到的列清单 */
static void
stmt_copy_free_from_table_columns(StmtCopyFromTable *tables, int count)
{
    int i;
    int j;

    for (i = 0; i < count; i++)
    {
        if (tables[i].attnames != NULL)
        {
            for (j = 0; j < tables[i].col_count; j++)
            {
                if (tables[i].attnames[j] != NULL)
                    pg_free(tables[i].attnames[j]);
            }
            pg_free(tables[i].attnames);
        }
        if (tables[i].attnums != NULL)
            pg_free(tables[i].attnums);
        tables[i].attnames = NULL;
        tables[i].attnums = NULL;
    }
}

/*
 * ReqID:TECC4.0 - Bug 907：列映射的可增长构造缓冲。
 *
 * 星号展开后条目数不再等于 targetList 长度，先按需追加、最后整体交给
 * statement_data（数组所有权随转移，释放由 copy_state 侧负责）。
 */
typedef struct StmtCopyMapBuilder
{
    int   *attnums;
    char **tables;
    int    count;
    int    capacity;
} StmtCopyMapBuilder;

static void
stmt_copy_map_append(StmtCopyMapBuilder *builder, int attnum, const char *table)
{
    if (builder->count >= builder->capacity)
    {
        int new_capacity = (builder->capacity == 0) ? 8 : builder->capacity * 2;

        builder->attnums = (int *)pg_realloc(builder->attnums,
                                            new_capacity * sizeof(int));
        builder->tables = (char **)pg_realloc(builder->tables,
                                              new_capacity * sizeof(char *));
        builder->capacity = new_capacity;
    }

    builder->attnums[builder->count] = attnum;
    builder->tables[builder->count] = (table != NULL) ? pg_strdup(table) : NULL;
    builder->count++;
}

static void
stmt_copy_map_free(StmtCopyMapBuilder *builder)
{
    int i;

    for (i = 0; i < builder->count; i++)
    {
        if (builder->tables[i] != NULL)
            pg_free(builder->tables[i]);
    }
    if (builder->attnums != NULL)
        pg_free(builder->attnums);
    if (builder->tables != NULL)
        pg_free(builder->tables);
    builder->attnums = NULL;
    builder->tables = NULL;
    builder->count = 0;
    builder->capacity = 0;
}

/*
 * ReqID:TECC4.0 - Bug 907：在 FROM 项（含 JOIN 树/嵌套 JOIN）里定位"含该密态列"的表。
 *
 *   - 限定列（a.name）按 FROM 别名或表名匹配，对不上就跳过该 FROM 项；
 *   - 未限定列（name）按 FROM 顺序取第一个命中表（近似 PG 名字解析）。
 *
 * 命中返回列元数据并把 *table_out 指向该表的 relname（指针属于解析树，仅在
 * 本次语句处理期间有效）；未命中原样返回 NULL。
 *
 * 修复前此处只认 fromClause 首项且必须是 RangeVar：显式 JOIN 下首项是 JoinExpr，
 * 表名直接取不到，映射退化成"首表 + attnum 猜"，多表 JOIN 的 \copy 导出因此
 * 整行输出密文。
 */
static TeccColumnCacheItem *
stmt_copy_resolve_from_item(const TeccStatementData *statement_data,
                            const Node *item,
                            const char *qualifier,
                            const char *col_name,
                            const char **table_out)
{
    if (statement_data == NULL || item == NULL || col_name == NULL)
        return NULL;

    if (nodeTag(item) == FE_T_JoinExpr)
    {
        const JoinExpr *join = (const JoinExpr *) item;
        TeccColumnCacheItem *hit;

        hit = stmt_copy_resolve_from_item(statement_data, join->larg,
                                          qualifier, col_name, table_out);
        if (hit != NULL)
            return hit;
        return stmt_copy_resolve_from_item(statement_data, join->rarg,
                                           qualifier, col_name, table_out);
    }

    if (nodeTag(item) != FE_T_RangeVar)
        return NULL;

    {
        const RangeVar *range_var = (const RangeVar *) item;
        TeccState *state;
        const char *database_name;
        const char *schema_name;
        TeccColumnCacheItem *ci;

        if (range_var->relname == NULL)
            return NULL;

        /* 限定名既不是该表的表名也不是其别名 → 本列不来自这张表 */
        if (qualifier != NULL &&
            pg_strcasecmp(qualifier, range_var->relname) != 0 &&
            (range_var->alias == NULL || range_var->alias->aliasname == NULL ||
             pg_strcasecmp(qualifier, range_var->alias->aliasname) != 0))
            return NULL;

        state = tecc_get_state(statement_data->conn);
        if (state == NULL)
            return NULL;

        database_name = (statement_data->conn != NULL &&
                         statement_data->conn->dbName != NULL)
                        ? statement_data->conn->dbName : "postgres";
        schema_name = tecc_stmt_resolve_table_schema(statement_data,
                                                     range_var->schemaname,
                                                     range_var->relname);
        ci = tecc_cache_get_encrypted_column_by_name(state, database_name,
                                                     schema_name,
                                                     range_var->relname,
                                                     col_name);
        if (ci != NULL && table_out != NULL)
            *table_out = range_var->relname;
        return ci;
    }
}

/*
 * Process COPY statement
 */
TeccStmtProcessorResult
tecc_stmt_processor_process_copy(const CopyStmt *copy_stmt,
                                TeccStatementData *statement_data)
{
    if (copy_stmt == NULL || statement_data == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing COPY statement (is_from=%s)",
                        copy_stmt->is_from ? "true" : "false");

    /* ==================== 1. 验证加密列限制 ==================== */
    /* 加密列只支持STDIN/STDOUT，不支持文件操作 */
    if (copy_stmt->filename != NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "COPY with filename not supported for encrypted columns: %s",
                           copy_stmt->filename);
        return TECC_STMT_PROCESSOR_ERROR;
    }

    /* ==================== 2. 处理表引用信息 ==================== */
    if (copy_stmt->relation != NULL) {
        /* 获取表名信息 */
        const char *table_name = copy_stmt->relation->relname;
        const char *schema_name = copy_stmt->relation->schemaname;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY %s table: %s.%s",
                           copy_stmt->is_from ? "FROM" : "TO",
                           schema_name ? schema_name : "(default)",
                           table_name);

        /* 记录表信息到statement_data，供后续处理使用 */
        /* 注意：TeccStatementData结构体目前没有table_name和schema_name字段 */
        /* 如果需要这些信息，可以存储在扩展字段或通过其他机制传递 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Table info: %s.%s (not stored in statement_data)",
                           schema_name ? schema_name : "(default)",
                           table_name);
    }

    /* ==================== 3. 处理COPY (SELECT ...) 查询 ==================== */
    if (copy_stmt->query != NULL) {
        /* COPY (SELECT ...) TO STDOUT */
        

        if (copy_stmt->is_from) {
            /* COPY FROM不支持SELECT查询（前端解析器已检查，这里双重验证） */
            tecc_processor_debug(TECC_LOG_ERROR,
                               "COPY FROM with SELECT query not supported");
            return TECC_STMT_PROCESSOR_ERROR;
        }

        /* 处理SELECT查询中的加密列 */
        /* 需要递归处理SELECT语句中的列引用 */
        


        /* 递归处理SELECT查询 */
        if (nodeTag(copy_stmt->query) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) copy_stmt->query;
            TeccStmtProcessorResult select_result;

            select_result = tecc_stmt_processor_process_select(select_stmt, statement_data);
            if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to process SELECT query in COPY statement: %d", select_result);
                return select_result;
            }

            /* Build COPY column mapping from SELECT target list.
             * Reuses the processor's parsed SELECT — no duplicate parsing. */
            if (select_stmt->targetList != NULL)
            {
                /* ReqID:TECC4.0 - Bug 907: 星号（* / tab.*）必须展开成逐列才能
                 * 定位到具体表的 attnum。展开的前提是"FROM 全是基表且无
                 * USING/NATURAL"；不满足就整条不建映射，交给结果行处理侧按
                 * identity 回退（与修复前行为一致），不许半展开。 */
                StmtCopyFromTable from_tables[STMT_COPY_MAX_FROM_TABLES];
                StmtCopyMapBuilder builder;
                int from_count = 0;
                bool expandable = false;
                bool need_expand = false;
                bool build_ok = true;

                memset(from_tables, 0, sizeof(from_tables));
                memset(&builder, 0, sizeof(builder));

                /* 先看是否需要展开 */
                {
                    ListCell *lc;

                    foreach(lc, select_stmt->targetList)
                    {
                        ResTarget *res = (ResTarget *)lfirst(lc);

                        if (res != NULL && stmt_target_is_star(res->val))
                        {
                            need_expand = true;
                            break;
                        }
                    }
                }

                /* 收集 FROM 基表（只取指针，不查库；查库推迟到真正展开时） */
                if (select_stmt->fromClause != NULL)
                {
                    ListCell *flc;
                    bool collect_ok = true;

                    foreach(flc, select_stmt->fromClause)
                    {
                        if (stmt_copy_collect_from_tables((const Node *)lfirst(flc),
                                                          from_tables, &from_count,
                                                          STMT_COPY_MAX_FROM_TABLES) < 0)
                        {
                            collect_ok = false;
                            break;
                        }
                    }
                    if (collect_ok && from_count > 0)
                        expandable = true;
                }

                /* 需要展开但不具备展开条件（子查询/VALUES/USING/NATURAL）→
                 * 不建映射，交给结果行处理侧按 identity 回退（与修复前一致），
                 * 不许半展开。 */
                if (!need_expand || expandable)
                {
                    ListCell *lc3;

                    foreach(lc3, select_stmt->targetList)
                    {
                        ResTarget *res = (ResTarget *)lfirst(lc3);
                        char *col_name = NULL;
                        char *qualifier = NULL;
                        bool is_star = false;

                        if (res == NULL || res->val == NULL)
                        {
                            stmt_copy_map_append(&builder, -1, NULL);
                            continue;
                        }

                        stmt_target_ref_info(res->val, &col_name, &qualifier,
                                             &is_star);

                        if (is_star)
                        {
                            int ti;
                            int pass;
                            int matched = 0;

                            /* 别名优先，没有别名命中再按表名（见
                             * stmt_copy_table_matches） */
                            for (pass = 0; pass < 2 && matched == 0; pass++)
                            {
                                for (ti = 0; ti < from_count; ti++)
                                {
                                    int ci;

                                    if (!stmt_copy_table_matches(&from_tables[ti],
                                                                 qualifier, pass))
                                        continue;
                                    matched++;
                                    if (stmt_copy_ensure_table_columns(statement_data,
                                                                       &from_tables[ti]) != 0)
                                    {
                                        build_ok = false;
                                        break;
                                    }
                                    for (ci = 0; ci < from_tables[ti].col_count; ci++)
                                        stmt_copy_map_append(&builder,
                                                             from_tables[ti].attnums[ci],
                                                             from_tables[ti].relname);
                                }
                                if (!build_ok)
                                    break;
                            }

                            /* tab.* 的限定名对不上任何 FROM 基表 → 无法展开 */
                            if (matched == 0)
                                build_ok = false;
                            if (!build_ok)
                                break;
                        }
                        else if (col_name != NULL &&
                                 select_stmt->fromClause != NULL)
                        {
                            ListCell *from_lc;

                            /* Bug 907：按"列真正所属的那张表"解析，而不是
                             * 一律拿 FROM 首表去猜 attnum。 */
                            foreach(from_lc, select_stmt->fromClause)
                            {
                                const char *hit_table = NULL;
                                TeccColumnCacheItem *ci = stmt_copy_resolve_from_item(
                                    statement_data,
                                    (const Node *)lfirst(from_lc),
                                    qualifier, col_name, &hit_table);

                                if (ci != NULL)
                                {
                                    stmt_copy_map_append(&builder,
                                                         ci->column_attnum,
                                                         hit_table);
                                    break;
                                }
                            }
                            if (from_lc == NULL)
                                stmt_copy_map_append(&builder, -1, NULL);
                        }
                        else
                        {
                            /* 非密态列 / 表达式 / 求不出归属：记 -1，由结果行
                             * 处理侧跳过解密 */
                            stmt_copy_map_append(&builder, -1, NULL);
                        }
                    }
                }

                stmt_copy_free_from_table_columns(from_tables, from_count);

                if (build_ok && builder.count > 0)
                {
                    statement_data->copy_column_mapping = builder.attnums;
                    statement_data->copy_column_table_names = builder.tables;
                    statement_data->copy_mapping_count = builder.count;
                }
                else
                {
                    /* 展开失败：全量回退 identity（不留下半份映射） */
                    stmt_copy_map_free(&builder);
                }
            }

	/* copy_table_name not needed — table_name is extracted from
	 * tecc_preprocess_copy's frontend parser which is more reliable. */
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "COPY query is not a SELECT statement (type: %d)", nodeTag(copy_stmt->query));
        }
    }

    /* ==================== 4. 处理COPY方向 ==================== */
    if (copy_stmt->is_from) {
        /* COPY FROM - 导入数据到加密列 */
        

        /* 实现加密列的COPY FROM处理 */
        /* 这涉及：
           1. 识别目标表中的加密列
           2. 为传入数据设置加密
           3. 修改COPY命令以处理加密数据
        */

        /* 检查目标表是否有加密列 */
        if (copy_stmt->relation != NULL) {
            const char *table_name = copy_stmt->relation->relname;



            /* 如果有显式列列表，检查这些列是否加密 */
            if (copy_stmt->attlist != NULL) {
                ListCell *lc;
                bool has_encrypted_columns = false;

                for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
                    Node *node = (Node *) lc->data.ptr_value;
                    if (node != NULL && nodeTag(node) == FE_T_String) {
                        char *col_name = (char *) node;

                        /* 检查此列是否加密 */
                        bool is_encrypted = false;

                        /* 首先检查statement_data中的加密列列表 */
                        if (statement_data->encrypted_columns != NIL) {
                            ListCell *lc2;
                            foreach(lc2, statement_data->encrypted_columns) {
                                TeccColumnRefData *encrypted_col = (TeccColumnRefData *) lfirst(lc2);
                                if (encrypted_col != NULL &&
                                    encrypted_col->column_name[0] != '\0' &&
                                    strcmp(encrypted_col->column_name, col_name) == 0) {
                                    is_encrypted = true;
                                    break;
                                }
                            }
                        }

                        /* 如果statement_data中没有找到，使用缓存模块检查 */
                        if (!is_encrypted && table_name != NULL) {
                            is_encrypted = stmt_check_column_encrypted(statement_data,
                                copy_stmt->relation ? copy_stmt->relation->schemaname : NULL,
                                table_name, col_name);
                        }

                        /* 如果发现加密列，标记需要改写 */
                        if (is_encrypted) {
                            has_encrypted_columns = true;

                        }
                    }
                }

                if (has_encrypted_columns) {
                    statement_data->needs_rewrite = 1;
                    statement_data->is_copy_command = true;

                }
			} else {
				/* 没有显式列列表，查询元数据确认是否有加密列 */
				TeccColumnCacheItem **tbl_cols = NULL;
				int tbl_col_cnt = 0;
				if (stmt_load_table_metadata(copy_stmt->relation, statement_data,
											 &tbl_cols, &tbl_col_cnt)
					&& tbl_col_cnt > 0)
				{
					statement_data->needs_rewrite = 1;
					statement_data->is_copy_command = true;
				}
			}

		}
    } else {
        /* COPY TO - 从加密列导出数据 */
        

        /* 实现加密列的COPY TO处理 */
        /* 这涉及：
           1. 识别源表中的加密列
           2. 为传出数据设置解密
           3. 修改COPY命令以处理解密数据
        */

        /* 检查源表是否有加密列 */
        if (copy_stmt->relation != NULL) {


            /* 如果有显式列列表，检查这些列是否加密 */
            if (copy_stmt->attlist != NULL) {
                ListCell *lc;
                bool has_encrypted_columns = false;

                for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
                    Node *node = (Node *) lc->data.ptr_value;
                    if (node != NULL && nodeTag(node) == FE_T_String) {
                        char *col_name = (char *) node;

                        /* 检查此列是否加密 */
                        bool is_encrypted = false;

                        /* 首先检查statement_data中的加密列列表 */
                        if (statement_data->encrypted_columns != NIL) {
                            ListCell *lc2;
                            foreach(lc2, statement_data->encrypted_columns) {
                                TeccColumnRefData *encrypted_col = (TeccColumnRefData *) lfirst(lc2);
                                if (encrypted_col != NULL &&
                                    encrypted_col->column_name[0] != '\0' &&
                                    strcmp(encrypted_col->column_name, col_name) == 0) {
                                    is_encrypted = true;
                                    break;
                                }
                            }
                        }

                        /* 如果发现加密列，标记需要改写 */
                        if (is_encrypted) {
                            has_encrypted_columns = true;
                            
                        }
                    }
                }

                if (has_encrypted_columns) {
                    statement_data->needs_rewrite = 1;
                    statement_data->is_copy_command = true;
                    
                }
			} else {
				/* 没有显式列列表，查询元数据确认是否有加密列 */
				TeccColumnCacheItem **tbl_cols2 = NULL;
				int tbl_col_cnt2 = 0;
				if (stmt_load_table_metadata(copy_stmt->relation, statement_data,
											 &tbl_cols2, &tbl_col_cnt2)
					&& tbl_col_cnt2 > 0)
				{
					statement_data->needs_rewrite = 1;
					statement_data->is_copy_command = true;
				}
			}
        }

    }

    /* ==================== 5. 提取列引用 ==================== */
    if (copy_stmt->attlist != NULL) {
        ListCell *lc;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has explicit column list with %d columns",
                           list_length(copy_stmt->attlist));

        for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
            Node *node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *col_data;

            if (node != NULL && nodeTag(node) == FE_T_String) {
                char *col_name = (char *) node;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "COPY column: %s", col_name);

                /* 创建列引用数据结构 */
                col_data = tecc_column_ref_data_create();
                if (col_data != NULL) {
                    tecc_column_ref_data_init(col_data,
                                             copy_stmt->relation ? copy_stmt->relation->relname : NULL,
                                             copy_stmt->relation ? copy_stmt->relation->schemaname : NULL,
                                             col_name,
                                             NULL);

                    /* TODO: 检查此列是否加密 - 需要查询元数据缓存 */
                    /* 目前先添加到列引用列表 */
                    statement_data->encrypted_columns = lappend(statement_data->encrypted_columns, col_data);
                }
            }
        }
    } else if (copy_stmt->relation != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has no explicit column list - all table columns implied");
		/* 没有显式列列表 */
    }

    /* ==================== 6. 处理COPY选项 ==================== */
    if (copy_stmt->options != NULL) {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has %d options", list_length(copy_stmt->options));

        /* 处理COPY选项，特别是与加密相关的选项 */
        for (lc = copy_stmt->options->head; lc != NULL; lc = lc->next) {
            DefElem *def_elem = (DefElem *) lc->data.ptr_value;
            if (def_elem != NULL && def_elem->defname != NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "COPY option: %s", def_elem->defname);

                /* 检查可能影响加密处理的选项 */
                if (strcmp(def_elem->defname, "format") == 0) {
                    /* 格式选项可能影响加密数据的处理 */
                    
                } else if (strcmp(def_elem->defname, "encoding") == 0) {
                    /* 编码选项可能影响加密数据的处理 */
                    
                }
            }
        }
    }

    /* ==================== 7. 设置SQL改写支持 ==================== */
    /* 使用stmt_location和stmt_len支持处理器模块的SQL改写 */
    if (copy_stmt->stmt_location >= 0 && copy_stmt->stmt_len > 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY statement location: %d, length: %d",
                           copy_stmt->stmt_location, copy_stmt->stmt_len);

        /* 将位置信息记录到statement_data，供SQL改写使用 */
        /* 注意：TeccStatementData结构体目前没有stmt_location和stmt_len字段 */
        /* 位置信息可以通过其他机制传递给SQL改写器 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Statement location info available (not stored in statement_data)");
    }

    /* 如果检测到加密列或需要特殊处理，标记需要SQL改写 */
    if (statement_data->needs_rewrite) {
        
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/* End - ReqID:TECC4.0 */

/*
 * Extract column references from a statement and check if they are encrypted
 */
static TeccStmtProcessorResult
stmt_extract_and_check_columns(const Node *stmt,
                              TeccStatementData *statement_data,
                              List **encrypted_columns)
{
    TeccStmtProcessorResult result = TECC_STMT_PROCESSOR_SUCCESS;
    List *stmt_columns = NIL;
    ListCell *lc;
    int is_encrypted;

    if (stmt == NULL || statement_data == NULL || encrypted_columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract column references based on statement type */
    switch (nodeTag(stmt))
    {
        case FE_T_SelectStmt:
            result = stmt_extract_select_columns((SelectStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_InsertStmt:
            result = stmt_extract_insert_columns((InsertStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_UpdateStmt:
            result = stmt_extract_update_columns((UpdateStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_DeleteStmt:
            result = stmt_extract_delete_columns((DeleteStmt *) stmt, &stmt_columns, statement_data);
            break;

        case FE_T_CreateStmt:
            result = stmt_extract_create_columns((CreateStmt *) stmt, &stmt_columns);
            break;

        case FE_T_AlterTableStmt:
            /* ALTER TABLE statement - extract columns from table modifications */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "ALTER TABLE statement column extraction not yet implemented");
            /* For now, return success with no columns */
            result = TECC_STMT_PROCESSOR_SUCCESS;
            break;

        case FE_T_CopyStmt:
            /* COPY statement - extract columns from table or query */
            result = stmt_extract_copy_columns((CopyStmt *) stmt, &stmt_columns, statement_data);
            break;

        default:
            /* Unsupported statement type - no columns to extract */
            return TECC_STMT_PROCESSOR_SUCCESS;
    }

    if (result != TECC_STMT_PROCESSOR_SUCCESS)
        return result;

    /* Check each column against cache to see if it's encrypted */
    if (stmt_columns != NULL) {
        for (lc = stmt_columns->head; lc != NULL; lc = lc->next)
        {
            TeccColumnRefData *col_ref = (TeccColumnRefData *) lc->data.ptr_value;
            if (col_ref == NULL)
                continue;

            /* Check if column is encrypted */
            /* ReqID:TECC4.0 - Use is_encrypted from column_ref_data (set by tecc_expand_column_ref_with_stmt_data) */
            is_encrypted = col_ref->is_encrypted ? 1 : 0;

            if (is_encrypted)
            {
                /* Add to encrypted columns list */
                *encrypted_columns = lappend(*encrypted_columns, col_ref);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Found encrypted column: %s.%s",
                                   col_ref->table_name, col_ref->column_name);
            }
            else
            {
                /* Free non-encrypted column reference */
                tecc_column_ref_data_destroy(col_ref);
            }
        }
    }

    /* Free the temporary column list (elements already freed or moved) */
    list_free(stmt_columns);

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from SELECT statement
 */
static TeccStmtProcessorResult
stmt_extract_select_columns(const SelectStmt *select_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;

    if (select_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from SELECT statement");

    /* ReqID:TECC4.0 - Traverse CTE definitions first */
    if (select_stmt->cteList != NIL)
    {
        ListCell *cte_lc;
        foreach(cte_lc, select_stmt->cteList)
        {
            CommonTableExpr *cte = (CommonTableExpr *) lfirst(cte_lc);
            if (cte->ctequery != NULL)
            {
                if (nodeTag(cte->ctequery) == FE_T_SelectStmt)
                {
                    stmt_extract_select_columns((SelectStmt *)cte->ctequery, columns, statement_data);
                }
                /* ReqID:TECC4.0 - DML CTE: extract columns from INSERT/UPDATE/DELETE */
                else if (nodeTag(cte->ctequery) == FE_T_InsertStmt)
                {
                    tecc_stmt_processor_process_insert((InsertStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_UpdateStmt)
                {
                    tecc_stmt_processor_process_update((UpdateStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
                else if (nodeTag(cte->ctequery) == FE_T_DeleteStmt)
                {
                    tecc_stmt_processor_process_delete((DeleteStmt *)cte->ctequery, (TeccStatementData *)statement_data);
                }
            }
        }
    }

    /* Extract columns from target list */
    if (select_stmt->targetList != NIL) {
        for (lc = select_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            Node *target_node = NULL;
            ResTarget *res_target = NULL;

            target_node = (Node *) lc->data.ptr_value;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            res_target = (ResTarget *) target_node;
            if (res_target->val == NULL)
                continue;

            /* Check if this is a column reference */
            if (nodeTag(res_target->val) == FE_T_ColumnRef)
            {
                /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
                ColumnRef *col_ref = (ColumnRef *) res_target->val;
                TeccColumnRefData *tecc_col = NULL;

                /* Use tecc_expand_column_ref_with_stmt_data to extract column information */
                if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                {
                    if (tecc_col != NULL)
                    {
                        *columns = lappend(*columns, tecc_col);
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Added column reference: %s (encrypted=%s)",
                                           tecc_col->alias_fqdn,
                                           tecc_col->is_encrypted ? "true" : "false");
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Failed to expand column reference in target list");
                }
            }
            else if (nodeTag(res_target->val) == FE_T_FuncCall)
            {
                /* Function call - extract column references from arguments */
                FuncCall *func_call = (FuncCall *) res_target->val;
                if (func_call->args != NULL)
                {
                    ListCell *arg_lc;
                    for (arg_lc = func_call->args->head; arg_lc != NULL; arg_lc = arg_lc->next)
                    {
                        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
                        Node *arg = NULL;
                        ColumnRef *col_ref = NULL;
                        TeccColumnRefData *tecc_col = NULL;

                        arg = (Node *) arg_lc->data.ptr_value;
                        if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
                        {
                            col_ref = (ColumnRef *) arg;

                            if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))

                            if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col, statement_data))
                            {
                                if (tecc_col != NULL)
                                {
                                    *columns = lappend(*columns, tecc_col);
                                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                                       "Added column reference from function argument: %s",
                                                       tecc_col->alias_fqdn);
                                }
                            }
                        }
                    }
                }
            }
            else if (nodeTag(res_target->val) == FE_T_A_Expr)
            {
                /* A_Expr (arithmetic expression, comparison, etc.) */
                A_Expr *a_expr = (A_Expr *) res_target->val;
                /* ReqID:TECC4.0 - Fix C90 mixed declaration warning - a_expr declared at block start */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing A_Expr in target list (kind: %d)", a_expr->kind);

                /* Extract columns from left, right, and escape expressions */
                if (a_expr->lexpr != NULL)
                    stmt_extract_columns_from_expression(a_expr->lexpr, columns, statement_data);
                if (a_expr->rexpr != NULL)
                    stmt_extract_columns_from_expression(a_expr->rexpr, columns, statement_data);
                if (a_expr->escape != NULL)
                    stmt_extract_columns_from_expression(a_expr->escape, columns, statement_data);
            }
            else if (nodeTag(res_target->val) == FE_T_TypeCast)
            {
                /* Type cast expression */
                TypeCast *type_cast = (TypeCast *) res_target->val;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing TypeCast in target list");

                /* Extract columns from the argument */
                if (type_cast->arg != NULL)
                    stmt_extract_columns_from_expression(type_cast->arg, columns, statement_data);
            }
            else if (nodeTag(res_target->val) == FE_T_SubLink)
            {
                /* ReqID:TECC4.0 - SubLink in target list (scalar subquery) */
                struct SubLink *sublink = (struct SubLink *)res_target->val;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing SubLink in target list");
                stmt_extract_columns_from_expression((Node *)sublink, columns, statement_data);
            }
            else
            {
                /* Other expression types - use generic expression extractor */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Processing other expression type in target list: %d", nodeTag(res_target->val));
                stmt_extract_columns_from_expression(res_target->val, columns, statement_data);
            }
        }
    }

    /* Extract columns from WHERE clause */
    if (select_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from WHERE clause");

        /* Extract column references from WHERE clause expression */
        stmt_extract_columns_from_expression(select_stmt->whereClause, columns, statement_data);
    }

    /* ReqID:TECC4.0 - Extract columns from DISTINCT ON clause */
    if (select_stmt->distinctClause != NIL)
    {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from DISTINCT ON clause");
        for (lc = list_head(select_stmt->distinctClause); lc != NULL; lc = lnext(lc))
        {
            Node *dist_expr = (Node *)lfirst(lc);
            if (dist_expr != NULL && nodeTag(dist_expr) != FE_T_Integer)
                stmt_extract_columns_from_expression(dist_expr, columns, statement_data);
        }
    }

    /* ReqID:TECC4.0 - Extract columns from ORDER BY (sortClause) */
    if (select_stmt->sortClause != NIL)
    {
        ListCell *lc;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from ORDER BY clause");
        for (lc = list_head(select_stmt->sortClause); lc != NULL; lc = lnext(lc))
        {
            SortBy *sortby = (SortBy *)lfirst(lc);
            if (sortby != NULL && sortby->node != NULL)
                stmt_extract_columns_from_expression(sortby->node, columns, statement_data);
        }
    }

    /* Extract columns from FROM clause */
    if (select_stmt->fromClause != NULL) {
        ListCell *lc;
        for (lc = select_stmt->fromClause->head; lc != NULL; lc = lc->next)
        {
            Node *from_item = (Node *) lc->data.ptr_value;

            if (nodeTag(from_item) == FE_T_RangeVar)
            {
                RangeVar *range_var = (RangeVar *) from_item;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause table: %s.%s",
                                   range_var->schemaname ? range_var->schemaname : "(default)",
                                   range_var->relname ? range_var->relname : "(null)");

                /* 对于简单的表引用，我们无法提取具体的列信息，
                   但可以记录表名供后续元数据查询使用 */
                /* 列提取主要在targetList和WHERE子句中完成 */
            }
            else if (nodeTag(from_item) == FE_T_RangeSubselect)
            {
                /* ReqID:TECC4.0 - RangeSubselect in FROM clause */
                RangeSubselect *rss = (RangeSubselect *) from_item;
                if (rss->subquery != NULL && nodeTag(rss->subquery) == FE_T_SelectStmt)
                {
                    SelectStmt *subselect = (SelectStmt *) rss->subquery;
                    List *subselect_columns = NIL;
                    TeccStmtProcessorResult subselect_result;

                    subselect_result = stmt_extract_select_columns(subselect, &subselect_columns, statement_data);
                    if (subselect_result == TECC_STMT_PROCESSOR_SUCCESS)
                    {
                        ListCell *col_lc;
                        for (col_lc = subselect_columns->head; col_lc != NULL; col_lc = col_lc->next)
                        {
                            TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                            if (tecc_col != NULL)
                            {
                                *columns = lappend(*columns, tecc_col);
                            }
                        }
                        list_free(subselect_columns);
                    }
                }
            }
            else if (nodeTag(from_item) == FE_T_SelectStmt)
            {
                /* 子查询 - 递归提取列 */
                SelectStmt *subselect = (SelectStmt *) from_item;
                List *subselect_columns = NIL;
                TeccStmtProcessorResult subselect_result;

                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause subselect detected");

                /* 递归提取子查询中的列 */
                subselect_result = stmt_extract_select_columns(subselect, &subselect_columns, statement_data);
                if (subselect_result == TECC_STMT_PROCESSOR_SUCCESS)
                {
                    /* 将子查询的列添加到主查询的列列表中 */
                    ListCell *col_lc;
                    for (col_lc = subselect_columns->head; col_lc != NULL; col_lc = col_lc->next)
                    {
                        TeccColumnRefData *tecc_col = (TeccColumnRefData *) col_lc->data.ptr_value;
                        if (tecc_col != NULL)
                        {
                            *columns = lappend(*columns, tecc_col);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Added column from subselect: %s",
                                               tecc_col->alias_fqdn);
                        }
                    }

                    /* 清理子查询列列表（列对象本身不释放，因为它们已添加到主列表） */
                    list_free(subselect_columns);
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Failed to extract columns from FROM subselect: %d", subselect_result);
                }
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "FROM clause item type: %d", nodeTag(from_item));
            }
        }
    }

    

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from INSERT statement
 */
static TeccStmtProcessorResult
stmt_extract_insert_columns(const InsertStmt *insert_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;
    const char *table_name;

    if (insert_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table name */
    table_name = insert_stmt->relation->relname;

    /* Extract columns from column list */
    if (insert_stmt->cols != NULL) {
        for (lc = insert_stmt->cols->head; lc != NULL; lc = lc->next)
        {
            Node *target_node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *tecc_col;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            {
                ResTarget *res_target = (ResTarget *) target_node;
                if (res_target->name == NULL)
                    continue;

                tecc_col = tecc_column_ref_data_create();
                if (tecc_col != NULL)
                {
                    tecc_column_ref_data_init(tecc_col, NULL, NULL, table_name, res_target->name);
                    *columns = lappend(*columns, tecc_col);
                }
            }
        }
    }

    /* Extract columns from VALUES clause */
    if (insert_stmt->selectStmt != NULL)
    {
        /* Extract columns from sub-select */
        /* ReqID:TECC4.0 - Recursively extract columns from sub-select */
        if (nodeTag(insert_stmt->selectStmt) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) insert_stmt->selectStmt;
            stmt_extract_select_columns(select_stmt, columns, statement_data);
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from UPDATE statement
 */
static TeccStmtProcessorResult
stmt_extract_update_columns(const UpdateStmt *update_stmt, List **columns,
                           const TeccStatementData *statement_data)
{
    ListCell *lc;
    const char *table_name;

    if (update_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table name */
    table_name = update_stmt->relation->relname;

    /* Extract columns from target list (SET clause) */
    if (update_stmt->targetList != NIL) {
        for (lc = update_stmt->targetList->head; lc != NULL; lc = lc->next)
        {
            Node *target_node = (Node *) lc->data.ptr_value;
            TeccColumnRefData *tecc_col;

            /* Check if this is a Const node */
            if (target_node == NULL || target_node->type == FE_T_Const) {
                /* It's a constant value or NULL, not a column reference */
                continue;
            }

            /* Otherwise, assume it's a ResTarget */
            /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            {
                ResTarget *res_target = (ResTarget *) target_node;
                if (res_target->name == NULL)
                    continue;

                tecc_col = tecc_column_ref_data_create();
                if (tecc_col != NULL)
                {
                    tecc_column_ref_data_init(tecc_col, NULL, NULL, table_name, res_target->name);
                    *columns = lappend(*columns, tecc_col);
                }
            }
        }
    }

    /* Extract columns from WHERE clause */
    if (update_stmt->whereClause != NULL)
    {
        /* Begin - ReqID:TECC4.0 - UPDATE语句WHERE子句列提取实现 */
        List *where_columns = NIL;
        bool has_encrypted_columns = false;
        ListCell *lc;

        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from UPDATE WHERE clause");

        /* Extract columns from WHERE clause expression */
        stmt_extract_columns_from_expression(update_stmt->whereClause, &where_columns, statement_data);

        /* Check if any extracted columns are encrypted */
        if (where_columns != NULL)
        {
            for (lc = where_columns->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL && tecc_col->is_encrypted)
                {
                    has_encrypted_columns = true;
                    
                }
            }

            /* If WHERE clause contains encrypted columns, log for debugging */
            if (has_encrypted_columns)
            {
                
            }

            /* Free the temporary column list */
            {
                ListCell *lc;
                for (lc = where_columns->head; lc != NULL; lc = lc->next)
                {
                    TeccColumnRefData *col_data = (TeccColumnRefData *)lc->data.ptr_value;
                    if (col_data != NULL)
                    {
                        tecc_column_ref_data_destroy(col_data);
                    }
                }
                list_free(where_columns);
            }
        }
        /* End - ReqID:TECC4.0 - UPDATE语句WHERE子句列提取实现 */
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from DELETE statement
 */
static TeccStmtProcessorResult
stmt_extract_delete_columns(const DeleteStmt *delete_stmt, List **columns,
                            const TeccStatementData *statement_data)
{
    const char *table_name;
    const char *schema_name;
    const char *catalog_name;

    if (delete_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table information */
    if (delete_stmt->relation == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    table_name = delete_stmt->relation->relname;
    schema_name = delete_stmt->relation->schemaname;
    catalog_name = delete_stmt->relation->catalogname;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracting columns from DELETE statement for table: %s.%s.%s",
                       catalog_name ? catalog_name : "",
                       schema_name ? schema_name : "",
                       table_name);

    /* Extract columns from WHERE clause */
    if (delete_stmt->whereClause != NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Extracting columns from DELETE WHERE clause");

        /* Use helper function to extract column references from WHERE clause expression */
        stmt_extract_columns_from_expression(delete_stmt->whereClause, columns, statement_data);

        /* Log extracted columns */
        if (*columns != NULL)
        {
            ListCell *lc;
            int col_count = 0;
            for (lc = (*columns)->head; lc != NULL; lc = lc->next)
            {
                TeccColumnRefData *tecc_col = (TeccColumnRefData *) lc->data.ptr_value;
                if (tecc_col != NULL)
                {
                    col_count++;
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "  Extracted column %d: %s (encrypted=%s)",
                                       col_count, tecc_col->alias_fqdn,
                                       tecc_col->is_encrypted ? "true" : "false");
                }
            }
            
        }
        else
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "No columns extracted from DELETE WHERE clause");
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DELETE statement has no WHERE clause");
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from CREATE TABLE statement
 */
static TeccStmtProcessorResult
stmt_extract_create_columns(const CreateStmt *create_stmt, List **columns)
{
    ListCell *lc;
    const char *table_name;
    const char *schema_name;
    const char *catalog_name;

    if (create_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    /* Extract table information */
    if (create_stmt->relation == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    table_name = create_stmt->relation->relname;
    schema_name = create_stmt->relation->schemaname;
    catalog_name = create_stmt->relation->catalogname;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from CREATE TABLE statement for table: %s.%s.%s",
                        catalog_name ? catalog_name : "",
                        schema_name ? schema_name : "",
                        table_name);

    /* Extract columns from table elements */
    if (create_stmt->tableElts != NIL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Processing table elements for CREATE TABLE");

        foreach(lc, create_stmt->tableElts)
        {
            Node *elem = (Node *) lfirst(lc);

            /* Check if this is a ColumnDef node */
            if (IsA(elem, ColumnDef))
            {
                ColumnDef *col_def = (ColumnDef *) elem;

                if (col_def->colname != NULL)
                {
                    TeccColumnRefData *tecc_col = tecc_column_ref_data_create();
                    if (tecc_col != NULL)
                    {
                        tecc_column_ref_data_init(tecc_col, catalog_name, schema_name, table_name, col_def->colname);
                        *columns = lappend(*columns, tecc_col);

                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Extracted column: %s.%s.%s.%s",
                                           catalog_name ? catalog_name : "",
                                           schema_name ? schema_name : "",
                                           table_name,
                                           col_def->colname);
                    }
                }
            }
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No table elements found in CREATE TABLE statement");
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/*
 * Context for column extraction walker callback.
 */
typedef struct ColumnExtractCtx
{
    List **columns;
    const TeccStatementData *statement_data;
} ColumnExtractCtx;

/* ReqID:TECC4.0 - Pre-visit callback: extract ColumnRef into columns list */
static bool
stmt_extract_columns_pre_visit(Node *node, void *context)
{
    ColumnExtractCtx *ctx = (ColumnExtractCtx *) context;

    if (nodeTag(node) == FE_T_ColumnRef)
    {
        ColumnRef *col_ref = (ColumnRef *) node;
        TeccColumnRefData *tecc_col = NULL;

        if (tecc_expand_column_ref_with_stmt_data(col_ref, &tecc_col,
                                                  ctx->statement_data))
        {
            if (tecc_col != NULL)
            {
                *ctx->columns = lappend(*ctx->columns, tecc_col);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Extracted column from expression: %s",
                                   tecc_col->alias_fqdn);
            }
        }
    }
    return true;
}

/*
 * Extract column references from expression using unified walker.
 */
static void
stmt_extract_columns_from_expression(const Node *expr, List **columns,
                                    const TeccStatementData *statement_data)
{
    ColumnExtractCtx ctx;
    TeccExprWalkerCallbacks callbacks;

    if (expr == NULL || columns == NULL)
        return;

    ctx.columns = columns;
    ctx.statement_data = statement_data;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.pre_visit = stmt_extract_columns_pre_visit;

    tecc_walk_expression_tree((Node *) expr, &callbacks, &ctx);
}

/*
 * Extract column references from COPY statement
 */
static TeccStmtProcessorResult
stmt_extract_copy_columns(const CopyStmt *copy_stmt, List **columns,
                          const TeccStatementData *statement_data)
{
    ListCell *lc;

    if (copy_stmt == NULL || columns == NULL)
        return TECC_STMT_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Extracting columns from COPY statement (is_from=%s)",
                        copy_stmt->is_from ? "true" : "false");

    /* 1. Extract columns from attlist (explicit column list) */
    if (copy_stmt->attlist != NULL) {
        for (lc = copy_stmt->attlist->head; lc != NULL; lc = lc->next) {
            Node *node = (Node *) lc->data.ptr_value;
            if (node != NULL && nodeTag(node) == FE_T_String) {
                char *col_name = (char *) node;

                /* Create column reference data */
                TeccColumnRefData *col_data = tecc_column_ref_data_create();
                if (col_data == NULL) {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "Failed to allocate memory for column reference");
                    continue;
                }

                /* Initialize with column name */
                tecc_column_ref_data_init(col_data,
                                         copy_stmt->relation ? copy_stmt->relation->relname : NULL,
                                         copy_stmt->relation ? copy_stmt->relation->schemaname : NULL,
                                         col_name,
                                         NULL);

                *columns = lappend(*columns, col_data);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Added COPY column reference: %s",
                                   col_name);
            }
        }
    }

    /* 2. If no explicit column list and we have a relation, all columns are implied */
    else if (copy_stmt->relation != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has no explicit column list - all columns implied");
        /* TODO: When cache module is available, fetch all columns from table */
    }

    /* 3. If COPY has a SELECT query, extract columns from the query */
    else if (copy_stmt->query != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "COPY has SELECT query - columns will be extracted from query");
        /* Extract columns from SELECT query */
        if (nodeTag(copy_stmt->query) == FE_T_SelectStmt)
        {
            SelectStmt *select_stmt = (SelectStmt *) copy_stmt->query;
            TeccStmtProcessorResult select_result;

            select_result = stmt_extract_select_columns(select_stmt, columns, statement_data);
            if (select_result != TECC_STMT_PROCESSOR_SUCCESS)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                                   "Failed to extract columns from COPY SELECT query: %d", select_result);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Successfully extracted columns from COPY SELECT query");
            }
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "COPY query is not a SELECT statement (type: %d)", nodeTag(copy_stmt->query));
        }
    }

    return TECC_STMT_PROCESSOR_SUCCESS;
}

/* ==================== Helper functions for metadata loading and checking ==================== */

/*
 * tecc_stmt_resolve_table_schema - 解析表引用的真实 schema
 *
 * 优先级（与 PG 未限定表名解析语义一致）：
 *   1. 显式限定名（schemaname != NULL）→ 直接用；
 *   2. 会话级临时表清单 → "pg_temp"（临时表优先于 search_path，普通表无加密列）；
 *   3. 服务器解析：按 search_path 查 pg_class（经 table_schema_cache，池连接）；
 *   4. fallback：current_schema → "public"（保证行为不劣于修复前）。
 *
 * 返回指针生命周期：限定名 → 入参指针；其余 → 缓存内指针或字符串字面量，
 * 均在当前语句处理期间有效（search_path 变更只发生在后续语句的同步中）。
 */
const char *
tecc_stmt_resolve_table_schema(const TeccStatementData *statement_data,
                               const char *schemaname,
                               const char *relname)
{
    TeccState *state;
    const char *db;
    const char *schema;

    /* 1. 显式限定名 */
    if (schemaname != NULL && schemaname[0] != '\0')
        return schemaname;

    if (relname == NULL || relname[0] == '\0')
        return "public";

    if (statement_data == NULL || statement_data->conn == NULL)
        return "public";

    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
        return "public";

    /* 2. 会话级临时表清单（PG 语义：pg_temp 先于 search_path） */
    if (tecc_state_is_temp_table(state, relname))
        return "pg_temp";

    /* 3. 服务器解析未限定表名（search_path 顺序，池连接查 pg_class） */
    db = PQdb(statement_data->conn);
    if (db != NULL)
    {
        schema = tecc_cache_get_table_schema(state, db, relname);
        if (schema != NULL && schema[0] != '\0')
            return schema;
    }

    /* 4. fallback：current_schema → public */
    schema = tecc_cache_get_guc_param(state, "current_schema");
    if (schema != NULL && schema[0] != '\0')
        return schema;
    return "public";
}

/*
 * Load table metadata from cache
 * This is a simplified implementation for task 12
 * Returns encrypted columns list via encrypted_columns_ptr and column_count_ptr
 * Caller is responsible for freeing the encrypted columns list
 */
static bool
stmt_load_table_metadata(const RangeVar *range_var, TeccStatementData *statement_data, TeccColumnCacheItem ***encrypted_columns_ptr, int *column_count_ptr)
{
    const char *table_name;
    const char *schema_name;
    TeccColumnCacheItem **encrypted_columns = NULL;
    int column_count = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccState *state = NULL;
    const char *database_name = NULL;

    if (range_var == NULL || range_var->relname == NULL)
        return false;

    table_name = range_var->relname;
    /* ReqID:TECC4.0 - 未限定表名不再硬编码 public：按 search_path 服务器解析
     * （临时表清单优先），与 PG 未限定表名解析语义一致 */
    schema_name = tecc_stmt_resolve_table_schema(statement_data,
                                                 range_var->schemaname,
                                                 range_var->relname);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Loading metadata for table: %s.%s (range_var=%p, relname=%p, schemaname=%p)",
                        schema_name, table_name, range_var, range_var->relname, range_var->schemaname);

    /* Get TECC state from statement data */
    if (statement_data != NULL && statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        database_name = PQdb(statement_data->conn);
        if (state == NULL) {
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Cannot get TECC state from connection, cache loading may fail");
        }
    } else {
        tecc_processor_debug(TECC_LOG_WARNING,
                            "Statement data or connection is NULL, cache loading may fail");
    }

    /* Query cache for all encrypted columns in this table */
    /* Use actual database name from connection; fallback to "postgres" if not available */
    encrypted_columns = tecc_cache_get_columns_by_table_name(state,
                                                            database_name ? database_name : "postgres",
                                                            schema_name,
                                                            table_name,
                                                            &column_count);

    /* ReqID:TECC4.0 - 缓存层在查询失败时已设置 state->last_error（TECC_ERR_DATA_FETCHER）。
     * 此处检测失败并上抛错误，避免加密表被误判为普通表导致明文泄漏。
     * 不再在主连接上直接 PQexec 兜底（原 fallback 绕过 data_fetcher 分层、
     * 缺 has_dek_privilege 权限字段、干扰主连接状态）。 */
    if ((encrypted_columns == NULL || column_count == 0) &&
        state != NULL && state->last_error.error_code == TECC_ERR_DATA_FETCHER)
    {
        tecc_processor_set_error(statement_data,
            TECC_PROCESSOR_ERROR,
            "failed to load encrypted column metadata for table \"%s.%s\": %s",
            schema_name, table_name,
            state->last_error.error_msg[0] != '\0' ? state->last_error.error_msg
                                                   : "data fetcher error");
        return false;
    }

    if (encrypted_columns != NULL && column_count > 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Found %d encrypted columns in table %s.%s",
                           column_count, schema_name, table_name);

        /* Return encrypted columns list to caller */
        if (encrypted_columns_ptr != NULL) {
            *encrypted_columns_ptr = encrypted_columns;
        }
        if (column_count_ptr != NULL) {
            *column_count_ptr = column_count;
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No encrypted columns found in table %s.%s",
                           schema_name, table_name);

        /* Return NULL/0 to caller */
        if (encrypted_columns_ptr != NULL) {
            *encrypted_columns_ptr = NULL;
        }
        if (column_count_ptr != NULL) {
            *column_count_ptr = 0;
        }

        /* Free memory if allocated */
        if (encrypted_columns != NULL) {
            pg_free(encrypted_columns);
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Finished loading metadata for table: %s.%s",
                        schema_name, table_name);

    return true;
}

/*
 * Check if a column is encrypted by querying the metadata cache
 */
static bool
stmt_check_column_encrypted(const TeccStatementData *statement_data, const char *schema_name, const char *table_name, const char *column_name)
{
    bool is_encrypted = false;

    if (table_name == NULL || column_name == NULL)
        return false;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Checking if column %s.%s is encrypted",
                        table_name, column_name);

    /* Query cache for encrypted column metadata */
    is_encrypted = stmt_query_metadata_cache_simple(statement_data, schema_name, table_name, column_name);

    if (is_encrypted) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Column %s.%s is encrypted",
                           table_name, column_name);
    } else {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Column %s.%s is not encrypted or not found in cache",
                           table_name, column_name);
    }

    return is_encrypted;
}

/*
 * Check if a column is in the encrypted columns list
 */
static bool
stmt_is_column_in_encrypted_list(const char *column_name, TeccColumnCacheItem **encrypted_columns, int column_count)
{
    if (column_name == NULL || encrypted_columns == NULL || column_count <= 0)
        return false;

    for (int i = 0; i < column_count; i++)
    {
        if (encrypted_columns[i] != NULL && encrypted_columns[i]->column_name != NULL)
        {
            if (strcmp(encrypted_columns[i]->column_name, column_name) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

/*
 * Query metadata cache for encrypted column metadata (simplified version)
 * This function queries the cache for a specific column in a table
 * Returns true if the column is encrypted, false otherwise
 */
static bool
stmt_query_metadata_cache_simple(const TeccStatementData *statement_data, const char *schema_name, const char *table_name, const char *column_name)
{
    TeccColumnCacheItem **encrypted_columns = NULL;
    int column_count = 0;
    bool is_encrypted = false;
    TeccState *state = NULL;
    const char *db_name = NULL;
    const char *lookup_schema = schema_name;

    if (table_name == NULL || column_name == NULL || statement_data == NULL)
        return false;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Querying metadata cache for column %s.%s",
                        table_name, column_name);

    /* Debug: Check statement_data and conn */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "statement_data=%p, statement_data->conn=%p",
                        statement_data, statement_data->conn);

    /* Get TeccState from PGconn */
    if (statement_data->conn != NULL) {
        state = tecc_get_state(statement_data->conn);
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Got TeccState from connection: %p", state);
    } else {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "statement_data->conn is NULL, cannot get TeccState");
    }

    if (state == NULL) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Cannot get TeccState from connection, cache query will fail");
        return false;
    }

    /* Get database name from connection */
    if (statement_data->conn != NULL) {
        db_name = PQdb(statement_data->conn);
        if (db_name == NULL || db_name[0] == '\0') {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Could not get database name from connection, using default 'postgres'");
            db_name = "postgres";
        }
    } else {
        db_name = "postgres";
    }

    /* 优先用调用方传入的 schema（限定表名如 sch1.tb1），
     * 未限定表名才回退 current_schema（search_path 首个模式） */
    lookup_schema = schema_name;
    if (lookup_schema == NULL || lookup_schema[0] == '\0') {
        lookup_schema = tecc_cache_get_guc_param(state, "current_schema");
        if (lookup_schema == NULL || lookup_schema[0] == '\0') {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Could not get current schema from cache manager, using default 'public'");
            lookup_schema = "public";
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Using database='%s', schema='%s', table='%s' for metadata query",
                        db_name, lookup_schema, table_name);

    /* Query cache for all encrypted columns in this table */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG LOOP: Calling tecc_cache_get_columns_by_table_name for %s.%s.%s",
                        db_name, lookup_schema, table_name);
    encrypted_columns = tecc_cache_get_columns_by_table_name(state,
                                                            db_name,
                                                            lookup_schema,
                                                            table_name,
                                                            &column_count);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG LOOP: tecc_cache_get_columns_by_table_name returned: encrypted_columns=%p, column_count=%d",
                        encrypted_columns, column_count);

    if (encrypted_columns != NULL && column_count > 0)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "DEBUG LOOP: Found %d encrypted columns, searching for column '%s'",
                           column_count, column_name);
        /* Search for the specific column */
        for (int i = 0; i < column_count; i++)
        {
            tecc_processor_debug(TECC_LOG_DEBUG2,
                               "DEBUG LOOP: Checking column[%d]: %s",
                               i, encrypted_columns[i] ? (encrypted_columns[i]->column_name ? encrypted_columns[i]->column_name : "(null)") : "(null)");
            if (encrypted_columns[i] != NULL &&
                encrypted_columns[i]->column_name != NULL &&
                strcmp(encrypted_columns[i]->column_name, column_name) == 0)
            {
                /* Found the column - it's encrypted */
                is_encrypted = true;
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Column %s.%s is encrypted (dek_id=%u, encryption_type=%d)",
                                   table_name, column_name,
                                   encrypted_columns[i]->dek_id,
                                   encrypted_columns[i]->encryption_type);
                break;
            }
        }

        /* Free the array (items are owned by cache) */
        pg_free(encrypted_columns);
    }

    return is_encrypted;
}
/* End - ReqID:TECC4.0 */

/*-------------------------------------------------------------------------
 *
 * hg_tecc_ddl.h
 *	  definition of the "HG TECC DDL recipe" system catalog (hg_tecc_ddl)
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/hg_tecc_ddl.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 * ReqID:TECC4.0 - 记录 KEK/DEK 的"可移植重建配方"：把每条 TECC 密钥语句的**已处理原文**
 * 按对象、按顺序存下来，使"原文"成为可查询的事实。用途见
 * contrib/citus/src/backend/distributed/commands/tecc_key_recipe.c
 * 的文件头注释（Citus 新增 worker 时补发既有密钥）。
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_TECC_DDL_H
#define HG_TECC_DDL_H

#include "catalog/genbki.h"
#include "catalog/hg_tecc_ddl_d.h"

/* ----------------
 *		hg_tecc_ddl definition.  cpp turns this into
 *		typedef struct FormData_hg_tecc_ddl
 * ----------------
 */
CATALOG(hg_tecc_ddl,11060,HgTeccDdlRelationId)
{
	Oid			oid;			/* oid */

	/*
	 * 对象所属的系统表 OID：HgKekRelationId（11000）或 HgDekRelationId（11010）。
	 * 用 BKI_LOOKUP(pg_class) 而非按名字判断，是为了让 genbki 在 initdb 阶段
	 * 就校验目标关系存在（与本 fork 其它 catalog 的引用方式一致）。
	 */
	Oid			object_class BKI_LOOKUP(pg_class) BKI_FORCE_NOT_NULL;

	/*
	 * 对象身份用**（命名空间, 名字）**而不是本节点 OID：OID 是节点本地的
	 * （CREATE KEK/DEK 在各节点各自 GetNewOidWithIndex），跨节点重放必须按名字。
	 * 这两列正好也是 hg_kek/hg_dek 自己唯一索引的列。身份由语句本身按创建时的
	 * 命名空间规则直接得出，不需要"建完再按 OID 回查"——后者既多余，又依赖
	 * 对象已可见 / 依赖 DeconstructQualifiedName 的返回值生命周期（它返回的是
	 * 指向语法树内部的指针，见 catalog/namespace.c:2858；曾因对它 pfree
	 * 造成 use-after-free）。
	 */
	Oid			key_namespace BKI_LOOKUP(pg_namespace) BKI_FORCE_NOT_NULL;

	NameData	object_name BKI_FORCE_NOT_NULL;

	/* 同一对象内的语句顺序，从 0 起；重放必须按此序 */
	int32		seq BKI_FORCE_NOT_NULL;

	/*
	 * 语句的已处理原文。要求同时满足两条才可存：
	 *   1) 按**名字**寻址（不含任何本节点 OID），否则换节点会打错对象；
	 *   2) 密钥材料已由前端注入（KEY_STORE/KEY_PATH/ENCRYPTED_VALUE/PUBLIC_KEY/
	 *      HE_* 等），这样各节点落下的是同一份材料，而不是各自再生成一份。
	 */
	text		command BKI_FORCE_NOT_NULL;
} FormData_hg_tecc_ddl;

/* ----------------
 *		Form_hg_tecc_ddl corresponds to a pointer to a tuple with
 *		the format of hg_tecc_ddl relation.
 * ----------------
 */
typedef FormData_hg_tecc_ddl *Form_hg_tecc_ddl;

DECLARE_UNIQUE_INDEX(hg_tecc_ddl_object_index, 11062, on hg_tecc_ddl using btree(object_class oid_ops, key_namespace oid_ops, object_name name_ops, seq int4_ops));
#define HgTeccDdlObjectIndexId 11062

DECLARE_UNIQUE_INDEX_PKEY(hg_tecc_ddl_oid_index, 11061, on hg_tecc_ddl using btree(oid oid_ops));
#define HgTeccDdlOidIndexId 11061

DECLARE_TOAST(hg_tecc_ddl, 11063, 11064);

#endif							/* HG_TECC_DDL_H */

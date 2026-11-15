/*-------------------------------------------------------------------------
 *
 * tecc_gen_bsgs_table.c
 *	  BSGS 静态表数据生成程序
 *
 * 直接 include tecc_ec_elgamal.c 复用核心算法的建表逻辑（TECC_STANDALONE_TEST
 * 纯 libc 编译），保证生成的表数据与运行时算法完全一致：算法参数（曲线/
 * capacity/素数）变化时，Makefile 依赖（tecc_ec_elgamal.c 更新 → 本程序
 * 重新编译 → 重新运行）自动触发重新生成，表数据始终与核心算法同源。
 *
 * 输出文件：
 *	  tecc_bsgs_tables_data.h — 参数指纹（曲线 NID + capacity）与 extern 声明
 *	  tecc_bsgs_tables_data.c — const 表数据数组（编译进 libpgcommon/libpq）
 *
 * 运行时 tecc_ec_bsgs_table_init 校验参数指纹：与静态数据一致则直接复用
 * （避免每个进程重新建表，HE 冷启动 0.6s → ~10ms）；不一致则回退运行时
 * 构建，保证正确性优先。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 *
 * src/common/tecc_algorithm/tecc_gen_bsgs_table.c
 *
 *-------------------------------------------------------------------------
 */
#define TECC_STANDALONE_TEST
#include <stdbool.h>	/* tecc_ec_elgamal.c 在 standalone 模式下使用 bool（postgres.h 提供） */
#include "tecc_ec_elgamal.c"

#include <stdio.h>

/* 生成程序仅复用建表逻辑（tecc_ec_bsgs_table_build_runtime），不链接
 * 静态表数据对象；提供空定义以满足 tecc_ec_bsgs_table_init 的 extern 引用。 */
const TeccECBSGSEntry tecc_bsgs_entries[TECC_EC_BSGS_TABLE_SIZE];

/* 输出头文件：参数指纹 + extern 声明 */
static void
write_header(FILE *fh, int nid)
{
	fprintf(fh, "/*-------------------------------------------------------------------------\n");
	fprintf(fh, " * 自动生成文件 - 勿手动修改。\n");
	fprintf(fh, " * 由 tecc_gen_bsgs_table 生成，直接复用 tecc_ec_elgamal.c 核心算法\n");
	fprintf(fh, " * 建表逻辑，与当前算法保持一致；算法参数变化时由 Makefile 依赖\n");
	fprintf(fh, " * 自动重新生成。\n");
	fprintf(fh, " *-------------------------------------------------------------------------\n");
	fprintf(fh, " */\n");
	fprintf(fh, "#ifndef TECC_BSGS_TABLES_DATA_H\n");
	fprintf(fh, "#define TECC_BSGS_TABLES_DATA_H\n\n");
	fprintf(fh, "#include \"common/tecc_algorithm/tecc_ec_elgamal.h\"\n\n");
	fprintf(fh, "/* 参数指纹：生成时取自核心算法的实际参数 */\n");
	fprintf(fh, "#define TECC_BSGS_DATA_GROUP_NID\t%d\n", nid);
	fprintf(fh, "#define TECC_BSGS_DATA_CAPACITY\t%u\n\n",
			(unsigned) TECC_EC_BSGS_TABLE_SIZE);
	fprintf(fh, "extern const TeccECBSGSEntry tecc_bsgs_entries[TECC_EC_BSGS_TABLE_SIZE];\n\n");
	fprintf(fh, "#endif /* TECC_BSGS_TABLES_DATA_H */\n");
}

/* 输出表数据数组（entries 已按 point_key 排序） */
static void
write_data(FILE *fc, const TeccECBSGSTable *table)
{
	uint32_t i;
	uint32_t j;

	fprintf(fc, "/*-------------------------------------------------------------------------\n");
	fprintf(fc, " * 自动生成文件 - 勿手动修改。\n");
	fprintf(fc, " * 由 tecc_gen_bsgs_table 生成，与 tecc_ec_elgamal.c 算法同源。\n");
	fprintf(fc, " *-------------------------------------------------------------------------\n");
	fprintf(fc, " */\n");
	fprintf(fc, "#include \"tecc_bsgs_tables_data.h\"\n\n");
	fprintf(fc, "const TeccECBSGSEntry tecc_bsgs_entries[TECC_EC_BSGS_TABLE_SIZE] = {\n");

	for (i = 0; i < table->size; i++)
	{
		fprintf(fc, "\t{ {");
		for (j = 0; j < TECC_EC_POINT_SIZE; j++)
			fprintf(fc, "0x%02x%s", table->entries[i].point_key[j],
					(j < TECC_EC_POINT_SIZE - 1) ? "," : "");
		fprintf(fc, "}, %u },\n", (unsigned) table->entries[i].value);
	}

	fprintf(fc, "};\n");
}

int
main(void)
{
	EC_GROUP	   *group;
	TeccECBSGSTable table;
	FILE		   *fh = NULL;
	FILE		   *fc = NULL;
	int				nid;

	group = tecc_ec_elgamal_create_group();
	if (group == NULL)
	{
		fprintf(stderr, "TECC: failed to create EC group\n");
		return 1;
	}
	nid = EC_GROUP_get_curve_name(group);

	/* 强制运行时构建（生成程序无静态数据可复用），与运行时 build_runtime 同源 */
	if (tecc_ec_bsgs_table_build_runtime(&table, group,
										 TECC_EC_BSGS_TABLE_SIZE) != 0)
	{
		fprintf(stderr, "TECC: failed to build BSGS table\n");
		EC_GROUP_free(group);
		return 1;
	}

	fh = fopen("tecc_bsgs_tables_data.h", "w");
	fc = fopen("tecc_bsgs_tables_data.c", "w");
	if (fh == NULL || fc == NULL)
	{
		fprintf(stderr, "TECC: failed to open output files\n");
		if (fh) fclose(fh);
		if (fc) fclose(fc);
		tecc_ec_bsgs_table_free(&table);
		EC_GROUP_free(group);
		return 1;
	}

	write_header(fh, nid);
	write_data(fc, &table);

	fclose(fh);
	fclose(fc);

	tecc_ec_bsgs_table_free(&table);
	EC_GROUP_free(group);
	return 0;
}

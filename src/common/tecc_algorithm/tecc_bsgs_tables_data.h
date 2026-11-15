/*-------------------------------------------------------------------------
 * 自动生成文件 - 勿手动修改。
 * 由 tecc_gen_bsgs_table 生成，直接复用 tecc_ec_elgamal.c 核心算法
 * 建表逻辑，与当前算法保持一致；算法参数变化时由 Makefile 依赖
 * 自动重新生成。
 *-------------------------------------------------------------------------
 */
#ifndef TECC_BSGS_TABLES_DATA_H
#define TECC_BSGS_TABLES_DATA_H

#include "common/tecc_algorithm/tecc_ec_elgamal.h"

/* 参数指纹：生成时取自核心算法的实际参数 */
#define TECC_BSGS_DATA_GROUP_NID	415
#define TECC_BSGS_DATA_CAPACITY	16384

extern const TeccECBSGSEntry tecc_bsgs_entries[TECC_EC_BSGS_TABLE_SIZE];

#endif /* TECC_BSGS_TABLES_DATA_H */

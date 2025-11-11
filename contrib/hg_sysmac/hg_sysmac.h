/*-------------------------------------------------------------------------
 *
 * hg_sysmac.h
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/hg_sysmac.h
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#ifndef HG_SYSMAC_H
#define HG_SYSMAC_H


#include "catalog/objectaddress.h"

#define SYSMAC_LABEL_TAG	"sysmac"

typedef struct seclabel_data
{
	int32       sensitivity;
	Bitmapset  *catagory;
}SecLabel;

/*
 * The possible result when comparing two seclabels.
 */
typedef enum
{
	LABEL_UNCOMPARABLE = 0,
	LABEL_EQUAL,
	LABEL_DOMINATE
}LABEL_Comparison;

typedef struct hg_sysmac_param
{
	bool global_on;
	bool global_table_level;
	bool global_column_level;
	bool global_row_level;
}hg_sysmac_param;

extern hg_sysmac_param *hg_sysmac_g;

/* Begin - bug#z5940 */
extern bool global_on;
extern bool global_table_level;
extern bool global_column_level;
extern bool global_row_level;
/* End - bug#z5940 */


/* label.c */
extern LABEL_Comparison seclabel_compare(SecLabel *label_l, SecLabel *label_r);
extern SecLabel* string_to_seclabel(const char *str_label, bool *format_result, bool do_convert);
extern char* seclabel_to_string(SecLabel *seclabel);
extern void check_relabel(const ObjectAddress *object, const char *seclabel);
extern char* get_obj_seclabel_internal(Oid classId, Oid objectId, int32 subId, bool missing_is_ok);
extern void set_obj_seclabel_internal(Oid classId, Oid objectId, int32 subId, const char* label, bool missing_is_ok);

/* dml.c */
extern bool sysmac_dml_privileges(List *rangeTabls, bool abort_on_violation);

/* hooks.c */
extern bool global_on;
extern bool global_table_level;
extern bool global_column_level;
extern bool global_row_level;

extern bool is_security_roles(void);

extern SecLabel* get_seclabel_from_bytea(bytea *input);

#define SEN_LEN (offsetof(SecLabel, catagory))
#define CAT_LEN(nwords) (offsetof(Bitmapset, words) + (nwords) * sizeof(bitmapword))
#define PG_GETARG_SECLABEL(n) ((SecLabel*)get_seclabel_from_bytea(PG_GETARG_BYTEA_PP(n)))

#endif

/* End - ReqID: SRS-SECURITY-LMAC */

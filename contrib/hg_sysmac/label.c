/*-------------------------------------------------------------------------
 *
 * label.c
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/label.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/relation.h"
#include "access/table.h"
#include "catalog/pg_authid.h"
#include "catalog/catalog.h"
#include "catalog/indexing.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "commands/seclabel.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/lsyscache.h"

#include "hg_sysmac.h"


/* 
 * For PG_GETARG_SECLABEL macro definition
 */
SecLabel*
get_seclabel_from_bytea(bytea *input)
{
	SecLabel   *seclabel = (SecLabel *)palloc(sizeof(SecLabel));

	seclabel->sensitivity = *((int32 *)VARDATA_ANY(input));
	if (VARSIZE_ANY_EXHDR(input) > SEN_LEN)
		seclabel->catagory = (Bitmapset *)(VARDATA_ANY(input) + SEN_LEN);
	else
		seclabel->catagory = NULL;

	return seclabel;
}

/*
 * Convert a string to the `SecLabel` data type
 */
SecLabel*
string_to_seclabel(const char *str_label, bool *format_result, bool do_convert)
{
	char	*p;				/*the cursor pointer move from beginning to end of input string*/
	char	*seclabel_p;	/*a seclabel part pointer for each elements in the input */
	char	*label_in_copy;	/*the copy of input*/
	char	*left_part_p;	/*the left part of a continuous catagory representation*/
	char 	*num_p;			/*the number in each elements*/
	char	delim;			/*the delimiter*/
	int 	sens;			/*the sensitivity*/
	int		lower_bit;		/*the number of a single catagory or the lower number of a continous catagory*/
	int		upper_bit;		/*the higher number of a continous catagory*/
	int		i;
	SecLabel *seclabel_out = NULL;
	bool	format_ok = true;

	if (do_convert)
	{
		seclabel_out = (SecLabel *)palloc(sizeof(SecLabel));
		seclabel_out->sensitivity = 0;
		seclabel_out->catagory = NULL;
	}

	/*becaseu the function will modify the origin input, so make a copy here*/
	label_in_copy = pstrdup(str_label);
	p = seclabel_p = label_in_copy;

	/*get the sensitivity part and check format*/
	while (*p && *p != ':')
	{
		p++;
	}

	delim = *p;
	if (delim != 0)
	{
		*p++ = 0;/*replace ':' with '\0'*/
	}

	if (*seclabel_p != 's')
	{
		format_ok = false;
		goto err;
	}

	if (*(seclabel_p + 1) == '\0')
	{
		format_ok = false;
		goto err;
	}

	for (num_p = seclabel_p + 1 ; *num_p != 0; num_p++)
	{
		if (!isdigit(*num_p))
		{
			format_ok = false;
			goto err;
		}
	}

	sens = atoi(seclabel_p + 1);

	if (sens < 0|| sens > 15)
	{
		format_ok = false;
		goto err;
	}

	if (do_convert)/*if do convert, assign the sensitivity to output*/
	{
		seclabel_out->sensitivity = sens;
	}

	/*get the catagory part and check format*/
	if (delim == ':')
	{
		while(1)
		{
			seclabel_p = p;

			while (*p && *p != ',')
			{
				p++;
			}

			delim = *p;

			if (delim != 0)
			{
				*p++ = 0;/*replace ',' with '\0'*/
			}

			if ((left_part_p = strchr(seclabel_p,'.')) != NULL)/*check whewher the catagory is continuous*/
			{
				*left_part_p++ = 0;/*replace '.' with '\0'*/
			}

			if (*seclabel_p != 'c')
			{
				format_ok = false;
				goto err;
			}

			if (*(seclabel_p + 1) == '\0')
			{
				format_ok = false;
				goto err;
			}

			for (num_p = seclabel_p + 1 ; *num_p != 0; num_p++)
			{
				if (!isdigit(*num_p))
				{
					format_ok = false;
					goto err;
				}
			}

			lower_bit = atoi(seclabel_p + 1);

			if (lower_bit < 0|| lower_bit > 1023)
			{
				format_ok = false;
				goto err;
			}

			if (do_convert)
			{
				seclabel_out->catagory = bms_add_member(seclabel_out->catagory, lower_bit);
			}

			if (left_part_p)/*if the catagory is continous, set every bit from low to high*/
			{
				if (*left_part_p != 'c')
				{
					format_ok = false;
					goto err;
				}

				if (*(left_part_p + 1) == '\0')
				{
					format_ok = false;
					goto err;
				}

				for (num_p = left_part_p + 1 ; *num_p != 0; num_p++)
				{
					if (!isdigit(*num_p))
					{
						format_ok = false;
						goto err;
					}
				}

				upper_bit = atoi(left_part_p+1);

				if (upper_bit < 0|| upper_bit > 1023||upper_bit < lower_bit)
				{
					format_ok = false;
					goto err;
				}

				if (do_convert)
				{
					for (i = lower_bit + 1; i <= upper_bit; i++)
					{
						seclabel_out->catagory = bms_add_member(seclabel_out->catagory, i);
					}
				}
			}

			if (delim != ',')
			{
				break;
			}
		}
	}

err:

	if (format_result)
		*format_result = format_ok;
	pfree(label_in_copy);
	return seclabel_out;
}

static unsigned int
seclabel_string_len(SecLabel *seclabel)
{
	unsigned int len = 0;
	char temp[16] = {0};
	int i,p;
	bool in_range = false;

	if (!seclabel)
		return 0;

	len += snprintf(temp, sizeof(temp), "s%d", seclabel->sensitivity);

	if (seclabel->catagory)
	{
		p = -1;
		i = bms_next_member(seclabel->catagory, p);
		len += snprintf(temp, sizeof(temp), ":c%d", i);
		p = i;

		while ((i = bms_next_member(seclabel->catagory, p)) > 0)
		{
			if (i - p == 1)
			{
				in_range = true;
			}
			else if (i - p > 1)
			{
				if (in_range)
				{
					len += snprintf(temp, sizeof(temp), ".c%d", p);
					len += snprintf(temp, sizeof(temp), ",c%d", i);
				}
				else
				{
					len += snprintf(temp, sizeof(temp), ",c%d", i);
				}
				in_range = false;
			}

			p = i;
		}

		/* if we end with in_range we set previous valid bit as end*/
		if (in_range)
		{
			len += snprintf(temp, sizeof(temp), ".c%d", p); /* Bug - #z6007 */
		}
	}

	return len;
}

/*
 * Convert the `SecLabel` data to a string
 */
char*
seclabel_to_string(SecLabel *seclabel)
{
	unsigned int len = seclabel_string_len(seclabel);
	char *result = NULL;
	char *res_p = NULL;
	int i,p;
	bool in_range = false;
	char delim;

	if (len == 0)
		return NULL;

	result = (char *)palloc(len + 1);
	res_p = result;
	res_p += sprintf(res_p, "s%d", seclabel->sensitivity);

	if (seclabel->catagory)
	{
		p = -1;
		i = bms_next_member(seclabel->catagory, p);
		res_p += sprintf(res_p, ":c%d", i);
		p = i;

		while ((i = bms_next_member(seclabel->catagory, p)) > 0)
		{
			if (i - p == 1)
			{
				in_range = true;
			}
			else if (i - p > 1)
			{
				if (in_range)
				{
					res_p += sprintf(res_p, ".c%d", p);
					if (i - p > 2)
						delim = '.';
					else
						delim = ',';
					res_p += sprintf(res_p, "%cc%d", delim, i);
				}
				else
				{
					res_p += sprintf(res_p, ",c%d", i);
				}
				in_range = false;
			}

			p = i;
		}

		/* if we end with in_range we set previous valid bit as end*/
		if (in_range)
		{
			res_p += sprintf(res_p, ".c%d", p);
		}
	}

	*(result + len) = 0;
	return result;
}

/*
 * Comparing seclabel data
 */
LABEL_Comparison
seclabel_compare(SecLabel *label_l, SecLabel *label_r)
{
	BMS_Comparison 		bitmap_result;	/*result of two label's catagory comparison*/
	LABEL_Comparison	result;			/*result of two label comparison*/

	result = LABEL_UNCOMPARABLE;

	if (label_l == NULL)
	{
		if (label_r == NULL)
			result = LABEL_EQUAL;
		else
			result = LABEL_UNCOMPARABLE;
	}
	else if (label_r == NULL)
	{
		result = LABEL_DOMINATE;
	}
	else
	{
		bitmap_result = bms_subset_compare(label_l->catagory, label_r->catagory);

		/*if leftside label's sensitivity bigger than rightside one's*/
		if (label_l->sensitivity > label_r->sensitivity)
		{
			/*if leftside label's catagory is the superset of rightside one's return dominate*/
			if (bitmap_result == BMS_SUBSET2 || bitmap_result == BMS_EQUAL)
			{
				result = LABEL_DOMINATE;
			}
		}
		else
		{
			/*if leftside label's sensitivity equals to rightside one's*/
			if (label_l->sensitivity == label_r->sensitivity)
			{
				/*if leftside label's catagory is equal rightside one's return equal*/
				if (bitmap_result == BMS_EQUAL)
				{
					result = LABEL_EQUAL;
				}
				/*if leftside label's catagory is superset of rightside one's return equal*/
				if (bitmap_result == BMS_SUBSET2)
				{
					result = LABEL_DOMINATE;
				}
			}
		}
	}

	return result;
}

static void
seclabel_format_check(const char* seclabel)
{
	bool format_ok;

	SecLabel *label = string_to_seclabel(seclabel, &format_ok, false);
	if (label)
		pfree(label);

	if (!format_ok)
		elog(ERROR,"wrong format of seclabel in \'%s\'", seclabel);
}

bool skip_format_check = false; // guc latter ?
void
check_relabel(const ObjectAddress *object, const char *seclabel)
{
#ifdef HAS_SEPOFPOWER
	if (!is_securedump)
		elog(ERROR, "only sso can set sysmac label");
#endif

	if (global_on && !skip_format_check) /* Bug - #z5940 */
		seclabel_format_check(seclabel);
}

/*
 * Get security labels from the pg_shseclabel or pg_seclabel
 */
char *
get_obj_seclabel_internal(Oid classId, Oid objectId, int32 subId, bool missing_is_ok)
{
	ObjectAddress object;
	char	   *label = NULL;
	char *object_name = NULL;

	object.classId = classId;
	object.objectId = objectId;
	object.objectSubId = subId;

	object_name = getObjectIdentity(&object, false);

	/*just for safe, we error out before we reach here*/
	if (!object_name)
		elog(ERROR, "Object not exist");

	label = GetSecurityLabel(&object, SYSMAC_LABEL_TAG);

	if (!label && !missing_is_ok)
		elog(ERROR, "label for object '%d' dose not exist", objectId);

	pfree(object_name);
	return label;
}

/*
 * Set security labels into the pg_shseclabel or pg_seclabel
 */
void
set_obj_seclabel_internal(Oid classId, Oid objectId, int32 subId,
						  const char* label, bool missing_is_ok)
{
	ObjectAddress object;
	char *object_name = NULL;

	object.classId = classId;
	object.objectId = objectId;
	object.objectSubId = subId;

	object_name = getObjectIdentity(&object, false);

	/*just for safe, we error out before we reach here*/
	if (!object_name)
		elog(ERROR, "Object not exist");

	seclabel_format_check(label);

	SetSecurityLabel(&object, SYSMAC_LABEL_TAG, label);

	pfree(object_name);
}

PG_FUNCTION_INFO_V1(get_user_seclabel);
Datum
get_user_seclabel(PG_FUNCTION_ARGS)
{
	Oid role_oid = PG_GETARG_OID(0);
	char *label = NULL;

	is_security_roles();

	label = get_obj_seclabel_internal(AuthIdRelationId, role_oid, InvalidOid, true);

	if (label)
		PG_RETURN_TEXT_P(cstring_to_text(label));
	else /* Bug - #z6002 */
		elog(ERROR, "This user does not have a seclabel.");
}

PG_FUNCTION_INFO_V1(set_user_seclabel);
Datum
set_user_seclabel(PG_FUNCTION_ARGS)
{
	Oid role_oid = PG_GETARG_OID(0);
	const char* label = TextDatumGetCString(PG_GETARG_DATUM(1));

	is_security_roles();

	set_obj_seclabel_internal(AuthIdRelationId, role_oid, InvalidOid, label, false);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}

PG_FUNCTION_INFO_V1(get_table_seclabel);
Datum
get_table_seclabel(PG_FUNCTION_ARGS)
{
	Oid rel_oid = PG_GETARG_OID(0);
	char *label = NULL;

	is_security_roles();

	label = get_obj_seclabel_internal(RelationRelationId, rel_oid, InvalidOid, true);

	if (label)
		PG_RETURN_TEXT_P(cstring_to_text(label));
	else /* Bug - #6002 */
		elog(ERROR, "This table does not have a seclabel.");
}

PG_FUNCTION_INFO_V1(set_table_seclabel);
Datum
set_table_seclabel(PG_FUNCTION_ARGS)
{
	char		relkind;
	char		relpersistence;
	Oid rel_oid = PG_GETARG_OID(0);
	const char* label = TextDatumGetCString(PG_GETARG_DATUM(1));

	is_security_roles();

	/* Begin - bug#z6002 */
	relkind = get_rel_relkind(rel_oid);
	relpersistence = get_rel_persistence(rel_oid);

	if (rel_oid < FirstNormalObjectId)
		elog(ERROR, "Setting seclabel for system objects is not supported.");

	if (relpersistence == 't')
		elog(ERROR, "Setting table seclabel for temp table is not supported.");

	switch (relkind)
	{
		case 'i':
			elog(ERROR, "Setting table seclabel for index is not supported.");
			break;

		case 'c':
			elog(ERROR, "Setting table seclabel for composite type is not supported.");
			break;

		case 'f':
			elog(ERROR, "Setting table seclabel for foreign table is not supported.");
			break;

		case 'I':
			elog(ERROR, "Setting table seclabel for partitioned index is not supported.");
			break;

		default:
			break;
	}
	/* End - bug#z6002 */

	set_obj_seclabel_internal(RelationRelationId, rel_oid, InvalidOid, label, false);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}

PG_FUNCTION_INFO_V1(get_column_seclabel);
Datum
get_column_seclabel(PG_FUNCTION_ARGS)
{
	Oid rel_oid = PG_GETARG_OID(0);
	int32  col_num = PG_GETARG_OID(1);
	char *label = NULL;

	is_security_roles();

	label = get_obj_seclabel_internal(RelationRelationId, rel_oid, col_num, true);

	if (label)
		PG_RETURN_TEXT_P(cstring_to_text(label));
	else /* Bug - #z6002 */
		elog(ERROR, "This column does not have a seclabel.");
}

PG_FUNCTION_INFO_V1(set_column_seclabel);
Datum
set_column_seclabel(PG_FUNCTION_ARGS)
{
	char		relkind;
	char		relpersistence;
	Oid rel_oid = PG_GETARG_OID(0);
	int32 col_num = PG_GETARG_OID(1);
	const char* label = TextDatumGetCString(PG_GETARG_DATUM(2));

	is_security_roles();

	/* Begin - bug#z6038 */
	if (col_num <= 0)
		elog(ERROR, "Invalid column number '%d', must be greater than 0.", col_num);
	/* End - bug#z6038 */

	/* Begin - bug#z6002 */
	relkind = get_rel_relkind(rel_oid);
	relpersistence = get_rel_persistence(rel_oid);

	if (rel_oid < FirstNormalObjectId)
		elog(ERROR, "Setting seclabel for system objects is not supported.");

	if (relpersistence == 't')
		elog(ERROR, "Setting column seclabel for temp table is not supported.");

	switch (relkind)
	{
		case 'i':
			elog(ERROR, "Setting column seclabel for index is not supported.");
			break;

		case 'c':
			elog(ERROR, "Setting column seclabel for composite type is not supported.");
			break;

		case 'f':
			elog(ERROR, "Setting column seclabel for foreign table is not supported.");
			break;

		case 'S':
			elog(ERROR, "Setting column seclabel for sequence is not supported.");
			break;

		case 'I':
			elog(ERROR, "Setting column seclabel for partitioned index is not supported.");
			break;

		default:
			break;
	}
	/* End - bug#z6002 */

	set_obj_seclabel_internal(RelationRelationId, rel_oid, col_num, label, false);

	PG_RETURN_TEXT_P(cstring_to_text("set"));
}


/* type functions */

PG_FUNCTION_INFO_V1(seclabel_in);
Datum
seclabel_in(PG_FUNCTION_ARGS)
{
	char	   *input = PG_GETARG_CSTRING(0);
	SecLabel   *label_in;
	bytea	   *result;
	size_t		len, sen_len, cat_len;
	bool		format_ok;

	label_in = string_to_seclabel(input, &format_ok, true);

	if (!format_ok)
		elog(ERROR, "wrong format of seclable in \'%s\'",input);

	/*set to varlen storage*/
	sen_len = SEN_LEN;
	if (label_in->catagory)
		cat_len = CAT_LEN(label_in->catagory->nwords);
	else
		cat_len = 0;

	len = sen_len + cat_len;
	result = (bytea *)palloc0(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), &label_in->sensitivity, sen_len);
	if (cat_len)
		memcpy(VARDATA(result) + sen_len, label_in->catagory, cat_len);

	pfree(label_in);

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(seclabel_out);
Datum
seclabel_out(PG_FUNCTION_ARGS)
{
	SecLabel   *label_in = PG_GETARG_SECLABEL(0);
	char 	   *result = seclabel_to_string(label_in);

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(seclabel_eq);
Datum
seclabel_eq(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_l, seclabel_r) == LABEL_EQUAL);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_ne);
Datum
seclabel_ne(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_l, seclabel_r) != LABEL_EQUAL);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_gt);
Datum
seclabel_gt(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_l, seclabel_r) == LABEL_DOMINATE);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_lt);
Datum
seclabel_lt(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_r, seclabel_l) == LABEL_DOMINATE);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_ge);
Datum
seclabel_ge(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_l, seclabel_r) != LABEL_UNCOMPARABLE);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_le);
Datum
seclabel_le(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	bool result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = (seclabel_compare(seclabel_r, seclabel_l) != LABEL_UNCOMPARABLE);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(seclabel_cmp);
Datum
seclabel_cmp(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l;
	SecLabel *seclabel_r;
	int result;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
		seclabel_l = PG_GETARG_SECLABEL(0);

	if (PG_ARGISNULL(1))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(1);

	result = seclabel_compare(seclabel_l, seclabel_r);

	PG_RETURN_INT32(result);
}

PG_FUNCTION_INFO_V1(unregister_seclabel_provider);
Datum
unregister_seclabel_provider(PG_FUNCTION_ARGS)
{
	const char* provider = TextDatumGetCString(PG_GETARG_DATUM(0));

	unregister_label_provider(provider);

	PG_RETURN_VOID();
}

/* End - ReqID: SRS-SECURITY-LMAC */

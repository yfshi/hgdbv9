/*-------------------------------------------------------------------------
 *
 * encrypted_column.h
 *	  Definitions for encrypted column processing
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/encrypted_column.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ENCRYPTED_COLUMN_H
#define ENCRYPTED_COLUMN_H

#include "postgres.h"
#include "nodes/parsenodes.h"
#include "catalog/pg_type.h"
#include "nodes/execnodes.h"

/* Encryption type values */
#define ENCRYPTION_TYPE_DET		"det"
#define ENCRYPTION_TYPE_OPE		"ope"
#define ENCRYPTION_TYPE_SEARCH	"search"

/* Encryption type bitmask values (compatible with hg_encrypted_columns) */
#define HG_ENCRYPTION_TYPE_DET		0x01
#define HG_ENCRYPTION_TYPE_OPE		0x02
#define HG_ENCRYPTION_TYPE_SEARCH	0x04
/* ReqID:TECC4.0 - Homomorphic encryption type flags */
#define HG_ENCRYPTION_TYPE_HE_ADD	0x08
#define HG_ENCRYPTION_TYPE_HE_MUL	0x10

/* Default encryption type */
#define DEFAULT_ENCRYPTION_TYPE	""

/* Encrypted data type OID range */
#define FIRST_ENCRYPTED_TYPE_OID	50000
#define LAST_ENCRYPTED_TYPE_OID		50008

/* Function result codes */
typedef enum
{
	HG_METADATA_SUCCESS = 0,
	HG_METADATA_ERROR = -1,
	HG_METADATA_NOT_FOUND = -2,
	HG_METADATA_PERMISSION_DENIED = -3,
	HG_METADATA_INVALID_PARAM = -4,
	HG_METADATA_CONSTRAINT_VIOLATION = -5
} HgMetadataResult;

/* Structure to hold parsed encrypted column parameters */
typedef struct EncryptedColumnParams
{
	char	   *column_encryption_key;	/* DEK name (required) */
	char	   *encryption_type;		/* Encryption type: det, ope, search */
	uint32		encryption_type_mask;	/* Bitmask representation */
	/* ReqID:TECC4.0 - 一列多 DEK：he-add/he-mul 各自的同态 DEK 名（可选） */
	char	   *he_add_dek;				/* he-add DEK name, or NULL */
	char	   *he_mul_dek;				/* he-mul DEK name, or NULL */
} EncryptedColumnParams;

/* Encrypted column creation parameters (for hg_encrypted_columns system table) */
typedef struct HgEncryptedColumnCreateParams
{
	Oid			table_oid;		/* Table OID */
	int16		column_attnum;	/* Column attribute number */
	const char *column_name;	/* Column name */
	Oid			data_type;		/* Base data type OID */
	Oid			column_key_id;	/* DEK OID used */
	bool		enable_det;		/* Enable DET encryption (for backward compatibility) */
	bool		enable_ope;		/* Enable OPE encryption (for backward compatibility) */
	bool		enable_search;	/* Enable SEARCH encryption (for backward compatibility) */
	uint32		encryption_type; /* Encryption type bitmask (new field) */
	int32		type_mod;		/* Type modifier (precision/scale for NUMERIC_ENC) */
	Oid			collation_oid;	/* ReqID:TECC4.0 - Collation OID for ICU sort key */
	/* ReqID:TECC4.0 - 一列多 DEK（P0-2）：he-add/he-mul 同态 DEK 的 OID */
	Oid			he_add_dek_id;	/* he-add DEK OID（InvalidOid=无） */
	Oid			he_mul_dek_id;	/* he-mul DEK OID（InvalidOid=无） */
} HgEncryptedColumnCreateParams;

/* Function declarations */

/* Encrypted column parameter parsing and validation */
extern EncryptedColumnParams *parse_encrypted_with_options(List *options);
extern bool validate_encrypted_column_params(EncryptedColumnParams *params, Oid column_type);
extern uint32 calculate_encryption_type_mask(const char *encryption_type_str);
extern bool is_encrypted_data_type(Oid type_oid);
extern Oid tecc_resolve_collation_oid(Oid collation_oid);
extern void tecc_validate_collation_for_encrypted_column(Oid collation_oid, uint32 enc_type_mask);

/* Metadata management functions */
extern HgMetadataResult hg_create_encrypted_column(const HgEncryptedColumnCreateParams *params);
extern bool hg_validate_encrypted_column_params(const HgEncryptedColumnCreateParams *params);
extern Oid record_encrypted_column_metadata(Oid table_oid, int16 attnum,
										   const char *column_name, Oid column_type,
										   int32 type_mod, Oid collation_oid,
										   EncryptedColumnParams *params);
extern const char *hg_metadata_result_to_string(HgMetadataResult result);

/* Process encrypted column during CREATE TABLE */
extern void process_encrypted_column_during_create(Oid table_oid, const char *table_name,
												  ColumnDef *column);

/* ReqID:TECC4.0 - CTAS encryption metadata propagation */
extern bool hg_get_encrypted_column_info(Oid table_oid, int16 attnum,
										 Oid *dek_oid, uint32 *encryption_type_mask);

/* ReqID:TECC4.0 - Rebuild ENCRYPTED WITH clause text for one column */
extern char *hg_get_encrypted_column_clause(Oid table_oid, AttrNumber attnum,
											bool *has_ope);
/* ReqID:TECC4.0 - Rebuild ENCRYPTED WITH options as parse-tree DefElems */
extern List *hg_get_encrypted_column_options(Oid table_oid, AttrNumber attnum);
/*
 * ReqID:TECC4.0 - 按名字返回 ENCRYPTED WITH 子句文本（malloc，调用方 free）。
 * 供 libpq 的 DDL-only 通路以弱符号方式同进程调用，取不到返回 NULL，绝不 ereport。
 */
extern char *hg_get_encrypted_clause_by_name(const char *schema, const char *table,
											 const char *column);
extern void populate_ctas_encryption_metadata(Oid new_table_oid, List *attrList,
											  List *source_tlist, List *rtable,
											  List *parsed_tlist, List *parsed_rtable);

/* ReqID:TECC4.0 - VIEW encryption metadata propagation */
extern void populate_view_encryption_metadata(Oid view_oid, List *targetList,
											   List *rtable);

/* ReqID:TECC4.0 - Encrypted slot validation (HE slot completeness) */
extern void tecc_validate_encrypted_slots(ResultRelInfo *resultRelInfo,
										  TupleTableSlot *slot);

/* ReqID:TECC4.0 - Fail-closed reaction when an encrypted-typed column has no
 * metadata row (see hg_tecc.strict_metadata).  `consequence` is a sentence
 * describing what the caller was about to do by treating the column as a
 * plain one. */
extern void tecc_report_missing_column_metadata(Oid table_oid, AttrNumber attnum,
												Oid column_type,
												const char *consequence);

/* ReqID:TECC4.0 - PARTITION OF: copy every encrypted column's metadata (DEK,
 * algorithm mask, collation, type modifier and the he_add_dek/he_mul_dek
 * bindings) from the partitioned parent onto the new partition.  Must run
 * BEFORE the partition's indexes are created, so that
 * check_index_support_for_encrypted_column() sees the column's algorithms.
 * Returns the number of columns copied. */
extern int	hg_inherit_encrypted_columns(Oid parent_oid, Oid child_oid);
extern void hg_process_partition_encrypted_columns(Oid parent_oid, Oid child_oid);

#endif /* ENCRYPTED_COLUMN_H */
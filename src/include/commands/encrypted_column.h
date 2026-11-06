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

/* Encryption type values */
#define ENCRYPTION_TYPE_DET		"det"
#define ENCRYPTION_TYPE_OPE		"ope"
#define ENCRYPTION_TYPE_SEARCH	"search"

/* Encryption type bitmask values (compatible with hg_encrypted_columns) */
#define HG_ENCRYPTION_TYPE_DET		0x01
#define HG_ENCRYPTION_TYPE_OPE		0x02
#define HG_ENCRYPTION_TYPE_SEARCH	0x04

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
} HgEncryptedColumnCreateParams;

/* Function declarations */

/* Encrypted column parameter parsing and validation */
extern EncryptedColumnParams *parse_encrypted_with_options(List *options);
extern bool validate_encrypted_column_params(EncryptedColumnParams *params, Oid column_type);
extern uint32 calculate_encryption_type_mask(const char *encryption_type_str);
extern bool is_encrypted_data_type(Oid type_oid);
extern Oid tecc_resolve_collation_oid(Oid collation_oid);

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
extern void populate_ctas_encryption_metadata(Oid new_table_oid, List *attrList,
											  List *source_tlist, List *rtable);

/* ReqID:TECC4.0 - CTAS encryption metadata propagation */
extern bool hg_get_encrypted_column_info(Oid table_oid, int16 attnum,
										 Oid *dek_oid, uint32 *encryption_type_mask);
extern void populate_ctas_encryption_metadata(Oid new_table_oid, List *attrList,
											  List *source_tlist, List *rtable);

#endif /* ENCRYPTED_COLUMN_H */
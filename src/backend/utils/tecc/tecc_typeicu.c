/*-------------------------------------------------------------------------
 *
 * tecc_typeicu.c
 *	  ICU collation support for encrypted data types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/tecc/tecc_typeicu.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <string.h>

#include "access/htup_details.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/cipherdatatype.h"
#include "utils/syscache.h"
#include "utils/tecc/tecc_typeicu.h"

/*
 * Field accessors for CipherDataType.
 *
 * Use the canonical mask-aware macros from utils/cipherdatatype.h.  The
 * predecessors here hardcoded byte offsets from a layout predating the
 * dek_dist_oid field (ope_length at +12), which silently read the wrong
 * slot; they are therefore not re-declared locally.
 */
#define CD_OPE_LEN(ptr)     CIPHERDATA_DATUM_OPE_LEN(ptr)
#define CD_OPE_PTR(ptr)     CIPHERDATA_DATUM_OPE_PTR(ptr)
#define CD_VERSION(ptr)     CIPHERDATA_DATUM_RAW_VERSION(ptr)

#ifdef USE_ICU
#include <unicode/uversion.h>
#endif

/* Begin - ReqID:TECC4.0 */

/*
 * check_icu_version_compatibility - Check ICU version compatibility
 *
 * This function checks if the ICU version used by PostgreSQL is compatible
 * with what the client might be using. Since ICU processing is done on the
 * client side, we only need to warn about potential incompatibilities.
 *
 * Returns: true if compatible or check passed, false with error if incompatible
 */
static bool pg_attribute_unused()
check_icu_version_compatibility(const char *collname)
{
#ifdef USE_ICU
	/* Get ICU version information */
	char icu_version[U_MAX_VERSION_STRING_LENGTH];
	u_getVersion((uint8_t *)icu_version);

	/* Check for known incompatible versions */
	/* ICU 50-52 had bugs in ucol_strcollUTF8() */
	if (strncmp(icu_version, "50", 2) == 0 ||
		strncmp(icu_version, "51", 2) == 0 ||
		strncmp(icu_version, "52", 2) == 0)
	{
		ereport(WARNING,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("ICU version %s has known bugs in collation functions", icu_version),
				 errdetail("ICU versions 50-52 have bugs in ucol_strcollUTF8(). Collation results may be incorrect."),
				 errhint("Upgrade ICU to version 53 or later for correct collation support.")));
		return false;
	}

	/* Log ICU version for debugging */
	ereport(DEBUG2,
			(errcode(ERRCODE_SUCCESSFUL_COMPLETION),
			 errmsg("ICU version: %s", icu_version)));

	return true;
#else
	/* No ICU support at compile time - compatibility check not applicable */
	/* Use collname parameter to avoid unused parameter warning */
	(void) collname;
	return true;
#endif
}

/*
 * cipherdata_compare_with_collation
 *		Compare two encrypted values with collation support
 *
 * Design principle:
 * - ICU collation processing is done on the client side
 * - Client converts plaintext strings to ICU sort keys, then encrypts them with OPE
 * - Server only needs to validate collation and compare OPE ciphertexts
 *
 * Returns: -1 if a < b, 0 if equal, 1 if a > b
 */
int
cipherdata_compare_with_collation(const CipherDataType *a,
								  const CipherDataType *b,
								  Oid collation_oid)
{
	/* 1. Basic validation - both values must be valid */
	if (!PointerIsValid(a) || !PointerIsValid(b) ||
		!CIPHERDATA_VERSION_VALID(CD_VERSION(a)) ||
		!CIPHERDATA_VERSION_VALID(CD_VERSION(b)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid encrypted data for comparison")));
	}

	/* 2. Check if OPE ciphertexts are available (required for collation-aware comparison) */
	if (CD_OPE_LEN(a) == 0 || CD_OPE_LEN(b) == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot compare encrypted values without OPE ciphertext"),
				 errdetail("Collation-aware comparison requires OPE encryption.")));
	}

	/* 3. If a collation is specified, validate it */
	if (collation_oid != InvalidOid)
	{
		HeapTuple	colltup;
		Form_pg_collation collform;
		const char *collname = NULL;

		/* Check if collation exists */
		colltup = SearchSysCache1(COLLOID, ObjectIdGetDatum(collation_oid));
		if (!HeapTupleIsValid(colltup))
		{
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("collation with OID %u does not exist", collation_oid)));
		}

		collform = (Form_pg_collation) GETSTRUCT(colltup);
		collname = NameStr(collform->collname);

		/* Check if this is an ICU collation */
		if (collform->collprovider == COLLPROVIDER_ICU)
		{
			/* For ICU collations, we need to check compatibility */
#ifdef USE_ICU
			/* PostgreSQL has ICU support at compile time */
			/* Check ICU version compatibility */
			if (!check_icu_version_compatibility(collname))
			{
				/* Version check failed (e.g., known buggy version) */
				ereport(WARNING,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("ICU version compatibility check failed for collation: %s", collname),
						 errdetail("Collation results may be incorrect due to ICU version issues.")));
			}

			ereport(DEBUG1,
					(errcode(ERRCODE_SUCCESSFUL_COMPLETION),
					 errmsg("using ICU collation: %s (provider: ICU)", collname),
					 errdetail("ICU processing is done on the client side.")));
#else
			/* PostgreSQL was built without ICU support */
			/* For encrypted types, this is acceptable because ICU processing is done on the client */
			ereport(NOTICE,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("ICU collation '%s' specified but PostgreSQL was built without ICU support", collname),
					 errdetail("ICU processing for encrypted types is done on the client side. Ensure the client has ICU support."),
					 errhint("For non-encrypted types, rebuild PostgreSQL with --with-icu.")));
#endif
		}
		else
		{
			/* Non-ICU collation */
			ereport(DEBUG2,
					(errcode(ERRCODE_SUCCESSFUL_COMPLETION),
					 errmsg("using collation: %s (provider: %c)", collname, collform->collprovider)));
		}

		ReleaseSysCache(colltup);
	}

	/* 4. Compare OPE ciphertexts (already contain ICU sort keys from client) */
	{
		int			result;
		Size		min_len;
		const char *a_ope = CD_OPE_PTR(a);
		const char *b_ope = CD_OPE_PTR(b);

		/* Compare the common prefix */
		min_len = Min(CD_OPE_LEN(a), CD_OPE_LEN(b));
		result = memcmp(a_ope, b_ope, min_len);

		if (result != 0)
			return (result < 0) ? -1 : 1;

		/* If common prefix is equal, compare lengths */
		if (CD_OPE_LEN(a) < CD_OPE_LEN(b))
			return -1;
		if (CD_OPE_LEN(a) > CD_OPE_LEN(b))
			return 1;

		return 0;
	}
}

/* End - ReqID:TECC4.0 */
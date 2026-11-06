/*-------------------------------------------------------------------------
 *
 * tecc_type_mapping.h
 *	  Type mapping utilities for TECC encrypted types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/tecc_type_mapping.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_TYPE_MAPPING_H
#define TECC_TYPE_MAPPING_H

#include "c.h"
#include "catalog/pg_type_enc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Begin - ReqID:TECC4.0 */

/*
 * Get type name from OID
 *
 * Returns the type name for a given encrypted type OID.
 * Returns NULL if the OID is not a valid encrypted type.
 */
extern const char *tecc_type_name(Oid type_oid);

/*
 * Get OID from type name
 *
 * Returns the OID for a given encrypted type name.
 * Returns InvalidOid if the name is not a valid encrypted type.
 */
extern Oid tecc_type_oid(const char *type_name);

/*
 * Check if OID is an encrypted type
 */
extern bool is_encrypted_type(Oid type_oid);

/*
 * Check if OID is an encrypted array type
 */
extern bool is_encrypted_array_type(Oid type_oid);

/*
 * Get base type OID from encrypted type OID
 *
 * For encrypted types, returns the corresponding plain type OID.
 * For encrypted array types, returns the corresponding plain array type OID.
 */
extern Oid get_base_type_from_encrypted(Oid enc_type_oid);

/*
 * Get encrypted type OID from base type OID
 *
 * For plain types, returns the corresponding encrypted type OID.
 * For plain array types, returns the corresponding encrypted array type OID.
 */
extern Oid get_encrypted_type_from_base(Oid base_type_oid);

/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif	/* TECC_TYPE_MAPPING_H */
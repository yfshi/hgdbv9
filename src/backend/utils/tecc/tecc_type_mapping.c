/*-------------------------------------------------------------------------
 *
 * tecc_type_mapping.c
 *	  Type mapping utilities for TECC encrypted types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/utils/tecc/tecc_type_mapping.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/tecc/tecc_type_mapping.h"
#include "catalog/pg_type_enc.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"

/* Begin - ReqID:TECC4.0 */

/* Type mapping structure */
typedef struct
{
	Oid		enc_type_oid;
	Oid		enc_array_oid;
	Oid		base_type_oid;
	Oid		base_array_oid;
	const char *type_name;
} TeccTypeMapping;

/* Mapping table for encrypted types */
static const TeccTypeMapping tecc_type_mappings[] = {
	/* int2_enc */
	{INT2_ENCOID, INT2_ENCARRAYOID, INT2OID, INT2ARRAYOID, "int2_enc"},
	/* int4_enc */
	{INT4_ENCOID, INT4_ENCARRAYOID, INT4OID, INT4ARRAYOID, "int4_enc"},
	/* int8_enc */
	{INT8_ENCOID, INT8_ENCARRAYOID, INT8OID, INT8ARRAYOID, "int8_enc"},
	/* float4_enc */
	{FLOAT4_ENCOID, FLOAT4_ENCARRAYOID, FLOAT4OID, FLOAT4ARRAYOID, "float4_enc"},
	/* float8_enc */
	{FLOAT8_ENCOID, FLOAT8_ENCARRAYOID, FLOAT8OID, FLOAT8ARRAYOID, "float8_enc"},
	/* numeric_enc */
	{NUMERIC_ENCOID, NUMERIC_ENCARRAYOID, NUMERICOID, NUMERICARRAYOID, "numeric_enc"},
	/* varchar_enc */
	{VARCHAR_ENCOID, VARCHAR_ENCARRAYOID, VARCHAROID, VARCHARARRAYOID, "varchar_enc"},
	/* bpchar_enc */
	{BPCHAR_ENCOID, BPCHAR_ENCARRAYOID, BPCHAROID, BPCHARARRAYOID, "bpchar_enc"},
	/* text_enc */
	{TEXT_ENCOID, TEXT_ENCARRAYOID, TEXTOID, TEXTARRAYOID, "text_enc"},
};

#define NUM_TECC_TYPES (sizeof(tecc_type_mappings) / sizeof(TeccTypeMapping))

/*
 * tecc_type_name - Get type name from OID
 */
const char *
tecc_type_name(Oid type_oid)
{
	int i;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (tecc_type_mappings[i].enc_type_oid == type_oid ||
			tecc_type_mappings[i].enc_array_oid == type_oid)
		{
			return tecc_type_mappings[i].type_name;
		}
	}

	return NULL;
}

/*
 * tecc_type_oid - Get OID from type name
 */
Oid
tecc_type_oid(const char *type_name)
{
	int i;

	if (type_name == NULL)
		return InvalidOid;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (strcmp(tecc_type_mappings[i].type_name, type_name) == 0)
		{
			return tecc_type_mappings[i].enc_type_oid;
		}
	}

	return InvalidOid;
}

/*
 * is_encrypted_type - Check if OID is an encrypted type
 */
bool
is_encrypted_type(Oid type_oid)
{
	int i;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (tecc_type_mappings[i].enc_type_oid == type_oid)
			return true;
	}

	return false;
}

/*
 * is_encrypted_array_type - Check if OID is an encrypted array type
 */
bool
is_encrypted_array_type(Oid type_oid)
{
	int i;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (tecc_type_mappings[i].enc_array_oid == type_oid)
			return true;
	}

	return false;
}

/*
 * get_base_type_from_encrypted - Get base type OID from encrypted type OID
 */
Oid
get_base_type_from_encrypted(Oid enc_type_oid)
{
	int i;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (tecc_type_mappings[i].enc_type_oid == enc_type_oid)
			return tecc_type_mappings[i].base_type_oid;
		if (tecc_type_mappings[i].enc_array_oid == enc_type_oid)
			return tecc_type_mappings[i].base_array_oid;
	}

	return InvalidOid;
}

/*
 * get_encrypted_type_from_base - Get encrypted type OID from base type OID
 */
Oid
get_encrypted_type_from_base(Oid base_type_oid)
{
	int i;

	for (i = 0; i < NUM_TECC_TYPES; i++)
	{
		if (tecc_type_mappings[i].base_type_oid == base_type_oid)
			return tecc_type_mappings[i].enc_type_oid;
		if (tecc_type_mappings[i].base_array_oid == base_type_oid)
			return tecc_type_mappings[i].enc_array_oid;
	}

	return InvalidOid;
}

/* End - ReqID:TECC4.0 */
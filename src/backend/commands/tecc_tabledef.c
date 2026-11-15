/*-------------------------------------------------------------------------
 *
 * tecc_tabledef.c
 *    Reconstruct CREATE TABLE DDL with encrypted column metadata.
 *
 * This module provides tecc_get_tabledef(regclass), which returns the
 * CREATE TABLE DDL for the given table, including ENCRYPTED WITH clauses
 * for columns that have encryption metadata in hg_encrypted_columns.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2024, HighGo Software
 *
 * IDENTIFICATION
 *    src/backend/commands/tecc_tabledef.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/table.h"
#include "catalog/namespace.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_class.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_collation_d.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/hg_dek.h"
#include "commands/encrypted_column.h"
#include "commands/tablespace.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/hg_key_utils.h"
#include "utils/ruleutils.h"

PG_FUNCTION_INFO_V1(tecc_get_tabledef);

/*
 * Append encryption type bitmask as human-readable algorithm string.
 *
 * Bitmask: 0x01=DET, 0x02=OPE, 0x04=SEARCH, 0x08=HE_ADD, 0x10=HE_MUL
 * Output example: "DET,OPE"
 */
static void
append_encryption_type_algo(StringInfo buf, int32 encryption_type)
{
    bool has_prev = false;

    if (encryption_type & HG_ENCRYPTION_TYPE_DET)
    {
        appendStringInfoString(buf, "DET");
        has_prev = true;
    }
    if (encryption_type & HG_ENCRYPTION_TYPE_OPE)
    {
        appendStringInfo(buf, "%sOPE", has_prev ? "," : "");
        has_prev = true;
    }
    if (encryption_type & HG_ENCRYPTION_TYPE_SEARCH)
    {
        appendStringInfo(buf, "%sSEARCH", has_prev ? "," : "");
        has_prev = true;
    }
    if (encryption_type & HG_ENCRYPTION_TYPE_HE_ADD)   /* ReqID:TECC4.0 */
    {
        appendStringInfo(buf, "%sEC-ELGAMAL", has_prev ? "," : "");
        has_prev = true;
    }
    if (encryption_type & HG_ENCRYPTION_TYPE_HE_MUL)   /* ReqID:TECC4.0 */
    {
        appendStringInfo(buf, "%sELGAMAL", has_prev ? "," : "");
    }
}

/*
 * tecc_get_tabledef - Reconstruct CREATE TABLE DDL including ENCRYPTED WITH.
 *
 * Syntax: tecc_get_tabledef(table_oid regclass) RETURNS text
 */
Datum
tecc_get_tabledef(PG_FUNCTION_ARGS)
{
    Oid         table_oid = PG_GETARG_OID(0);
    StringInfoData buf;
    HeapTuple   tp;
    Form_pg_class classForm;
    char       *nspname;
    char       *relname;
    Relation    attr_rel;
    SysScanDesc scan;
    HeapTuple   attr_tup;
    bool        first_col = true;

    /* -------- Look up table in pg_class -------- */
    tp = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
    if (!HeapTupleIsValid(tp))
        ereport(ERROR, (errmsg("relation with OID %u does not exist",
                               table_oid)));

    classForm = (Form_pg_class) GETSTRUCT(tp);
    nspname = get_namespace_name(classForm->relnamespace);
    relname = NameStr(classForm->relname);

    initStringInfo(&buf);

    /* -------- CREATE [UNLOGGED] TABLE schema.tablename ( -------- */
    if (classForm->relpersistence == RELPERSISTENCE_UNLOGGED)
        appendStringInfoString(&buf, "CREATE UNLOGGED TABLE ");
    else
        appendStringInfoString(&buf, "CREATE TABLE ");

    appendStringInfo(&buf, "%s (",
                     quote_qualified_identifier(nspname, relname));

    /* -------- Scan columns from pg_attribute -------- */
    attr_rel = table_open(AttributeRelationId, AccessShareLock);
    {
        ScanKeyData skey;

        ScanKeyInit(&skey, Anum_pg_attribute_attrelid,
                    BTEqualStrategyNumber, F_OIDEQ,
                    ObjectIdGetDatum(table_oid));
        scan = systable_beginscan(attr_rel, AttributeRelidNumIndexId,
                                  true, NULL, 1, &skey);

        while ((attr_tup = systable_getnext(scan)))
        {
            Form_pg_attribute att = (Form_pg_attribute) GETSTRUCT(attr_tup);
            char       *attname;
            char       *typename;

            /* Skip system columns and dropped columns */
            if (att->attnum <= 0 || att->attisdropped)
                continue;

            attname = NameStr(att->attname);
            typename = format_type_with_typemod(att->atttypid,
                                                att->atttypmod);

            if (!first_col)
                appendStringInfoString(&buf, ",");
            first_col = false;

            /* Column name + type */
            appendStringInfo(&buf, "\n    %s %s",
                             quote_identifier(attname), typename);

            /* -------- GENERATED {ALWAYS|BY DEFAULT} AS IDENTITY -------- */
            if (att->attidentity)
            {
                appendStringInfo(&buf, " GENERATED %s AS IDENTITY",
                                 (att->attidentity == 'a') ?
                                     "ALWAYS" : "BY DEFAULT");
            }

            /* -------- DEFAULT (skip if identity or generated) -------- */
            if (att->atthasdef && att->attidentity == '\0' &&
                att->attgenerated == '\0')
            {
                Relation    adrel;
                SysScanDesc adscan;
                ScanKeyData adkey;
                HeapTuple   adtup;

                adrel = table_open(AttrDefaultRelationId, AccessShareLock);
                ScanKeyInit(&adkey, Anum_pg_attrdef_adrelid,
                            BTEqualStrategyNumber, F_OIDEQ,
                            ObjectIdGetDatum(table_oid));
                adscan = systable_beginscan(adrel, AttrDefaultIndexId,
                                            true, NULL, 1, &adkey);
                while ((adtup = systable_getnext(adscan)))
                {
                    Form_pg_attrdef adform = (Form_pg_attrdef) GETSTRUCT(adtup);

                    if (adform->adnum == att->attnum)
                    {
                        Datum       adbin_datum;
                        bool        adbin_null;
                        text       *adbin_text;
                        Datum       def_result;

                        adbin_datum = heap_getattr(adtup,
                                                   Anum_pg_attrdef_adbin,
                                                   RelationGetDescr(adrel),
                                                   &adbin_null);
                        if (!adbin_null)
                        {
                            adbin_text = DatumGetTextPP(adbin_datum);
                            def_result = DirectFunctionCall2(
                                pg_get_expr,
                                PointerGetDatum(adbin_text),
                                ObjectIdGetDatum(table_oid));
                            appendStringInfo(&buf, " DEFAULT %s",
                                             TextDatumGetCString(def_result));
                        }
                        break;
                    }
                }
                systable_endscan(adscan);
                table_close(adrel, AccessShareLock);
            }

            /* -------- NOT NULL -------- */
            if (att->attnotnull)
                appendStringInfoString(&buf, " NOT NULL");

            /* -------- COLLATE -------- */
            if (OidIsValid(att->attcollation) &&
                att->attcollation != DEFAULT_COLLATION_OID)
            {
                char *collname = generate_collation_name(att->attcollation);
                appendStringInfo(&buf, " COLLATE %s", collname);
            }

            /* -------- ENCRYPTED WITH (dek = name, algo = ...) -------- */
            {
                HeapTuple ectp;

                ectp = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
                                       ObjectIdGetDatum(table_oid),
                                       Int16GetDatum(att->attnum));
                if (HeapTupleIsValid(ectp))
                {
                    Form_hg_encrypted_columns ecform =
                        (Form_hg_encrypted_columns) GETSTRUCT(ectp);

                    /* DEK name */
                    appendStringInfo(&buf, " ENCRYPTED WITH (dek = %s",
                                     quote_identifier(
                                         get_dek_name(ecform->column_key_id)));

                    /* Algorithm flags */
                    if (ecform->encryption_type &
                        (HG_ENCRYPTION_TYPE_DET |
                         HG_ENCRYPTION_TYPE_OPE |
                         HG_ENCRYPTION_TYPE_SEARCH |
                         HG_ENCRYPTION_TYPE_HE_ADD |    /* ReqID:TECC4.0 */
                         HG_ENCRYPTION_TYPE_HE_MUL))    /* ReqID:TECC4.0 */
                    {
                        appendStringInfoString(&buf, ", algo = '");
                        append_encryption_type_algo(&buf,
                                                    ecform->encryption_type);
                        appendStringInfoChar(&buf, '\'');
                    }

                    appendStringInfoChar(&buf, ')');
                    ReleaseSysCache(ectp);
                }
            }
        }
        systable_endscan(scan);
    }
    table_close(attr_rel, AccessShareLock);

    /* -------- CHECK constraints (inline) -------- */
    {
        Relation    con_rel;
        SysScanDesc con_scan;
        ScanKeyData con_key;
        HeapTuple   con_tup;

        con_rel = table_open(ConstraintRelationId, AccessShareLock);
        ScanKeyInit(&con_key, Anum_pg_constraint_conrelid,
                    BTEqualStrategyNumber, F_OIDEQ,
                    ObjectIdGetDatum(table_oid));
        con_scan = systable_beginscan(con_rel,
                                      ConstraintRelidTypidNameIndexId,
                                      true, NULL, 1, &con_key);
        while ((con_tup = systable_getnext(con_scan)))
        {
            Form_pg_constraint con = (Form_pg_constraint) GETSTRUCT(con_tup);

            if (con->contype == CONSTRAINT_CHECK && con->conislocal)
            {
                Datum       def_result;

                def_result = DirectFunctionCall1Coll(
                    pg_get_constraintdef, InvalidOid,
                    ObjectIdGetDatum(con->oid));
                appendStringInfo(&buf, ",\n    CONSTRAINT %s %s",
                                 quote_identifier(NameStr(con->conname)),
                                 TextDatumGetCString(def_result));
            }
        }
        systable_endscan(con_scan);
        table_close(con_rel, AccessShareLock);
    }

    /* -------- Closing parenthesis -------- */
    appendStringInfoString(&buf, "\n)");

    /* -------- WITH (storage_options) -------- */
    {
        Datum       options_datum;
        bool        isnull;

        options_datum = SysCacheGetAttr(RELOID, tp,
                                        Anum_pg_class_reloptions, &isnull);
        if (!isnull)
        {
            char *options_str = TextDatumGetCString(options_datum);

            if (options_str != NULL && strlen(options_str) > 0)
                appendStringInfo(&buf, "\nWITH (%s)", options_str);
        }
    }

    /* -------- TABLESPACE -------- */
    if (OidIsValid(classForm->reltablespace))
    {
        char *tblspc = get_tablespace_name(classForm->reltablespace);

        if (tblspc != NULL)
            appendStringInfo(&buf, "\nTABLESPACE %s",
                             quote_identifier(tblspc));
    }

    ReleaseSysCache(tp);

    PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}

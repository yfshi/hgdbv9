#include "postgres.h"

#include "access/printtup.h"
#include "c.h"
#include "catalog/pg_enum.h"
#include "tcop/pquery.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/bytea.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memdebug.h"
#include "utils/memutils.h"
#include "utils/syscache.h"
#include "utils/varbit.h"

#include "../include/ivorysql_my.h"
#include "../include/message.h"
#include "../include/server.h"
#include "../include/printtup.h"
#include "../include/mqformat.h"

/**
 * Begin - bug#9024
 * ORM framework like those in GO language utilize the length and decimal fields
 * of response packets, which can significantly impact data consistency.
 * It's imperative to maintain consistency with MySQL.
 */
static void
get_column_length_and_decimal(FormData_pg_attribute *attr, uint32 *length, uint32 *decimals)
{
    uint32 len = 0;
    uint32 dec = 0;
    Oid typoid = attr->atttypid;
    int mbmaxlen = pg_database_encoding_max_length();
    int typmod = attr->atttypmod;

    switch (typoid)
    {
        // number
        case TINYINTOID:
            len = 4;
            break;
        case INT2OID:
            len = 6;
            break;
        case MEDIUMINTOID:
            len = 9;
            break;
        case INT4OID:
            len = 11;
            break;
        case INT8OID:
            len = 20;
            break;
        case UINT1OID:
            len = 3;
            break;
        case UINT2OID:
            len = 5;
            break;
        case UINT3OID:
            len = 8;
            break;
        case UINT4OID:
            len = 10;
            break;
        case UINT8OID:
            len = 20;
            break;
        case FLOAT4OID:
            len = 12;
            dec = 31;
            break;
        case FLOAT8OID:
            len = 22;
            dec = 31;
            break;
        
        // numeric
        case NUMERICOID:
            dec = ((typmod - VARHDRSZ) & 0xffff);
            len = ((typmod - VARHDRSZ) >> 16) & 0xffff;

            // Decimal point and sign of positive and negative
            len += 2;
            break;

        // character
        case CHAROID:
            len = 1 * mbmaxlen; // only 1 character
            break;
        case BPCHAROID:
        case VARCHAROID:
            len = (typmod - VARHDRSZ) * mbmaxlen;
            break;
        case TEXTOID:
            len = 262140;
            break;
        case TINYTEXTOID:
            len = 1020;
            break;
        case MEDIUMTEXTOID:
            len = 67108860;
            break;
        case LONGTEXTOID:
            len = 4294967295;
            break;

        // binary
        case MYSBITOID:
            len = typmod;
            break;
        case BINARYOID:
        case VARBINARYOID:
            len = typmod - VARHDRSZ;
            break;
        case MYBLOBOID:
            len = 65535;
            break;
        case TINYBLOBOID:
            len = 255;
            break;
        case MEDIUMBLOBOID:
            len = 16777215;
            break;
        case LONGBLOBOID:
            len = 4294967295;
            break;

        // time
        case YEAROID:
            len = 4;
            break;
        case MYSDATEOID:
            len = 10;
            break;
        case MYSTIMEOID:
            len = 10;
            break;
        case MYSTIMESOID:
            len = 10;
            dec = typmod;

            // A space between date and time
            len += (1 + dec);
            break;
        case MYSDATETIMEOID:
        case MYSTIMESTAMPOID:
            len = 19;
            break;
        case MYSDATETIMESOID:
        case MYSTIMESTAMPSOID:
            len = 19;
            dec = typmod;

            // A space between date and time
            len += (1 + dec);
            break;

        // json
        case JSONOID:
            len = 4294967295;
            break;
        
        default:
            if (type_is_enum(typoid))
            {
                int			nelems;
	            CatCList   	*list;

                list = SearchSysCacheList1(ENUMTYPOIDNAME, ObjectIdGetDatum(typoid));
                nelems = list->n_members;
                
                for (int i = 0; i < nelems; i++)
                {
                    HeapTuple exists_tup = &(list->members[i]->tuple);
                    Form_pg_enum exists_en = (Form_pg_enum) GETSTRUCT(exists_tup);
                    char *enumlabel = NameStr(exists_en->enumlabel);
                    int label_len = strlen(enumlabel);

                    if (label_len > len)
                        len = label_len;
                }

                len *= mbmaxlen;

	            ReleaseCatCacheList(list);
            }
            else
            {
                len = 4294967295;
            }

            // TO DO set

            break;
    }

    *length = len;
    *decimals = dec;
}
/* End - bug#9024 */

static mysql_column_definition_t *
make_column_definition_with_attr(FormData_pg_attribute *attr)
{
    // FIELD packet
    mysql_column_definition_t *field = (mysql_column_definition_t *) palloc0(sizeof(mysql_column_definition_t));

    const TypeItem *item = GetItemByTypeOid(attr->atttypid, false);

    /* Begin - OA#2772 */
    /**
     * Column names are result metadata and should follow character_set_results
     * rather than always being converted to UTF8
     */
    field->name = pg_server_to_any(attr->attname.data,
                                   strlen(attr->attname.data),
                                   PG_UTF8);
    /* End - OA#2772 */

    field->type = item->mysql_type_id;

    /* Begin - bug#9024 */
    get_column_length_and_decimal(attr, &field->length, &field->decimals);
    /* End - bug#9024 */

    field->flags = item->flags;

    /* Begin - OA#2772 */
    field->charset = get_mysql_result_collation_id(item);
    /* End - OA#2772 */

    return field;
}

static void
fill_field_flag(TupleDesc typeinfo, int nattr, mysql_column_definition_t *field)
{
    FormData_pg_attribute *att = TupleDescAttr(typeinfo, nattr);
    Oid        typid = att->atttypid;
    uint32     flags = field->flags;

    if (att->attnotnull)
        flags |= NOT_NULL_FLAG;

    if (att->attidentity == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
        flags |= AUTO_INCREMENT_FLAG;
    
    if (type_is_enum(typid))
        flags |= ENUM_FLAG;

    field->flags = flags;
}

static void
text_row_description_message(TupleDesc typeinfo, List *targetlist)
{
    StringInfo buf = makeStringInfo();
    FormData_pg_attribute *attrs = typeinfo->attrs;
    int natts = typeinfo->natts;
    ListCell *tlist_item = list_head(targetlist);
    uint8 metadata_follows = mysql_conn_info.resultset_metadata;
    
    // metadata_follows
    if (has_client_capabilities(CLIENT_OPTIONAL_RESULTSET_METADATA))
    {
        mq_append_int1(buf, metadata_follows);
    }

    // column_count
    mq_append_int_lenenc(buf, natts);

    mq_putmessage(buf->data, buf->len);

    // Field metadata
    if (!has_client_capabilities(CLIENT_OPTIONAL_RESULTSET_METADATA) ||
        metadata_follows == RESULTSET_METADATA_FULL)
    {
        for (int i = 0; i < natts; ++i)
        {
            FormData_pg_attribute *attr = &attrs[i];
            
            // FIELD packet
            mysql_column_definition_t *field = make_column_definition_with_attr(attr);

            fill_field_flag(typeinfo, i, field);

            while (tlist_item && ((TargetEntry *) lfirst(tlist_item))->resjunk)
            {
                tlist_item = lnext(targetlist, tlist_item);
            }

            if (tlist_item != NULL)
            {
                tlist_item = lnext(targetlist, tlist_item);
            }

            send_column_definition41_packet(field);
            pfree(field);
        }
    }

    // End of metadata
    if (!has_client_capabilities(CLIENT_DEPRECATE_EOF))
        send_eof(0, 0);

    DestroyStringInfo(buf);
}

static void
binary_row_description_message(TupleDesc typeinfo, List *targetlist)
{
    StringInfo buf = makeStringInfo();
    FormData_pg_attribute *attrs = typeinfo->attrs;
    int natts = typeinfo->natts;
    ListCell *tlist_item = list_head(targetlist);

    mq_append_int_lenenc(buf, natts);

    mq_putmessage(buf->data, buf->len);

    for (int i = 0; i < natts; ++i)
    {
        mysql_column_definition_t *field = make_column_definition_with_attr(&attrs[i]);

        fill_field_flag(typeinfo, i, field);

        while (tlist_item && ((TargetEntry *) lfirst(tlist_item))->resjunk)
        {
            tlist_item = lnext(targetlist, tlist_item);
        }

        if (tlist_item != NULL)
        {
            tlist_item = lnext(targetlist, tlist_item);
        }

        send_column_definition41_packet(field);
        pfree(field);
    }

    /* Begin OA-2717 */
    if (!has_client_capabilities(CLIENT_DEPRECATE_EOF))
        send_eof(0, 0);
    /* End - OA-2717 */

    DestroyStringInfo(buf);
}

/*
 * Get the lookup info that printtup() needs
 */
static void
printtup_prepare_info(DR_printtup *myState, TupleDesc typeinfo, int numAttrs)
{
    int      i;
    int16   *formats = myState->portal->formats;

    /* get rid of any old data */
    if (myState->myinfo != NULL)
        pfree(myState->myinfo);

    myState->myinfo = NULL;
    myState->attrinfo = typeinfo;
    myState->nattrs = numAttrs;

    if (numAttrs <= 0)
        return;

    if (myState->portal != NULL && myState->portal->tupDesc != NULL)
    {
        #ifdef USE_ASSERT_CHECKING
        Assert(numAttrs <= myState->portal->tupDesc->natts);
        #else
        if (numAttrs > myState->portal->tupDesc->natts)
        {
            ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("num attrs from DN is %d, mismatch num attrs %d in portal", numAttrs, myState->portal->tupDesc->natts)));
        }
        #endif
    }

    myState->myinfo = (PrinttupAttrInfo *) palloc0(numAttrs * sizeof(PrinttupAttrInfo));

    for (i = 0; i < numAttrs; i++)
    {
        PrinttupAttrInfo *thisState = myState->myinfo + i;
        int16 format = (formats ? formats[i] : 0);
        Form_pg_attribute attr = TupleDescAttr(typeinfo, i);

        /*
         * for analyze global stats, because DN will send sample rows to CN,
         * if we encounter droped columns, we should send it to CN. but atttypid of dropped column
         * is invalid in pg_attribute, it will generate error, so we should do special process for the reason.
         */
        if (typeinfo->attrs[i].attisdropped)
            typeinfo->attrs[i].atttypid = UNKNOWNOID;

        thisState->format = format;
        if (format == 0)
        {
            getTypeOutputInfo(attr->atttypid,
							  &thisState->typoutput,
							  &thisState->typisvarlena);
			fmgr_info(thisState->typoutput, &thisState->finfo);
        }
        else if (format == 1)
        {
            getTypeBinaryOutputInfo(typeinfo->attrs[i].atttypid, &thisState->typsend, &thisState->typisvarlena);
            fmgr_info(thisState->typsend, &thisState->finfo);
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("unsupported format code: %d", format)));
        }
    }
}

/* Begin - OA#2772 */
static char *
mysql_convert_result_encoding(const char *str, int len)
{
    int result_encoding;

    if (str == NULL)
        return NULL;

    /*
     * character_set_results = NULL or empty means no conversion.
     */
    if (CHARACTER_SET_RESULTS_IS_VALIDY())
        return unconstify(char *, str);

    /*
     * MySQL binary charset means no character set conversion.
     */
    if (pg_strcasecmp(character_set_results, "binary") == 0)
        return unconstify(char *, str);

    result_encoding = pg_char_to_encoding(character_set_results);

    if (!PG_VALID_ENCODING(result_encoding))
        return unconstify(char *, str);

    return pg_server_to_any(str, len, result_encoding);
}
/* End - OA#2772 */

static void
send_textproto(TupleTableSlot *slot, DR_printtup *myState, int natts, StringInfo buf)
{
    /*
     * send the attributes of this tuple
     */
    for (int i = 0; i < natts; ++i)
    {
        Oid     typeOid;
        char    *outputstr = NULL;
        Datum   attr = slot->tts_values[i];
        PrinttupAttrInfo *thisState = myState->myinfo + i;

        if (slot->tts_isnull[i])
        {
            mq_append_int1(buf, NULL_STRING_MARK);
            continue;
        }

        if (slot->tts_tupleDescriptor->attrs[i].attisdropped)
        {
            mq_append_string_lenenc(buf, "", -1);
            continue;
        }

        /*
         * Here we catch undefined bytes in datums that are returned to the
         * client without hitting disk; see comments at the related check in
         * PageAddItem().  This test is most useful for uncompressed,
         * non-external datums, but we're quite likely to see such here when
         * testing new C functions.
         */
        if (thisState->typisvarlena)
            VALGRIND_CHECK_MEM_IS_DEFINED(DatumGetPointer(attr),
										  VARSIZE_ANY(attr));

        typeOid = slot->tts_tupleDescriptor->attrs[i].atttypid;

        /* Begin - bug#6619 */
        if (typeOid == MYSBITOID)
        {
            bytea *arg = DatumGetByteaPP(DirectFunctionCall1(bittobinary, attr));
            mq_append_string_lenenc(buf, VARDATA_ANY(arg), VARSIZE_ANY_EXHDR(arg));
        }
        /* End - bug#6619 */
        else if (type_is_blob(typeOid))
        {
            bytea *arg = DatumGetByteaPP(attr);
            mq_append_string_lenenc(buf, VARDATA_ANY(arg), VARSIZE_ANY_EXHDR(arg));
        }
        else
        {
            char *clientstr;
            outputstr = OutputFunctionCall(&thisState->finfo, attr);
            /* Begin - OA#2772 */
            clientstr = mysql_convert_result_encoding(outputstr, strlen(outputstr));
            /* End - OA#2772 */
            mq_append_string_lenenc(buf, clientstr, strlen(clientstr));

            if (clientstr != outputstr)
                pfree(clientstr);
        }
    }
}

static void
mysdate2tm(DateADT date, struct pg_tm *tm)
{
    tm->tm_year = date / 10000;
    tm->tm_mon = (date % 10000) / 100;
    tm->tm_mday = date % 100;
}

static void
mystime2tm(TimeADT time, struct pg_tm *tm, bool *neg)
{
    if (time < 0)
    {
        time *= -1;
        *neg = true;
    }
    else
    {
        *neg = false;
    }

    tm->tm_hour = time / 10000;
    tm->tm_min = (time % 10000) / 100;
    tm->tm_sec = time % 100;
}

static void
mystimes2tm(Numeric val, struct pg_tm *tm, fsec_t *fsec, bool *neg)
{
    lldiv_t lld;

    numeric_to_lldiv_t(val, &lld);

    if (lld.quot < 0)
    {
        lld.quot *= -1;
        *neg = true;
    }
    else
    {
        *neg = false;
    }

    tm->tm_hour = lld.quot / 10000;
    tm->tm_min = (lld.quot % 10000) / 100;
    tm->tm_sec = lld.quot % 100;

    *fsec = lld.rem / 1000;
}

static void
mysdatetime2tm(TimeADT datetime, struct pg_tm *tm)
{
    bool    neg;
    DateADT date = datetime / 1000000;
    TimeADT time = datetime % 1000000;

    mysdate2tm(date, tm);
    mystime2tm(time, tm, &neg);
}

static void
mysdatetimes2tm(Numeric val, struct pg_tm *tm, fsec_t *fsec)
{
    TimeADT datetime;
    DateADT date;
    TimeADT time;
    lldiv_t lld;
    bool    neg;

    numeric_to_lldiv_t(val, &lld);

    datetime = lld.quot;
    date = datetime / 1000000;
    time = datetime % 1000000;

    mysdate2tm(date, tm);
    mystime2tm(time, tm, &neg);

    *fsec = lld.rem / 1000;
}

static void
mystimestamp2tm(TimeADT timestamp, struct pg_tm *tm)
{
    DateADT date = timestamp / 1000000;
    TimeADT time = timestamp % 1000000;
    bool    neg;

    mysdate2tm(date, tm);
    mystime2tm(time, tm, &neg);
}

static void
mystimestamps2tm(Numeric val, struct pg_tm *tm, fsec_t *fsec)
{
    TimeADT timestamp;
    DateADT date;
    TimeADT time;
    lldiv_t lld;
    bool    neg;

    numeric_to_lldiv_t(val, &lld);

    timestamp = lld.quot;

    date = timestamp / 1000000;
    time = timestamp % 1000000;

    mysdate2tm(date, tm);
    mystime2tm(time, tm, &neg);

    *fsec = lld.rem / 1000;
}

static void
append_data_by_mysql_type(const TypeItem *item, Datum binval, StringInfo buf, PrinttupAttrInfo *thisState)
{
    Oid typoid = item->oid;

    switch (typoid)
    {
        case TINYINTOID:
        case UINT1OID:
        case BOOLOID:
            {
                int8 val = DatumGetInt8(binval);
                mq_append_int1(buf, val);
                break;
            }
        case INT2OID:
        case UINT2OID:
            {
                int16 val = DatumGetInt16(binval);
                mq_append_int2(buf, val);
                break;
            }
        case OIDOID:
        case MEDIUMINTOID:
        case INT4OID:
        case UINT3OID:
        case UINT4OID:
            {
                int32 val = DatumGetInt32(binval);
                mq_append_int4(buf, val);
                break;
            }
        case INT8OID:
        case UINT8OID:
            {
                int64 val = DatumGetInt64(binval);
                mq_append_int8(buf, val);
                break;
            }
        case FLOAT4OID:
            {
                int32 num = DatumGetInt32(binval);
                mq_append_int4(buf, num);
                break;
            }
        case FLOAT8OID:
            {
                int64 num = DatumGetInt64(binval);
                mq_append_int8(buf, num);
                break;
            }
        case NUMERICOID:
            {
                // output as string
                char *val = DatumGetCString(DirectFunctionCall1(numeric_out, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSBITOID:
                binval = DirectFunctionCall1(bittobinary, binval); /* fallthrough */
        case BINARYOID:
        case VARBINARYOID:
        case IVY_HEXOID:
        case MYBLOBOID:
        case TINYBLOBOID:
        case MEDIUMBLOBOID:
        case LONGBLOBOID:
            {
                bytea   *val = DatumGetByteaPP(binval);
                mq_append_string_lenenc(buf, VARDATA_ANY(val), VARSIZE_ANY_EXHDR(val));
                break;
            }
        case YEAROID:
            {
                char *val = DatumGetCString(DirectFunctionCall1(year_out, binval));
                int16  y = pg_atoi(val, sizeof(int), ' ');
                mq_append_int2(buf, y);
                pfree(val);
                break;
            }
        case MYSDATEOID:
            {
                struct pg_tm tt, *tm = &tt;
                DateADT date = DatumGetDateADT(binval);

                mysdate2tm(date, tm);

                mq_append_int1(buf, 0x04);
                mq_append_int2(buf, tm->tm_year);
                mq_append_int1(buf, tm->tm_mon);
                mq_append_int1(buf, tm->tm_mday);
                
                break;
            }
        case MYSTIMEOID:
        case MYSTIMESOID:
            {
                struct pg_tm tt, *tm = &tt;
                fsec_t fsec;
                bool neg;
                
                if (item->oid == MYSTIMESOID)
                {                    
                    mystimes2tm(DatumGetNumeric(binval), tm, &fsec, &neg);
                }
                else
                {
                    mystime2tm(DatumGetTimeADT(binval), tm, &neg);
                    fsec = 0;
                }

                mq_append_int1(buf, 0x0c);

                if (neg)
                    mq_append_int1(buf, 0x01);
                else
                    mq_append_int1(buf, 0x00);

                mq_append_int4(buf, 0x00);
                mq_append_int1(buf, tm->tm_hour);
                mq_append_int1(buf, tm->tm_min);
                mq_append_int1(buf, tm->tm_sec);
                mq_append_int4(buf, fsec);
                break;
            }
        case MYSDATETIMEOID:
        case MYSDATETIMESOID:
            {
                struct pg_tm tt, *tm = &tt;
                fsec_t fsec;

                if (typoid == MYSDATETIMESOID)
                {
                    mysdatetimes2tm(DatumGetNumeric(binval), tm, &fsec);
                }
                else
                {
                    mysdatetime2tm(DatumGetTimeADT(binval), tm);
                    fsec = 0;
                }

                mq_append_int1(buf, 0x0b);
                mq_append_int2(buf, tm->tm_year);
                mq_append_int1(buf, tm->tm_mon);
                mq_append_int1(buf, tm->tm_mday);
                mq_append_int1(buf, tm->tm_hour);
                mq_append_int1(buf, tm->tm_min);
                mq_append_int1(buf, tm->tm_sec);
                mq_append_int4(buf, fsec);
                break;
            }
        case MYSTIMESTAMPOID:
        case MYSTIMESTAMPSOID:
            {
                struct pg_tm tt, *tm = &tt;
                fsec_t fsec;

                if (item->oid == MYSTIMESTAMPSOID)
                {
                    mystimestamps2tm(DatumGetNumeric(binval), tm, &fsec);
                }
                else
                {
                    mystimestamp2tm(DatumGetTimeADT(binval), tm);
                    fsec = 0;
                }

                mq_append_int1(buf, 0x0b);
                mq_append_int2(buf, tm->tm_year);
                mq_append_int1(buf, tm->tm_mon);
                mq_append_int1(buf, tm->tm_mday);
                mq_append_int1(buf, tm->tm_hour);
                mq_append_int1(buf, tm->tm_min);
                mq_append_int1(buf, tm->tm_sec);
                mq_append_int4(buf, fsec);
                break;
            }
        case POINTOID:
        case LINEOID:
        case POLYGONOID:
            {
                char *val;
                if (item->oid == POINTOID)
                    val = DatumGetCString(DirectFunctionCall1(point_out, binval));
                else if (item->oid == LINEOID)
                    val = DatumGetCString(DirectFunctionCall1(line_out, binval));
                else if (item->oid == POLYGONOID)
                    val = DatumGetCString(DirectFunctionCall1(poly_out, binval));
                else
                {
                    assert(0);
                    break;
                }

                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }

        case CHAROID:
        case BPCHAROID:
        case VARCHAROID:
        case TEXTOID:
        case TINYTEXTOID:
        case MEDIUMTEXTOID:
        case LONGTEXTOID:
        case JSONOID:
        case JSONBOID:
        default:
            {
                bool		isvarlena;
                Oid			outfunc;
	            FmgrInfo	fmgrinfo;
                char        *raw;
                char        *str;

                getTypeOutputInfo(typoid, &outfunc, &isvarlena);
	            fmgr_info(outfunc, &fmgrinfo);
                
                raw = OutputFunctionCall(&fmgrinfo, binval);
                /* Begin - OA#2772 */
                str = mysql_convert_result_encoding(raw, strlen(raw));
                /* End - OA#2772 */

                mq_append_string_lenenc(buf, str, -1);

                if (str != raw)
                    pfree(str);

                pfree(raw);

                break;
            }
    }
}

static void
send_binaryproto(TupleTableSlot *slot, DR_printtup *myState, int natts, StringInfo buf)
{
    int         len = (natts + 7 + 2) / 8;
    bits8       *null_bitmap;
    TupleDesc   desc = slot->tts_tupleDescriptor;

    null_bitmap = (bits8 *) palloc0(len * sizeof(bits8));

    // [0x00] packet header
    mq_append_int1(buf, 0x00);

    for (int j = 0; j < natts; j++)
    {
        if (slot->tts_isnull[j] || desc->attrs[j].attisdropped)
        {
            int byte_pos = (j + 2) / 8;
            int bit_pos = (j + 2) % 8;
            null_bitmap[byte_pos] |= 1 << bit_pos;
        }
    }

    for (int k = 0; k < len; k++)
    {
        mq_append_int1(buf, null_bitmap[k]);
    }

    // values for non-null columns
    for (int i = 0; i < natts; i++)
    {
        Datum                binval;
        PrinttupAttrInfo    *thisState;
        const TypeItem      *item;

        if (slot->tts_isnull[i] || desc->attrs[i].attisdropped)
            continue;

        thisState = myState->myinfo + i;
        binval = slot->tts_values[i];
        item = GetItemByTypeOid(desc->attrs[i].atttypid, false);
        append_data_by_mysql_type(item, binval, buf, thisState);
    }

    pfree(null_bitmap);
}

static bool
my_printtup(TupleTableSlot *slot, DestReceiver *self)
{
    DR_printtup     *myState = (DR_printtup *)self;
    TupleDesc        typeinfo = slot->tts_tupleDescriptor;
    StringInfo       buf = makeStringInfo();
    int              natts = typeinfo->natts;
    MemoryContext    oldcontext;

    /* Set or update my derived attribute info, if needed */
    if (myState->attrinfo != typeinfo || myState->nattrs != natts)
        printtup_prepare_info(myState, typeinfo, natts);

    /* Make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

    oldcontext = MemoryContextSwitchTo(myState->tmpcontext);

    if (current_packet_type == COM_STMT_EXECUTE ||
        current_packet_type == COM_STMT_FETCH)
        send_binaryproto(slot, myState, natts, buf);
    else
        send_textproto(slot, myState, natts, buf);

    mq_putmessage(buf->data, buf->len);

    DestroyStringInfo(buf);

    MemoryContextSwitchTo(oldcontext);
    MemoryContextReset(myState->tmpcontext);

    return true;
}

static void
my_printtup_startup(DestReceiver *self, int operation, TupleDesc typeinfo)
{
    DR_printtup *myState = (DR_printtup *)self;
    Portal		 portal = myState->portal;

    myState->tmpcontext = AllocSetContextCreate(CurrentMemoryContext,
												"mysql printtup",
												ALLOCSET_DEFAULT_SIZES);

    /**
     * Row description message will be sent in COM_STMT_EXECUTE's
     * reponse when open the cursor.
     */
    if (current_packet_type == COM_STMT_FETCH)
        return;

    /*
     * If we are supposed to emit row descriptions, then send the tuple
     * descriptor of the tuples.
     */
    if (myState->sendDescrip)
    {
        if (current_packet_type == COM_STMT_EXECUTE)
            binary_row_description_message(typeinfo, FetchPortalTargetList(portal));
        else
            text_row_description_message(typeinfo, FetchPortalTargetList(portal));
    }
}

/* ----------------
 *		printtup_shutdown
 * ----------------
 */
static void
my_printtup_shutdown(DestReceiver *self)
{
    DR_printtup *myState = (DR_printtup *)self;

    if (myState->myinfo != NULL)
        pfree(myState->myinfo);

    if (myState->tmpcontext)
		MemoryContextDelete(myState->tmpcontext);

    myState->myinfo = NULL;
    myState->attrinfo = NULL;
	myState->tmpcontext = NULL;
}

/* ----------------
 *		printtup_destroy
 * ----------------
 */
static void
my_printtup_destroy(DestReceiver *self)
{
    pfree(self);
}

DestReceiver *
mysql_printtup_create_DR(CommandDest dest)
{
    DR_printtup *self = (DR_printtup *) palloc0(sizeof(DR_printtup));

    self->pub.receiveSlot = my_printtup;
    self->pub.rStartup = my_printtup_startup;
    self->pub.rShutdown = my_printtup_shutdown;
    self->pub.rDestroy = my_printtup_destroy;
    self->pub.mydest = dest;

    self->sendDescrip = (dest == DestRemote || dest == DestRemoteExecute);

    self->attrinfo = NULL;
    self->nattrs = 0;
    self->myinfo = NULL;
	self->tmpcontext = NULL;

    return (DestReceiver *)self;
}

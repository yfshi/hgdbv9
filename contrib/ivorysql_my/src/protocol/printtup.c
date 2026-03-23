#include "postgres.h"

#include "access/printtup.h"
#include "c.h"
#include "tcop/pquery.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/lsyscache.h"
#include "utils/memdebug.h"
#include "utils/memutils.h"
#include "utils/varbit.h"

#include "../include/ivorysql_my.h"
#include "../include/message.h"
#include "../include/server.h"
#include "../include/printtup.h"
#include "../include/mqformat.h"

static mysql_column_definition_t *
make_column_definition_with_attr(FormData_pg_attribute *attr)
{
    // FIELD packet
    mysql_column_definition_t *field = (mysql_column_definition_t *) palloc0(sizeof(mysql_column_definition_t));

    const TypeItem *item = GetItemByTypeOid(attr->atttypid, false);

    field->name = attr->attname.data;

    switch (item->mysql_type_id)
    {
        // since mysql-jdbc 5.1.47 don't support enum and set.
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_SET:
            field->type = MYSQL_TYPE_STRING;
            break;
        default:
            field->type = item->mysql_type_id;
            break;
    }

    field->flags = item->flags;
    field->charset = item->charset_flag;

    if (attr->atttypmod != -1)
        field->length = attr->atttypmod;
    else
        field->length = DEFAULLT_DATA_FIELD_LEN;

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
            outputstr = OutputFunctionCall(&thisState->finfo, attr);
            mq_append_string_lenenc(buf, outputstr, -1);
        }
    }
}

static void
append_data_by_mysql_type(const TypeItem *item, Datum binval, StringInfo buf, PrinttupAttrInfo *thisState)
{
    switch (item->mysql_type_id)
    {
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
            {
                int32 val = DatumGetInt32(binval);
                mq_append_int4(buf, val);
                break;
            }
        case MYSQL_TYPE_LONGLONG:
            {
                int64 val = DatumGetInt64(binval);
                mq_append_int8(buf, val);
                break;
            }
        case MYSQL_TYPE_SHORT:
            {
                int16 val = DatumGetInt16(binval);
                mq_append_int2(buf, val);
                break;
            }
        case MYSQL_TYPE_YEAR:
            {
                #define MIN_YEAR_NUM 1901
                #define YearADT_to_Year(x) ((x) == 0 ? 0 : (int32)(x) + MIN_YEAR_NUM - 1)

                int16 year = DatumGetInt16(binval);
                int16 val = year >= 0 ? YearADT_to_Year(year) : YearADT_to_Year(-year) % 100;
                mq_append_int2(buf, val);
                break;
            }
        case MYSQL_TYPE_TINY:
            {
                int8 val = DatumGetInt8(binval);
                mq_append_int1(buf, val);
                break;
            }
        case MYSQL_TYPE_DOUBLE:
            {
                int64 num = DatumGetInt64(binval);
                mq_append_int8(buf, num);
                break;
            }
        case MYSQL_TYPE_FLOAT:
            {
                int32 num = DatumGetInt32(binval);
                mq_append_int4(buf, num);
                break;
            }
        case MYSQL_TYPE_STRING:
            {
                char *val;

                /* since all MySQL unknown type are map to MYSQL_TYPE_STRING, we use type out func here */
                if (thisState == NULL)
                {
                    /* should not happen */
                    ereport(ERROR,
                            (errcode(ERRCODE_PROTOCOL_VIOLATION),
                             errmsg("There is no output function for type: %s(%u)", item->typname, item->oid)));
                }

                val = OutputFunctionCall(&thisState->finfo, binval);
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_JSON:
            {
                char *val = TextDatumGetCString(binval);
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_TINY_BLOB:
            {
                char *val;
                bytea_output = BYTEA_OUTPUT_ESCAPE;
                val = DatumGetCString(DirectFunctionCall1(byteaout, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_BIT:
            {
                char *val = DatumGetCString(DirectFunctionCall1(bit_out, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_GEOMETRY:
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
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            {
                char *val = DatumGetCString(DirectFunctionCall1(numeric_out, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_ENUM:
            {
                char *val = DatumGetCString(DirectFunctionCall1(enum_out, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                break;
            }
        case MYSQL_TYPE_SET:
            {
                /* TO DO
                char *val = DatumGetCString(DirectFunctionCall1(set_out, binval));
                mq_append_string_lenenc(buf, val, -1);
                pfree(val);
                */
                break;
            }
        case MYSQL_TYPE_DATE:
            {
                struct pg_tm tt, *tm = &tt;
                DateADT date = DatumGetDateADT(binval);
                j2date(date + POSTGRES_EPOCH_JDATE, &(tm->tm_year), &(tm->tm_mon), &(tm->tm_mday));
                mq_append_int1(buf, 0x04);
                mq_append_int2(buf, tm->tm_year);
                mq_append_int1(buf, tm->tm_mon);
                
                break;
            }
        case MYSQL_TYPE_TIMESTAMP:
            {
                int tz;
                fsec_t fsec;
                struct pg_tm tt, *tm = &tt;
                const char *tzn = NULL;
                TimestampTz val = DatumGetTimestampTz(binval);

                timestamp2tm(val, &tz, tm, &fsec, &tzn, NULL);

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
        case MYSQL_TYPE_DATETIME:
            {
                Timestamp val = DatumGetTimestamp(binval);
                struct pg_tm tt, *tm = &tt;
                fsec_t fsec;
                timestamp2tm(val, NULL, tm, &fsec, NULL, NULL);
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
        case MYSQL_TYPE_TIME:
            {
                TimeADT val = DatumGetTimeADT(binval);
                struct pg_tm tt, *tm = &tt;
                fsec_t fsec;
                time2tm(val, tm, &fsec);

                mq_append_int1(buf, 0x0c);
                val > 0 ? mq_append_int1(buf, 0x00) : mq_append_int1(buf, 0x01);
                mq_append_int4(buf, 0x00);
                mq_append_int1(buf, tm->tm_hour);
                mq_append_int1(buf, tm->tm_min);
                mq_append_int1(buf, tm->tm_sec);
                mq_append_int4(buf, fsec);
                break;
            }
        case MYSQL_TYPE_NULL:
            break;
        default:
            break;
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

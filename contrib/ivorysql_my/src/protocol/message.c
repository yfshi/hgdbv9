#include "postgres.h"

#include "commands/prepare.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "libpq/libpq.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "optimizer/optimizer.h"
#include "parser/parse_type.h"
#include "parser/parse_coerce.h"
#include "storage/ipc.h"
#include "tcop/pquery.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"

#include "server.h"
#include "message.h"
#include "mqformat.h"

#define ERR_PACKET_SQLSTATE_LEN             5
#define MAX_ERRMSG_LEN                      512
#define PROTO_DATE_LEN                      4
#define PROTO_DATETIME_LEN                  7
#define PROTO_DATETIMESTAMP_LEN             11
#define PROTO_TIME_LEN                      8
#define PROTE_TIMESTATMP_LEN                12

#define MYSQL_STMT_NAME_FMT                 "mysql_stmt_%d"
#define MYSQL_PORTAL_NAME_FMT               "mysql_portal_%d"

static int statement_id = 0;

static void
send_err_packet(mysql_err_packet_t *err_packet)
{
    int errmsg_len;
    StringInfo buf = makeStringInfo();

    mq_append_int1(buf, 0xff); /* ERR */
    mq_append_int2(buf, err_packet->errcode); /* errorcode */

    if (has_client_capabilities(CLIENT_PROTOCOL_41))
    {
        mq_append_string_eof(buf, "#");
        if (err_packet->sqlstate && strlen(err_packet->sqlstate) > 0)
            mq_append_string_len(buf, err_packet->sqlstate, ERR_PACKET_SQLSTATE_LEN);
        else
            mq_append_string_len(buf, "07000", 5);
    }

    errmsg_len = err_packet->errmsg ? strlen(err_packet->errmsg) : 0;
    if (errmsg_len >= MAX_ERRMSG_LEN)
        errmsg_len = MAX_ERRMSG_LEN;

    if (errmsg_len > 0)
        mq_append_string_len(buf, err_packet->errmsg, errmsg_len);

    mq_putmessage(buf->data, buf->len);

    pq_flush();
    DestroyStringInfo(buf);
}

void
mysql_send_error_message(ErrorData *edata)
{
    mysql_err_packet_t *err_packet;

    if (edata->elevel < ERROR)
        return;

    err_packet = (mysql_err_packet_t *) palloc0(sizeof(mysql_err_packet_t));
    err_packet->errcode = edata->sqlerrcode;
    err_packet->errmsg = edata->message;
    err_packet->sqlstate = "HY000";

    send_err_packet(err_packet);

    pfree(err_packet);
}

void
mysql_send_ready_for_query(CommandDest dest)
{
    switch (dest)
    {
        case DestRemote:
		case DestRemoteExecute:
		case DestRemoteSimple:
            /* Flush output at end of cycle in any case. */
            pq_flush();
            break;
        case DestNone:
		case DestDebug:
		case DestSPI:
		case DestTuplestore:
		case DestIntoRel:
		case DestCopyOut:
		case DestSQLFunction:
		case DestTransientRel:
		case DestTupleQueue:
        default:
            break;
    }
}

int
mysql_read_command(StringInfo buf)
{
    return mq_getmessage(buf, 0);
}

void
net_send_ok(uint64 affected_rows, int16 warnings, int16 status_flag, uint64 last_insert_id, char *message, bool eof_identifier)
{
    StringInfo buf = makeStringInfo();

    if (eof_identifier && has_client_capabilities(CLIENT_DEPRECATE_EOF))
        mq_append_int1(buf, 0xfe);
    else
        mq_append_int1(buf, 0);

    mq_append_int_lenenc(buf, affected_rows);

    mq_append_int_lenenc(buf, last_insert_id);
 
    if (has_client_capabilities(CLIENT_PROTOCOL_41))
    {
        mq_append_int2(buf, status_flag);
        mq_append_int2(buf, warnings);
    }
    else if (has_client_capabilities(CLIENT_TRANSACTIONS))
    {
        mq_append_int2(buf, status_flag);
    }

    if (has_client_capabilities(CLIENT_SESSION_TRACK))
    {
        if (status_flag & SERVER_SESSION_STATE_CHANGED || (message && message[0]))
        {
            mq_append_string_lenenc(buf, message, strlen(message));
        }

        if (status_flag & SERVER_SESSION_STATE_CHANGED)
        {
            // session state information
            mq_append_string_lenenc(buf, "", 0);
        }
    }
    else if (message && message[0])
    {
        mq_append_string_eof(buf, message);
    }

    mq_putmessage(buf->data, buf->len);

    DestroyStringInfo(buf);
}

static void
net_send_eof(int16 warnings, int16 status)
{
    StringInfo buf = makeStringInfo();

    mq_append_int1(buf, 0xfe);

    if (has_client_capabilities(CLIENT_PROTOCOL_41))
    {
        mq_append_int2(buf, warnings);
        mq_append_int2(buf, status);
    }

    mq_putmessage(buf->data, buf->len);

    DestroyStringInfo(buf);
}

void
send_ok(uint64 affected_rows, int16 warnings, int16 server_status, uint64 last_insert_id, char *message)
{
    net_send_ok(affected_rows, warnings, server_status, last_insert_id, message, false);
}

void
send_eof(int16 warnings, int16 server_status)
{
    if (has_client_capabilities(CLIENT_DEPRECATE_EOF))
        net_send_ok(0, warnings, server_status, 0, NULL, true);
    else
        net_send_eof(warnings, server_status);
}

static void
send_com_stmt_prepare_ok_packet(int statementId, int columns, int params)
{
    StringInfo buf = makeStringInfo();

    mq_append_int1(buf, 0x00);
    mq_append_int4(buf, statementId);
    mq_append_int2(buf, columns);
    mq_append_int2(buf, params);
    mq_append_int1(buf, 0x00);

    // number of warnings
    mq_append_int2(buf, 0x00);

    if (has_client_capabilities(CLIENT_OPTIONAL_RESULTSET_METADATA))
    {
        // metadata_follow
        mq_append_int1(buf, RESULTSET_METADATA_FULL);
    }

    mq_putmessage(buf->data, buf->len);
}

void
mysql_end_command(const QueryCompletion *qc, CommandDest dest, bool force_undecorated_output)
{
    uint16      status = ServerStatus();
    uint64      nprocessed = qc->nprocessed;
    uint64      last_insert_id;
    CommandTag	tag = qc->commandTag;

    last_insert_id = DatumGetUInt64(DirectFunctionCall1(mys_get_lastval, (Datum) 0));
    
    if (current_packet_type == COM_STMT_CLOSE)
    {
        // no response packet is sent to the client
    }
    else if (current_packet_type == COM_STMT_PREPARE)
    {
        Query   *query;
        int     column_count;
        int     param_count;
        char    stmt_name[NAMEDATALEN];
        PreparedStatement *entry;
        
        snprintf(stmt_name, NAMEDATALEN, MYSQL_STMT_NAME_FMT, statement_id);

        entry = FetchPreparedStatement(stmt_name, true);
        query = (Query *) linitial(entry->plansource->query_list);

        column_count = list_length(query->targetList);
        param_count = entry->plansource->num_params;

        send_com_stmt_prepare_ok_packet(statement_id, column_count, param_count);
    }
    else if (tag == CMDTAG_SELECT    ||
             tag == CMDTAG_SHOW      ||
             tag == CMDTAG_EXPLAIN)
    {
        send_eof(0, status);
    }
    /* Begin - bug#7011 */
    else if (tag == CMDTAG_CALL)
    {
        if (nprocessed)
            send_eof(0, status);
        else
            send_ok(0, 0, status, last_insert_id, NULL);
    }
    /* End - bug#7011 */
    else
    {
        send_ok(nprocessed, 0, status, last_insert_id, NULL);
    }
}

static mysql_column_definition_t *
make_column_definition_with_name(char *name, char *tableName)
{
    mysql_column_definition_t *field = (mysql_column_definition_t *) palloc0(sizeof(mysql_column_definition_t));

    field->name = name;
    field->type = MYSQL_TYPE_VAR_STRING;
    field->length = DEFAULLT_DATA_FIELD_LEN;
    field->db = MyProcPort->database_name;
    field->table = tableName;
    field->org_table = tableName;
    field->charset = 0x2d;

    return field;
}

void
send_column_definition41_packet(mysql_column_definition_t *field)
{
    StringInfo buf = makeStringInfo();
    char *catalog = field->catalog ? field->catalog : "def";
    char *db = field->db ? field->db : "";
    char *table = field->table ? field->table : "";
    char *org_table = field->org_table ? field->org_table : "";
    const char *name = field->name ? field->name : "";
    char *org_name = field->org_name ? field->org_name : "";

    mq_append_string_lenenc(buf, catalog, strlen(catalog));
    mq_append_string_lenenc(buf, db, strlen(db));
    mq_append_string_lenenc(buf, table, strlen(table));
    mq_append_string_lenenc(buf, org_table, strlen(org_table));
    mq_append_string_lenenc(buf, name, strlen(name));
    mq_append_string_lenenc(buf, org_name, strlen(org_name));

    /* length of the following block, 12 byte */
    mq_append_int_lenenc(buf, 0x0c);            /* next_length (lenenc_int) */
    mq_append_int2(buf, field->charset);        /* character_set (2) */
    mq_append_int4(buf, field->length);         /* maximum length of the field (4) */
    mq_append_int1(buf, field->type);           /* column_type (1) */
    mq_append_int2(buf, field->flags);          /* flags (2) */
    mq_append_int1(buf, field->decimals);       /* decimals */
    mq_append_int2(buf, 0x00);                  /* filler */

    if (field->is_com_field_list)
        mq_append_string_lenenc(buf, field->default_value, -1);

    mq_putmessage(buf->data, buf->len);
    DestroyStringInfo(buf);
}

/* Begin - Bug#6356 */
static Datum
mys_evaluate_param(Datum value, Oid src_type, Oid dst_type)
{
    Type    type = typeidType(src_type);
    bool    typebyval = typeByVal(type);
    int16   typlen = typeLen(type);

    Const *con = makeConst(src_type,
                           -1,         /* typmod -1 is OK for all cases */
                           InvalidOid, /* all cases are uncollatable types */
                           typlen,
                           value,
                           false,
                           typebyval);

    Node *node = coerce_type(NULL, (Node *) con, src_type, dst_type, -1, COERCION_EXPLICIT, COERCE_EXPLICIT_CAST, -1);

    node = eval_const_expressions(NULL, node);

    Assert(IsA(node, Const));

    ReleaseSysCache(type);

    return ((Const *) node)->constvalue;
}
/* End - Bug#6356 */

static void
parse_one_param_value(StringInfo buf, uint32 flag, ParamExternData *param)
{
    #define SIGNED_BIT (1 << 15)

    Datum   value = 0;
    int     is_unsigned = flag & SIGNED_BIT;
    enum    enum_field_types type = flag & ~SIGNED_BIT;
    Oid     param_type = param->ptype;
    Oid     src_type = InvalidOid;

    switch(type)
    {
        case MYSQL_TYPE_NULL:
            value = (Datum) 0;
            param->isnull = true;
            break;
        case MYSQL_TYPE_TINY:
        {
            uint8 v;
            mq_get_int1(buf, &v);

            if (is_unsigned)
            {
                value = UInt8GetDatum(v);
                src_type = UINT1OID;
            }
            else
            {
                value = Int8GetDatum(v);
                src_type = TINYINTOID;
            }

            break;
        }
        case MYSQL_TYPE_SHORT:
        {
            uint32 v;
            mq_get_int2(buf, &v);

            if (is_unsigned)
            {
                value = UInt16GetDatum(v);
                src_type = UINT2OID;
            }
            else
            {
                value = Int16GetDatum(v);
                src_type = INT2OID;
            }

            break;
        }
        case MYSQL_TYPE_LONG:
        {
            uint32 v;
            mq_get_int4(buf, &v);

            if (is_unsigned)
            {
                value = UInt32GetDatum(v);
                src_type = UINT4OID;
            }
            else
            {
                value = Int32GetDatum(v);
                src_type = INT4OID;
            }

            break;
        }
        case MYSQL_TYPE_LONGLONG:
        {
            uint64 v;
            mq_get_int8(buf, &v);

            if (is_unsigned)
            {
                value = UInt64GetDatum(v);
                src_type = UINT8OID;
            }
            else
            {
                value = Int64GetDatum(v);
                src_type = INT8OID;
            }
            
            break;
        }
        case MYSQL_TYPE_FLOAT:
        {
            uint32 v;
            mq_get_int4(buf, &v);
            src_type = FLOAT4OID;
            value = Float4GetDatum((float4) v);
            break;
        }
        case MYSQL_TYPE_DOUBLE:
        {
            uint64 v;
            mq_get_int8(buf, &v);
            src_type = FLOAT8OID;
            value = Float8GetDatum((float8) v);
            break;
        }
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
        {
            char *str = mq_get_string_lenenc(buf, NULL, NULL);
            value = DirectFunctionCall3(numeric_in,
                                        CStringGetDatum(str),
                                        ObjectIdGetDatum(InvalidOid),
                                        Int32GetDatum(-1));
            src_type = NUMERICOID;
            break;
        }
        case MYSQL_TYPE_TIME:
            {
                uint8 len;
                uint8 negative;
                struct pg_tm tt, *tm = &tt;

                memset(tm, 0, sizeof(struct pg_tm));

                mq_get_int1(buf, &len);

                if (len == PROTO_TIME_LEN)
                {
                    TimeADT	time;

                    mq_get_int1(buf, &negative);
                    mq_get_int4(buf, (uint32 *) &tm->tm_mday);
                    mq_get_int1(buf, (uint8 *) &tm->tm_hour);
                    mq_get_int1(buf, (uint8 *) &tm->tm_min);
                    mq_get_int1(buf, (uint8 *)&tm->tm_sec);

                    tm->tm_hour += tm->tm_mday * HOURS_PER_DAY;

                    time = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

                    value = TimeADTGetDatum(time);
                    src_type = MYSTIMEOID;
                }
                else if (len == PROTE_TIMESTATMP_LEN)
                {
                    fsec_t  fsec;
                    char    str[128];
                    Oid		typinput;
                    Oid		typioparam;

                    mq_get_int1(buf, (uint8 *) &negative);
                    mq_get_int4(buf, (uint32 *) &tm->tm_mday);
                    mq_get_int1(buf, (uint8 *) &tm->tm_hour);
                    mq_get_int1(buf, (uint8 *) &tm->tm_min);
                    mq_get_int1(buf, (uint8 *) &tm->tm_sec);
                    mq_get_int4(buf, (uint32 *) &fsec);

                    tm->tm_hour += tm->tm_mday * HOURS_PER_DAY;

                    if (tm->tm_hour > 838)
                    {
                        tm->tm_hour = 838;
                        tm->tm_min = 59;
                        tm->tm_sec = 59;
                    }

                    snprintf(str, sizeof(str), "%s%d:%d:%d.%u", negative ? "-" : "",
                             tm->tm_hour, tm->tm_min, tm->tm_sec, fsec);

                    getTypeInputInfo(param_type, &typinput, &typioparam);

                    value = OidInputFunctionCall(typinput, str, typioparam, -1);
                    src_type = MYSTIMESOID;
                }

                break;
            }
        case MYSQL_TYPE_DATE:
        {
            DateADT		date;
            struct pg_tm tt, *tm = &tt;

            memset(tm, 0, sizeof(struct pg_tm));

            mq_get_int2(buf, (uint32 *) &tm->tm_year);
            mq_get_int1(buf, (uint8 *) &tm->tm_mon);
            mq_get_int1(buf, (uint8 *) &tm->tm_mday);
            
            date = tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday;

            value = DateADTGetDatum(date);
            src_type = MYSDATEOID;
            break;
        }
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
            {
                uint8   len;
                struct pg_tm tt, *tm = &tt;

                memset(tm, 0, sizeof(struct pg_tm));

                mq_get_int1(buf, &len);

                if (len == PROTO_DATETIME_LEN)
                {
                    TimeADT		datetime;

                    mq_get_int2(buf, (uint32 *) &tm->tm_year);
                    mq_get_int1(buf, (uint8 *) &tm->tm_mon);
                    mq_get_int1(buf, (uint8 *) &tm->tm_mday);
                    mq_get_int1(buf, (uint8 *) &tm->tm_hour);
                    mq_get_int1(buf, (uint8 *) &tm->tm_min);
                    mq_get_int1(buf, (uint8 *) &tm->tm_sec);
                    
                    datetime = (tm->tm_year * 10000 + tm->tm_mon * 100 + tm->tm_mday) * 1000000 + 
                                tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;

                    value = TimeADTGetDatum(datetime);
                    src_type = MYSDATETIMEOID;
                }
                else if (len == PROTO_DATETIMESTAMP_LEN)
                {
                    fsec_t  fsec;
                    char    str[512];
                    Oid		typinput;
                    Oid		typioparam;

                    mq_get_int2(buf, (uint32 *) &tm->tm_year);
                    mq_get_int1(buf, (uint8 *) &tm->tm_mon);
                    mq_get_int1(buf, (uint8 *) &tm->tm_mday);
                    mq_get_int1(buf, (uint8 *) &tm->tm_hour);
                    mq_get_int1(buf, (uint8 *) &tm->tm_min);
                    mq_get_int1(buf, (uint8 *) &tm->tm_sec);
                    mq_get_int4(buf, (uint32 *) &fsec);

                    snprintf(str, sizeof(str), "%d-%d-%d %d:%d:%d.%u",
                             tm->tm_year, tm->tm_mon, tm->tm_mday,
                             tm->tm_hour, tm->tm_min, tm->tm_sec, fsec);

                    getTypeInputInfo(param_type, &typinput, &typioparam);
                    
                    value = OidInputFunctionCall(typinput, str, typioparam, -1);
                    src_type = param_type;
                }

                break;
            }
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        {
            // Data send with COM_SEND_LONG_DATA
            return;
        }
        default: // string
        {
            Oid		typinput;
            Oid		typioparam;
            char    *str = mq_get_string_lenenc(buf, NULL, NULL);

            getTypeInputInfo(param_type, &typinput, &typioparam);
                    
            value = OidInputFunctionCall(typinput, str, typioparam, -1);
            
            src_type = param_type;
            break;
        }
    }

    /**
     * Begin - Bug#6356
     * evaluate current parameter
     */
    if (OidIsValid(src_type) && OidIsValid(param_type) && src_type != param_type)
    {
        value = mys_evaluate_param(value, src_type, param_type);
    }
    /* End - Bug#6356 */

    param->value = value;
}

static void
parse_com_stmt_execute_params(StringInfo buf, PreparedStatement *entry, com_stmt_exec_request_t *req)
{
    int num_params = entry->plansource->num_params;
    int param_count = num_params;

    if (num_params == 0)
        return;

    if (has_client_capabilities(CLIENT_QUERY_ATTRIBUTES))
    {
        uint32 size;
        param_count = mq_get_int_lenenc(buf, &size);
    }

    if (param_count > 0)
    {
        char                *null_bitmap;
        uint8               new_params_bind_flag;
        int                 len = (param_count + 7) / 8;
        ParamListInfo       paramLI = req->paramLI; // from portal
        ParamExternData     *params = paramLI->params;
        int                 *flags = entry->plansource->param_type_and_flags;

        null_bitmap = mq_get_string_len(buf, len);
        
        mq_get_int1(buf, &new_params_bind_flag);

        if (new_params_bind_flag)
        {
            for (int i = 0; i < param_count; i++)
            {
                // parameter type
                mq_get_int2(buf, (uint32 *) &flags[i]);

                // parameter name
                if (has_client_capabilities(CLIENT_QUERY_ATTRIBUTES))
                {
                    mq_get_string_lenenc(buf, NULL, NULL);
                }
            }
        }

        /* recv parameter value */
        for (int i = 0; i < param_count; i++)
        {
            params[i].isnull = null_bitmap[i / 8] & (1 << (i & 7));

            if (params[i].isnull) 
                continue;

            parse_one_param_value(buf, flags[i], &params[i]);
        }

        req->null_bitmap = null_bitmap;
        req->new_params_bind_flag = new_params_bind_flag;
        req->param_count = param_count;
    }
}

static char *
parse_com_query_request(StringInfo buf)
{
    char    *sql;
    uint32  size;
    char    *null_bitmap;
    uint32  *flags = NULL;
    uint64  param_count = 0;
    uint8   new_params_bind_flag;
    ParamListInfo paramLI = NULL;
    ParamExternData *params = NULL;
    
    if (has_client_capabilities(CLIENT_QUERY_ATTRIBUTES))
    {
        // paramter count
        param_count = mq_get_int_lenenc(buf, &size);

        // parameter set count, always 1
        mq_get_int_lenenc(buf, &size);
    
        if (param_count > 0)
        {
            int len = (param_count + 7) / 8;
            
            flags = palloc0(param_count * sizeof(uint32));
            paramLI = makeParamList(param_count);
            params = paramLI->params;

            null_bitmap = mq_get_string_len(buf, len);

            mq_get_int1(buf, &new_params_bind_flag); // always 1

            for (int i = 0; i < param_count; i++)
            {
                char *param_name = NULL;

                // param_type_and_flag
                mq_get_int2(buf, &flags[i]);

                // parameter name
                param_name = mq_get_string_lenenc(buf, NULL, NULL);

                pfree(param_name);
            }

            for (int i = 0; i < param_count; i++)
            {
                parse_one_param_value(buf, flags[i], &params[i]);
            }

            // just free query attribute
            pfree(flags);
            pfree(paramLI);
            pfree(null_bitmap);
        }
    }

    sql = mq_get_string_eof(buf, NULL);
    
    return sql;
}

static void
execute_com_field_list(char *tableName)
{
    int rc;
    char sql[512];

    /* Begin - bug#6850 */
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    /* End - bug#6850 */

    if ((rc = SPI_connect()) != SPI_OK_CONNECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                        errmsg("mysql SPI_connect failed: %s", SPI_result_code_string(rc)),
                        errdetail("SPI_connect failed")));
    }

    PushActiveSnapshot(GetTransactionSnapshot()); /* bug#6850 */

    snprintf(sql, sizeof(sql), "show columns from %s", tableName);

    rc = SPI_execute(sql, false, 0);

    if (rc == SPI_OK_SELECT)
    {
        /* Begin - bug#6850 */
        if (SPI_processed == 0)
        {
            ereport(ERROR, errmsg("Table \'%s\' doesn't exist", tableName));
        }
        /* End - bug#6850 */

        if (SPI_processed > 0)
        {
            TupleDesc spi_tupdesc = SPI_tuptable->tupdesc;
            for (uint32 i = 0; i < SPI_processed; i++)
            {
                HeapTuple spi_tuple = SPI_tuptable->vals[i];
                char *name = SPI_getvalue(spi_tuple, spi_tupdesc, SPI_fnumber(spi_tupdesc, "Field"));
                char *default_value = SPI_getvalue(spi_tuple, spi_tupdesc, SPI_fnumber(spi_tupdesc, "Default"));            
                mysql_column_definition_t *field = make_column_definition_with_name(name, tableName);

                field->default_value = default_value;
                field->is_com_field_list = true;

                send_column_definition41_packet(field);

                pfree(field);
            }

            send_eof(0, 0);
        }
    }

    SPI_finish();

    CommitTransactionCommand();
}

static Portal
prepare_portal_if_not_exist(int statement_id)
{
    Portal      portal;
    CachedPlan  *cplan;
    List        *plan_list;
    char	    *query_string;
    int         param_count;
    MemoryContext oldContext;
    PreparedStatement *entry;
    ParamListInfo paramLI = NULL;
    char        stmt_name[NAMEDATALEN];
    char        portal_name[NAMEDATALEN];

    snprintf(stmt_name, NAMEDATALEN, MYSQL_STMT_NAME_FMT, statement_id);
    snprintf(portal_name, NAMEDATALEN, MYSQL_PORTAL_NAME_FMT, statement_id);

    portal = GetPortalByName(portal_name);

    if (PortalIsValid(portal))
        return portal;

    start_xact_command();
    
    entry = FetchPreparedStatement(stmt_name, true);

    param_count = entry->plansource->num_params;

    // make a portal and cached it
    portal = CreatePortal(portal_name, false, false);

    oldContext = MemoryContextSwitchTo(portal->portalContext);

    query_string = pstrdup(entry->plansource->query_string);

    MemoryContextSwitchTo(oldContext);

    cplan = GetCachedPlan(entry->plansource, paramLI, NULL, NULL);
	plan_list = cplan->stmt_list;

    // make empty ParamListInfo, and fill it in COM_STMT_EXECUTE or COM_STMT_SEND_LONG_DATA
    paramLI = makeParamList(param_count);

    for (int i = 0; i < param_count; i++)
    {
        ParamExternData *prm = &paramLI->params[i];
        prm->ptype = entry->plansource->param_types[i];
		prm->pflags = PARAM_FLAG_CONST;
        prm->value = (Datum) 0;
    }

    PortalDefineQuery(portal,
                      pstrdup(stmt_name),
                      query_string,
                      entry->plansource->commandTag,
                      plan_list,
                      cplan);
    
    PortalStart(portal, paramLI, 0, InvalidSnapshot);

    portal = GetPortalByName(portal_name);

    Assert(portal != NULL);

    return portal;
}

static int
execute_com_stmt_prepare(const char *client_sql)
{
    StringInfo  sql;
    Query       *query;
    int         param_count;
    int         column_count;
    PreparedStatement *entry;
    char        stmt_name[NAMEDATALEN];
    
    sql = makeStringInfo();
    statement_id += 1;

    snprintf(stmt_name, NAMEDATALEN, MYSQL_STMT_NAME_FMT, statement_id);

    appendStringInfo(sql, "PREPARE %s AS %s", stmt_name, client_sql);

    /* Set statement_timestamp() */
    SetCurrentStatementStartTimestamp();
    
    exec_simple_query(sql->data);

    DestroyStringInfo(sql);

    entry = FetchPreparedStatement(stmt_name, true);
    query = (Query *) linitial(entry->plansource->query_list);

    column_count = list_length(query->targetList);
    param_count = entry->plansource->num_params;

    for (int i = 0; i < param_count; i++)
    {
        const TypeItem *item;
        mysql_column_definition_t *param_field = make_column_definition_with_name("?", NULL);
        item = GetItemByTypeOid(entry->plansource->param_types[i], false);
        param_field->type = item->mysql_type_id;
        param_field->charset = item->charset_flag;
        send_column_definition41_packet(param_field);
    }

    if (!has_client_capabilities(CLIENT_DEPRECATE_EOF))
        send_eof(0, 0);

    for (int i = 0; i < column_count; i++)
    {
        mysql_column_definition_t *column_field = make_column_definition_with_name("", NULL);
        send_column_definition41_packet(column_field);
    }

    if (!has_client_capabilities(CLIENT_DEPRECATE_EOF))
        send_eof(0, 0);

    return 0;
}

static com_stmt_exec_request_t *
parse_com_stmt_exec_request(StringInfo buf)
{
    Portal		        portal;
    PreparedStatement   *entry;
    char                stmt_name[NAMEDATALEN];
    com_stmt_exec_request_t *req;
    
    req = (com_stmt_exec_request_t *) palloc0(sizeof(com_stmt_exec_request_t));

    mq_get_int4(buf, &req->statement_id);
    mq_get_int1(buf, &req->open_cursor);
    mq_get_int4(buf, &req->iteration_count);    // always 1.

    snprintf(stmt_name, NAMEDATALEN, MYSQL_STMT_NAME_FMT, req->statement_id);

    portal = prepare_portal_if_not_exist(req->statement_id);
    entry = FetchPreparedStatement(stmt_name, true);

    if (entry == NULL ||
        (entry->plansource->num_params == 0 &&
         (!has_client_capabilities(CLIENT_QUERY_ATTRIBUTES) ||
          !(req->open_cursor & PARAMETER_COUNT_AVAILABLE))))
    {
        return req;
    }

    req->paramLI = portal->portalParams;

    parse_com_stmt_execute_params(buf, entry, req);

    return req;
}

static int
execute_prepare_stmt(com_stmt_exec_request_t *req)
{
    Portal      portal;
    char        portal_name[NAMEDATALEN];

    snprintf(portal_name, NAMEDATALEN, MYSQL_PORTAL_NAME_FMT, req->statement_id);

    portal = prepare_portal_if_not_exist(req->statement_id);

    if (!PortalIsValid(portal))
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_CURSOR),
                 errmsg("portal \"%s\" does not exist", portal_name)));

    /* Set statement_timestamp() */
    SetCurrentStatementStartTimestamp();
    
    exec_execute_message(portal_name, -1);

    /**
     * Finish current transaction and free portal malloced in COM_STMT_PREPARE.
     * It's same as pg's 'E' packet.
     */
    finish_xact_command();

    return 0;
}

static void
recompose_blob_param(ParamExternData *param, char *data, int len)
{
    bytea   *ret;
    int      bc;
    
    if (param->value == (Datum) 0)
    {
        bc = len + VARHDRSZ;
        ret = palloc(bc);
        memcpy(VARDATA(ret), data, len);
    }
    else
    {
        bytea   *old = DatumGetByteaPP(param->value);
        char    *olddata = VARDATA_ANY(old);
        int     oldlen = VARSIZE_ANY_EXHDR(old);
        
        bc = oldlen + len + VARHDRSZ;

        ret = palloc(bc);
        
        memcpy(VARDATA(ret), olddata, oldlen);
        memcpy(VARDATA(ret) + oldlen, data, len);
        
        pfree(old);
    }
	
    SET_VARSIZE(ret, bc);

    param->value = PointerGetDatum(ret);
}

static void
read_send_long_data_request(StringInfo buf)
{
    Portal  portal;
    char    *payload;
    uint32  param_id;
    uint32  statement_id;
    int32   payloadlen = 0;
    ParamExternData *param;
    MemoryContext oldContext;
    ParamListInfo   paramLI = NULL;
    char    portal_name[NAMEDATALEN];

    mq_get_int4(buf, &statement_id);
    mq_get_int2(buf, &param_id);
    payload = mq_get_string_eof(buf, &payloadlen);

    snprintf(portal_name, NAMEDATALEN, MYSQL_PORTAL_NAME_FMT, statement_id);

    portal = prepare_portal_if_not_exist(statement_id);

    if (!PortalIsValid(portal))
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_CURSOR),
                 errmsg("portal \"%s\" does not exist", portal_name)));

    paramLI = portal->portalParams;
    param = &paramLI->params[param_id];

    oldContext = MemoryContextSwitchTo(portal->portalContext);

    recompose_blob_param(param, payload, payloadlen);

    MemoryContextSwitchTo(oldContext);

    pfree(payload);
}

static void
execute_com_stmt_reset(StringInfo buf)
{
    uint32  statement_id;
    uint16  status = ServerStatus();
 
    mq_get_int4(buf, &statement_id);

    send_ok(0, 0, status, 0, NULL);
}

static void
execute_com_stmt_close(StringInfo buf)
{
    uint32  statement_id;
    char    stmt_name[NAMEDATALEN];
    StringInfo sql = makeStringInfo();

    mq_get_int4(buf, &statement_id);

    snprintf(stmt_name, NAMEDATALEN, MYSQL_STMT_NAME_FMT, statement_id);

    appendStringInfo(sql, "DEALLOCATE %s", stmt_name);

    /* Set statement_timestamp() */
    SetCurrentStatementStartTimestamp();
    
    exec_simple_query(sql->data);

    DestroyStringInfo(sql);
}

int
mysql_process_command(StringInfo buf)
{
    current_packet_type = pq_getmsgbyte(buf);

    switch (current_packet_type)
    {
        case COM_PING:
            send_ok(0, 0, 0, 0, NULL);
            break;
        case COM_QUIT:
            proc_exit(0);
            break;
        case COM_INIT_DB:
            {
                char *schema = mq_get_string_eof(buf, NULL);
                StringInfo sql = makeStringInfo();
                
                appendStringInfo(sql, "use %s", schema);

                /* Set statement_timestamp() */
                SetCurrentStatementStartTimestamp();

                exec_simple_query(sql->data);
                
                DestroyStringInfo(sql);
                pfree(schema);

                break;
            }
        case COM_QUERY:
            {
                char *sql = parse_com_query_request(buf);

                /* Set statement_timestamp() */
                SetCurrentStatementStartTimestamp();

                exec_simple_query(sql);
                pfree(sql);

                break;
            }
        case COM_FIELD_LIST:
            {
                char *table_name = mq_get_string_null(buf);
                execute_com_field_list(table_name);
                pfree(table_name);
                break;
            }
        case COM_STMT_PREPARE:
            {
                char *sql = mq_get_string_eof(buf, NULL);
                execute_com_stmt_prepare(sql);
                pfree(sql);
                break;
            }
        case COM_STMT_EXECUTE:
            {
                com_stmt_exec_request_t *req;
                req = parse_com_stmt_exec_request(buf);
                execute_prepare_stmt(req);
                break;
            }
        case COM_STMT_SEND_LONG_DATA:
            read_send_long_data_request(buf);
            break;
        case COM_STMT_RESET:
            execute_com_stmt_reset(buf);
            break;
        case COM_STMT_CLOSE:
            execute_com_stmt_close(buf);
            break;
        default:
            // COM_CREATE_DB, COM_DROP_DB have deprecated by mysql-server
            ereport(ERROR,
                    (errcode(ERRCODE_INTERNAL_ERROR),
                     errmsg("mysql server protocol not supported.")));
    }

    return 0;
}

void
send_auth_switch_packet(char *method, uint8 *scramble)
{
    StringInfo buf = makeStringInfo();

    mq_append_int1(buf, 0xfe);
    mq_append_string_null(buf, method);
    mq_append_string_eof(buf, (char *)scramble);
    mq_append_int1(buf, 0x00);
    mq_putmessage(buf->data, buf->len);
}

char *
read_switch_response(StringInfo buf)
{
	return mq_get_string_eof(buf, NULL);
}

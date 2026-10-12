#ifndef MYS_MQFORMAT_H
#define MYS_MQFORMAT_H
#include "postgres.h"

#include <assert.h>

#include "lib/stringinfo.h"
#include "libpq/pqcomm.h"
#include "libpq/pqformat.h"

#define MAX_LEN_ENCODE_BYTE     251			// 0xfb
#define MAX_LEN_ENCODE_2_BYTE   65536		// POW(2, 16)
#define MAX_LEN_ENCODE_3_BYTE   16777216 	// POW(2, 24)

#define NULL_STRING_MARK        251			// 0xfb

#define BYTE_1_LEN              1
#define BYTE_2_LEN              2
#define BYTE_3_LEN              3
#define BYTE_4_LEN              4
#define BYTE_8_LEN              8

#define PROTO_HEADER_LEN       4

#define SHIFT_BYTE_STEP         8
#define MAX_LEFT_SHIFT_BIT      32
#define MAX_LEFT_SHIFT_INT      4

extern void mq_putmessage(const char * packet, size_t len);
extern int mq_getmessage(StringInfo s);

static inline void
mq_append_int_len(StringInfo buf, uint64 num, int len)
{
    for (int i = 0; i < len; i++)
    {
        pq_sendbyte(buf, num & 0xff);
        num >>= 8;
    }
}

/* append a binary [u]int8 to a StringInfo buffer */
static inline void
mq_append_int1(StringInfo buf, uint8 i)
{
    mq_append_int_len(buf, i, BYTE_1_LEN);
}

/* append a binary [u]int16 to a StringInfo buffer */
static inline void
mq_append_int2(StringInfo buf, uint16 i)
{
    mq_append_int_len(buf, i, BYTE_2_LEN);
}

/* append a binary [u]int24 to a StringInfo buffer */
static inline void
mq_append_int3(StringInfo buf, uint32 i)
{
    mq_append_int_len(buf, i, BYTE_3_LEN);
}

/* append a binary [u]int32 to a StringInfo buffer */
static inline void
mq_append_int4(StringInfo buf, uint32 i)
{
    mq_append_int_len(buf, i, BYTE_4_LEN);
}

/* append a binary [u]int64 to a StringInfo buffer */
static inline void
mq_append_int8(StringInfo buf, uint64 i)
{
    mq_append_int_len(buf, i, BYTE_8_LEN);
}

#define mq_append_payload_len(buf, len) mq_append_int3(buf, len)
#define mq_append_sequence_id(buf, id)  mq_append_int1(buf, id)

static inline int
mq_append_int_lenenc(StringInfo buf, uint64 i)
{
    if (i < MAX_LEN_ENCODE_BYTE)
    {
        mq_append_int1(buf, i);
    }
    else if (i < MAX_LEN_ENCODE_2_BYTE)
    {
        mq_append_int1(buf, 0xfc);
        mq_append_int2(buf, i);
    }
    else if (i < MAX_LEN_ENCODE_3_BYTE)
    {
        mq_append_int1(buf, 0xfd);
        mq_append_int3(buf, i);
    }
    else
    {
        mq_append_int1(buf, 0xfe);
        mq_append_int8(buf, i);
    }

    return 0;
}

static inline void
mq_append_string_null(StringInfo buf, const char *data)
{
    appendBinaryStringInfoNT(buf, data, strlen(data) + 1);
}

static inline void
mq_append_string_len(StringInfo buf, const char *data, uint32 len)
{
    appendBinaryStringInfo(buf, data, len);
}

static inline void
mq_append_string_lenenc(StringInfo buf, const char *data, int dataLen)
{
    size_t len = data ? strlen(data) : 0;

    if (dataLen != -1)
        len = dataLen;

    if (!data)
    {
        mq_append_int1(buf, NULL_STRING_MARK);
    }
    else if (len == 0)
    {
        mq_append_int1(buf, 0x00);
    }
    else
    {
        mq_append_int_lenenc(buf, len);
        mq_append_string_len(buf, data, len);
    }
}

static inline void
mq_append_string_eof(StringInfo buf, const char *data)
{
    if (data == NULL)
        return;

    appendBinaryStringInfo(buf, data, strlen(data));
}

static inline void
mq_get_int_len(StringInfo buf, void *num, int len)
{
    int     i,
            shift;
    uint32  l = 0,
            h = 0;
    char    *data = buf->data + buf->cursor;

    if (len < 0 || len > (buf->len - buf->cursor))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROTOCOL_VIOLATION),
                 errmsg("insufficient data left in message")));
    }

    /**
     * for some reason left-shift > 32 leads to negative numbers
     */
    for (i = 0, shift = 0; i < len && i < MAX_LEFT_SHIFT_INT; i++, shift += SHIFT_BYTE_STEP)
    {
        l |= ((unsigned char) data[i] << shift);
    }

    for (shift = 0; i < len; i++, shift += SHIFT_BYTE_STEP)
    {
        h |= ((unsigned char) data[i] << shift);
    }

    if (h)
    {
        *((uint64 *) num) = l | ((uint64) h << MAX_LEFT_SHIFT_BIT);
    }
    else
    {
        *((uint32 *) num) = l;
    }

    buf->cursor += len;
}

static inline void
mq_get_int1(StringInfo buf, uint8 *num)
{
    *num = buf->data[buf->cursor++];
}

static inline void
mq_get_int2(StringInfo buf, uint32 *num)
{
    mq_get_int_len(buf, (void *) num, BYTE_2_LEN);
    *num = *num & 0xffff;
}

static inline void
mq_get_int3(StringInfo buf, uint32 *num)
{
    mq_get_int_len(buf, (void *) num, BYTE_3_LEN);
    *num = *num & 0xffffff;
}

static inline void
mq_get_int4(StringInfo buf, uint32 *num)
{
    mq_get_int_len(buf, (void *) num, BYTE_4_LEN);
}

static inline void
mq_get_int8(StringInfo buf, uint64 *num)
{
    *num = 0; /* bug#9241 */
    mq_get_int_len(buf, (void *) num, BYTE_8_LEN);
}

/*
 * Implentment in Mysql source code with
 * function net_field_length().
 */
static inline uint64 
mq_get_int_lenenc(StringInfo buf, uint32 *len)
{
    uint8   first;
    uint64  num;

    mq_get_int1(buf, &first);

    *len = 1;

    if (first < 0xfb)
        return first;
    
    if (first == 0xfb)
        return 0;

    *len = 2;

    if (first == 0xfc)
    {
        mq_get_int2(buf, (uint32 *) &num);
        return num;
    }

    *len = 3;

    if (first == 0xfd)
    {
        mq_get_int3(buf, (uint32 *) &num);
        return num;
    }
    
    assert(first == 0xfe);

    *len = 8;
    
    mq_get_int8(buf, &num);
    return num;
}

static inline void
mq_skip_bytes(StringInfo buf, int len)
{
    buf->cursor += len;
}

static inline char *
mq_get_string_len(StringInfo msg, int len)
{
    if (len < 0 || len > (msg->len - msg->cursor))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROTOCOL_VIOLATION),
                 errmsg("insufficient data left in message")));
    }

    if (len > 0)
    {
        char *buf = (char *) palloc0(len + 1);
        memcpy(buf, &msg->data[msg->cursor], len);
        buf[len] = 0x00;
        msg->cursor += len;
        return buf;
    }

    return NULL;
}

static inline char *
mq_get_string_null(StringInfo msg)
{
    /*
     * It's safe to use strlen() here because a StringInfo is guaranteed to
     * have a trailing null byte.  But check we found a null inside the
     * message.
     */
    int slen = strlen(&msg->data[msg->cursor]);

    return mq_get_string_len(msg, slen + 1);
}

static inline char *
mq_get_string_eof(StringInfo msg, int *len)
{
    if (len)
        *len = msg->len - msg->cursor;
    return mq_get_string_len(msg, msg->len - msg->cursor);
}

/**
 * string with length encode is composed of number and string.
 * argument length means the string length.
 * argument size means total length, consist of string length and number size.
 */
static inline char *
mq_get_string_lenenc(StringInfo msg, uint32 *length, uint32 *size)
{
    uint32  lenenc_len;
    uint64  str_len = mq_get_int_lenenc(msg, &lenenc_len);

    if (length)
        *length = str_len;

    if (size)
        *size = lenenc_len + str_len;

    // stringInfo only alloc int-length size memroy here,
    // mysql proto define the 8 byte int type, but not used so far.
    return mq_get_string_len(msg, str_len);
}
#endif
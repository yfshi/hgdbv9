#include "postgres.h"

#include "libpq/libpq.h"
#include "utils/guc.h"
#include "../include/server.h"
#include "../include/mqformat.h"

#define PACKET_MAX_LENGTH   0xffffff

static uint8 next_seq = 0;

static int
mq_send_one_packet(const char *data, size_t len, uint8 seq)
{
    StringInfoData buffer;
    StringInfo buf = &buffer;

    initStringInfo(buf);

    Assert(len <= PACKET_MAX_LENGTH);

    enlargeStringInfo(buf, PROTO_HEADER_LEN);

    mq_append_payload_len(buf, len);
    mq_append_sequence_id(buf, seq);

    if (internal_putbytes(buf->data, PROTO_HEADER_LEN))
        return EOF;

    if (internal_putbytes(data, len))
        return EOF;

    return STATUS_OK;
}

void
mq_putmessage(const char *data, size_t left)
{
    size_t  sent = 0;

    /**
     * If the length of the last packet sent is 0xffffff,
     * an additional packet with a length of 0 needs to be sent.
     */
    while (left || sent == PACKET_MAX_LENGTH)
    {
        if (left < PACKET_MAX_LENGTH)
            sent = left;
        else
            sent = PACKET_MAX_LENGTH;

        mq_send_one_packet(data, sent, next_seq++);

        data += sent;
        left -= sent;
    }
}

static int
mq_recv_packet_header(uint32 *len, uint8 *seq)
{
    StringInfoData  buffer;
    StringInfo      buf = &buffer;

    initStringInfo(buf);

    enlargeStringInfo(buf, PROTO_HEADER_LEN);

    if (pq_getbytes(buf->data, PROTO_HEADER_LEN) == EOF)
        return EOF;

    buf->len += PROTO_HEADER_LEN;

    mq_get_int3(buf, (void *) len);
    mq_get_int1(buf, (void *) seq);

    return 0;
}

static uint32
mq_recv_one_packet(StringInfo buf)
{
    uint32  len;
    uint8   seq;
    
    if (mq_recv_packet_header(&len, &seq) == EOF)
        return EOF;

    next_seq = seq + 1;

    /**
     * If the length of message is exactly 1, MySQL will
     * send a packet with a length of 0 to mark the completion
     * of the transmission.
     */
    if (len == 0)
        return 0;

    if (len > 0)
    {
        enlargeStringInfo(buf, len);

        pq_getbytes(buf->data + buf->len, len);

        buf->len += len;
    }

    return len;
}

int
mq_getmessage(StringInfo buf)
{
    uint32 len = PACKET_MAX_LENGTH;

    pq_startmsgread();

    resetStringInfo(buf);

    while (len == PACKET_MAX_LENGTH)
    {
        len = mq_recv_one_packet(buf);

        if (len == EOF)
            return EOF;
    }

    pq_endmsgread();

    return STATUS_OK;
}

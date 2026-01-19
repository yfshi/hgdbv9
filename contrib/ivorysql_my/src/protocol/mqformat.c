#include "postgres.h"

#include "libpq/libpq.h"
#include "mqformat.h"
#include "utils/guc.h"
#include "server.h"

int next_seqid = 0;

int
mq_putmessage(const char *packet, size_t len)
{
    StringInfo  header;
    uint16  status = ServerStatus();

    header = makeStringInfo();
    mq_append_payload_len(header, len);
    mq_append_sequence_id(header, next_seqid++);

	if (internal_putbytes(header->data, header->len))
    {
        status = EOF;
        goto fail;
    }

    if (internal_putbytes(packet, len))
    {
        status = EOF;
        goto fail;
    }

fail:
    DestroyStringInfo(header);
	return status;
}

int
mq_getmessage(StringInfo buf, uint32 maxlen)
{
    uint32  len;
    uint8   seq_id;

    pq_startmsgread();

    resetStringInfo(buf);

    enlargeStringInfo(buf, PROTO_PAYLOAD_LEN);

    // Read PROTO_PAYLOAD_LEN bytes from recvbuf
    if (pq_getbytes(buf->data, PROTO_PAYLOAD_LEN) == EOF)
    {
        return EOF;
    }

    buf->len = PROTO_PAYLOAD_LEN;

    // convert payload length bytes stream to uint32
    mq_get_payload_len(buf, &len);

    // read one byte sequence id
    seq_id = pq_getbyte();
    if (seq_id == EOF)
    {
        ereport(COMMERROR,
                (errcode(ERRCODE_PROTOCOL_VIOLATION),
                 errmsg("unexpected EOF within seq_id word")));
        return EOF;
    }

    next_seqid = ++seq_id;

    // Read rest of payload length bytes into buf
    if (len > 0)
    {
        PG_TRY();
        {
            enlargeStringInfo(buf, len);
        }
        PG_CATCH();
        {
            if (pq_discardbytes(len) == EOF)
            {
                ereport(COMMERROR,
                        (errcode(ERRCODE_PROTOCOL_VIOLATION),
                         errmsg("incomplete message from client")));
            }

            pq_endmsgread();
            PG_RE_THROW();
        }
        PG_END_TRY();

        // read playload_lenght bytes into buf->data
        if (pq_getbytes(buf->data, len) == EOF)
        {
            ereport(COMMERROR,
                    (errcode(ERRCODE_PROTOCOL_VIOLATION),
                     errmsg("incomplete message from client")));
            return EOF;
        }

        buf->len = len;
        buf->cursor = 0;
        buf->data[len] = '\0';
    }

    pq_endmsgread();

    return 0;
}
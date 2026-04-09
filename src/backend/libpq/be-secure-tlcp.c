/*-------------------------------------------------------------------------
 *
 * be-secure-tlcp.c
 *  TLCP encryption support
 *
 * Portions Copyright (c) 2018-2021, PostgreSQL Global Development Group
 *
 *  src/backend/libpq/be-secure-tlcp.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID:SRS-SECURITY-TLCP */

#include "pg_config.h"

#ifdef USE_GMSSL

#include "postgres.h"

#include <gmssl/mem.h>
#include <gmssl/sm2.h>
#include <gmssl/tls.h>
#include "utils/guc.h"
#include "libpq/be-secure-tlcp.h"
#include "utils/elog.h"

static TLS_CTX tlcp_ctx;
static TLS_CONNECT tlcp_conn;

static bool ssl_is_server_start;

static int
tlcp_get_pass(const char *tlcp_pass_file, bool isServerStart);

int
be_tlcp_init(bool isServerStart)
{
	int server_ciphers[] = { TLS_cipher_ecc_sm4_cbc_sm3, };
	int loglevel = isServerStart ? FATAL : LOG;

	memset(&tlcp_ctx, 0, sizeof(TLS_CTX));
	if (tlcp_get_pass(tlcp_pass_file, isServerStart) != 1)
	{
		ereport(loglevel,
				errmsg("get pass fail %s\n", tlcp_pass_file));
		return -1;
	}

	if (tls_ctx_init(&tlcp_ctx, TLS_protocol_tlcp, TLS_server_mode) != 1)
	{
		ereport(loglevel,
				(errmsg("could not create TLCP ")));
		return -1;
	}

	if (tls_ctx_set_cipher_suites(&tlcp_ctx, server_ciphers,
		sizeof(server_ciphers)/sizeof(int)) != 1)
	{
		ereport(loglevel,
				(errmsg("Set server cipher fail")));
		return -1;
	}

	if (tls_ctx_set_tlcp_server_certificate_and_keys(&tlcp_ctx,
		tlcp_cert_file, tlcp_sign_key_file, tlcp_sign_pass, tlcp_enc_key_file, tlcp_enc_pass) != 1)
	{
		ereport(loglevel,
				(errmsg("Set server certificate and keys fail")));
		return -1;
	}

	/* when the parrameter 'tlcp_ca_cert_file' is no set, one-way authentication is used */
	if (tlcp_ca_cert_file && tlcp_ca_cert_file[0] != '\0')
	{
		if (tls_ctx_set_ca_certificates(&tlcp_ctx, tlcp_ca_cert_file, TLS_DEFAULT_VERIFY_DEPTH) != 1)
		{
			ereport(loglevel,
					(errmsg("Set server ca certificate fail")));
			return -1;
		}
	}

	/* used by the callback */
	ssl_is_server_start = isServerStart;

	return 0;
}

int
be_tlcp_open_server(Port *port)
{
	int ret = 0;

	memset(&tlcp_conn, '\0', sizeof(TLS_CONNECT));
	if (tls_init(&tlcp_conn, &tlcp_ctx) != 1)
	{
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("could not initialize TLCP connection: TLCP init")));
		return -1;
	}

	if (tls_set_socket(&tlcp_conn, port->sock) != 1)
	{
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("could not create TLCP socket fail")));
		return -1;
	}

	port->ssl_in_use = true;
	port->tlcp_conn = &tlcp_conn;

aloop:
	if ((ret = tls_do_handshake(port->tlcp_conn)) != 1)
	{
		int			err;   // BUG Z7353
		if (ret > 1)
			goto aloop;

		/* Begin - BUG Z7353 */
		err = tlcp_get_error();

		switch (err)
		{
			case TLCP_protocol_version:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection:  unknown protocol")));
				break;
			case TLCP_unexpected_message:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: read unexpected message!")));
				break;
			case TLCP_insufficient_security:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: read insufficient security!")));
				break;
			case TLCP_internal_error:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: internal error!")));
			case TLCP_bad_certificate:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: bad certificate!")));
				break;
			case TLCP_decrypt_error:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: decrypt error!")));
				break;
			case TLCP_client_certificate_error:
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("could not accept TLCP connection: read client certificate error!")));
				break;
			case TLCP_close_notify:
				goto aloop;
				break;
			case TLCP_handshake_success:
				return 0;
				break;
			default:
				ereport(COMMERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg(" TLCP handshake fail, err=%d", err)));
				break;
		}
		/* End - BUG Z7353 */

		return -1;
	}

	return 0;
}

/* this function like secure_raw_read */
ssize_t
be_tlcp_read(Port *port, void *ptr, size_t len, int *waitfor)
{
	size_t len_limit = len;
	size_t recv_len = 0;
	int rv = -1;  /* fail:rv < 0; success: rv > 0 */

	if ((rv = tls_recv(port->tlcp_conn, (uint8_t *)ptr, len_limit, &recv_len)) != 1)
	{
		return rv;
	}

	return recv_len;
}

ssize_t
be_tlcp_write(Port *port, void *ptr, size_t len, int *waitfor)
{
	size_t len_limit = len;
	size_t sent_len = 0;
	int rv = -1; /* fail:rv < 0; success: rv > 0 */

	if ((rv = tls_send(port->tlcp_conn, (uint8_t *)ptr, len_limit, &sent_len)) != 1)
	{
		return rv;
	}

	return sent_len;
}

void
be_tlcp_close(Port *port)
{
	tls_socket_close(port->tlcp_conn->sock);

	port->ssl_in_use = false;

}

void
be_tlcp_destroy(void)
{
	;
}

char *
be_tlcp_get_certificate_hash(Port *port, size_t *len)
{
	unsigned int hash_size = MASTER_SECRET_SIZE;
	char	   *cert_hash = NULL;

	cert_hash = palloc(hash_size);
	memcpy(cert_hash, port->tlcp_conn->master_secret, hash_size);
	*len = hash_size;

	return cert_hash;
}

const char *
be_tlcp_get_version(Port *port, char *str, int len)
{
	memset(str, '\0', len);
	if (port->tlcp_conn)
	{
		snprintf(str, len, "%s %d.%d", tls_protocol_name(port->tlcp_conn->protocol), port->tlcp_conn->protocol >> 8, port->tlcp_conn->protocol & 0xff);
		return (const char *)str;
	}
	else
		return NULL;
}

const char *
be_tlcp_get_cipher(Port *port)
{
	if (port->tlcp_conn)
		return tls_cipher_suite_name(port->tlcp_conn->cipher_suites[0]);
	else
		return NULL;
}

static int
tlcp_get_pass(const char *tlcp_pass_file, bool isServerStart)
{
	char line[MAXPGPATH];
	FILE *file_p;
	int i = 0;
	int loglevel = isServerStart ? FATAL : LOG;

	file_p = fopen(tlcp_pass_file, "r");
	if (file_p == NULL)
	{
		ereport(loglevel,
				errmsg("can't open file %s\n", tlcp_pass_file));
		return -1;
	}

	/* read tlcp_pass_file, line one is tlcp_sign_pass; line two is tlcp_enc_pass  */
	memset(line, '\0', MAXPGPATH);
	for (i = 0; i < 2; i++)
	{
		if (fgets(line, sizeof(line), file_p) != NULL)
		{
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n')
			{
				line[len - 1] = '\0';
			}

			if (i == 0)
			{
				tlcp_sign_pass = strdup(line);
			}
			else if (i == 1)
			{
				tlcp_enc_pass = strdup(line);
			}
		}
		else
		{
			ereport(loglevel,
					errmsg("can't read %s password", tlcp_pass_file));
			fclose(file_p);
			return -1;
		}
	}
	fclose(file_p);

	return 1;
}

#endif
/* End - ReqID:SRS-SECURITY-TLCP */

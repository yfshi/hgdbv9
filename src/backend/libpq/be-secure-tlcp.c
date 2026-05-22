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
/* Begin - BUG#Z7577 */
#include <gmssl/asn1.h>
#include <gmssl/oid.h>
#include <gmssl/x509.h>
/* End - BUG#Z7577 */

#include "utils/memutils.h" // BUG#Z7895

static TLS_CTX tlcp_ctx;

static bool ssl_is_server_start;

static int tlcp_get_pass(const char *tlcp_pass_file, bool isServerStart);
/* Begin - BUG#Z7577 */
static int x509_get_directory_name(char *str,int str_len, int fmt, int ind, const char *label,
						int tag, const uint8_t *d, size_t dlen);
static int tlcp_format_print(char *buffer, int buff_len, int format,
			int indent, const char *str, const char *label, const char *name);
static void x509_get_object_identifier(char *str,int str_len, int format, int indent,
							const char *label, const char *name,
							const uint32_t *nodes, size_t nodes_cnt);
static void tlcp_format_string(char *str,int str_len, int fmt, int ind,
					const char *label, const uint8_t *d, size_t dlen);
static int x509_get_attr_type_and_value(char *str,int str_len, int fmt, int ind, const char *label,
										const uint8_t *d, size_t dlen);
static int x509_get_rdn(char *str,int str_len, int fmt, int ind,
						const char *label, const uint8_t *d, size_t dlen);
static int x509_get_name(char *str,int str_len, int fmt, int ind, const char *label,
						const uint8_t *d, size_t dlen);
/* End - BUG#Z7577 */

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

	/* Begin - bug#z7895 */
	port->tlcp_conn =
				MemoryContextAllocZero(TopMemoryContext, sizeof(TLS_CONNECT));

	if (tls_init(port->tlcp_conn, &tlcp_ctx) != 1)
	{
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("could not initialize TLCP connection: TLCP init")));
		tls_cleanup(port->tlcp_conn);
		pfree(port->tlcp_conn);
		port->tlcp_conn = NULL;
		return -1;
	}

	if (tls_set_socket(port->tlcp_conn, port->sock) != 1)
	{
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("could not create TLCP socket fail")));
		tls_cleanup(port->tlcp_conn);
		pfree(port->tlcp_conn);
		port->tlcp_conn = NULL;
		return -1;
	}
	/* End - bug#z7895 */

	port->ssl_in_use = true;

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
				break;
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

		/* Begin - bug#z7895 */
		tls_cleanup(port->tlcp_conn);
		pfree(port->tlcp_conn);
		port->tlcp_conn = NULL;
		port->ssl_in_use = false;
		/* End - bug#z7895 */

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
	/* Begin - bug#z7895 */
	if (port->tlcp_conn)
	{
		(void) tls_shutdown(port->tlcp_conn);
		tls_cleanup(port->tlcp_conn);
		pfree(port->tlcp_conn);
		port->tlcp_conn = NULL;
	}
	/* End - bug#z7895 */

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

/* Begin - BUG#Z7577 */
static int
x509_get_directory_name(char *str,int str_len, int fmt, int ind, const char *label,
						int tag, const uint8_t *d, size_t dlen)
{
	const uint8_t *ptr = d;
	size_t tmp_dlen = dlen;
	int use_len = 0;
	int remaind_len = 0;
	int i = 0;

	if (label == NULL)
		return 1;

	use_len = strlen(str);

	remaind_len = str_len - use_len - 1;
	if (strcmp("countryName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/C=");
	}
	else if (strcmp("stateOrProvinceName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/ST=");
	}
	else if (strcmp("localityName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/L=");
	}
	else if (strcmp("organizationName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/O=");
	}
	else if (strcmp("organizationalUnitName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/OU=");
	}
	else if (strcmp("commonName", label) == 0)
	{
		snprintf(str + use_len, remaind_len, "/CN=");
	}
	else
	{
		while (tmp_dlen--)
		{
			ptr++;
		}
		return 1;
	}

	use_len = strlen(str);
	remaind_len = str_len - use_len - tmp_dlen;
	if (use_len + tmp_dlen > str_len)
		remaind_len = str_len - use_len;

	for (i = 0; i < tmp_dlen; i++)
	{
		snprintf(str + use_len + i, remaind_len - i, "%c", *ptr++);
	}

	return 1;
}

static int
tlcp_format_print(char *buffer, int buff_len, int format,
			int indent, const char *str, const char *label, const char *name)
{
	int i;
	int len = 0;

	/* check buffer is enough */
	len = strlen(buffer);
	if (len + indent > buff_len)
	{
		return 1;
	}

	for (i = 0; i < indent; i++)
	{
		snprintf(buffer + len + i, buff_len - len - i, " ");
	}

	len = strlen(buffer);
	snprintf(buffer + len, buff_len - len, "%s%s", label, name);

	return 1;
}

static void
x509_get_object_identifier(char *str,int str_len, int format, int indent,
							const char *label, const char *name,
							const uint32_t *nodes, size_t nodes_cnt)
{
	size_t i;
	int len = 0;

	tlcp_format_print(str, str_len, format, indent, "%s: %s", label, name ? name : "(unknown)");
	if (nodes)
	{
		len = strlen(str);
		snprintf(str + len, str_len - len, " (");
		len = strlen(str);
		for (i = 0; i < nodes_cnt - 1; i++)
		{
			snprintf(str + len + i, str_len - len - i, "%d.", (int)nodes[i]);
		}

		len = strlen(str);
		snprintf(str + len, str_len - len, "%d)", nodes[i]);
	}

	return;
}


static void
tlcp_format_string(char *str,int str_len, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen)
{
	int len = 0;
	int i = 0;

	/* check buffer is enough */
	len = strlen(str);
	if (len + ind + dlen > str_len)
	{
		while (dlen--)
		{
			d++;
		}

		return;
	}

	/* print "   " */
	for (i = 0; i < ind; i++)
	{
		snprintf(str + len + i, str_len - len - i, " ");
	}

	/* print key */
	len = strlen(str);
	snprintf(str + len, str_len - len, "/%s=", label);

	/* print value */
	for (i = 0; i < dlen; i++)
	{
		snprintf(str + len + i, str_len - len - i, "%c", *d++);
	}

	return;
}

/* get type and value */
static int
x509_get_attr_type_and_value(char *str,int str_len, int fmt, int ind, const char *label,
										const uint8_t *d, size_t dlen)
{
	int oid, tag;
	const uint8_t *val;
	size_t vlen;

	if (fmt & ASN1_FMT_FULL)
	{
		ind += 4;

		if (x509_name_type_from_der(&oid, &d, &dlen) != 1)
			goto err;

		x509_get_object_identifier(str, str_len, fmt, ind, "type", x509_name_type_name(oid), NULL, 0);

		if (oid == OID_email_address)
		{
			if (asn1_ia5_string_from_der((const char **)&val, &vlen, &d, &dlen) != 1)
				goto err;

			tlcp_format_string(str, str_len, fmt, ind, "value", val, vlen);
		}
		else
		{
			if (x509_directory_name_from_der(&tag, &val, &vlen, &d, &dlen) != 1)
				goto err;

			x509_get_directory_name(str, str_len, fmt, ind, "value", tag, val, vlen);
		}
	}
	else
	{
		if (x509_name_type_from_der(&oid, &d, &dlen) != 1)
		{
			goto err;
		}

		if (oid == OID_email_address)
		{
			if (asn1_ia5_string_from_der((const char **)&val, &vlen, &d, &dlen) != 1)
				goto err;
			tlcp_format_string(str, str_len, fmt, ind, "emailAddress", val, vlen);
		}
		else
		{
			if (x509_directory_name_from_der(&tag, &val, &vlen, &d, &dlen) != 1)
				goto err;

			x509_get_directory_name(str, str_len, fmt, ind, x509_name_type_name(oid), tag, val, vlen);
		}
	}

	if (asn1_length_is_zero(dlen) != 1)
		goto err;

	return 1;
err:
	ereport(WARNING, errmsg("Get ssl_client_dn or ssl_issuer_dn info fail"));
	return -1;
}

/* get and parser rnd info */
static int x509_get_rdn(char *str,int str_len, int fmt, int ind,
						const char *label, const uint8_t *d, size_t dlen)
{
	const uint8_t *p = NULL;
	size_t len;
	int cur_len = 0;

	if (fmt & ASN1_FMT_FULL)
	{
		cur_len = strlen(str);
		snprintf(str + strlen(str), str_len - cur_len, "%s\n", label);
		ind += 4;
	}

	if (asn1_sequence_from_der(&p, &len, &d, &dlen) != 1)
	{
		return -1;
	}
	x509_get_attr_type_and_value(str, str_len, fmt, ind, "AttributeTypeAndValue", p, len);

	while (dlen)
	{
		if (asn1_sequence_from_der(&p, &len, &d, &dlen) != 1)
		{
			return -1;
		}
		x509_get_attr_type_and_value(str, str_len, fmt, ind + 4, "AttributeTypeAndValue", p, len);
	}

	return 1;
}

/* public function :get subject or issuer name */
static int
x509_get_name(char *str,int str_len, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen)
{
	const uint8_t *p;
	size_t len;

	if (label)
	{
		ind += 4;
	}

	while (dlen)
	{
		if (asn1_set_from_der(&p, &len, &d, &dlen) != 1)
		{
			return -1;
		}

		x509_get_rdn(str, str_len, fmt, ind, "RelativeDistinguishedName", p, len);
	}
	return 1;
}

/* get issuer name and serial number */
void
be_tlcp_get_issuer_and_serial(Port *port, char *serial, size_t serial_len,
											 char *issuer_dn, size_t issuer_dn_len)
{
	const uint8_t *serial_number;
	size_t serial_number_len;
	const uint8_t *issuer;
	size_t issuer_len;

	char *cur_serial = serial;

	if (port && port->tlcp_conn && port->tlcp_conn->client_certs_len != 0)
	{
		x509_cert_get_issuer_and_serial_number(port->tlcp_conn->client_certs,
											port->tlcp_conn->client_certs_len,
											&issuer, &issuer_len, &serial_number, &serial_number_len);
		memset(cur_serial, '\0', serial_len);
		for (int i = 0; i < serial_number_len; i++)
		{
			if (serial_len - i <= 0)
				break;

			snprintf(cur_serial + i, serial_len - i, "%d", serial_number[i]);
		}

		x509_get_name(issuer_dn, issuer_dn_len, 0, 4, "issuer", issuer, issuer_len);
	}
	else
	{
		serial[0] = '\0';
		issuer_dn[0] = '\0';
	}

	return;
}

/* get subject name */
void
be_tlcp_get_subject_name(Port *port, char *client_dn, size_t client_dn_len)
{
	const uint8_t *subject = NULL;
	size_t subject_len;

	if (port && port->tlcp_conn && port->tlcp_conn->client_certs_len != 0)
	{
		x509_cert_get_subject(port->tlcp_conn->client_certs, port->tlcp_conn->client_certs_len,
			&subject, &subject_len);

		x509_get_name(client_dn, client_dn_len, 0, 4, "subject", subject, subject_len);
	}
	else
	{
		client_dn[0] = '\0';
	}

	return;
}
/* End - BUG#Z7577 */

#endif
/* End - ReqID:SRS-SECURITY-TLCP */

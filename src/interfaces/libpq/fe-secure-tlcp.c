/*-------------------------------------------------------------------------
 *
 * fe-secure-tlcp.c
 *   The front-end (client) tcp support for TLCP
 *
 * Portions Copyright (c) 2016-2021, PostgreSQL Global Development Group
 *
 *  src/interfaces/libpq/fe-secure-tlcp.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID:SRS-SECURITY-TLCP */
#ifdef WIN32
#include "win32.h"
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#endif

#include "postgres_fe.h"
#include "libpq-int.h"

#include <sys/stat.h>
#include <gmssl/tls.h>

#ifdef USE_GMSSL
#define MAX_LINE_LENGTH 1024

static TLS_CTX client_ctx;

static int client_ciphers[] = { TLS_cipher_ecc_sm4_cbc_sm3, };

static char *
get_tlcp_key_pass(PGconn *conn)
{
	char *pass = NULL;
	char homedir[MAXPGPATH];
	char passfile[MAXPGPATH];
	char line[MAX_LINE_LENGTH];
	struct stat bufenc;
	FILE *file_p;

	if (!(conn->tlcppassfile && strlen(conn->tlcppassfile) > 0))
	{
			pqGetHomeDirectory(homedir, sizeof(homedir));
			snprintf(passfile, sizeof(passfile), "%s/%s", homedir, CLIENT_TLCP_PASS_FILE);
	}
	else											/* won't need homedir */
	{
			strlcpy(passfile, conn->tlcppassfile, sizeof(passfile));
	}

	if (passfile[0] != '\0' && stat(passfile, &bufenc) == 0)
	{
		file_p = fopen(passfile, "r");
		if (file_p == NULL)
		{
			appendPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("can't open file %s\n"), passfile);
			return NULL;
		}

		if (fgets(line, sizeof(line), file_p) != NULL)
		{
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n')
			{
				line[len - 1] = '\0';
			}

			pass = palloc0(len + 1);
			if (pass == NULL)
			{
				appendPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("malloc fail,can't read %s password \n"), passfile);
				fclose(file_p);
				return NULL;
			}
			memcpy(pass, line, len + 1);
		}
		else
		{
			appendPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("can't read %s password \n"), passfile);
			fclose(file_p);
			return NULL;
		}
		fclose(file_p);
	}
	else
	{
		appendPQExpBuffer(&conn->errorMessage,
						  libpq_gettext("can't find file %s \n"), passfile);
	}

	return pass;
}

bool
check_TLCP_is_required(PGconn *conn)
{
	if (conn->ssltlcp)
	{
		if (!(((conn->ssltlcp[0] == '1' || conn->ssltlcp[0] == '0') && conn->ssltlcp[1] == '\0')||
						conn->ssltlcp[0] == '\0' ||
						(pg_strcasecmp(conn->ssltlcp, "true") == 0) ||
						(pg_strcasecmp(conn->ssltlcp, "false") == 0) ||
						(pg_strcasecmp(conn->ssltlcp, "") == 0)))
		{
			conn->status = CONNECTION_BAD;

			appendPQExpBufferStr(&conn->errorMessage,
								 libpq_gettext("Paramter: ssltlcp only support 0, 1, true or false\n"));
			return false;
		}
	}

	return true;
}

bool
hg_is_TLCP(PGconn *conn)
{
	bool		istlcp;

	if (!conn)
		return false;

	if (conn->ssltlcp && ((conn->ssltlcp[0] == '1' && conn->ssltlcp[1] == '\0')||
						(pg_strcasecmp(conn->ssltlcp, "true") == 0)))
	{
		istlcp = true;
	}
	else
	{
		istlcp = false;
	}

	return istlcp;
}

int
pgtlcp_init(PGconn *conn, bool do_ssl, bool do_crypto)
{
	if (tls_socket_lib_init() != 1)
	{
		appendPQExpBuffer(&conn->errorMessage,
						  libpq_gettext("client TLCP could not lib init\n"));
		return -1;
	}

	return 0;
}

static int
initialize_TLCP(PGconn *conn)
{
	char		cacertfile[MAXPGPATH];
	char		certfile[MAXPGPATH];
	char		keyfile[MAXPGPATH];
	char		homedir[MAXPGPATH];

	struct stat bufenc;

	if (tls_ctx_init(&client_ctx, TLS_protocol_tlcp, TLS_client_mode) != 1)
	{
		appendPQExpBuffer(&conn->errorMessage,
						  libpq_gettext("client TLCP ctx init fail \n"));
		return -1;
	}

	if (tls_ctx_set_cipher_suites(&client_ctx, client_ciphers,
			sizeof(client_ciphers)/sizeof(client_ciphers[0])) != 1)
	{
		appendPQExpBuffer(&conn->errorMessage,
						  libpq_gettext("client TLCP set cipher suites fail \n"));
		return -1;
	}

	pqGetHomeDirectory(homedir, sizeof(homedir));

	/* get rootcacert.pem path */
	if (!(conn->tlcpcacert && strlen(conn->tlcpcacert) > 0))
	{
		snprintf(cacertfile, sizeof(cacertfile), "%s/%s", homedir, CLIENT_CA_CERT_FILE);
	}
	else
	{
		strlcpy(cacertfile, conn->tlcpcacert, sizeof(cacertfile));
	}

	/* get clientcert.pem path */
	if (!(conn->tlcpcert && strlen(conn->tlcpcert) > 0))
	{
		snprintf(certfile, sizeof(certfile), "%s/%s", homedir, CLIENT_CA_CERT_FILE);
	}
	else
	{
		strlcpy(certfile, conn->tlcpcert, sizeof(certfile));
	}

	/* get clientkey.pem path */
	if (!(conn->tlcpkey && strlen(conn->tlcpkey) > 0))
	{
		snprintf(keyfile, sizeof(keyfile), "%s/%s", homedir, CLIENT_CA_CERT_FILE);
	}
	else
	{
		strlcpy(keyfile, conn->tlcpkey, sizeof(keyfile));
	}

	if (stat(cacertfile, &bufenc) == 0)
	{
		if (tls_ctx_set_ca_certificates(&client_ctx, cacertfile, TLS_DEFAULT_VERIFY_DEPTH) != 1)
		{
			appendPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("client TLCP set ca certificates suites fail:%s \n"), cacertfile);
			return -1;
		}
	}

	if (stat(certfile, &bufenc) == 0)
	{
		char *pass = NULL;
		if ((pass = get_tlcp_key_pass(conn)) == NULL)
		{
			return -1;
		}

		if (tls_ctx_set_certificate_and_key(&client_ctx, certfile, keyfile, pass) != 1)
		{
			appendPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("client TLCP set certfile or keyfile suites fail:\n"));
			return -1;
		}
		free(pass);
	}

	conn->client_conn = malloc(sizeof(TLS_CONNECT));
	if (conn->client_conn == NULL)
	{
		appendPQExpBuffer(&conn->errorMessage,
			  libpq_gettext("TLCP malloc memory fail\n"));
		return -1;
	}

	memset(conn->client_conn, 0, sizeof(TLS_CONNECT));
	if (tls_init(conn->client_conn, &client_ctx) != 1
		|| tls_set_socket(conn->client_conn, conn->sock) != 1)
	{
		appendPQExpBuffer(&conn->errorMessage,
				  libpq_gettext("create socket error:\n"));
		return PGRES_POLLING_FAILED;
	}

	conn->ssl_in_use = true;

	return 0;
}

static PostgresPollingStatusType
open_client_TLCP(PGconn *conn)
{
	int ret = tls_do_handshake(conn->client_conn);
	if (ret != 1) {
		/* when ret == 1 handshake ok!
			ret > 1 we want to read
			else handshake error */
		if (ret > 1)
			return PGRES_POLLING_READING;

		appendPQExpBuffer(&conn->errorMessage,
				  libpq_gettext("shake hand error:\n"));
		return PGRES_POLLING_FAILED;
	}

	return PGRES_POLLING_OK;

}

PostgresPollingStatusType
pgtlcp_open_client(PGconn *conn)
{
	if (conn->client_conn == NULL)
	{
		if (initialize_TLCP(conn) != 0)
		{
			/* initialize_SSL already put a message in conn->errorMessage */
			pgtlcp_close(conn);
			return PGRES_POLLING_FAILED;
		}
	}

	return open_client_TLCP(conn);
}

void
pgtlcp_close(PGconn *conn)
{
	if (conn->ssl_in_use)
	{
		conn->ssl_in_use = false;
	}

	if (conn->client_conn)
	{
		free(conn->client_conn);
		conn->client_conn = NULL;
	}
}

ssize_t
pgtlcp_read(PGconn *conn, void *ptr, size_t len)
{
	size_t len_limit = len;
	size_t recv_len = 0;
	int rv = -1;
	char sebuf[PG_STRERROR_R_BUFLEN];
	int 		result_errno = 0;

	SOCK_ERRNO_SET(0);

	if ((rv = tls_recv(conn->client_conn, (uint8_t *)ptr, len_limit, &recv_len)) != 1)
	{
		result_errno = SOCK_ERRNO;

		/* Set error message if appropriate */
		switch (result_errno)
		{
#ifdef EAGAIN
			case EAGAIN:
#endif
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
			case EWOULDBLOCK:
#endif
			case EINTR:
				/* no error message, caller is expected to retry */
				break;

			case EPIPE:
			case ECONNRESET:
				appendPQExpBuffer(&conn->errorMessage,
								  libpq_gettext("TLCP error: %s\n"),
								  SOCK_STRERROR(result_errno, sebuf, sizeof(sebuf)));
				break;

			case 0:
				/* If errno didn't get set, treat it as regular EOF */
				rv = 0;
				break;

			default:
				appendPQExpBuffer(&conn->errorMessage,
								  libpq_gettext("TLCP:could not receive data from server: %s\n"),
								  SOCK_STRERROR(result_errno,
												sebuf, sizeof(sebuf)));

				break;
		}

		recv_len = rv;
	}

	SOCK_ERRNO_SET(result_errno);

	return recv_len;

}

ssize_t
pgtlcp_write(PGconn *conn, const void *ptr, size_t len)
{
	size_t len_limit = len;
	size_t sent_len = 0;

	if (tls_send(conn->client_conn, (uint8_t *)ptr, len_limit, &sent_len) != 1)
	{
		appendPQExpBuffer(&conn->errorMessage,
				  libpq_gettext("send failure, close connection\n"));
		return -1;
	}

	return sent_len;
}

char *
pgtlcp_get_peer_certificate_hash(PGconn *conn, size_t *len)
{
	unsigned int hash_size = MASTER_SECRET_SIZE;
	char	   *cert_hash = NULL;

	cert_hash = malloc(hash_size);
	memcpy(cert_hash, conn->client_conn->master_secret, hash_size);
	*len = hash_size;

	return cert_hash;
}

void *
PQgettlcp(PGconn *conn)
{
	if (!conn)
		return NULL;
	return conn->client_conn;
}

void *
PQtlcpStruct(PGconn *conn, const char *struct_name)
{
	if (!conn)
		return NULL;
	if (strcmp(struct_name, "OpenSSL") == 0)
		return conn->client_conn;
	return NULL;
}


const char *const *
PQtlcpAttributeNames(PGconn *conn)
{
	static const char *const result[] = {
		"library",
		"cipher",
		"compression",
		"protocol",
		NULL
	};

	return result;
}

const char *
PQtlcpAttribute(PGconn *conn, const char *attribute_name, char *str, int len)
{
	if (!conn)
		return NULL;
	if (conn->client_conn == NULL)
		return NULL;

	if (strcmp(attribute_name, "library") == 0)
		return "GMSSL";

	if (strcmp(attribute_name, "cipher") == 0)
		return tls_cipher_suite_name(client_ctx.cipher_suites[0]);

	if (strcmp(attribute_name, "compression") == 0)
	{
		const char *compress = tls_compression_method_name(TLS_compression_null);
		if (compress && strcmp(compress, "no_compression") == 0)
			return "off";
		else
			return "on";
	}

	if (strcmp(attribute_name, "protocol") == 0)
	{
		memset(str, 0, len);
		snprintf(str, len, "%s %d.%d", tls_protocol_name(client_ctx.protocol), client_ctx.protocol >> 8, client_ctx.protocol & 0xff);
		return (const char *)str;
	}

	return NULL;				/* unknown attribute */
}

int
PQtlcpInUse(PGconn *conn)
{
	if (!conn)
		return 0;
	if (hg_is_TLCP(conn))
		return 1;
	return 0;
}

#endif
/* End - ReqID:SRS-SECURITY-TLCP */

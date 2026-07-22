#include "postgres.h"

#include <openssl/sha.h>

#include "access/xact.h"
#include "access/xlog.h"
#include "catalog/namespace.h"
#include "commands/user.h"
#include "common/ip.h"
#include "libpq/crypt.h"
#include "libpq/libpq.h"
#include "miscadmin.h"
#include "replication/walsender.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"

#include "../../charset/ctype.h" /* ReqID：SRS-MYSQL-CHARSET-INTRODUCER */
#include "../../include/auth.h"
#include "../../include/server.h"
#include "../../include/message.h"
#include "../../include/mqformat.h"

#define MYS_ENCODING_UTF8MB3_GENERAL_CI         0x21

#define PRINTABLE_CHARS_COUNT                   62
#define HANDSHAKE_RESPONSE_RESERVED_BYTES       23

#define AUTH_PLUGIN_DATA_PART_1                 8
#define CAPABILITY_UPPER_BITS                   16
#define HANDSHAKEV10_RESERVED_BYTES             10

typedef struct
{
    enum PasswordType type;
    mysql_auth_plugin_t *plugin;
} auth_plugin_cache_t;

static mysql_encryt_password_hook_type      pre_mysql_encrypt_password_hook = NULL;

static auth_plugin_cache_t auth_plugins[] = {
	{ PASSWORD_TYPE_MYS_NATIVE, &mysql_native_password },
#ifdef USE_OPENSSL
    { PASSWORD_TYPE_MYS_CACHING_SHA2, &caching_sha2_password },
#endif
#ifdef USE_GMSSL
    { PASSWORD_TYPE_SM3, &sm3_password },
#endif
};

static char PRINTABLE_CHARS[PRINTABLE_CHARS_COUNT + 1] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static mysql_auth_plugin_t *
get_auth_plugin_by_pwtype(enum PasswordType pwtype)
{
    mysql_auth_plugin_t *auth_plugin = NULL;

    for (int i = 0; i < sizeof(auth_plugins) / sizeof(auth_plugin_cache_t) && !auth_plugin; i++)
    {
        if (auth_plugins[i].type == pwtype)
            auth_plugin = auth_plugins[i].plugin;
    }

    return auth_plugin;
}

static mysql_handshake_t *
make_mysqld_handshakev10_packet(uint8 *scramble)
{
    mysql_handshake_t *handshake = (mysql_handshake_t *) palloc0(sizeof(mysql_handshake_t));

    handshake->protocol_version = 10; // always is 10
    handshake->capabilities = get_client_capabilities();
    handshake->auth_plugin_name = pstrdup("mysql_native_password");
    // Caution is required when modifying this value, as it may cause JDBC to fail to connect.
    handshake->server_version_str = pstrdup("8.0.28-ivorysql-server");
    handshake->charset = MYS_ENCODING_UTF8MB3_GENERAL_CI;
    handshake->server_status = SERVER_STATUS_AUTOCOMMIT;
    handshake->thread_id = MyProcPid;

    // generate a random challenge
    if (!pg_strong_random(handshake->auth_plugin_data, AUTH_PLUGIN_DATA_LEN))
    {
        ereport(ERROR,
                (errmsg("Failed to Generate the random number when make handshake V10")));
    }

    for (int i = 0, index = 0; i < AUTH_PLUGIN_DATA_LEN; i++)
    {
        index = (unsigned char) handshake->auth_plugin_data[i] % PRINTABLE_CHARS_COUNT;
        handshake->auth_plugin_data[i] = PRINTABLE_CHARS[index];
    }

    handshake->auth_plugin_data[AUTH_PLUGIN_DATA_LEN] = 0x00;

    memcpy(scramble, handshake->auth_plugin_data, AUTH_PLUGIN_DATA_LEN);
    scramble[AUTH_PLUGIN_DATA_LEN] = 0x00;

    return handshake;
}

static void
send_handshakev10_packet(StringInfo buf, mysql_handshake_t *handshake)
{
    mq_append_int1(buf, handshake->protocol_version);
    mq_append_string_null(buf, handshake->server_version_str);
    mq_append_int4(buf, handshake->thread_id);
    mq_append_string_len(buf, handshake->auth_plugin_data, AUTH_PLUGIN_DATA_PART_1);
    mq_append_int1(buf, 0x00); /* filler */

    mq_append_int2(buf, handshake->capabilities & 0xffff);
    mq_append_int1(buf, handshake->charset);
    mq_append_int2(buf, handshake->server_status);
    mq_append_int2(buf, (handshake->capabilities >> CAPABILITY_UPPER_BITS) & 0xffff);

    if (handshake->capabilities & CLIENT_PLUGIN_AUTH)
        mq_append_int1(buf, AUTH_PLUGIN_DATA_LEN + 1);
    else
        mq_append_int1(buf, 0x00);

    /* add the fillers */
    for (int i = 0; i < HANDSHAKEV10_RESERVED_BYTES; i++)
    {
        mq_append_int1(buf, 0x00);
    }

    if (handshake->capabilities & CLIENT_SECURE_CONNECTION)
    {
        mq_append_string_null(buf, handshake->auth_plugin_data + AUTH_PLUGIN_DATA_PART_1);
    }

    if (handshake->capabilities & CLIENT_PLUGIN_AUTH)
    {
        mq_append_string_null(buf, handshake->auth_plugin_name);
    }

    mq_putmessage(buf->data, buf->len);
}

static void
read_handshake_reponse_header(StringInfo buf, Port *port, mysql_handshake_response_t *auth)
{
    if (has_client_capabilities(CLIENT_PROTOCOL_41))
    {
        mq_get_int4(buf, &auth->client_capabilities);
        mq_get_int4(buf, &auth->max_packet_size);
        mq_get_int1(buf, &auth->charset);
        mq_skip_bytes(buf, HANDSHAKE_RESPONSE_RESERVED_BYTES); // all is 0

        set_client_capabilities(auth->client_capabilities);
    }
    else
    {
        mq_get_int2(buf, &auth->client_capabilities);
        mq_get_int3(buf, &auth->max_packet_size);
    }

    set_client_capabilities(auth->client_capabilities);
}

static void
read_handshake_response320(StringInfo buf, Port *port, mysql_handshake_response_t *auth)
{
    read_handshake_reponse_header(buf, port, auth);

    auth->username = mq_get_string_null(buf);

    if (has_client_capabilities(CLIENT_CONNECT_WITH_DB))
    {
        auth->auth_response = mq_get_string_null(buf);
        auth->database = mq_get_string_null(buf);
    }
    else
    {
        auth->auth_response = mq_get_string_eof(buf, NULL);
    }
}

static void
read_handshake_response41(StringInfo buf, Port *port, mysql_handshake_response_t *auth)
{
    read_handshake_reponse_header(buf, port, auth);

    auth->username = mq_get_string_null(buf);

    if (has_client_capabilities(CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA))
    {
        auth->auth_response = mq_get_string_lenenc(buf, NULL, NULL);
    }
    else
    {
        uint8 len;
        mq_get_int1(buf, &len);
        auth->auth_response = mq_get_string_len(buf, len);
    }

    if (has_client_capabilities(CLIENT_CONNECT_WITH_DB))
    {
        auth->database = mq_get_string_null(buf);
    }

    if (has_client_capabilities(CLIENT_PLUGIN_AUTH))
    {
        auth->client_plugin_name = mq_get_string_null(buf);
    }
    
    if (has_client_capabilities(CLIENT_CONNECT_ATTRS))
    {
        uint32  size;
        uint64  length = mq_get_int_lenenc(buf, &size);

        if (length > buf->len - buf->cursor)
        {
            ereport(ERROR, 
                    errmsg("parse client connect attributes fail"));
        }

        auth->nattrs = 0;

        while(length)
        {
            uint32 size;
            mysql_conn_attr_t *attr = &auth->attrs[auth->nattrs++];

            attr->key = mq_get_string_lenenc(buf, NULL, &size);
            length -= size;

            attr->value = mq_get_string_lenenc(buf, NULL, &size);
            length -= size;
        }
    }

    if (has_client_capabilities(CLIENT_ZSTD_COMPRESSION_ALGORITHM))
    {
        mq_get_int1(buf, &auth->zstd_compression_level);
    }
}

static void
read_ssl_request(StringInfo buf, Port *port, mysql_handshake_response_t *auth)
{
    Assert(has_client_capabilities(CLIENT_SSL));

    read_handshake_reponse_header(buf, port, auth);

    // do ssl exchange
    if (secure_open_server(port) == -1)
    {
        ereport(ERROR, 
                errmsg("mysql ssl start fail"));
    }
    
    // recv client response
    if (mq_getmessage(buf) != STATUS_OK)
    {
        DestroyStringInfo(buf);
        return;
    }
}

/**
 * Implentment in Mysql source code with
 * function parse_client_handshake_packet()
 */
static mysql_handshake_response_t *
read_handshake_response(StringInfo buf, Port *port)
{
    mysql_handshake_response_t *auth = (mysql_handshake_response_t *) palloc0(sizeof(mysql_handshake_response_t));

    /**
     * Just try to read 2 bytes to judge handshakeResponse packet's version,
     * then reset read position.
     */
    mq_get_int2(buf, &auth->client_capabilities);
    buf->cursor -= 2;

    set_client_capabilities(auth->client_capabilities);

    if (has_client_capabilities(CLIENT_SSL))
    {
        read_ssl_request(buf, port, auth);
    }

    if (has_client_capabilities(CLIENT_PROTOCOL_41))
        read_handshake_response41(buf, port, auth);
    else
        read_handshake_response320(buf, port, auth);

    return auth;
}

static void
free_handshake(mysql_handshake_t *handshake)
{
    if (handshake->server_version_str)
        pfree(handshake->server_version_str);

    if (handshake->auth_plugin_name)
        pfree(handshake->auth_plugin_name);

    pfree(handshake);
}

static void
free_handshake_response(mysql_handshake_response_t *resp)
{
    if (resp->username)
        pfree(resp->username);
    
    if (resp->auth_response)
        pfree(resp->auth_response);
    
    if (resp->database)
        pfree(resp->database);

    if (resp->client_plugin_name)
        pfree(resp->client_plugin_name);
    
    for (int i = 0; i < resp->nattrs; ++i)
    {
        pfree(resp->attrs[i].key);
        pfree(resp->attrs[i].value);
    }
    
    pfree(resp);
}

int
mysql_conn_handshake(Port *port)
{
	char        local_host[NI_MAXHOST];
	char        local_port[NI_MAXSERV];
	int         lp;
	MysqlInstanceEntry *entry;
    MemoryContext oldcontext;
    mysql_handshake_t *handshake;
    mysql_handshake_response_t  *handshakeResp;
    StringInfo buf;

    /* Begin - OA-2543
	 * If we're going to reject the connection due to database state, say so
	 * now instead of wasting cycles on an authentication exchange. (This also
	 * allows a pg_ping utility to be written.)
	 */
	switch (port->canAcceptConnections)
	{
		case CAC_STARTUP:
			ereport(FATAL,
					(errcode(ERRCODE_CANNOT_CONNECT_NOW),
					 errmsg("the database system is starting up")));
			break;
		case CAC_NOTCONSISTENT:
			if (EnableHotStandby)
				ereport(FATAL,
						(errcode(ERRCODE_CANNOT_CONNECT_NOW),
						 errmsg("the database system is not yet accepting connections"),
						 errdetail("Consistent recovery state has not been yet reached.")));
			else
				ereport(FATAL,
						(errcode(ERRCODE_CANNOT_CONNECT_NOW),
						 errmsg("the database system is not accepting connections"),
						 errdetail("Hot standby mode is disabled.")));
			break;
		case CAC_SHUTDOWN:
			ereport(FATAL,
					(errcode(ERRCODE_CANNOT_CONNECT_NOW),
					 errmsg("the database system is shutting down")));
			break;
		case CAC_RECOVERY:
			ereport(FATAL,
					(errcode(ERRCODE_CANNOT_CONNECT_NOW),
					 errmsg("the database system is in recovery mode")));
			break;
		case CAC_TOOMANY:
			ereport(FATAL,
					(errcode(ERRCODE_TOO_MANY_CONNECTIONS),
					 errmsg("sorry, too many clients already")));
			break;
		case CAC_SUPERUSER:
			/* OK for now, will check in InitPostgres */
			break;
		case CAC_OK:
			break;
	}
    /* End - OA-2543 */
    
    buf = makeStringInfo();

    // send handshakev10 packet
    handshake = make_mysqld_handshakev10_packet(mysql_conn_info.scramble);
    send_handshakev10_packet(buf, handshake);

    // flush send buffer data to client
    pq_flush();

    // read user login request packet
    if (mq_getmessage(buf) != STATUS_OK)
    {
        DestroyStringInfo(buf);
        return EOF;
    }

    // read handshake response packet
    handshakeResp = read_handshake_response(buf, port);
    if (handshakeResp == NULL)
    {
        DestroyStringInfo(buf);
        return EOF;
    }

    set_client_capabilities(handshakeResp->client_capabilities);
    
    mysql_conn_info.charset = handshakeResp->charset;
    mysql_conn_info.max_packet_size = handshakeResp->max_packet_size;

    if (handshakeResp->auth_response)
    {
        memcpy(mysql_conn_info.auth_response, handshakeResp->auth_response, CLIENT_PASSWORD_SHA256_TOKEN_LEN);
        mysql_conn_info.auth_response[CLIENT_PASSWORD_SHA256_TOKEN_LEN] = 0x00;
    }
    
    oldcontext = MemoryContextSwitchTo(TopMemoryContext);

    port->user_name = pstrdup(handshakeResp->username);

    /* Resolve PG database from the port-to-database mapping */
	/* Begin OA#2577 */
	pg_getnameinfo_all(&port->laddr.addr, port->laddr.salen,
						local_host, sizeof(local_host),
						local_port, sizeof(local_port),
						NI_NUMERICHOST | NI_NUMERICSERV);

	lp = atoi(local_port);

	entry = lookup_mysql_instance(lp);
	Assert(entry != NULL);

	port->database_name = pstrdup(entry->database);
	/* End OA#2577 */

    MemoryContextSwitchTo(oldcontext);

    // Set default scheam value and check && set search_path in mysql_client_authentication_finish()
    if (handshakeResp->database)
        mysql_conn_info.schema = pstrdup(handshakeResp->database);
    
    free_handshake(handshake);
    free_handshake_response(handshakeResp);

    DestroyStringInfo(buf);

    return STATUS_OK;
}

static int
exec_mysql_password_auth(Port *port)
{
    int           ret = STATUS_ERROR;
    char         *logdetail;
    char         *rolpassword;
    PasswordType  pwtype;
    mysql_auth_plugin_t *auth_plugin = NULL;

    rolpassword = get_role_password(port->user_name, &logdetail);

    if (!rolpassword)
        return STATUS_ERROR;
    
    pwtype = get_password_type(rolpassword);

    auth_plugin = get_auth_plugin_by_pwtype(pwtype);

    if (!auth_plugin)
    {
        ereport(ERROR,
                errmsg("Mysql compatition mode unsupport this auth method"));
    }

    // parse shadow password stored in pg_authid and do password check
    ret = auth_plugin->passwd_check(port, rolpassword);

    // clean
    if (rolpassword)
        pfree(rolpassword);

    return ret;
}

/*
 * Copy from ClientAuthentication()
 */
void
mysql_client_authentication(Port *port)
{
	/*
	 * Get the authentication method to use for this frontend/database
	 * combination.  Note: we do not parse the file at this point; this has
	 * already been done elsewhere.  hba.c dropped an error message into the
	 * server logfile if parsing the hba config file failed.
	 */
	hba_getauthmethod(port);

	CHECK_FOR_INTERRUPTS();

	/*
	 * This is the first point where we have access to the hba record for the
	 * current connection, so perform any verifications based on the hba
	 * options field that should be done *before* the authentication here.
	 */
	if (port->hba->clientcert != clientCertOff)
	{
		/* If we haven't loaded a root certificate store, fail */
		if (!secure_loaded_verify_locations())
			ereport(FATAL,
					(errcode(ERRCODE_CONFIG_FILE_ERROR),
					 errmsg("client certificates can only be checked if a root certificate store is available")));

		/*
		 * If we loaded a root certificate store, and if a certificate is
		 * present on the client, then it has been verified against our root
		 * certificate store, and the connection would have been aborted
		 * already if it didn't verify ok.
		 */
		if (!port->peer_cert_valid)
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					 errmsg("connection requires a valid client certificate")));
	}

	/*
	 * Now proceed to do the actual authentication check
	 */
	switch (port->hba->auth_method)
	{
		case uaReject:
			/*
			 * An explicit "reject" entry in pg_hba.conf.  This report exposes
			 * the fact that there's an explicit reject entry, which is
			 * perhaps not so desirable from a security standpoint; but the
			 * message for an implicit reject could confuse the DBA a lot when
			 * the true situation is a match to an explicit reject.  And we
			 * don't want to change the message for an implicit reject.  As
			 * noted below, the additional information shown here doesn't
			 * expose anything not known to an attacker.
			 */
			{
				char		hostinfo[NI_MAXHOST];
				const char *encryption_state;

				pg_getnameinfo_all(&port->raddr.addr, port->raddr.salen,
								   hostinfo, sizeof(hostinfo),
								   NULL, 0,
								   NI_NUMERICHOST);

				encryption_state =
#ifdef ENABLE_GSS
					(port->gss && port->gss->enc) ? _("GSS encryption") :
#endif
#ifdef USE_SSL
					port->ssl_in_use ? _("SSL encryption") :
#endif
					_("no encryption");

				if (am_walsender && !am_db_walsender)
					ereport(FATAL,
							(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					/* translator: last %s describes encryption state */
							 errmsg("pg_hba.conf rejects replication connection for host \"%s\", user \"%s\", %s",
									hostinfo, port->user_name,
									encryption_state)));
				else
					ereport(FATAL,
							(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					/* translator: last %s describes encryption state */
							 errmsg("pg_hba.conf rejects connection for host \"%s\", user \"%s\", database \"%s\", %s",
									hostinfo, port->user_name,
									port->database_name,
									encryption_state)));
				break;
			}
		case uaImplicitReject:
			/*
			 * No matching entry, so tell the user we fell through.
			 *
			 * NOTE: the extra info reported here is not a security breach,
			 * because all that info is known at the frontend and must be
			 * assumed known to bad guys.  We're merely helping out the less
			 * clueful good guys.
			 */
			{
				char		hostinfo[NI_MAXHOST];
				const char *encryption_state;

				pg_getnameinfo_all(&port->raddr.addr, port->raddr.salen,
								   hostinfo, sizeof(hostinfo),
								   NULL, 0,
								   NI_NUMERICHOST);

				encryption_state =
#ifdef ENABLE_GSS
					(port->gss && port->gss->enc) ? _("GSS encryption") :
#endif
#ifdef USE_SSL
					port->ssl_in_use ? _("SSL encryption") :
#endif
					_("no encryption");

#define HOSTNAME_LOOKUP_DETAIL(port) \
				(port->remote_hostname ? \
				 (port->remote_hostname_resolv == +1 ? \
				  errdetail_log("Client IP address resolved to \"%s\", forward lookup matches.", \
								port->remote_hostname) : \
				  port->remote_hostname_resolv == 0 ? \
				  errdetail_log("Client IP address resolved to \"%s\", forward lookup not checked.", \
								port->remote_hostname) : \
				  port->remote_hostname_resolv == -1 ? \
				  errdetail_log("Client IP address resolved to \"%s\", forward lookup does not match.", \
								port->remote_hostname) : \
				  port->remote_hostname_resolv == -2 ? \
				  errdetail_log("Could not translate client host name \"%s\" to IP address: %s.", \
								port->remote_hostname, \
								gai_strerror(port->remote_hostname_errcode)) : \
				  0) \
				 : (port->remote_hostname_resolv == -2 ? \
					errdetail_log("Could not resolve client IP address to a host name: %s.", \
								  gai_strerror(port->remote_hostname_errcode)) : \
					0))

				if (am_walsender && !am_db_walsender)
					ereport(FATAL,
							(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					/* translator: last %s describes encryption state */
							 errmsg("no pg_hba.conf entry for replication connection from host \"%s\", user \"%s\", %s",
									hostinfo, port->user_name,
									encryption_state),
							 HOSTNAME_LOOKUP_DETAIL(port)));
				else
					ereport(FATAL,
							(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					/* translator: last %s describes encryption state */
							 errmsg("no pg_hba.conf entry for host \"%s\", user \"%s\", database \"%s\", %s",
									hostinfo, port->user_name,
									port->database_name,
									encryption_state),
							 HOSTNAME_LOOKUP_DETAIL(port)));
				break;
			}
        case uaNative:
#ifdef USE_OPENSSL
        case uaCachingSha256:
#endif
#ifdef USE_GMSSL
        case uaSM3:
#endif
            if (exec_mysql_password_auth(port) != STATUS_OK)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_INVALID_PASSWORD),
                         errmsg("Access denied for user '%s'@'%s' (using password: YES)", port->user_name, port->remote_host)));
            }
            break;
#ifdef USE_GMSSL
        case uaMD5SM3:
        case uaSM3MD5:
        case uaSCRAMsm3:
        case uaSCRAMsm3sha256:
        case uaSCRAMsha256sm3:
#endif
        case uaMD5:
        case uaSCRAMsha256:
        case uaGSS:
		case uaSSPI:
		case uaPeer:
		case uaIdent:
		case uaPassword:
		case uaPAM:
		case uaBSD:
		case uaLDAP:
		case uaRADIUS:
		case uaCert:
			{
                /* the above authentication methods are not supported for mysql */
                char    hostinfo[NI_MAXHOST];

                pg_getnameinfo_all(&port->raddr.addr, port->raddr.salen,
                                   hostinfo, sizeof(hostinfo),
                                   NULL, 0,
                                   NI_NUMERICHOST);

                ereport(FATAL,
                        (errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
                         errmsg("pg_hba.conf entry specifies unsupported mysql authentication for host \"%s\", user \"%s\", database \"%s\"",
                                hostinfo, port->user_name, port->database_name),
                         errhint("Supported methods are trust")));
                break;
            }
		case uaTrust:
			// auth success
			break;
	}
}

void
mysql_client_authentication_finish(Port *port)
{
    StringInfoData pathbuf;
    CHARSET_INFO *charset_info;
    /* Begin - bug#6321 */
    if (mysql_conn_info.schema)
    {
        SetCurrentStatementStartTimestamp();
		StartTransactionCommand();

        if (!OidIsValid(get_namespace_oid(mysql_conn_info.schema, true)))
        {
            ereport(ERROR,
                    (errcode(ERRCODE_UNDEFINED_SCHEMA),
                     errmsg("Unknown database %s", mysql_conn_info.schema)));
        }

        initStringInfo(&pathbuf);
        appendStringInfoString(&pathbuf, quote_identifier(mysql_conn_info.schema));
        SetConfigOption("search_path", pathbuf.data, PGC_USERSET, PGC_S_OVERRIDE); /* OA-REQ-2037 */
        pfree(pathbuf.data);
        /* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
        charset_info = search_charset_info_by_cs_number(mysql_conn_info.charset);

        SetConfigOption("client_encoding",
                        charset_info->csname,
                        PGC_USERSET,
                        PGC_S_CLIENT);
        /* End - SRS-MYSQL-CHARSET-INTRODUCER */

        CommitTransactionCommand();
    }
    /* End - bug#6321 */

    send_ok(0, 0, 0, 0, NULL);

    pq_flush();
}

/**
 * Generate shadow password for CreateRole() and AlterRole()
 */
static char *
mysql_encrypt_password(PasswordType target_type, const char *rolename, const char *password)
{
    mysql_auth_plugin_t *auth_plugin = NULL;
	
    auth_plugin = get_auth_plugin_by_pwtype(target_type);

    Assert (auth_plugin != NULL);

    return auth_plugin->password_generate(password, strlen(password));
}

void
register_mysql_auth()
{
    pre_mysql_encrypt_password_hook = mysql_encrypt_password_hook;
	mysql_encrypt_password_hook = mysql_encrypt_password;
}

void
unregister_mysql_auth()
{
    mysql_encrypt_password_hook = pre_mysql_encrypt_password_hook;
}

uint8 *
do_auth_switch(char *auth, uint8 *scramble)
{
    uint8 *auth_response = NULL;
    StringInfo buf = makeStringInfo();

    send_auth_switch_packet(auth, scramble);

    if (mq_getmessage(buf) != STATUS_OK)
        goto fail;

    auth_response = (uint8 *) read_switch_response(buf);

fail:
    DestroyStringInfo(buf);

    return auth_response;
}
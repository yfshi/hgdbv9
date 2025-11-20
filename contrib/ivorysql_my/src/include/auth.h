#ifndef MYS_AUTH_H
#define MYS_AUTH_H

#include "postgres.h"
#include "libpq/crypt.h"

typedef struct {
    PasswordType type;
    void  (*clean)();
    char *(*password_generate)(const char *plain_password, int len);
    void  (*password_parse)(char *);        // parser rolpassword stored in pg_authid
    int   (*passwd_check)(Port *port);      // called after password_parse
} mysql_auth_plugin_t;

extern mysql_auth_plugin_t mysql_native_password;

#ifdef USE_OPENSSL
extern mysql_auth_plugin_t caching_sha2_password;
extern void create_mysql_auth_cache(void);
#endif

extern void register_mysql_auth(void);
extern void unregister_mysql_auth(void);
extern int mysql_conn_handshake(Port* port);
extern void mysql_client_authentication(Port *port);
extern void mysql_client_authentication_finish(Port *port);
#endif
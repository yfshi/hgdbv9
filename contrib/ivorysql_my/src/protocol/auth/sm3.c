#include "postgres.h"

#include "libpq/crypt.h"

#include "../../include/auth.h"
#include "../../include/message.h"
#include "../../include/mqformat.h"
#include "../../include/server.h"

#ifdef USE_GMSSL
#define SM3_PASSWORD_TAG         "sm3_password"

static int
sm3_password_check(Port *port, char *rolpassword)
{
    int	    status = STATUS_ERROR;
    char    *logdetail = NULL;
    uint8   *scramble = mysql_conn_info.scramble;
    uint8   *auth_response;

    scramble[4] = '\0';

    auth_response = do_auth_switch(SM3_PASSWORD_TAG, scramble);

    if (auth_response == NULL)
        return status;
    
    status = sm3_crypt_verify(port->user_name, 
                              rolpassword,
                              (char *) auth_response,
                              (char *) scramble,
                              4,
                              &logdetail);

    return status;
}

mysql_auth_plugin_t sm3_password = {
    .type = PASSWORD_TYPE_SM3,
    .password_generate = NULL,
    .passwd_check = sm3_password_check,
};
#endif
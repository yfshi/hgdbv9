#include "postgres.h"
#include "common/base64.h"
#include "common/cryptohash.h"
#include "common/sha1.h"

#include "../../include/auth.h"
#include "../../include/message.h"
#include "../../include/mqformat.h"
#include "../../include/server.h"

#define NATIVE_PASSWORD_NPART       1
#define NATIVE_DIGEST_INDEX         0
#define NATIVE_PASSWORD_TAG         "mysql_native_password"
#define SHA1_HASH_SIZE              20

static char *part[NATIVE_PASSWORD_NPART];

static void
compute_sha1_hash(uint8 *dest, size_t dest_size, const uint8 *buf, size_t len)
{
    pg_cryptohash_ctx *ctx = NULL;
    ctx = pg_cryptohash_create(PG_SHA1);
    pg_cryptohash_init(ctx);
    pg_cryptohash_update(ctx, buf, len);
    pg_cryptohash_final(ctx, dest, dest_size);
    pg_cryptohash_free(ctx);
}

static void
compute_sha1_hash_multi(uint8 *dest, size_t dest_size, const uint8 *buf1, int len1, const uint8 *buf2, int len2)
{
    pg_cryptohash_ctx *ctx = NULL;
    ctx = pg_cryptohash_create(PG_SHA1);
    pg_cryptohash_init(ctx);
    pg_cryptohash_update(ctx, buf1, len1);
    pg_cryptohash_update(ctx, buf2, len2);
    pg_cryptohash_final(ctx, dest, dest_size);
    pg_cryptohash_free(ctx);
}

static void
my_xor(uint8 *to, const uint8 *s1, const uint8 *s2, uint32 len)
{
    const uint8 *s1_end = s1 + len;
    while (s1 < s1_end) 
        *to++ = *s1++ ^ *s2++;
}

/*
 * Caculate s2 from plain password, then
 * format it to rolpassword stored in pg_authid.
 */
static char *
native_password_generate(const char *plain_password, int len)
{
    char    base64[512];
    char    rolpassword[1024];
    int     base64_len;

    uint8   s1[SCRAMBLE_LENGTH + 1],
            s2[SCRAMBLE_LENGTH + 1];

    // s1 = sha1(password)
    compute_sha1_hash(s1, sizeof(s1), (uint8 *) plain_password, strlen(plain_password));

    // s2 = sha1(s2)
    compute_sha1_hash(s2, sizeof(s2), s1, SCRAMBLE_LENGTH);

    base64_len = pg_b64_encode((char *) s2, SCRAMBLE_LENGTH, base64, sizeof(base64));

    if (base64_len < 0)
    {
        elog(ERROR, "mysql_native_password: could not encode s2");
    }

    snprintf(rolpassword, sizeof(rolpassword), "%s$%.*s", NATIVE_PASSWORD_TAG, base64_len, base64);

    return pstrdup(rolpassword);
}

static void
native_password_parse(char *rolpassword)
{
    int  len1,
         len2;
    char *tag;
    char *raw;
    char *digest_base64;

    raw = pstrdup(rolpassword);
    tag = strtok(raw, "$");

    if (!tag || strcmp(NATIVE_PASSWORD_TAG, tag))
    {
        ereport(ERROR,
                errmsg("rolpassword is not mysql_native_password encoding!"));
    }

    digest_base64 = strtok(NULL, "$");
    len1 = strlen(digest_base64);
    len2 = pg_b64_dec_len(len1);

    part[NATIVE_DIGEST_INDEX] = (char *) palloc0(len2 + 1);

    if (pg_b64_decode(digest_base64, len1, part[NATIVE_DIGEST_INDEX], len2) < 0)
    {
        elog(ERROR, "mysql_native_password: could not decode digest");
    }

    pfree(raw);
}

static void
native_password_clean()
{
    for (int i = 0; i < NATIVE_PASSWORD_NPART; ++i)
    {
        if (part[i])
        {
            pfree(part[i]);
            part[i] = NULL;
        }
    }
}

/*
 * password stored in pg_authid is s2
 * Client:
 *  s1 = sha1(password)
 *  s2 = sha1(s1)
 *  s3 = sha1(scramble, s2)
 *  auth_response = XOR(s3, s1)
 */
static int
native_password_check(Port *port, char *rolpassword)
{
    int ret = STATUS_ERROR;
    uint8 s3[SCRAMBLE_LENGTH + 1],  // caculate with with s2 stored in pg_authid
          _s1[SCRAMBLE_LENGTH + 1], // caculate with client's auth_response
          _s2[SCRAMBLE_LENGTH + 1]; // caculate with client's auth_response
    char  *s2;
    uint8 *auth_response = NULL;
    uint8 *scramble = mysql_conn_info.scramble;

    native_password_parse(rolpassword);

    s2  = part[NATIVE_DIGEST_INDEX];

    auth_response = do_auth_switch(NATIVE_PASSWORD_TAG, scramble);

    /**
     * Begin - bug6787
     * Auth switch response from client may be empty.
     */
    if (auth_response == NULL)
        return ret;
    /* End - bug6787 */

    // s3 = sha1(scramble, s2)
    compute_sha1_hash_multi(s3, sizeof(s3), scramble, SHA1_HASH_SIZE, (uint8 *) s2, SHA1_DIGEST_LENGTH);

    // s1 = xor[xor(s3, s1), s3]
    my_xor(_s1, auth_response, s3, SCRAMBLE_LENGTH);

    // s2 = sha1(s1)
    compute_sha1_hash(_s2, sizeof(_s2), _s1, SCRAMBLE_LENGTH);

    if (strncmp(s2, (char *) _s2, SCRAMBLE_LENGTH) == 0)
        ret = STATUS_OK;
    
    pfree(auth_response);

    native_password_clean();

    return ret;
}

mysql_auth_plugin_t mysql_native_password = {
    .type = PASSWORD_TYPE_MYS_NATIVE,
    .password_generate = native_password_generate,
    .passwd_check = native_password_check,
};

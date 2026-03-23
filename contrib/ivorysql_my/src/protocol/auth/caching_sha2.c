#include "postgres.h"

#ifdef USE_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "c.h"
#include "common/base64.h"
#include "common/hashfn.h"
#include "storage/lock.h"
#include "storage/shmem.h"
#include "utils/guc.h"

#include "../../include/auth.h"
#include "../../include/message.h"
#include "../../include/mqformat.h"
#include "../../include/server.h"

#define SHA2_HASH_SIZE                      32
#define CACHING_SHA2_NPART                  4
#define CACHING_SHA2_DIGEST_TYPE_INDEX      0
#define CACHING_SHA2_SALT_INDEX             1
#define CACHING_SHA2_DIGEST_INDEX           2
#define CACHING_SHA2_ITERATION_INDEX        3

#define CACHING_PASSWORD_TAG        "caching_sha2_password"
#define SALT_LENGTH                 20
#define ROUNDS_DEFAULT              5000
#define ROUNDS_MIN                  ROUNDS_DEFAULT
#define ROUNDS_MAX                  (0xFFF * 1000)
#define MIXCHARS                    32
#define CRYPT_SALT_LENGTH           20
#define CRYPT_MAGIC_LENGTH          3
#define CRYPT_PARAM_LENGTH          13
#define SHA256_HASH_LENGTH          43
#define CRYPT_MAX_PASSWORD_SIZE     (CRYPT_SALT_LENGTH + SHA256_HASH_LENGTH + CRYPT_MAGIC_LENGTH + CRYPT_PARAM_LENGTH)

#define DIGEST_CTX                  EVP_MD_CTX
#define DIGEST_LEN                  SHA256_DIGEST_LENGTH

#define crypt_alg_magic             "$5"
#define crypt_alg_magic_len         2
#define ROUNDS                      "rounds="
#define ROUNDSLEN                   7
#define ITERATION_MULTIPLIER        1000

#define STORED_SHA256_DIGEST_LENGTH 43

#define MAX_CIPHER_LENGTH           1024

#define b64_from_24bit(B2, B1, B0, N)               \
{                                                   \
    uint32_t w = ((B2) << 16) | ((B1) << 8) | (B0); \
    int n = (N);                                    \
    while (--n >= 0 && ctbufflen > 0)               \
    {                                               \
        *p++ = b64t[w & 0x3f];                      \
        w >>= 6;                                    \
        ctbufflen--;                                \
    }                                               \
}

typedef struct {
    char username[NAMEDATALEN];
    char fastpasswd[SHA2_HASH_SIZE];
} UserCachedLinesHashEntry;

static char *part[CACHING_SHA2_NPART];
static unsigned char b64t[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static char request_public_key = '\2';
static char fast_auth_success = '\3';
static char perform_full_authentication = '\4';

static HTAB     *UserCachedLinesHash;
static LWLock   *UserCachedLinesHashLock;

static void
generate_user_salt(char *buffer, int buffer_len)
{
    char *end = buffer + buffer_len - 1;
    RAND_bytes((unsigned char *)buffer, buffer_len);

    /* Sequence must be a legal UTF8 string */
    for (; buffer < end; buffer++)
    {
        *buffer &= 0x7f;
        if (*buffer == '\0' || *buffer == '$')
            *buffer = *buffer + 1;
    }

    /* Make sure the buffer is terminated properly */
    *end = '\0';
}

static void
xor_string(char *to, int to_len, char *pattern, int pattern_len)
{
    int loop = 0;
    while (loop <= to_len)
    {
        *(to + loop) ^= *(pattern + loop % pattern_len);
        ++loop;
    }
}

static void
DIGESTCreate(DIGEST_CTX **ctx)
{
    if (ctx != NULL)
        *ctx = EVP_MD_CTX_create();
}

static void
DIGESTInit(DIGEST_CTX *ctx)
{
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
}

static void
DIGESTUpdate(DIGEST_CTX *ctx, const void *plaintext, int len)
{
    EVP_DigestUpdate(ctx, plaintext, len);
}

static void
DIGESTFinal(void *txt, DIGEST_CTX *ctx)
{
    EVP_DigestFinal_ex(ctx, (unsigned char *)txt, NULL);
}

static void
DIGESTDestroy(DIGEST_CTX **ctx)
{
    if (ctx != NULL)
    {
        EVP_MD_CTX_destroy(*ctx);
        *ctx = NULL;
    }
}

static uint32
getrounds(const char *s)
{
    const char *r;
    const char *p;
    char *e;
    long val;

    if (s == NULL)
        return 0;

    if ((r = strstr(s, ROUNDS)) == NULL)
        return 0;

    if (strncmp(r, ROUNDS, ROUNDSLEN) != 0)
        return 0;

    p = r + ROUNDSLEN;
    errno = 0;
    val = strtol(p, &e, 10);

    /*
     * An error occurred or there is non-numeric stuff at the end
     * which isn't one of the crypt(3c) special chars ',' or '$'
     */
    if (errno != 0 || val < 0 || !(*e == '\0' || *e == ',' || *e == '$'))
        return 0;

    return (uint32_t)val;
}

static char *
my_crypt_genhash(char *ctbuffer, size_t ctbufflen, const char *plaintext,
                 size_t plaintext_len, const char *salt,
                 const char **params, unsigned int *num_rounds)
{
    size_t  i;
    int     salt_len;
    int     srounds = 0;
    bool    custom_rounds = false;
    char    *p;
    char    *P, *Pp, *PPbuf;
    char    *S, *Sp, *SSbuf;
    unsigned char A[DIGEST_LEN];
    unsigned char B[DIGEST_LEN];
    unsigned char DP[DIGEST_LEN];
    unsigned char DS[DIGEST_LEN];
    DIGEST_CTX  *ctxA,
                *ctxB,
                *ctxC,
                *ctxDP,
                *ctxDS = NULL;
    unsigned int rounds = num_rounds && (*num_rounds <= ROUNDS_MAX && *num_rounds >= ROUNDS_MIN) ? *num_rounds : ROUNDS_DEFAULT;

    /* Create Digest context. */
    DIGESTCreate(&ctxA);
    DIGESTCreate(&ctxB);
    DIGESTCreate(&ctxC);
    DIGESTCreate(&ctxDP);
    DIGESTCreate(&ctxDS);

    if (num_rounds)
        *num_rounds = rounds;

    /* skip our magic string */
    if (strncmp(salt, crypt_alg_magic, crypt_alg_magic_len) == 0)
        salt += crypt_alg_magic_len + 1;

    srounds = getrounds(salt);

    if (srounds != 0)
    {
        rounds = Max(ROUNDS_MIN, Min(srounds, ROUNDS_MAX));
        custom_rounds = true;
        p = strchr(salt, '$');
        if (p != NULL)
            salt = p + 1;
    }

    salt_len = Min(strcspn(salt, "$"), CRYPT_SALT_LENGTH);
    // plaintext_len = strlen(plaintext);

    /* 1. */
    DIGESTInit(ctxA);

    /* 2. The password first, since that is what is most unknown */
    DIGESTUpdate(ctxA, plaintext, plaintext_len);

    /* 3. Then the raw salt */
    DIGESTUpdate(ctxA, salt, salt_len);

    /* 4. - 8. */
    DIGESTInit(ctxB);
    DIGESTUpdate(ctxB, plaintext, plaintext_len);
    DIGESTUpdate(ctxB, salt, salt_len);
    DIGESTUpdate(ctxB, plaintext, plaintext_len);
    DIGESTFinal(B, ctxB);

    /* 9. - 10. */
    for (i = plaintext_len; i > MIXCHARS; i -= MIXCHARS)
    DIGESTUpdate(ctxA, B, MIXCHARS);
    DIGESTUpdate(ctxA, B, i);

    /* 11. */
    for (i = plaintext_len; i > 0; i >>= 1) 
    {
        if ((i & 1) != 0)
        {
            DIGESTUpdate(ctxA, B, MIXCHARS);
        }
        else
        {
            DIGESTUpdate(ctxA, plaintext, plaintext_len);
        }
    }

    /* 12. */
    DIGESTFinal(A, ctxA);

    /* 13. - 15. */
    DIGESTInit(ctxDP);
    for (i = 0; i < plaintext_len; i++)
        DIGESTUpdate(ctxDP, plaintext, plaintext_len);
    DIGESTFinal(DP, ctxDP);

    /* 16. */
    PPbuf = (char *) palloc0(plaintext_len);
    Pp = P = PPbuf;
    for (i = plaintext_len; i >= MIXCHARS; i -= MIXCHARS)
    {
        Pp = (char *) (memcpy(Pp, DP, MIXCHARS)) + MIXCHARS;
    }

    (void)memcpy(Pp, DP, i);

    /* 17. - 19. */
    DIGESTInit(ctxDS);
    for (i = 0; i < 16U + (uint8_t) A[0]; i++)
    {
        DIGESTUpdate(ctxDS, salt, salt_len);
    }
    DIGESTFinal(DS, ctxDS);

    /* 20. */
    SSbuf = (char *) palloc0(salt_len);
    Sp = S = SSbuf;

    for (i = salt_len; i >= MIXCHARS; i -= MIXCHARS)
    {
        Sp = (char *) (memcpy(Sp, DS, MIXCHARS)) + MIXCHARS;
    }

    memcpy(Sp, DS, i);

    /*  21. */
    for (i = 0; i < rounds; i++)
    {
        DIGESTInit(ctxC);

        if ((i & 1) != 0)
        {
            DIGESTUpdate(ctxC, P, plaintext_len);
        }
        else
        {
            if (i == 0)
                DIGESTUpdate(ctxC, A, MIXCHARS);
            else
                DIGESTUpdate(ctxC, DP, MIXCHARS);
        }

        if (i % 3 != 0)
            DIGESTUpdate(ctxC, S, salt_len);

        if (i % 7 != 0)
            DIGESTUpdate(ctxC, P, plaintext_len);

        if ((i & 1) != 0)
        {
            if (i == 0)
                DIGESTUpdate(ctxC, A, MIXCHARS);
            else
                DIGESTUpdate(ctxC, DP, MIXCHARS);
        }
        else
        {
            DIGESTUpdate(ctxC, P, plaintext_len);
        }
        DIGESTFinal(DP, ctxC);
    }

    /* 22. Now make the output string */
    if (custom_rounds)
    {
        (void) snprintf(ctbuffer, ctbufflen, "%s$rounds=%zu$", crypt_alg_magic, (size_t) rounds);
    }
    else
    {
        (void) snprintf(ctbuffer, ctbufflen, "%s$", crypt_alg_magic);
    }

    (void) strncat(ctbuffer, (const char *)salt, salt_len);
    (void) strlcat(ctbuffer, "$", ctbufflen);

    p = ctbuffer + strlen(ctbuffer);
    ctbufflen -= strlen(ctbuffer);

    b64_from_24bit(DP[0], DP[10], DP[20], 4);
    b64_from_24bit(DP[21], DP[1], DP[11], 4);
    b64_from_24bit(DP[12], DP[22], DP[2], 4);
    b64_from_24bit(DP[3], DP[13], DP[23], 4);
    b64_from_24bit(DP[24], DP[4], DP[14], 4);
    b64_from_24bit(DP[15], DP[25], DP[5], 4);
    b64_from_24bit(DP[6], DP[16], DP[26], 4);
    b64_from_24bit(DP[27], DP[7], DP[17], 4);
    b64_from_24bit(DP[18], DP[28], DP[8], 4);
    b64_from_24bit(DP[9], DP[19], DP[29], 4);
    b64_from_24bit(0, DP[31], DP[30], 3);
    *p = '\0';

    (void) memset(A, 0, sizeof(A));
    (void) memset(B, 0, sizeof(B));
    (void) memset(DP, 0, sizeof(DP));
    (void) memset(DS, 0, sizeof(DS));

    /* 23. Clearing the context */
    DIGESTDestroy(&ctxA);
    DIGESTDestroy(&ctxB);
    DIGESTDestroy(&ctxC);
    DIGESTDestroy(&ctxDP);
    DIGESTDestroy(&ctxDS);
    return (ctbuffer);
}

static char *
generate_full_digest(const char *plain_password, char *salt, unsigned int iterations)
{
    char buffer[CRYPT_MAX_PASSWORD_SIZE + 1] = {0};

    my_crypt_genhash(buffer, CRYPT_MAX_PASSWORD_SIZE,
                     plain_password, strlen(plain_password),
                     salt, NULL, &iterations);

    buffer[3 + SALT_LENGTH + 1 + STORED_SHA256_DIGEST_LENGTH] = '\0';

    return pstrdup(buffer + 3 + SALT_LENGTH + 1);
}

static char *
caching_sha2_password_generate(const char *plain_password, int len)
{
    char *digest;
    char ret[1024];
    char base64_salt[512],
         base64_digest[512];
    char salt[SALT_LENGTH + 1] = {0};
    int  base64_salt_len,
         base64_digest_len;
    unsigned int iterations = ROUNDS_DEFAULT;

    generate_user_salt(salt, SALT_LENGTH + 1);

    /*
     * Returned value in buffer would be in format:
     *   $5$<SALT_LENGTH><STORED_SHA256_DIGEST_LENGTH>
     * We need to extract STORED_SHA256_DIGEST_LENGTH chars from it
     */
    digest = generate_full_digest(plain_password, salt, iterations);

    base64_salt_len = pg_b64_enc_len(SCRAMBLE_LENGTH);
    pg_b64_encode((char *) salt, SCRAMBLE_LENGTH, base64_salt, base64_salt_len);

    base64_digest_len = pg_b64_enc_len(STORED_SHA256_DIGEST_LENGTH);
    pg_b64_encode((char *) digest, STORED_SHA256_DIGEST_LENGTH, base64_digest, base64_digest_len);

    snprintf(ret, sizeof(ret), "%s$A$00%d$%.*s$%.*s", CACHING_PASSWORD_TAG, iterations / ITERATION_MULTIPLIER,
             base64_salt_len, base64_salt,
             base64_digest_len, base64_digest);

    pfree(digest);

    return pstrdup(ret);
}

static void
caching_sha2_password_parse(char *rolpassword)
{
    char *tag,
         *digest_type,
         *iterations,
         *salt,
         *digest;

    char *raw = pstrdup(rolpassword);

    tag = strtok(raw, "$");
    if (!tag || strcmp(CACHING_PASSWORD_TAG, tag))
    {
        ereport(ERROR,
                errmsg("rolpassword is not caching_sha2_password encoding!"));
    }

    digest_type = strtok(NULL, "$");
    iterations = strtok(NULL, "$"); // 005
    salt = strtok(NULL, "$");
    digest = strtok(NULL, "$");

    part[CACHING_SHA2_DIGEST_TYPE_INDEX] = pstrdup(digest_type);
    part[CACHING_SHA2_ITERATION_INDEX] = pstrdup(iterations);
    part[CACHING_SHA2_SALT_INDEX] = (char *) palloc0(SALT_LENGTH + 1);
    part[CACHING_SHA2_DIGEST_INDEX] = (char *) palloc0(STORED_SHA256_DIGEST_LENGTH + 1);

    pg_b64_decode(salt, strlen(salt), part[CACHING_SHA2_SALT_INDEX], SALT_LENGTH + 1);
    pg_b64_decode(digest, strlen(digest), part[CACHING_SHA2_DIGEST_INDEX], STORED_SHA256_DIGEST_LENGTH + 1);

    pfree(raw);
}

static void
send_fast_authenticate_packet()
{
    StringInfo fast_OK = makeStringInfo(); // send fast ok packet
    mq_append_int1(fast_OK, '\1');
    mq_append_int1(fast_OK, fast_auth_success);
    mq_putmessage(fast_OK->data, fast_OK->len);
    pq_flush();
    DestroyStringInfo(fast_OK);
}

static UserCachedLinesHashEntry *
UserCachedLinesHashTableAccess(HASHACTION action, char *username, char *fastpassword)
{
    bool found = false;
    UserCachedLinesHashEntry *result = NULL;
    
    Assert(username);

    result = (UserCachedLinesHashEntry *) hash_search(UserCachedLinesHash, username, action, &found);
    if (action == HASH_ENTER) 
    {
        memcpy(result->fastpasswd, fastpassword, SHA2_HASH_SIZE);
        return result;
    } 
    else if (action == HASH_FIND) 
    {
        if (found)
            return result;
    } 
    else if (action == HASH_REMOVE) 
    {
        if (found)
            return result;
    }

    return NULL;
}

static void
compute_sha2_hash(char *digest, const char *buf, size_t len)
{
    EVP_MD_CTX *sha2_context = EVP_MD_CTX_create();
    EVP_DigestInit_ex(sha2_context, EVP_sha256(), NULL);
    EVP_DigestUpdate(sha2_context, buf, len);
    EVP_DigestFinal_ex(sha2_context, (unsigned char *)digest, NULL);
    EVP_MD_CTX_destroy(sha2_context);
    sha2_context = NULL;
}

static void
compute_sha2_hash_multi(char *digest, const char *s1, size_t len1, const char *s2, size_t len2)
{
    EVP_MD_CTX *sha2_context = EVP_MD_CTX_create();
    EVP_DigestInit_ex(sha2_context, EVP_sha256(), NULL);
    EVP_DigestUpdate(sha2_context, s1, len1);
    EVP_DigestUpdate(sha2_context, s2, len2);
    EVP_DigestFinal_ex(sha2_context, (unsigned char *)digest, NULL);
    EVP_MD_CTX_destroy(sha2_context);
    sha2_context = NULL;
}

static bool
caching_sha2_fast_authenticate(Port *port)
{
    char _s1[SHA2_HASH_SIZE + 1],
         *s2,
         _s2[SHA2_HASH_SIZE + 1],
         s3[SHA2_HASH_SIZE + 1],
         *salt = (char *) mysql_conn_info.scramble,
         *_digest = (char *) mysql_conn_info.auth_response;

    UserCachedLinesHashEntry *result;

    LWLockAcquire(UserCachedLinesHashLock, LW_SHARED);
    result = UserCachedLinesHashTableAccess(HASH_FIND, port->user_name, NULL);
    LWLockRelease(UserCachedLinesHashLock);

    if (result == NULL)
        return false;

    s2 = result->fastpasswd;

    // s3 = sha2(salt, s2)
    compute_sha2_hash_multi(s3, s2, SHA2_HASH_SIZE, salt, SCRAMBLE_LENGTH);

    /* 
     * digest = xor(s3, s1)
     * s1 = xor(digest, s3)
     */
    memcpy(_s1, _digest, SHA2_HASH_SIZE);
    xor_string(_s1, SHA2_HASH_SIZE, s3, SHA2_HASH_SIZE);

    // s2 = sha2(s1)
    compute_sha2_hash(_s2, _s1, SHA2_HASH_SIZE);

    if (memcmp(s2, _s2, SHA2_HASH_SIZE) != 0)
        return false;
    
    send_fast_authenticate_packet();
    
    return true;
}

static void
send_full_authenticate_packet()
{
    StringInfo buf = makeStringInfo();
    mq_append_int1(buf, '\1');
    mq_append_int1(buf, perform_full_authentication);
    mq_putmessage(buf->data, buf->len);
    pq_flush();
    DestroyStringInfo(buf);
}

static bool
channel_is_secure(Port *port)
{
    // ssl
    if (port->ssl_in_use)
        return true;

    // unix domain socket
    if (port->remote_hostname && strcmp(port->remote_hostname, "[local]") == 0)
        return true;

    return false;
}

static char *
generate_fast_digest(char *plain_password)
{
    char s1[SHA2_HASH_SIZE],
         s2[SHA2_HASH_SIZE];

    compute_sha2_hash(s1, plain_password, strlen(plain_password));

    compute_sha2_hash(s2, (char *) s1, SHA2_HASH_SIZE);

    return pstrdup(s2);
}

static int
get_cipher_length(RSA *public_key)
{
    return RSA_size(public_key);
}

static bool
decrypt_RSA_private_key(unsigned char *pkt, int cipher_length, unsigned char *plain_text, size_t plain_text_len, RSA *private_key)
{
    if (RSA_private_decrypt(cipher_length, pkt, plain_text, private_key, RSA_PKCS1_OAEP_PADDING) == -1)
        return true;

    return false;
}

static RSA *
read_key_file(char *key_path, bool is_priv_key, char **key_text_buffer)
{
    RSA *key_ptr;
    const char *key_type = is_priv_key ? "private" : "public";
    FILE *fp = fopen(key_path, "r");

    if (!fp)
    {
        ereport(ERROR,
                errmsg("RSA %s key not found: %s, set %s_key_path in ivorysql.mysql.conf first",
                       key_type, key_path, key_type));
    }

    key_ptr = is_priv_key ? PEM_read_RSAPrivateKey(fp, 0, 0, 0) :
                            PEM_read_RSA_PUBKEY(fp, 0, 0, 0);

    if (!key_ptr)
    {
        char error_buf[512];
        ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
        ERR_clear_error();
        ereport(ERROR,
                errmsg("Fail to parse RSA %s key: %s: %s", key_type, key_path, error_buf));
    }

    if (!is_priv_key && key_text_buffer)
    {
        int filesize;
        int items_read;

        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        // Buffer start with 0x01 and end with 0x00
        *key_text_buffer = (char *) palloc0(filesize + 2);
        
        // Buffer must start with 0x01, reason don't know
        *key_text_buffer[0] = '\1';

        items_read = fread(*key_text_buffer + 1, filesize, 1, fp);

        if (items_read != 1)
        {
            ereport(ERROR, 
                    errmsg("Fail to read key file: %s", key_path));
        }

        // Buffer end with '\0'
        (*key_text_buffer)[filesize + 1] = '\0';
    }

    fclose(fp);

    return key_ptr;
}

static int
caching_sha2_full_authenticate(Port *port)
{
    int      ret = STATUS_ERROR;
    char    *salt,
            *digest,
            *_digest,
            *plain_password = NULL;
    char     plain_text[MAX_CIPHER_LENGTH + 1];
    uint8    _request_public_key;
    uint32   iterations;
    RSA     *public_key = NULL,
            *private_key = NULL;
    StringInfo buf = makeStringInfo();

    digest = part[CACHING_SHA2_DIGEST_INDEX]; // set in password_parse() callback
    salt = part[CACHING_SHA2_SALT_INDEX];
    iterations = atoi(part[CACHING_SHA2_ITERATION_INDEX]) * ITERATION_MULTIPLIER;

    send_full_authenticate_packet();

    if (mq_getmessage(buf) != STATUS_OK)
        goto fail;

    if (!channel_is_secure(port))
    {
        char    *cipher;
        char    *public_key_buffer;
        int      cipher_length;
        
        // recv & check request_public_key
        mq_get_int1(buf, &_request_public_key);
        if (_request_public_key != request_public_key)
            goto fail;

        private_key = read_key_file(sha256_password_private_key_path, true, NULL);
        public_key = read_key_file(sha256_password_public_key_path, false, &public_key_buffer);

        /**
         * Send public key file content.
         * This buffer must start with 0x01, but reason don't know.
         * If don't start with 0x01, response packet may be HandShakeResponse Packet,
         * which is not implemented in Mysql source code.
         * So, do special treatment when get key file content in function read_key_file().
         */
        mq_putmessage(public_key_buffer, strlen(public_key_buffer));

        pq_flush();
        
        // recv encrypted password
        if (mq_getmessage(buf) != STATUS_OK)
            goto fail;

        // all data in buffer is encrypted password
        cipher = buf->data;
        cipher_length = buf->len;

        if (get_cipher_length(public_key) != cipher_length)
            goto fail;

        if (decrypt_RSA_private_key((unsigned char *)cipher, cipher_length, (unsigned char *) plain_text, sizeof(plain_text) - 1, private_key))
            goto fail;

        plain_text[cipher_length] = '\0';
        xor_string((char *)plain_text, cipher_length, (char *)mysql_conn_info.scramble, SCRAMBLE_LENGTH);

        plain_password = plain_text;
    }
    else
        plain_password = mq_get_string_null(buf);
    
    _digest = generate_full_digest(plain_password, salt, iterations);

    if (memcmp(digest, _digest, STORED_SHA256_DIGEST_LENGTH) == 0)
    {
        char *fast = generate_fast_digest(plain_password);

        LWLockAcquire(UserCachedLinesHashLock, LW_EXCLUSIVE);
        UserCachedLinesHashTableAccess(HASH_ENTER, port->user_name, fast);
        LWLockRelease(UserCachedLinesHashLock);

        ret = STATUS_OK;
    }

fail:
    if (public_key)
        RSA_free(public_key);

    if (private_key)
        RSA_free(private_key);

    DestroyStringInfo(buf);
    return ret;
}

/**
 * Implentment in Mysql8 source code with
 * function caching_sha2_password_authenticate().
 */
static int
caching_sha2_password_check(Port *port)
{
    if (caching_sha2_fast_authenticate(port))
        return STATUS_OK;

    return caching_sha2_full_authenticate(port);
}

static void
caching_sha2_password_clean()
{
    for (int i = 0; i < CACHING_SHA2_NPART; ++i)
    {
        if (part[i])
        {
            pfree(part[i]);
            part[i] = NULL;
        }
    }
}

#ifdef USE_OPENSSL
void
create_mysql_auth_cache()
{
    bool	found;
    HASHCTL info;

    MemSet(&info, 0, sizeof(info));
    info.keysize = NAMEDATALEN;
    info.entrysize = sizeof(UserCachedLinesHashEntry);

    UserCachedLinesHash = ShmemInitHash("mysql auth cache",
                                        512,
                                        512,
                                        &info,
                                        HASH_ELEM | HASH_STRINGS);

    UserCachedLinesHashLock = ShmemInitStruct("mysql auth cache lock", sizeof(LWLock), &found);

    if (!found)
    {
        int tranche_id = LWLockNewTrancheId();
        LWLockRegisterTranche(tranche_id, "mysql auth cache lock");
		LWLockInitialize(UserCachedLinesHashLock, tranche_id);
    }
}
#endif

mysql_auth_plugin_t caching_sha2_password = {
    .type = PASSWORD_TYPE_MYS_CACHING_SHA2,
    .clean = caching_sha2_password_clean,
    .password_generate = caching_sha2_password_generate,
    .password_parse = caching_sha2_password_parse,
    .passwd_check = caching_sha2_password_check,
};
#endif
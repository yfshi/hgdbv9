#ifndef _PG_SM3_H_
#define _PG_SM3_H_

#define PG_SM3_BLOCK_LENGTH			64
#define PG_SM3_DIGEST_LENGTH			32
#define PG_SM3_STATE_WORDS		8
#define PG_SM3_DIGEST_STRING_LENGTH	(PG_SM3_DIGEST_LENGTH * 2 + 1)

#define SM3_PASSWD_CHARSET	"0123456789abcdef"
#define SM3_PASSWD_LEN	67

extern bool pg_sm3_hash(const void *buff, size_t len, char *hexsum);
extern bool pg_sm3_binary(const void *buff, size_t len, void *outbuf);
extern bool pg_sm3_encrypt(const char *passwd, const char *salt,
						   size_t salt_len, char *buf);

#endif

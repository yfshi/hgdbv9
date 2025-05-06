#ifndef PG_MFA_H
#define PG_MFA_H
#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

char *pg_base64_encode(const unsigned char *input, int length);
unsigned char *pg_base64_decode(const char *input, int *length);
char *pg_pkey_sign(const char *prikey, char *opt_salt, size_t *sig_len);
char *pg_get_cert(const char *file, char *opt_salt, size_t *cert_len);
char *pg_cert_sign(const char *file, char *opt_salt, size_t *sig_len);

#endif
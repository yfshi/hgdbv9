#ifndef PG_MFA_H
#define PG_MFA_H
#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

unsigned char *pq_pkey_sign(const char *prikey, char *opt_salt, size_t *sig_len);
char *pq_get_cert(const char *file, char *opt_salt, size_t *cert_len);
unsigned char *pq_cert_sign(const char *file, const char *opt_salt, size_t *sig_len);

#endif
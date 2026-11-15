/*
 * tecc_ktool_common.h
 *   Shared declarations for tecc_ktool commands and MEK handling.
 *
 *   src/bin/tecc_ktool/tecc_ktool_common.h
 */

#ifndef TECC_KTOOL_COMMON_H
#define TECC_KTOOL_COMMON_H

#include "postgres_fe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

/* KEK file header constants (mirrors kms_adapter.c) */
#define KEK_FILE_MAGIC       "TECCKEK"
#define KEK_FILE_MAGIC_LEN   7
#define KEK_FILE_HEADER_SIZE 16
#define KEK_FILE_SALT_SIZE   16
#define KEK_FILE_IV_SIZE     16
#define KEK_FILE_HMAC_SIZE   32
#define KEK_FILE_MIN_SIZE    (KEK_FILE_HEADER_SIZE + 4 + \
                             KEK_FILE_SALT_SIZE + KEK_FILE_IV_SIZE + \
                             KEK_FILE_HMAC_SIZE)

#define MEK_BIN_LEN          32
#define MEK_HEX_LEN          64

/* Command return codes */
#define CMD_OK           0
#define CMD_ERROR        1
#define CMD_USAGE_ERROR  2

/* ---------- MEK (fixed master key) ---------- */
extern int get_mek(unsigned char *mek_out);

/* ---------- Command handlers ---------- */
extern int cmd_generate(int argc, char *argv[]);
extern int cmd_list(int argc, char *argv[]);
extern int cmd_info(int argc, char *argv[]);
extern int cmd_delete(int argc, char *argv[]);
extern int cmd_validate(int argc, char *argv[]);

/* ---------- KEK file helpers ---------- */

/*
 * Read the header of a KEK file and extract metadata.
 * Returns 0 on success, -1 on error.
 * If mek is not NULL, also validates the HMAC.
 */
extern int kek_file_read_header(const char *path,
                                const unsigned char *mek,
                                int *out_key_type,  /* 0=unknown, 1=sym, 2=asym */
                                size_t *out_key_size,
                                time_t *out_ctime);

/*
 * Validate a KEK file's format (magic, version, sizes).
 * Returns 0 if valid, -1 if invalid.
 */
extern int kek_file_validate_format(const char *path);

/*
 * Validate a plain SM2 public-key (.pub) file: fixed 65-byte uncompressed
 * point whose coordinates lie on sm2p256v1.  .pub files carry no TECCKEK
 * envelope and no HMAC (public data), so this structural + on-curve check is
 * the only tamper detection available for them.
 * Returns 0 if valid, -1 if invalid.  errbuf (optional) receives a reason.
 */
extern int tecc_sm2_pub_check(const char *path, char *errbuf, size_t errsz);

#endif /* TECC_KTOOL_COMMON_H */

/* ---------------------------------------------------------------------
 *
 * File: be-secure-tlcp.h
 *
 * Abstract:
 *		This file contains the definition of the TLCP authentication function.
 *
 * Author: tanyang@highgo.com, 20260108.
 *
 * Copyright:
 *		Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 	src/include/libpq/be-secure-tlcp.h
 *
 * Add files that require "SRS-SECURITY-TLCP"
 *
 * ---------------------------------------------------------------------
 */

#ifndef PG_BE_SECURE_TLCP_H
#define PG_BE_SECURE_TLCP_H
#include "libpq/libpq-be.h"

/* Begin - ReqID:SRS-SECURITY-TLCP */
#ifdef USE_GMSSL
extern int
be_tlcp_init(bool isServerStart);
extern int be_tlcp_open_server(Port *port);
extern ssize_t
be_tlcp_read(Port *port, void *ptr, size_t len, int *waitfor);
extern ssize_t
be_tlcp_write(Port *port, void *ptr, size_t len, int *waitfor);

extern void
be_tlcp_close(Port *port);
extern void
be_tlcp_destroy(void);
extern char
*be_tlcp_get_certificate_hash(Port *port, size_t *len);
extern const char *
be_tlcp_get_cipher(Port *port);
extern const char *
be_tlcp_get_version(Port *port, char *str, int len);

#endif
/* End - ReqID:SRS-SECURITY-TLCP */

#endif

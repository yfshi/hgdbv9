/*--------------------------------------------------------------------
 * 
 * File: ivorysql_my.h
 *
 * Abstract: 
 * 		This file contains extern declarations for ivorysql_my itself.
 *
 * Authored by qinshiyu@highgo.com, 20230803.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/ivorysql_my.h
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *--------------------------------------------------------------------
 */
#ifndef IVORYSQL_MYS_H_
#define IVORYSQL_MYS_H_

#include "postgres.h"

#define UNSIGNED_FLAG   1
#define SIGNED_FLAG     0

typedef unsigned int uint;
typedef unsigned char uchar;

extern int16 my_atoi_int2(const char *s);
extern int32 my_atoi_int4(const char *s);
extern uint64 my_strntoull10rnd_8bit(const char *str, size_t length, int unsigned_flag, char **endptr, int *error);

#endif	/* IVORYSQL_MYS_H_ */

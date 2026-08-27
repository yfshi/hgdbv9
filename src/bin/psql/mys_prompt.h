/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright (c) 2022, PostgreSQL Global Development Group
 *
 * src/bin/psql/mys_prompt.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 */

#ifndef MYS_PROMPT_H
#define MYS_PROMPT_H

#include "fe_utils/conditional.h"
/* enum promptStatus_t is now defined by psqlscan.h */
#include "mysql_fe_utils/mys_psqlscan.h"

char	   *mys_get_prompt(Mys_promptStatus_t status, ConditionalStack cstack);

#endif							/* MYS_PROMPT_H */

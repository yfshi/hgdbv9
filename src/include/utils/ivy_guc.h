/*-------------------------------------------------------------------------
 *
 * ivy_guc.h
 *
 * Support for grand unified configuration scheme(for ivorysql), including SET
 * command, configuration file, and command line options
 *
 * Authored by lanke@highgo.com, 20220819.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *        src/include/utils/ivy_guc.h
 *
 *-------------------------------------------------------------------------
 */
#include "utils/guc_tables.h"
/* BEGIN - SQL PARSER */
extern int	database_mode;
extern PGDLLIMPORT int	compatible_db;
/* END - SQL PARSER */

extern void build_ivy_guc_variables(int *num_vars);
extern void add_ivy_guc_var(struct config_generic **guc_vars, int *num_vars);


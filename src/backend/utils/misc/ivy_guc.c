/*-------------------------------------------------------------------------
 *
 * ivy_guc.c
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
 *        src/backend/utils/misc/ivy_guc.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
	
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYSLOG
#include <syslog.h>
#endif
#include <unistd.h>

#include "access/commit_ts.h"
#include "access/gin.h"
#include "access/rmgr.h"
#include "access/tableam.h"
#include "access/toast_compression.h"
#include "access/transam.h"
#include "access/twophase.h"
#include "access/xact.h"
#include "access/xlog_internal.h"
#include "catalog/namespace.h"
#include "catalog/pg_authid.h"
#include "catalog/storage.h"
#include "commands/async.h"
#include "commands/prepare.h"
#include "commands/trigger.h"
#include "commands/user.h"
#include "commands/vacuum.h"
#include "commands/variable.h"
#include "common/string.h"
#include "funcapi.h"
#include "jit/jit.h"
#include "libpq/auth.h"
#include "libpq/libpq.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "optimizer/cost.h"
#include "optimizer/geqo.h"
#include "optimizer/optimizer.h"
#include "optimizer/paths.h"
#include "optimizer/planmain.h"
#include "parser/parse_expr.h"
#include "parser/parse_type.h"
#include "parser/parser.h"
#include "parser/scansup.h"
#include "pgstat.h"
#include "postmaster/autovacuum.h"
#include "postmaster/bgworker_internals.h"
#include "postmaster/bgwriter.h"
#include "postmaster/postmaster.h"
#include "postmaster/syslogger.h"
#include "postmaster/walwriter.h"
#include "replication/logicallauncher.h"
#include "replication/reorderbuffer.h"
#include "replication/slot.h"
#include "replication/syncrep.h"
#include "replication/walreceiver.h"
#include "replication/walsender.h"
#include "storage/bufmgr.h"
#include "storage/dsm_impl.h"
#include "storage/fd.h"
#include "storage/large_object.h"
#include "storage/pg_shmem.h"
#include "storage/predicate.h"
#include "storage/proc.h"
#include "storage/standby.h"
#include "tcop/tcopprot.h"
#include "tsearch/ts_cache.h"
#include "utils/acl.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/float.h"
#include "utils/guc_tables.h"
#include "utils/memutils.h"
#include "utils/pg_locale.h"
#include "utils/pg_lsn.h"
#include "utils/plancache.h"
#include "utils/portal.h"
#include "utils/ps_status.h"
#include "utils/queryjumble.h"
#include "utils/rls.h"
#include "utils/snapmgr.h"
#include "utils/tzparser.h"
#include "utils/inval.h"
#include "utils/varlena.h"
#include "utils/xml.h"
#include "utils/ora_compatible.h"

/* BEGIN - SQL PARSER */
#include "utils/ora_compatible.h"
/* END - SQL PARSER */
/* Begin - ReqID:SRS-SQL-MERGE */
#include "executor/nodeModifyTable.h"
#include "parser/parse_node.h"
#include "access/heapam.h"
#include "parser/parse_merge.h"
/* End - ReqID:SRS-SQL-MERGE */
#include "utils/ivy_guc.h"


/* BEGIN - case sensitive indentify */
int				identifier_case_switch = INTERCHANGE;
bool  			identifier_case_from_pg_dump = false;
bool			enable_case_switch = true;
/* END - case sensitive indentify */

/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
char	   *nls_length_semantics = NULL;
char	   *nls_date_format = "";
char	   *nls_timestamp_format = "";
char	   *nls_timestamp_tz_format = "";
int 		datetime_ignore_nls_mask = 0;
/* End - ReqID:SRS-GKC-DATATYPE_001 */

/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
bool			enable_emptystring_to_NULL = false;
/* End - ReqID:EMPTY_STIRNG_TO_NULL */

/* Begin - ReqID:SRS-SQL-INSERT */
bool		insert_enable_alias = false;
/* End - ReqID:SRS-SQL-INSERT */


/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
static bool check_nls_length_semantics(char **newval, void **extra, GucSource source);
/* End - ReqID:SRS-GKC-DATATYPE_001 */

/* BEGIN - SQL PARSER */
/* The comments shown as blow define the
 * value range of guc parameters "database_mode"
 * and "compatible_db".
 */
static const struct config_enum_entry db_mode_options[] = {
	{"pg", DB_PG, false},
	{"oracle", DB_ORACLE, false},
	{"0", DB_PG, false},
	{"1", DB_ORACLE, false},
	{NULL, 0, false}
};

static const struct config_enum_entry db_parser_options[] = {
	{"pg", PG_PARSER, false},
	{"oracle", ORA_PARSER, false},
	{NULL, 0, false}
/* END - SQL PARSER */
};

/* BEGIN - case sensitive indentify */
static const struct config_enum_entry case_conversion_mode[] = {
	{"normal", NORMAL, false},
	{"interchange", INTERCHANGE, false},
	{"lowercase", LOWERCASE, false},
	{"0", NORMAL, false},
	{"1", INTERCHANGE, false},
	{"2", LOWERCASE, false},
	{NULL, 0, false}
};
/* END - case sensitive indentify */

/* BEGIN - SQL PARSER */
int	database_mode = DB_PG;
int	compatible_db = PG_PARSER;
/* END - SQL PARSER */


static struct config_bool Ivy_ConfigureNamesBool[] =
{
	/* BEGIN - case sensitive indentify */
	{
		/* Not for general use --- used by pg_dump */
		{"ivorysql.identifier_case_from_pg_dump", PGC_USERSET, UNGROUPED,
			gettext_noop("Shows whether the identifer with quote is from pg dump."),
			NULL,
			GUC_REPORT | GUC_NO_SHOW_ALL | GUC_NO_RESET_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&identifier_case_from_pg_dump,
		false,
		NULL, NULL, NULL
	},
	{
		{"ivorysql.enable_case_switch", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("whether enable case conversion feature in oracle compatible mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&enable_case_switch,
		true,
		NULL, NULL, NULL
	},
	/* END - case sensitive indentify */

	/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
	{
		{"ivorysql.enable_emptystring_to_NULL", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("whether convert empty string to NULL."),
			NULL
		},
		&enable_emptystring_to_NULL,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:EMPTY_STIRNG_TO_NULL */

	/* Begin - ReqID:SRS-SQL-INSERT */
	{
		{"ivorysql.insert_enable_alias", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Whether to enable INSERT alias-qualified columns."),
			NULL
		},
		&insert_enable_alias,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-INSERT */

	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, false, NULL, NULL, NULL
	}
};

static struct config_int Ivy_ConfigureNamesInt[] =
{
	/* Begin - ReqID:LISTEN-MULTI-PORT */
	{
		{"ivorysql.port", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets the Oracle TCP port the server listens on."),
			NULL
		},
		&OraPortNumber,
		1521, 1, 65535,
		NULL, NULL, NULL
	},
	/* End - ReqID:LISTEN-MULTI-PORT */

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	{
		{"ivorysql.datetime_ignore_nls_mask", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Sets the datetime type input is not controlled by the NLS parameter."),
			NULL
		},
		&datetime_ignore_nls_mask,
		0, 0, 15,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-GKC-DATATYPE_001 */

	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0, 0, 0, NULL, NULL, NULL
	}
};

static struct config_string Ivy_ConfigureNamesString[] =
{
	/* Begin - ReqID:LISTEN-MULTI-PORT */
	{
		{"ivorysql.listen_addresses", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets oracle host name or IP address(es) to listen to."),
			NULL,
			GUC_LIST_INPUT
		},
		&OraListenAddresses,
		"localhost",
		NULL, NULL, NULL
	},
	/* End - ReqID:LISTEN-MULTI-PORT */

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	{
		{"ivorysql.nls_length_semantics", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for charater data type."),
			gettext_noop("Valid values are CHAR, BYTE."),
			GUC_IS_NAME
		},
		&nls_length_semantics,
		"byte",
		check_nls_length_semantics, NULL, NULL
	},

	{
		{"ivorysql.nls_date_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for date type."),
			NULL
		},
		&nls_date_format,
		"YYYY-MM-DD",
		NULL, NULL, NULL
	},

	{
		{"ivorysql.nls_timestamp_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for timestamp type."),
			NULL
		},
		&nls_timestamp_format,
		"YYYY-MM-DD HH24:MI:SS.FF6",
		NULL, NULL, NULL
	},

	{
		{"ivorysql.nls_timestamp_tz_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for timestamp with time zone type."),
			NULL
		},
		&nls_timestamp_tz_format,
		"YYYY-MM-DD HH24:MI:SS.FF6 TZH:TZM",
		NULL, NULL, NULL
	},
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, NULL, NULL, NULL, NULL
	}
};

static struct config_real Ivy_ConfigureNamesReal[] =
{
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0.0, 0.0, 0.0, NULL, NULL, NULL
	}
};

static struct config_enum Ivy_ConfigureNamesEnum[] =
{
	/* BEGIN - SQL PARSER */
	{
		{"ivorysql.database_mode", PGC_INTERNAL, PRESET_OPTIONS,
			gettext_noop("Set database mode"),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&database_mode,
		DB_PG, db_mode_options,
		NULL, NULL, NULL
	},

	{
		{"ivorysql.compatible_mode", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("Set default sql parser compatibility mode"),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&compatible_db,
		PG_PARSER, db_parser_options,
		NULL, NULL, NULL
	},
	/* END - SQL PARSER */

	/* BEGIN - case sensitive indentify */
	{
		{"ivorysql.identifier_case_switch", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Set character case conversion mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&identifier_case_switch,
		INTERCHANGE,case_conversion_mode,
		NULL,NULL,NULL
	},
	/* END - case sensitive indentify */

	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL
	}
};

/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
static bool
check_nls_length_semantics(char **newval, void **extra, GucSource source)
{
	if (newval == NULL
			|| (newval != NULL && pg_strcasecmp(*newval, "CHAR")  && pg_strcasecmp(*newval, "BYTE")))
		return false;

	return true;
}
/* End - ReqID:SRS-GKC-DATATYPE_001 */

void
build_ivy_guc_variables(int *num_vars)
{
	int			i;
	for (i = 0; Ivy_ConfigureNamesBool[i].gen.name; i++)
	{
		struct config_bool *conf = &Ivy_ConfigureNamesBool[i];

		/* Rather than requiring vartype to be filled in by hand, do this: */
		conf->gen.vartype = PGC_BOOL;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesInt[i].gen.name; i++)
	{
		struct config_int *conf = &Ivy_ConfigureNamesInt[i];

		conf->gen.vartype = PGC_INT;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesReal[i].gen.name; i++)
	{
		struct config_real *conf = &Ivy_ConfigureNamesReal[i];

		conf->gen.vartype = PGC_REAL;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesString[i].gen.name; i++)
	{
		struct config_string *conf = &Ivy_ConfigureNamesString[i];

		conf->gen.vartype = PGC_STRING;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesEnum[i].gen.name; i++)
	{
		struct config_enum *conf = &Ivy_ConfigureNamesEnum[i];

		conf->gen.vartype = PGC_ENUM;
		(*num_vars)++;
	}
}

void
add_ivy_guc_var(struct config_generic **guc_vars, int *num_vars)
{
	int	i;
	for (i = 0; Ivy_ConfigureNamesBool[i].gen.name; i++)
	{
		guc_vars[*num_vars] = &Ivy_ConfigureNamesBool[i].gen;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesInt[i].gen.name; i++)
	{
		guc_vars[*num_vars] = &Ivy_ConfigureNamesInt[i].gen;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesReal[i].gen.name; i++)
	{
		guc_vars[*num_vars] = &Ivy_ConfigureNamesReal[i].gen;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesString[i].gen.name; i++)
	{
		guc_vars[*num_vars] = &Ivy_ConfigureNamesString[i].gen;
		(*num_vars)++;
	}

	for (i = 0; Ivy_ConfigureNamesEnum[i].gen.name; i++)
	{
		guc_vars[*num_vars] = &Ivy_ConfigureNamesEnum[i].gen;
		(*num_vars)++;
	}
}


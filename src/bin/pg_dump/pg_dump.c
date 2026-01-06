/*-------------------------------------------------------------------------
 *
 * pg_dump.c
 *	  pg_dump is a utility for dumping out a postgres database
 *	  into a script file.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	pg_dump will read the system catalogs in a database and dump out a
 *	script that reproduces the schema in terms of SQL that is understood
 *	by PostgreSQL
 *
 *	Note that pg_dump runs in a transaction-snapshot mode transaction,
 *	so it sees a consistent snapshot of the database including system
 *	catalogs. However, it relies in part on various specialized backend
 *	functions like pg_get_indexdef(), and those things tend to look at
 *	the currently committed state.  So it is possible to get 'cache
 *	lookup failed' error if someone performs DDL changes while a dump is
 *	happening. The window for this sort of thing is from the acquisition
 *	of the transaction snapshot to getSchemaData() (when pg_dump acquires
 *	AccessShareLock on every table it intends to dump). It isn't very large,
 *	but it can happen.
 *
 *	http://archives.postgresql.org/pgsql-bugs/2010-02/msg00187.php
 *
 * IDENTIFICATION
 *	  src/bin/pg_dump/pg_dump.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

/* Begin - ReqID:SRS-SQL-SEQ */
#include "postgres.h"
#include "catalog/pg_type.h"
#include "commands/sequence.h"
/* End - ReqID:SRS-SQL-SEQ */
#include "access/attnum.h"
#include "access/sysattr.h"
#include "access/transam.h"
#include "catalog/pg_aggregate_d.h"
#include "catalog/pg_am_d.h"
#include "catalog/pg_attribute_d.h"
#include "catalog/pg_cast_d.h"
#include "catalog/pg_class_d.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_default_acl_d.h"
#include "catalog/pg_largeobject_d.h"
#include "catalog/pg_largeobject_metadata_d.h"
#include "catalog/pg_proc_d.h"
#include "catalog/pg_trigger_d.h"
#include "catalog/pg_type_d.h"
#include "common/connect.h"
#include "dumputils.h"
#include "fe_utils/string_utils.h"
#include "getopt_long.h"
#include "libpq/libpq-fs.h"
#include "parallel.h"
/* BEGIN - SQL PARSER */
#include "pg_backup_archiver.h"
/* END - SQL PARSER */
#include "pg_backup_db.h"
#include "pg_backup_utils.h"
#include "pg_dump.h"
/* BEGIN - ReqID:SRS-CMD-PGDUMP */
#include "pg_dump_oracle.h"
#include "pg_dump_native.h"
#include "pg_dump_pg.h"
#include "utils/ora_compatible.h"
#include "common/string.h"
/* END - ReqID:SRS-CMD-PGDUMP */
#include "storage/block.h"
/* Begin - SQL PARSER */
#include "oracle_fe_utils/ora_string_utils.h"
/* END - SQL PARSER */
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
#ifdef USE_LICENSE
#include "license.h"
#endif
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */

/* BEGIN - ReqID:SRS-CMD-pg2highgo */
#include "utils/datetime.h"
/* END - ReqID:SRS-CMD-pg2highgo */

typedef struct
{
	const char *descr;			/* comment for an object */
	Oid			classoid;		/* object class (catalog OID) */
	Oid			objoid;			/* object OID */
	int			objsubid;		/* subobject (table column #) */
} CommentItem;

typedef struct
{
	const char *provider;		/* label provider of this security label */
	const char *label;			/* security label for an object */
	Oid			classoid;		/* object class (catalog OID) */
	Oid			objoid;			/* object OID */
	int			objsubid;		/* subobject (table column #) */
} SecLabelItem;

typedef enum OidOptions
{
	zeroIsError = 1,
	zeroAsStar = 2,
	zeroAsNone = 4
} OidOptions;
/* BEGIN - ReqID:SRS-CMD-PGDUMP */
/*
 * The modification brought by SRS-CMD-PGDUMP is to remove the static
 * modifier of these variables, because these variables are not used
 * in the current file.
 */
/* BEGIN - ReqID:SRS-MISC-USER */
/* BEGIN - bug#Z5417 */
char role_catalog[ROLE_CATALOG_LENGTH];
/* END - bug#Z5417 */
#define PG_AUTHID "pg_authid"
#define PG_ROLES  "pg_roles "
int	no_role_passwords = 0;
/* END - ReqID:SRS-MISC-USER */

/* global decls */
bool dosync = true;		/* Issue fsync() to make dump durable on disk. */

/* subquery used to convert user ID (eg, datdba) to user name */
const char *username_subquery;

/*
 * For 8.0 and earlier servers, pulled from pg_database, for 8.1+ we use
 * FirstNormalObjectId - 1.
 */
Oid	g_last_builtin_oid; /* value of the last builtin oid */

/* The specified names/patterns should to match at least one entity */
int	strict_names = 0;

/*
 * Object inclusion/exclusion lists
 *
 * The string lists record the patterns given by command-line switches,
 * which we then convert to lists of OIDs of matching objects.
 */
SimpleStringList schema_include_patterns = {NULL, NULL};
SimpleOidList schema_include_oids = {NULL, NULL};
SimpleStringList schema_exclude_patterns = {NULL, NULL};
SimpleOidList schema_exclude_oids = {NULL, NULL};

SimpleStringList table_include_patterns = {NULL, NULL};
SimpleOidList table_include_oids = {NULL, NULL};
SimpleStringList table_exclude_patterns = {NULL, NULL};
SimpleOidList table_exclude_oids = {NULL, NULL};
SimpleStringList tabledata_exclude_patterns = {NULL, NULL};
SimpleOidList tabledata_exclude_oids = {NULL, NULL};
SimpleStringList foreign_servers_include_patterns = {NULL, NULL};
SimpleOidList foreign_servers_include_oids = {NULL, NULL};

SimpleStringList extension_include_patterns = {NULL, NULL};
SimpleOidList extension_include_oids = {NULL, NULL};

const CatalogId nilCatalogId = {0, 0};

/* override for standard extra_float_digits setting */
bool have_extra_float_digits = false;
int	extra_float_digits;

/*
 * These variables are static to avoid the notational cruft of having to pass
 * them into findTableByOid() and friends.  For each of these arrays, we build
 * a sorted-by-OID index array immediately after the objects are fetched,
 * and then we use binary search in findTableByOid() and friends.  (qsort'ing
 * the object arrays themselves would be simpler, but it doesn't work because
 * pg_dump.c may have already established pointers between items.)
 */
DumpableObject **tblinfoindex;
DumpableObject **typinfoindex;
DumpableObject **funinfoindex;
DumpableObject **oprinfoindex;
DumpableObject **aminfoindex;
DumpableObject **collinfoindex;
DumpableObject **nspinfoindex;
DumpableObject **extinfoindex;
DumpableObject **pubinfoindex;
int	numTables;
int	numTypes;
/* Begin - ReqID:SRS-SQL-PACKAGE */
int	numPkgs;
/* End - ReqID:SRS-SQL-PACKAGE */
int	numFuncs;
int numSyns;	/* ReqID:SRS-SQL-SYNONYM */
int	numOperators;
int	numAccessMethods;
int	numCollations;
int	numNamespaces;
int	numExtensions;
int	numPublications;

/* This is an array of object identities, not actual DumpableObjects */
ExtensionMemberId *extmembers;
int	numextmembers;


/*
 * Variables for mapping DumpId to DumpableObject
 */
DumpableObject **dumpIdMap = NULL;
int	allocedDumpIds = 0;
DumpId lastDumpId = 0;	/* Note: 0 is InvalidDumpId */
/*
 * Variables for mapping CatalogId to DumpableObject
 */
bool catalogIdMapValid = false;
DumpableObject **catalogIdMap = NULL;
int	numCatalogIds = 0;

DumpMethods *dM;
/* END - ReqID:SRS-CMD-PGDUMP */
Oid firstnormaloid = 0;	/* BUGZ1295 */
/* BEGIN - ReqID:SRS-MYSQL-PGDUMP */
const char *oracledumpoptions[] = {"pg", "oracle", "p", "o"};
#define ORAMOPTIONSNUM 4
const char *mysqldumpoptions[] = {"pg", "mysql", "p", "m"};
#define MYSMOPTIONSNUM 4
/* END - ReqID:SRS-MYSQL-PGDUMP */

/* BEGIN - ReqID:SRS-CMD-pg2highgo */
char day_time[MAXDATELEN + 1];
/* END - ReqID:SRS-CMD-pg2highgo */
char *passwordforUser;/* BUGZ1579 */
/*
 * The default number of rows per INSERT when
 * --inserts is specified without --rows-per-insert
 */
#define DUMP_DEFAULT_ROWS_PER_INSERT 1

/*
 * Macro for producing quoted, schema-qualified name of a dumpable object.
 */
#define fmtQualifiedDumpable(obj) \
	fmtQualifiedId((obj)->dobj.namespace->dobj.name, \
				   (obj)->dobj.name)

static void help(const char *progname);
static ArchiveFormat parseArchiveFormat(const char *format, ArchiveMode *mode);
static void ConnectDatabasefordatabase_mode(const ConnParams *cparams, bool isReconnect, char *password);
/* BEGIN - ReqID:SRS-MYSQL-PGDUMP */
static char *downMoptionString(char *options);
static bool Moption(const char *options, const char **moptions, int length);
static void DatabaseCheckMoption(const char *options);
/* END - ReqID:SRS-MYSQL-PGDUMP */

int
main(int argc, char **argv)
{
	int			c;
	const char *filename = NULL;
	const char *format = "p";
	TableInfo  *tblinfo;
	int			numTables;
	DumpableObject **dobjs;
	int			numObjs;
	DumpableObject *boundaryObjs;
	int			i;
	int			optindex;
	char	   *endptr;
	RestoreOptions *ropt;
	Archive    *fout;			/* the script file */
	bool		g_verbose = false;
	const char *dumpencoding = NULL;
	const char *dumpsnapshot = NULL;
	char	   *use_role = NULL;
	long		rowsPerInsert;
	int			numWorkers = 1;
	int			compressLevel = -1;
	int			plainText = 0;
	ArchiveFormat archiveFormat = archUnknown;
	ArchiveMode archiveMode;

	static DumpOptions dopt;
	/* BEGIN - ReqID:SRS-CMD-pg2highgo */
	time_t		timer;
	/* END - ReqID:SRS-CMD-pg2highgo */

	static struct option long_options[] = {
		{"data-only", no_argument, NULL, 'a'},
		{"blobs", no_argument, NULL, 'b'},
		{"no-blobs", no_argument, NULL, 'B'},
		{"clean", no_argument, NULL, 'c'},
		{"create", no_argument, NULL, 'C'},
		{"dbname", required_argument, NULL, 'd'},
		{"extension", required_argument, NULL, 'e'},
		{"file", required_argument, NULL, 'f'},
		{"format", required_argument, NULL, 'F'},
		{"host", required_argument, NULL, 'h'},
		{"jobs", 1, NULL, 'j'},
		{"no-reconnect", no_argument, NULL, 'R'},
		{"no-owner", no_argument, NULL, 'O'},
		{"port", required_argument, NULL, 'p'},
		{"schema", required_argument, NULL, 'n'},
		{"exclude-schema", required_argument, NULL, 'N'},
		{"schema-only", no_argument, NULL, 's'},
		{"superuser", required_argument, NULL, 'S'},
		{"table", required_argument, NULL, 't'},
		{"exclude-table", required_argument, NULL, 'T'},
		{"no-password", no_argument, NULL, 'w'},
		{"password", no_argument, NULL, 'W'},
		{"username", required_argument, NULL, 'U'},
		{"verbose", no_argument, NULL, 'v'},
		{"no-privileges", no_argument, NULL, 'x'},
		{"no-acl", no_argument, NULL, 'x'},
		{"compress", required_argument, NULL, 'Z'},
		{"encoding", required_argument, NULL, 'E'},
		{"help", no_argument, NULL, '?'},
		{"version", no_argument, NULL, 'V'},

		/*
		 * the following options don't have an equivalent short option letter
		 */
		{"attribute-inserts", no_argument, &dopt.column_inserts, 1},
		{"binary-upgrade", no_argument, &dopt.binary_upgrade, 1},
		{"column-inserts", no_argument, &dopt.column_inserts, 1},
		{"disable-dollar-quoting", no_argument, &dopt.disable_dollar_quoting, 1},
		{"disable-triggers", no_argument, &dopt.disable_triggers, 1},
		{"enable-row-security", no_argument, &dopt.enable_row_security, 1},
		{"exclude-table-data", required_argument, NULL, 4},
		{"extra-float-digits", required_argument, NULL, 8},
		{"if-exists", no_argument, &dopt.if_exists, 1},
		{"inserts", no_argument, NULL, 9},
		{"lock-wait-timeout", required_argument, NULL, 2},
		{"no-tablespaces", no_argument, &dopt.outputNoTablespaces, 1},
		{"quote-all-identifiers", no_argument, &quote_all_identifiers, 1},
		{"load-via-partition-root", no_argument, &dopt.load_via_partition_root, 1},
		{"role", required_argument, NULL, 3},
		{"section", required_argument, NULL, 5},
		{"serializable-deferrable", no_argument, &dopt.serializable_deferrable, 1},
		{"snapshot", required_argument, NULL, 6},
		{"strict-names", no_argument, &strict_names, 1},
		{"use-set-session-authorization", no_argument, &dopt.use_setsessauth, 1},
		{"no-comments", no_argument, &dopt.no_comments, 1},
		{"no-publications", no_argument, &dopt.no_publications, 1},
		{"no-role-passwords", no_argument, &no_role_passwords, 1},
		{"no-security-labels", no_argument, &dopt.no_security_labels, 1},
		{"no-subscriptions", no_argument, &dopt.no_subscriptions, 1},
		{"no-synchronized-snapshots", no_argument, &dopt.no_synchronized_snapshots, 1},
		{"no-toast-compression", no_argument, &dopt.no_toast_compression, 1},
		{"no-unlogged-table-data", no_argument, &dopt.no_unlogged_table_data, 1},
		{"no-sync", no_argument, NULL, 7},
		{"on-conflict-do-nothing", no_argument, &dopt.do_nothing, 1},
		{"rows-per-insert", required_argument, NULL, 10},
		{"include-foreign-data", required_argument, NULL, 11},
		{"restrict-key", required_argument, NULL, 25},
		{"first-normal-objectid", required_argument, NULL, 12}, /* BUGZ1295 */
		{NULL, 0, NULL, 0}
	};

	pg_logging_init(argv[0]);
	pg_logging_set_level(PG_LOG_WARNING);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_dump"));

	/*
	 * Initialize what we need for parallel execution, especially for thread
	 * support on Windows.
	 */
	init_parallel_dump_utils();

	progname = get_progname(argv[0]);

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			help(progname);
			exit_nicely(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_dump (PostgreSQL) " PG_VERSION);
			exit_nicely(0);
		}
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
#ifdef USE_LICENSE
		if (strcmp(argv[1], "--highgo-version") == 0 || strcmp(argv[1], "-G") == 0)
		{
			hg_load_client_license(argv[0]);
			printf("pg_dump %s\n", hg_get_highgoVersion());
			exit_nicely(0);
		}
#endif
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
	}

	InitDumpOptions(&dopt);

	while ((c = getopt_long(argc, argv, "abBcCd:e:E:f:F:h:j:M:n:N:Op:RsS:t:T:U:vwWxZ:",	/* ReqID:SRS-CMD-PGDUMP */
							long_options, &optindex)) != -1)
	{
		switch (c)
		{
			case 'a':			/* Dump data only */
				dopt.dataOnly = true;
				break;

			case 'b':			/* Dump blobs */
				dopt.outputBlobs = true;
				break;

			case 'B':			/* Don't dump blobs */
				dopt.dontOutputBlobs = true;
				break;

			case 'c':			/* clean (i.e., drop) schema prior to create */
				dopt.outputClean = 1;
				break;

			case 'C':			/* Create DB */
				dopt.outputCreateDB = 1;
				break;

			case 'd':			/* database name */
				dopt.cparams.dbname = pg_strdup(optarg);
				break;

			case 'e':			/* include extension(s) */
				simple_string_list_append(&extension_include_patterns, optarg);
				dopt.include_everything = false;
				break;

			case 'E':			/* Dump encoding */
				dumpencoding = pg_strdup(optarg);
				break;

			case 'f':
				filename = pg_strdup(optarg);
				break;

			case 'F':
				format = pg_strdup(optarg);
				break;

			case 'h':			/* server host */
				dopt.cparams.pghost = pg_strdup(optarg);
				break;

			case 'j':			/* number of dump jobs */
				numWorkers = atoi(optarg);
				break;

			/* BEGIN - ReqID:SRS-CMD-PGDUMP */
			case 'M':
				DumpM = downMoptionString(pg_strdup(optarg));	/* ReqID:SRS-MYSQL-PGDUMP */
				break;
			/* END - ReqID:SRS-CMD-PGDUMP */
			case 'n':			/* include schema(s) */
				simple_string_list_append(&schema_include_patterns, optarg);
				dopt.include_everything = false;
				break;

			case 'N':			/* exclude schema(s) */
				simple_string_list_append(&schema_exclude_patterns, optarg);
				break;

			case 'O':			/* Don't reconnect to match owner */
				dopt.outputNoOwner = 1;
				break;

			case 'p':			/* server port */
				dopt.cparams.pgport = pg_strdup(optarg);
				break;

			case 'R':
				/* no-op, still accepted for backwards compatibility */
				break;

			case 's':			/* dump schema only */
				dopt.schemaOnly = true;
				break;

			case 'S':			/* Username for superuser in plain text output */
				dopt.outputSuperuser = pg_strdup(optarg);
				break;

			case 't':			/* include table(s) */
				simple_string_list_append(&table_include_patterns, optarg);
				dopt.include_everything = false;
				break;

			case 'T':			/* exclude table(s) */
				simple_string_list_append(&table_exclude_patterns, optarg);
				break;

			case 'U':
				dopt.cparams.username = pg_strdup(optarg);
				break;

			case 'v':			/* verbose */
				g_verbose = true;
				pg_logging_increase_verbosity();
				break;

			case 'w':
				dopt.cparams.promptPassword = TRI_NO;
				break;

			case 'W':
				dopt.cparams.promptPassword = TRI_YES;
				break;

			case 'x':			/* skip ACL dump */
				dopt.aclsSkip = true;
				break;

			case 'Z':			/* Compression Level */
				compressLevel = atoi(optarg);
				if (compressLevel < 0 || compressLevel > 9)
				{
					pg_log_error("compression level must be in range 0..9");
					exit_nicely(1);
				}
				break;

			case 0:
				/* This covers the long options. */
				break;

			case 2:				/* lock-wait-timeout */
				dopt.lockWaitTimeout = pg_strdup(optarg);
				break;

			case 3:				/* SET ROLE */
				use_role = pg_strdup(optarg);
				break;

			case 4:				/* exclude table(s) data */
				simple_string_list_append(&tabledata_exclude_patterns, optarg);
				break;

			case 5:				/* section */
				set_dump_section(optarg, &dopt.dumpSections);
				break;

			case 6:				/* snapshot */
				dumpsnapshot = pg_strdup(optarg);
				break;

			case 7:				/* no-sync */
				dosync = false;
				break;

			case 8:
				have_extra_float_digits = true;
				extra_float_digits = atoi(optarg);
				if (extra_float_digits < -15 || extra_float_digits > 3)
				{
					pg_log_error("extra_float_digits must be in range -15..3");
					exit_nicely(1);
				}
				break;

			case 9:				/* inserts */

				/*
				 * dump_inserts also stores --rows-per-insert, careful not to
				 * overwrite that.
				 */
				if (dopt.dump_inserts == 0)
					dopt.dump_inserts = DUMP_DEFAULT_ROWS_PER_INSERT;
				break;

			case 10:			/* rows per insert */
				errno = 0;
				rowsPerInsert = strtol(optarg, &endptr, 10);

				if (endptr == optarg || *endptr != '\0' ||
					rowsPerInsert <= 0 || rowsPerInsert > INT_MAX ||
					errno == ERANGE)
				{
					pg_log_error("rows-per-insert must be in range %d..%d",
								 1, INT_MAX);
					exit_nicely(1);
				}
				dopt.dump_inserts = (int) rowsPerInsert;
				break;

			case 11:			/* include foreign data */
				simple_string_list_append(&foreign_servers_include_patterns,
										  optarg);
				break;
			/* BEGIN - BUGZ1295 */
			case 12:
				firstnormaloid = atooid(pg_strdup(optarg));
				break;
			/* END - BUGZ1295 */
			case 25:
				dopt.restrict_key = pg_strdup(optarg);
				break;
			default:
				fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
				exit_nicely(1);
		}
	}

	/*
	 * Non-option argument specifies database name as long as it wasn't
	 * already specified with -d / --dbname
	 */
	if (optind < argc && dopt.cparams.dbname == NULL)
		dopt.cparams.dbname = argv[optind++];

	/* Complain if any arguments remain */
	if (optind < argc)
	{
		pg_log_error("too many command-line arguments (first is \"%s\")",
					 argv[optind]);
		fprintf(stderr, _("Try \"%s --help\" for more information.\n"),
				progname);
		exit_nicely(1);
	}

	/* BEGIN - ReqID:SRS-MISC-USER */
	if (no_role_passwords)
		snprintf(role_catalog, ROLE_CATALOG_LENGTH - 1, "%s", PG_ROLES);  /* bug#Z5417 */
	else
		snprintf(role_catalog, ROLE_CATALOG_LENGTH - 1, "%s", PG_AUTHID);  /* bug#Z5417 */
	/* END - ReqID:SRS-MISC-USER */

	/* --column-inserts implies --inserts */
	if (dopt.column_inserts && dopt.dump_inserts == 0)
		dopt.dump_inserts = DUMP_DEFAULT_ROWS_PER_INSERT;

	/*
	 * Binary upgrade mode implies dumping sequence data even in schema-only
	 * mode.  This is not exposed as a separate option, but kept separate
	 * internally for clarity.
	 */
	if (dopt.binary_upgrade)
		dopt.sequence_data = 1;

	if (dopt.dataOnly && dopt.schemaOnly)
	{
		pg_log_error("options -s/--schema-only and -a/--data-only cannot be used together");
		exit_nicely(1);
	}

	if (dopt.schemaOnly && foreign_servers_include_patterns.head != NULL)
		fatal("options -s/--schema-only and --include-foreign-data cannot be used together");

	if (numWorkers > 1 && foreign_servers_include_patterns.head != NULL)
		fatal("option --include-foreign-data is not supported with parallel backup");

	if (dopt.dataOnly && dopt.outputClean)
	{
		pg_log_error("options -c/--clean and -a/--data-only cannot be used together");
		exit_nicely(1);
	}

	if (dopt.if_exists && !dopt.outputClean)
		fatal("option --if-exists requires option -c/--clean");

	/*
	 * --inserts are already implied above if --column-inserts or
	 * --rows-per-insert were specified.
	 */
	if (dopt.do_nothing && dopt.dump_inserts == 0)
		fatal("option --on-conflict-do-nothing requires option --inserts, --rows-per-insert, or --column-inserts");

	/* Identify archive format to emit */
	archiveFormat = parseArchiveFormat(format, &archiveMode);

	/* archiveFormat specific setup */
	if (archiveFormat == archNull)
	{
		plainText = 1;

		/*
		 * If you don't provide a restrict key, one will be appointed for you.
		 */
		if (!dopt.restrict_key)
			dopt.restrict_key = generate_restrict_key();
		if (!dopt.restrict_key)
			fatal("could not generate restrict key");
		if (!valid_restrict_key(dopt.restrict_key))
			fatal("invalid restrict key");
	}
	else if (dopt.restrict_key)
		fatal("option --restrict-key can only be used with --format=plain");

	/* Custom and directory formats are compressed by default, others not */
	if (compressLevel == -1)
	{
#ifdef HAVE_LIBZ
		if (archiveFormat == archCustom || archiveFormat == archDirectory)
			compressLevel = Z_DEFAULT_COMPRESSION;
		else
#endif
			compressLevel = 0;
	}

#ifndef HAVE_LIBZ
	if (compressLevel != 0)
		pg_log_warning("requested compression not available in this installation -- archive will be uncompressed");
	compressLevel = 0;
#endif

	/*
	 * If emitting an archive format, we always want to emit a DATABASE item,
	 * in case --create is specified at pg_restore time.
	 */
	if (!plainText)
		dopt.outputCreateDB = 1;

	/*
	 * On Windows we can only have at most MAXIMUM_WAIT_OBJECTS (= 64 usually)
	 * parallel jobs because that's the maximum limit for the
	 * WaitForMultipleObjects() call.
	 */
	if (numWorkers <= 0
#ifdef WIN32
		|| numWorkers > MAXIMUM_WAIT_OBJECTS
#endif
		)
		fatal("invalid number of parallel jobs");

	/* Parallel backup only in the directory archive format so far */
	if (archiveFormat != archDirectory && numWorkers > 1)
		fatal("parallel backup only supported by the directory format");
	/* BEGIN - ReqID:SRS-CMD-PGDUMP */
	ConnectDatabasefordatabase_mode(&dopt.cparams, false, NULL);
	/* END - ReqID:SRS-CMD-PGDUMP */
	/* BEGIN - ReqID:SRS-CMD-PGDUMP SRS-MYSQL-PGDUMP */
	/* The reason for checking here is to provide
	 * different option values according to
	 * different database modes
	 */
	DatabaseCheckMoption(DumpM);
	/* END - ReqID:SRS-CMD-PGDUMP SRS-MYSQL-PGDUMP */

	/* Open the output file */
	fout = dM->CreateArchive(filename, archiveFormat, compressLevel, dosync,
						 archiveMode, dM->setupDumpWorker);

	/* Make dump options accessible right away */
	SetArchiveOptions(fout, &dopt, NULL);

	/* Register the cleanup hook */
	on_exit_close_archive(fout);

	/* Let the archiver know how noisy to be */
	fout->verbose = g_verbose;


	/*
	 * We allow the server to be back to 8.0, and up to any minor release of
	 * our own major version.  (See also version check in pg_dumpall.c.)
	 */
	fout->minRemoteVersion = 80000;
	fout->maxRemoteVersion = (PG_VERSION_NUM / 100) * 100 + 99;

	fout->numWorkers = numWorkers;
	/* Begin - BUGZ1579 */
	if(passwordforUser != NULL)
		((ArchiveHandle *)fout)->savedPassword = passwordforUser;
	/* End - BUGZ1579 */
	/*
	 * Open the database using the Archiver, so it knows about it. Errors mean
	 * death.
	 */
	dM->ConnectDatabase(fout, &dopt.cparams, false);

	if (database_mode != DB_NATIVE && database_mode != DB_MYSQL)	/* ReqID:SRS-MYSQL-PGDUMP, Add not equal to MySQL judgment condition. */
	{
		/* BEGIN - SQL PARSER */
		dM->getDbCompatibleMode(((ArchiveHandle *) fout)->connection);
		/* END - SQL PARSER */

		/* BEGIN - ReqID:SRS-MISC-USER */
		dM->getDatabaseMode(((ArchiveHandle *) fout)->connection);
		/* END - ReqID:SRS-MISC-USER */

		/* Begin - ReqID:SRS-MISC-GUC */
		dM->setIdentifierCaseSwitch(((ArchiveHandle *) fout)->connection);		/* BUG#M229 */
		/* End - ReqID:SRS-MISC-GUC */
		dM->setNLSdateFormat(((ArchiveHandle *) fout)->connection);				/* BUG:z774 */
	}

	dM->setup_connection(fout, dumpencoding, dumpsnapshot, use_role);

	/* Begin - ReqID:SRS-MYSQL-CASE */
	GetMySQLCompPara(fout);
	/* End - ReqID:SRS-MYSQL-CASE */
	/*
	 * Disable security label support if server version < v9.1.x (prevents
	 * access to nonexistent pg_seclabel catalog)
	 */
	if (fout->remoteVersion < 90100)
		dopt.no_security_labels = 1;

	/*
	 * On hot standbys, never try to dump unlogged table data, since it will
	 * just throw an error.
	 */
	if (fout->isStandby)
		dopt.no_unlogged_table_data = true;

	/* Select the appropriate subquery to convert user IDs to names */
	if (fout->remoteVersion >= 80100)
		username_subquery = "SELECT rolname FROM pg_catalog.pg_roles WHERE oid =";
	else
		username_subquery = "SELECT usename FROM pg_catalog.pg_user WHERE usesysid =";

	/* check the version for the synchronized snapshots feature */
	if (numWorkers > 1 && fout->remoteVersion < 90200
		&& !dopt.no_synchronized_snapshots)
		fatal("Synchronized snapshots are not supported by this server version.\n"
			  "Run with --no-synchronized-snapshots instead if you do not need\n"
			  "synchronized snapshots.");

	/* check the version when a snapshot is explicitly specified by user */
	if (dumpsnapshot && fout->remoteVersion < 90200)
		fatal("Exported snapshots are not supported by this server version.");

	/*
	 * Find the last built-in OID, if needed (prior to 8.1)
	 *
	 * With 8.1 and above, we can just use FirstNormalObjectId - 1.
	 */
	if (fout->remoteVersion < 80100)
		g_last_builtin_oid = dM->findLastBuiltinOid_V71(fout);
	else
	/* BEGIN - BUGZ1295 */
	{
		if(firstnormaloid == 0)
			g_last_builtin_oid = FirstNormalObjectId - 1;
		else
			g_last_builtin_oid = firstnormaloid - 1;
	}
	/* END - BUGZ1295 */

	pg_log_info("last built-in OID is %u", g_last_builtin_oid);

	/* Expand schema selection patterns into OID lists */
	if (schema_include_patterns.head != NULL)
	{
		dM->expand_schema_name_patterns(fout, &schema_include_patterns,
									&schema_include_oids,
									strict_names);
		if (schema_include_oids.head == NULL)
			fatal("no matching schemas were found");
	}
	dM->expand_schema_name_patterns(fout, &schema_exclude_patterns,
								&schema_exclude_oids,
								false);
	/* non-matching exclusion patterns aren't an error */

	/* Expand table selection patterns into OID lists */
	if (table_include_patterns.head != NULL)
	{
		dM->expand_table_name_patterns(fout, &table_include_patterns,
								   &table_include_oids,
								   strict_names);
		if (table_include_oids.head == NULL)
			fatal("no matching tables were found");
	}
	dM->expand_table_name_patterns(fout, &table_exclude_patterns,
							   &table_exclude_oids,
							   false);

	dM->expand_table_name_patterns(fout, &tabledata_exclude_patterns,
							   &tabledata_exclude_oids,
							   false);

	dM->expand_foreign_server_name_patterns(fout, &foreign_servers_include_patterns,
										&foreign_servers_include_oids);

	/* non-matching exclusion patterns aren't an error */

	/* Expand extension selection patterns into OID lists */
	if (extension_include_patterns.head != NULL)
	{
		dM->expand_extension_name_patterns(fout, &extension_include_patterns,
									   &extension_include_oids,
									   strict_names);
		if (extension_include_oids.head == NULL)
			fatal("no matching extensions were found");
	}

	/*
	 * Dumping blobs is the default for dumps where an inclusion switch is not
	 * used (an "include everything" dump).  -B can be used to exclude blobs
	 * from those dumps.  -b can be used to include blobs even when an
	 * inclusion switch is used.
	 *
	 * -s means "schema only" and blobs are data, not schema, so we never
	 * include blobs when -s is used.
	 */
	if (dopt.include_everything && !dopt.schemaOnly && !dopt.dontOutputBlobs)
		dopt.outputBlobs = true;

	/*
	 * Now scan the database and create DumpableObject structs for all the
	 * objects we intend to dump.
	 */
	tblinfo = dM->getSchemaData(fout, &numTables);

	if (fout->remoteVersion < 80400)
		dM->guessConstraintInheritance(tblinfo, numTables);

	if (!dopt.schemaOnly)
	{
		dM->getTableData(&dopt, tblinfo, numTables, 0);
		dM->buildMatViewRefreshDependencies(fout);
		if (dopt.dataOnly)
			dM->getTableDataFKConstraints();
	}

	if (dopt.schemaOnly && dopt.sequence_data)
		dM->getTableData(&dopt, tblinfo, numTables, RELKIND_SEQUENCE);

	/*
	 * In binary-upgrade mode, we do not have to worry about the actual blob
	 * data or the associated metadata that resides in the pg_largeobject and
	 * pg_largeobject_metadata tables, respectively.
	 *
	 * However, we do need to collect blob information as there may be
	 * comments or other information on blobs that we do need to dump out.
	 */
	if (dopt.outputBlobs || dopt.binary_upgrade)
		dM->getBlobs(fout);

	/*
	 * Collect dependency data to assist in ordering the objects.
	 */
	dM->getDependencies(fout);

	/* Lastly, create dummy objects to represent the section boundaries */
	boundaryObjs = dM->createBoundaryObjects();

	/* Get pointers to all the known DumpableObjects */
	dM->getDumpableObjects(&dobjs, &numObjs);

	/*
	 * Add dummy dependencies to enforce the dump section ordering.
	 */
	dM->addBoundaryDependencies(dobjs, numObjs, boundaryObjs);

	/*
	 * Sort the objects into a safe dump order (no forward references).
	 *
	 * We rely on dependency information to help us determine a safe order, so
	 * the initial sort is mostly for cosmetic purposes: we sort by name to
	 * ensure that logically identical schemas will dump identically.
	 */
	dM->sortDumpableObjectsByTypeName(dobjs, numObjs);

	dM->sortDumpableObjects(dobjs, numObjs,
						boundaryObjs[0].dumpId, boundaryObjs[1].dumpId);

	/*
	 * Create archive TOC entries for all the objects to be dumped, in a safe
	 * order.
	 */

	/*
	 * First the special entries for ENCODING, STDSTRINGS, and SEARCHPATH.
	 */
	dM->dumpEncoding(fout);
	dM->dumpStdStrings(fout);
	dM->dumpSearchPath(fout);

	/* The database items are always next, unless we don't want them at all */
	if (dopt.outputCreateDB)
		dM->dumpDatabase(fout);

	/* BEGIN - ReqID:SRS-CMD-pg2highgo */
	if (dopt.binary_upgrade && (fout->remoteVersion >=140000 && fout->remoteVersion <150000))
	{
		timer = time(NULL);
		strftime(day_time, sizeof(day_time), "_%Y%m%d%H%M%S", localtime(&timer));
	}
	/* END - ReqID:SRS-CMD-pg2highgo */


	/* Now the rearrangeable objects. */
	for (i = 0; i < numObjs; i++)
		dM->dumpDumpableObject(fout, dobjs[i]);

	/*
	 * Set up options info to ensure we dump what we want.
	 */
	ropt = NewRestoreOptions();
	ropt->filename = filename;

	/* if you change this list, see dumpOptionsFromRestoreOptions */
	ropt->cparams.dbname = dopt.cparams.dbname ? pg_strdup(dopt.cparams.dbname) : NULL;
	ropt->cparams.pgport = dopt.cparams.pgport ? pg_strdup(dopt.cparams.pgport) : NULL;
	ropt->cparams.pghost = dopt.cparams.pghost ? pg_strdup(dopt.cparams.pghost) : NULL;
	ropt->cparams.username = dopt.cparams.username ? pg_strdup(dopt.cparams.username) : NULL;
	ropt->cparams.promptPassword = dopt.cparams.promptPassword;
	ropt->dropSchema = dopt.outputClean;
	ropt->dataOnly = dopt.dataOnly;
	ropt->schemaOnly = dopt.schemaOnly;
	ropt->if_exists = dopt.if_exists;
	ropt->column_inserts = dopt.column_inserts;
	ropt->dumpSections = dopt.dumpSections;
	ropt->aclsSkip = dopt.aclsSkip;
	ropt->superuser = dopt.outputSuperuser;
	ropt->createDB = dopt.outputCreateDB;
	ropt->noOwner = dopt.outputNoOwner;
	ropt->noTablespace = dopt.outputNoTablespaces;
	ropt->disable_triggers = dopt.disable_triggers;
	ropt->use_setsessauth = dopt.use_setsessauth;
	ropt->disable_dollar_quoting = dopt.disable_dollar_quoting;
	ropt->dump_inserts = dopt.dump_inserts;
	ropt->no_comments = dopt.no_comments;
	ropt->no_publications = dopt.no_publications;
	ropt->no_security_labels = dopt.no_security_labels;
	ropt->no_subscriptions = dopt.no_subscriptions;
	ropt->lockWaitTimeout = dopt.lockWaitTimeout;
	ropt->include_everything = dopt.include_everything;
	ropt->enable_row_security = dopt.enable_row_security;
	ropt->sequence_data = dopt.sequence_data;
	ropt->binary_upgrade = dopt.binary_upgrade;
	ropt->restrict_key = dopt.restrict_key ? pg_strdup(dopt.restrict_key) : NULL;

	if (compressLevel == -1)
		ropt->compression = 0;
	else
		ropt->compression = compressLevel;

	ropt->suppressDumpWarnings = true;	/* We've already shown them */

	SetArchiveOptions(fout, &dopt, ropt);

	/* Mark which entries should be output */
	dM->ProcessArchiveRestoreOptions(fout);

	/*
	 * The archive's TOC entries are now marked as to which ones will actually
	 * be output, so we can set up their dependency lists properly. This isn't
	 * necessary for plain-text output, though.
	 */
	if (!plainText)
		dM->BuildArchiveDependencies(fout);

	/*
	 * And finally we can do the actual output.
	 *
	 * Note: for non-plain-text output formats, the output file is written
	 * inside CloseArchive().  This is, um, bizarre; but not worth changing
	 * right now.
	 */
	if (plainText)
		dM->RestoreArchive(fout);

	CloseArchive(fout);

	exit_nicely(0);
}


static void
help(const char *progname)
{
	printf(_("%s dumps a database as a text file or to other formats.\n\n"), progname);
	printf(_("Usage:\n"));
	printf(_("  %s [OPTION]... [DBNAME]\n"), progname);

	printf(_("\nGeneral options:\n"));
	printf(_("  -f, --file=FILENAME          output file or directory name\n"));
	printf(_("  -F, --format=c|d|t|p         output file format (custom, directory, tar,\n"
			 "                               plain text (default))\n"));
	printf(_("  -j, --jobs=NUM               use this many parallel jobs to dump\n"));
	printf(_("  -v, --verbose                verbose mode\n"));
	printf(_("  -V, --version                output version information, then exit\n"));
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
#ifdef USE_LICENSE
	printf(_("  -G, --highgo-version         output version of highgo information, then exit\n"));
#endif
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
	printf(_("  -Z, --compress=0-9           compression level for compressed formats\n"));
	printf(_("  --lock-wait-timeout=TIMEOUT  fail after waiting TIMEOUT for a table lock\n"));
	printf(_("  --no-sync                    do not wait for changes to be written safely to disk\n"));
	printf(_("  -?, --help                   show this help, then exit\n"));

	printf(_("\nOptions controlling the output content:\n"));
	printf(_("  -a, --data-only              dump only the data, not the schema\n"));
	printf(_("  -b, --blobs                  include large objects in dump\n"));
	printf(_("  -B, --no-blobs               exclude large objects in dump\n"));
	printf(_("  -c, --clean                  clean (drop) database objects before recreating\n"));
	printf(_("  -C, --create                 include commands to create database in dump\n"));
	printf(_("  -e, --extension=PATTERN      dump the specified extension(s) only\n"));
	printf(_("  -E, --encoding=ENCODING      dump the data in encoding ENCODING\n"));
	printf(_("  -M                           dump the specified compatible_mode data only(pg, oracle, mysql)\n"));	/* BUGZ1299 */
	printf(_("  -n, --schema=PATTERN         dump the specified schema(s) only\n"));
	printf(_("  -N, --exclude-schema=PATTERN do NOT dump the specified schema(s)\n"));
	printf(_("  -O, --no-owner               skip restoration of object ownership in\n"
			 "                               plain-text format\n"));
	printf(_("  -s, --schema-only            dump only the schema, no data\n"));
	printf(_("  -S, --superuser=NAME         superuser user name to use in plain-text format\n"));
	printf(_("  -t, --table=PATTERN          dump the specified table(s) only\n"));
	printf(_("  -T, --exclude-table=PATTERN  do NOT dump the specified table(s)\n"));
	printf(_("  -x, --no-privileges          do not dump privileges (grant/revoke)\n"));
	printf(_("  --binary-upgrade             for use by upgrade utilities only\n"));
	printf(_("  --column-inserts             dump data as INSERT commands with column names\n"));
	printf(_("  --disable-dollar-quoting     disable dollar quoting, use SQL standard quoting\n"));
	printf(_("  --disable-triggers           disable triggers during data-only restore\n"));
	printf(_("  --enable-row-security        enable row security (dump only content user has\n"
			 "                               access to)\n"));
	printf(_("  --exclude-table-data=PATTERN do NOT dump data for the specified table(s)\n"));
	printf(_("  --extra-float-digits=NUM     override default setting for extra_float_digits\n"));
	printf(_("  --if-exists                  use IF EXISTS when dropping objects\n"));
	printf(_("  --include-foreign-data=PATTERN\n"
			 "                               include data of foreign tables on foreign\n"
			 "                               servers matching PATTERN\n"));
	printf(_("  --inserts                    dump data as INSERT commands, rather than COPY\n"));
	printf(_("  --load-via-partition-root    load partitions via the root table\n"));
	printf(_("  --no-comments                do not dump comments\n"));
	printf(_("  --no-publications            do not dump publications\n"));
	printf(_("  --no-security-labels         do not dump security label assignments\n"));
	printf(_("  --no-subscriptions           do not dump subscriptions\n"));
	printf(_("  --no-synchronized-snapshots  do not use synchronized snapshots in parallel jobs\n"));
	printf(_("  --no-tablespaces             do not dump tablespace assignments\n"));
	printf(_("  --no-toast-compression       do not dump TOAST compression methods\n"));
	printf(_("  --no-unlogged-table-data     do not dump unlogged table data\n"));
	printf(_("  --on-conflict-do-nothing     add ON CONFLICT DO NOTHING to INSERT commands\n"));
	printf(_("  --quote-all-identifiers      quote all identifiers, even if not key words\n"));
	printf(_("  --restrict-key=RESTRICT_KEY  use provided string as psql \\restrict key\n"));
	printf(_("  --rows-per-insert=NROWS      number of rows per INSERT; implies --inserts\n"));
	printf(_("  --section=SECTION            dump named section (pre-data, data, or post-data)\n"));
	printf(_("  --serializable-deferrable    wait until the dump can run without anomalies\n"));
	printf(_("  --snapshot=SNAPSHOT          use given snapshot for the dump\n"));
	printf(_("  --strict-names               require table and/or schema include patterns to\n"
			 "                               match at least one entity each\n"));
	printf(_("  --use-set-session-authorization\n"
			 "                               use SET SESSION AUTHORIZATION commands instead of\n"
			 "                               ALTER OWNER commands to set ownership\n"));
	printf(_("  --first-normal-objectid=OID  set FirstNormalObjectId in dump\n"));	/* BUGZ1295 */
	printf(_("\nConnection options:\n"));
	printf(_("  -d, --dbname=DBNAME      database to dump\n"));
	printf(_("  -h, --host=HOSTNAME      database server host or socket directory\n"));
	printf(_("  -p, --port=PORT          database server port number\n"));
	printf(_("  -U, --username=NAME      connect as specified database user\n"));
	printf(_("  -w, --no-password        never prompt for password\n"));
	printf(_("  -W, --password           force password prompt (should happen automatically)\n"));
	printf(_("  --role=ROLENAME          do SET ROLE before dump\n"));

	printf(_("\nIf no database name is supplied, then the PGDATABASE environment\n"
			 "variable value is used.\n\n"));
	printf(_("Report bugs to <%s>.\n"), PACKAGE_BUGREPORT);
	printf(_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
}
/* BEGIN - ReqID:SRS-CMD-PGDUMP */
static ArchiveFormat
parseArchiveFormat(const char *format, ArchiveMode *mode)
{
	ArchiveFormat archiveFormat;

	*mode = archModeWrite;

	if (pg_strcasecmp(format, "a") == 0 || pg_strcasecmp(format, "append") == 0)
	{
		/* This is used by pg_dumpall, and is not documented */
		archiveFormat = archNull;
		*mode = archModeAppend;
	}
	else if (pg_strcasecmp(format, "c") == 0)
		archiveFormat = archCustom;
	else if (pg_strcasecmp(format, "custom") == 0)
		archiveFormat = archCustom;
	else if (pg_strcasecmp(format, "d") == 0)
		archiveFormat = archDirectory;
	else if (pg_strcasecmp(format, "directory") == 0)
		archiveFormat = archDirectory;
	else if (pg_strcasecmp(format, "p") == 0)
		archiveFormat = archNull;
	else if (pg_strcasecmp(format, "plain") == 0)
		archiveFormat = archNull;
	else if (pg_strcasecmp(format, "t") == 0)
		archiveFormat = archTar;
	else if (pg_strcasecmp(format, "tar") == 0)
		archiveFormat = archTar;
	else
		fatal("invalid output format \"%s\" specified", format);
	return archiveFormat;
}

static void
ConnectDatabasefordatabase_mode(const ConnParams *cparams, bool isReconnect, char *pass)
{
	PGconn *connforonce;
	trivalue	prompt_password;
	char		*password;
	bool	new_pass;
	PGresult	*res;	/* BUGZ1295 */
	char	*sql;	/* BUGZ1295 */
	prompt_password = isReconnect ? TRI_NO : cparams->promptPassword;

	password = pass;

	if (prompt_password == TRI_YES && password == NULL)
		password = simple_prompt("Password: ", false);

	do
	{
		const char *keywords[8];
		const char *values[8];
		int			i = 0;

		/*
		 * If dbname is a connstring, its entries can override the other
		 * values obtained from cparams; but in turn, override_dbname can
		 * override the dbname component of it.
		 */
		keywords[i] = "host";
		values[i++] = cparams->pghost;
		keywords[i] = "port";
		values[i++] = cparams->pgport;
		keywords[i] = "user";
		values[i++] = cparams->username;
		keywords[i] = "password";
		values[i++] = password;
		keywords[i] = "dbname";
		values[i++] = cparams->dbname;
		if (cparams->override_dbname)
		{
			keywords[i] = "dbname";
			values[i++] = cparams->override_dbname;
		}
		keywords[i] = "fallback_application_name";
		values[i++] = progname;
		keywords[i] = NULL;
		values[i++] = NULL;
		Assert(i <= lengthof(keywords));

		new_pass = false;
		connforonce = PQconnectdbParams(keywords, values, true);

		if (!connforonce)
			fatal("could not connect to database");

		if (PQstatus(connforonce) == CONNECTION_BAD &&
			PQconnectionNeedsPassword(connforonce) &&
			password == NULL &&
			prompt_password != TRI_NO)
		{
			PQfinish(connforonce);
			password = simple_prompt("Password: ", false);
			new_pass = true;
		}
	} while (new_pass);


	/* check to see that the backend connection was successfully made */
	if (PQstatus(connforonce) == CONNECTION_BAD)
	{
		if (isReconnect)
			fatal("reconnection failed: %s",
				  PQerrorMessage(connforonce));
		else
			fatal("%s",
				  PQerrorMessage(connforonce));
	}

	getDatabaseMode(connforonce);
	/* Begin - BUGZ1579 */
	if(password != NULL)
		passwordforUser = password;
	/* End - BUGZ1579 */
	switch(database_mode)
	{
		case DB_ORACLE:
			dM = &dumpMethodsOracle;
			break;
		case DB_PG:
			dM = &dumpMethodsPg;
			break;
		case DB_NATIVE:
			dM = &dumpMethodsNative;
			break;
		/* BEGIN - ReqID:SRS-MYSQL-PGDUMP */
		case DB_MYSQL:
			dM = &dumpMethodsMysql;
			break;
		/* END - ReqID:SRS-MYSQL-PGDUMP */
		default:
			fatal("pointer switch failed");
	}
/* BEGIN - BUGZ1295 */
	sql = psprintf("SELECT oid FROM pg_database WHERE datname != 'template0' AND datname != 'template1' ORDER BY oid;");	/* BUGZ1332 */
	res = PQexec(connforonce, sql);
	if (firstnormaloid == 0)
	{
		firstnormaloid = atooid(PQgetvalue(res, 0, 0));

		/* When we use the command highgo2pg,
			the OID values of the database we obtain are between 16384 and 32767,
			which should be kept as they are. */
		if (firstnormaloid <= 16384)
			firstnormaloid = 16384;
		/* BEGIN - ReqID:SRS-CMD-highgo2pg */
		else if (firstnormaloid >= 32767)
		/* END - ReqID:SRS-CMD-highgo2pg */
			firstnormaloid = 32767;
	}

	free(sql);
	PQclear(res);
	/* END - BUGZ1295 */
	PQfinish(connforonce);

}

/* END - ReqID:SRS-CMD-PGDUMP */

/* BEGIN - ReqID:SRS-MYSQL-PGDUMP */
/*
 * Convert string to lowercase
 */
static char *downMoptionString(char *options)
{
	/* Avoid the function down_character from
	 * throwing an exception for NULL
	 */
	if (options != NULL)
		return down_character(options, strlen(options), 0 /*PG_SQL_ASCII*/);

	return NULL;
}

/*
 * Determine whether it is the value of M parameter
 */
static bool Moption(const char *options, const char **moptions, int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		if (strcmp(moptions[i], options) == 0)
			return true;
	}

	return false;
}

/*
 * Check the value of the M option in different database modes
 */
static void DatabaseCheckMoption(const char *options)
{
	if (options == NULL)
		return;

	if (database_mode == DB_ORACLE)
	{
		if (!Moption(options, oracledumpoptions, ORAMOPTIONSNUM))
			fatal("Oracle database wrong value for -M");
	}
	else if (database_mode == DB_MYSQL)
	{
		if (!Moption(options, mysqldumpoptions, MYSMOPTIONSNUM))
			fatal("MySQL database wrong value for -M");
	}
	else
		fatal("Except for Oracle and MySQL database modes, other database modes do not support -M");	/* BUGZ1346 */
}
/* END - ReqID:SRS-MYSQL-PGDUMP */

#include "hg_alarm.h"
#include "common/jsonapi.h"
#include "common/file_perm.h"
#include "common/string.h"
#include "getopt_long.h"
#include "pg_config.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define PID_FILE		"hg_alarm.pid"

const char  *hga_progname = NULL;
PGconn 		*hga_mainconn;

typedef long pgpid_t;
/* Main connection */
GUC			 GlobalUserConfig;

typedef enum
{
	CMD_NONE,
	CMD_START,
	CMD_STOP
} Cmd;

static struct option long_options[] = {
		/* conf file */
		{"config-file", required_argument, NULL, 'c'},
		/* connection related */
		{"database", required_argument, NULL, 'd'},
		{"host", required_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{"username", required_argument, NULL, 'U'},
		{"no-password", no_argument, NULL, 'w'},
		{"password", no_argument, NULL, 'W'},
		/* log file */
		{"log-directory", required_argument, NULL, 'l'},
		{"log-segsize", required_argument, NULL, 's'},
		/* mail */
		{"email", required_argument, NULL, 'e'},
		/* version and help */
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, '?'},
		{NULL, 0, NULL, 0}
	};

typedef struct 
{
	char   *interval_cmd;
	char   *logdir_cmd;
	char   *logsize_cmd;
	char   *database_cmd;
	char   *username_cmd;
	char   *host_cmd;
	char   *port_cmd;
	char   *conf_cmd;
	char   *email_cmd;
} UserOpt;

UserOpt		user_opt = {
	.interval_cmd = NULL,
	.logdir_cmd = NULL,
	.logsize_cmd = NULL,
	.database_cmd = NULL,
	.username_cmd = NULL,
	.host_cmd = NULL,
	.port_cmd = NULL,
	.conf_cmd = NULL,
	.email_cmd = NULL
};

static void
cleanup(void)
{
	/* close database connections */
	if (hga_mainconn)
		disconnectDatabase(hga_mainconn);
	/* close Logger */
	LoggerClose();
}

/*
 * help
 */
static void
help(const char *progname)
{
	printf("%s helps examining the file structure used by PostgreSQL.\n\n"
		   "Usage:\n"
		   "  %s [OPERATION] [OPTION]...\n\n"
		   "  OPERATION: \n"
		   "    start                     start the hg_alarm\n"
		   "    stop                      stop the hg_alarm\n"
		   "\n OPTION: \n"
		   "   -c, --config-file          configuration file (in json format)\n"
		   "   -e, --email                user to receive an email if any warning happened\n"
		   "   -i, --interval=INTERVAL    interval for one checking round (in seconds)\n"
		   "   -l, --log-directory        where to save log files \n"
		   "   -s, --log-segsize          max size of a log segment \n"
		   "\nOptions:\n"
		   "   -V, --version              output version information, then exit\n"
		   "   -?, --help                 show this help, then exit\n"
		   "\nConnection options:\n"
		   "   -d, --database=DBNAME      database to connect to\n"
		   "   -h, --host=HOSTNAME        database server host\n"
		   "   -p, --port=PORT            database server port number\n"
		   "   -U, --username=USERNAME    connect as specified database user\n"
		   "   -w, --no-password          never prompt password\n"
		   "   -W, --password             force password prompt \n"
		   "\nNotice: A config file template can be found at \"$SHARE/doc/postgresql/contrib\"\n\n",
			progname, progname);
}

static void
advice(const char *progname)
{
	pg_log_fatal("Try \"%s --help\" for more information.", progname);
}

static pgpid_t
get_pgpid(void)
{
	FILE	   *pidf;
	long		pid;

	pidf = fopen(PID_FILE, "r");
	if (pidf == NULL)
	{
		/* No pid file, not an error on startup */
		if (errno == ENOENT)
			return 0;
		else
		{
			pg_log_fatal("could not open PID file \"%s\": %s.",
						 PID_FILE, strerror(errno));
			exit(1);
		}
	}
	if (fscanf(pidf, "%ld", &pid) != 1)
	{
		/* Is the file empty? */
		if (ftell(pidf) == 0 && feof(pidf))
			pg_log_fatal("The PID file \"%s\" is empty.",
						 PID_FILE);
		else
			pg_log_fatal("Invalid data in PID file \"%s\".",
						 PID_FILE);
		exit(1);
	}
	fclose(pidf);
	return (pgpid_t) pid;
}

static void
set_pgpid(pgpid_t pid)
{
	FILE	   *pidf;

	pidf = fopen(PID_FILE, "w");
	if (pidf == NULL)
	{
		pg_log_fatal("Could not create PID file \"%s\"", PID_FILE);
		exit(1);
	}
	if (fprintf(pidf, "%ld", (long) pid) <= 0)
	{
		pg_log_fatal("Could not write PID file \"%s\"", PID_FILE);
		fclose(pidf);
		exit(1);
	}
	
	fclose(pidf);
}


static void
do_stop(void)
{
	pgpid_t		pid;
	int			sig;

	pid = get_pgpid();
	sig = SIGTERM;

	if (pid == 0)
	{
		pg_log_fatal("PID file \"%s\" does not exist.", PID_FILE);
		pg_log_fatal("Is hg_alarm running?");
		exit(1);
	}
	else if (pid < 0)			/* standalone backend, not postmaster */
	{
		pg_log_fatal("cannot stop hg_alarm (PID: %ld).",
					 pid);
		exit(1);
	}

	/*
	 * Send SIGTERM
	 */
	if (kill((pid_t) pid, sig) != 0)
	{
		pg_log_fatal("could not send stop signal (PID: %ld): %s.", pid,
					 strerror(errno));
		exit(1);
	}

	/*
	 * Remove the pid file
	 */
	unlink(PID_FILE);
}

/*
 * Main entry
 */
int 
main(int argc, char *argv[])
{
	char	cwd[MAXPGPATH];
	char	control_file[MAXPGPATH] = "";
	int	c;
	int		optindex;
	pgpid_t	pid;
	int		command = CMD_NONE;
	enum trivalue	prompt_password = TRI_DEFAULT;

	/*
	 * Init pg_log_xxx module and get progname.
	 */
	pg_logging_init(argv[0]);
	hga_progname = get_progname(argv[0]);

	/*
	 * Current dir readable?
	 */
	if (getcwd(cwd, MAXPGPATH) == NULL)
	{
		pg_log_fatal("cannot read current directory: ");
		exit(1);
	}

	/*
	 * Simple help/version query.
	 */
	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			help(hga_progname);
			exit(0);
		}
		else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("hg_alarm-v1.0");
			exit(0);
		}
	}

	while ((c = getopt_long(argc, argv, "c:d:e:h:i:l:p:s:U:wW", long_options, &optindex)) != -1)
	{
		switch (c)
		{
			case 'c':
				user_opt.conf_cmd = pg_strdup(optarg);
				break;
			case 'd':
				user_opt.database_cmd = pg_strdup(optarg);
				break;
			case 'e':
				user_opt.email_cmd = pg_strdup(optarg);
				break;
			case 'h':
				user_opt.host_cmd = pg_strdup(optarg);
				break;
			case 'i':
				user_opt.interval_cmd = pg_strdup(optarg);
				break;
			case 'p':
				user_opt.port_cmd = pg_strdup(optarg);
				break;
			case 'l':
				user_opt.logdir_cmd = pg_strdup(optarg);
				break;
			case 's':
				user_opt.logsize_cmd = pg_strdup(optarg);
				break;
			case 'U':
				user_opt.username_cmd = pg_strdup(optarg);
				break;
			case 'w':
				prompt_password = TRI_NO;
				break;
			case 'W':
				prompt_password = TRI_YES;
				break;
			default:
				advice(hga_progname);
				exit(1);
				break;
		}
	}

	/* 
	 * Operation mode.
	 */
	if (optind < argc)
	{
		if (strcmp(argv[optind], "start") == 0)
			command = CMD_START;
		else if (strcmp(argv[optind], "stop") == 0)
			command = CMD_STOP;
		else
		{
			pg_log_fatal("unrecognized operation mode: \"%s\"", argv[optind]);
			advice(hga_progname);
			exit(1);
		}
	}

	if (command == CMD_NONE)
	{
		pg_log_fatal("no operation specified");
		advice(hga_progname);
		exit(1);
	}

	/*
	 * Simply do_stop for stop mode and exit
	 */
	if (command == CMD_STOP)
	{
		do_stop();
		exit(0);
	}

	/* -------------------------------------------------------
	 * 			 Codes below only for start mode  
	 * -------------------------------------------------------
	 */

	/*
	 * A configuration file must be provided.
	 */
	if (user_opt.conf_cmd == NULL)
	{
		pg_log_fatal("No configuration file provided.");
		exit(1);
	}

	/* Make absolute control file path */
	if (is_absolute_path(user_opt.conf_cmd))
		strlcpy(control_file, user_opt.conf_cmd, MAXPGPATH);
	else
		join_path_components(control_file, cwd, user_opt.conf_cmd);
	canonicalize_path(control_file);

	memset(&GlobalUserConfig, 0, sizeof(GUC));

	/*
	 * Parse the conf file.
	 */
	if (!parse_json_option(control_file, &GlobalUserConfig))
		exit(1);

	/*
	 * Overwrite configs in json file by cmd input.
	 */
	if (user_opt.database_cmd)
		GlobalUserConfig.guc_database = user_opt.database_cmd;
	if (user_opt.username_cmd)
		GlobalUserConfig.guc_username = user_opt.username_cmd;
	if (user_opt.host_cmd)
		GlobalUserConfig.guc_host = user_opt.host_cmd;
	if (user_opt.port_cmd)
		GlobalUserConfig.guc_port = user_opt.port_cmd;
	if (user_opt.logdir_cmd)
		GlobalUserConfig.guc_log_directory = user_opt.logdir_cmd;
	if (user_opt.email_cmd)
			GlobalUserConfig.guc_email_user = user_opt.email_cmd;
	if (user_opt.interval_cmd)
	{
		int32		 val;
		const char	*hintmsg;

		GlobalUserConfig.guc_interval_raw = user_opt.interval_cmd;

		if (!parse_int(GlobalUserConfig.guc_interval_raw, &val, GUC_UNIT_S, &hintmsg))
		{
			pg_log_info("invalid value: %s", GlobalUserConfig.guc_interval_raw);
			if (hintmsg)
			{
				pg_log_fatal("%s", hintmsg);
				exit(1);
			}
		}

		if (val < 1 || val > 60 * 60 * 24)
		{
			pg_log_info("invalid value: %s", GlobalUserConfig.guc_interval_raw);
			pg_log_fatal("Valid interval is between 1 second to 1 hour.");
			exit(1);
		}

		GlobalUserConfig.guc_interval = val;
	}

	if (user_opt.logsize_cmd)
	{
		int32		 val;
		const char	*hintmsg;

		GlobalUserConfig.guc_log_segsize_raw = user_opt.logsize_cmd;

		if (!parse_int(GlobalUserConfig.guc_log_segsize_raw, &val, GUC_UNIT_MB, &hintmsg))
		{
			pg_log_info("invalid value: %s", GlobalUserConfig.guc_log_segsize_raw);
			if (hintmsg)
			{
				pg_log_fatal("%s", hintmsg);
				exit(1);
			}
		}

		if (val < 1 || val > 10)
		{
			pg_log_info("invalid value: %s", GlobalUserConfig.guc_log_segsize_raw);
			pg_log_fatal("Valid interval is between 1MB second to 10MB.");
			exit(1);
		}

		GlobalUserConfig.guc_log_segsize = val;
	}
	
	/*
	 * Apply default values.
	 */
	if (GlobalUserConfig.guc_interval == 0)
		GlobalUserConfig.guc_interval = 1;
	if (GlobalUserConfig.guc_log_segsize == 0)
		GlobalUserConfig.guc_log_segsize = 10;

	/*
	 * Get user-input password:
	 * 	1. -W specified
	 *  2. No password found in conf, and -w not specified
	 */
	if (prompt_password == TRI_YES)
		GlobalUserConfig.guc_password = simple_prompt("Password: ", false);

	if (prompt_password != TRI_NO
		&& GlobalUserConfig.guc_password == NULL)
		GlobalUserConfig.guc_password = simple_prompt("Password: ", false);

	/*
	 * -w specified and no password found in conf.
	 * error report it.
	 */
	if (!GlobalUserConfig.guc_password)
	{
		pg_log_fatal("No password provided");
		exit(1);
	}
	
	apply_default_connection_params(&GlobalUserConfig);
	valid_connection_param(&GlobalUserConfig);

	do
	{
		struct stat sb;

		if (stat(PID_FILE, &sb) == 0)
		{
			pg_log_fatal("PID file %s already exists, have another hg_alarm already started? \n"
						 "You can simply remove it if not",
						  PID_FILE);
			exit(1);
		}
	} while (0);

	/*
	 * Fork a new child to work, and exit the parent.
	 */
	fflush(stdout);
	fflush(stderr);

	pid = fork();
	/* Failed */
	if (pid < 0)
	{
		cleanup();
		pg_log_fatal("could not start: %s.", strerror(errno));
		exit(1);
	}
	/* Let parent go */
	if (pid > 0)
	{
		set_pgpid(pid);
		exit(0);
	}

	/*
	 * Register process exit cleanup hook
	 */
	atexit(cleanup);

	/*
	 * Main database connection.
	 */
	do
	{
		/* Try connectting the database */
		ConnParams	 	cp;

		cp.pghost = GlobalUserConfig.guc_host;
		cp.pgport = GlobalUserConfig.guc_port;
		cp.dbname = GlobalUserConfig.guc_database;
		cp.pguser = GlobalUserConfig.guc_username;
		cp.password = GlobalUserConfig.guc_password;
		cp.prompt_password = TRI_NO;
		cp.override_dbname = NULL;

		hga_mainconn = connectDatabase(&cp, hga_progname, false, false, false, NULL);
	} while(0);

	/* 
	 * Logger initialization 
	 */
	do
	{
		if (!CreateLogger(GlobalUserConfig.guc_log_directory, 
						  GlobalUserConfig.guc_log_segsize,
						  GlobalUserConfig.guc_email_user))
			pg_log_warning("Logger init failed, no log files will be generated.");
	} while (0);

	/*
	 * Child MainLoop
	 */
	while (true)
	{
		/*
		 * Do what we are here for
		 */
		if (GlobalUserConfig.guc_memory_limit > 0)
			(void) do_memory_check(hga_mainconn, GlobalUserConfig.guc_memory_limit);
		if (GlobalUserConfig.guc_datadir_limit > 0)
			(void) do_datadir_check(hga_mainconn, GlobalUserConfig.guc_datadir_limit);
		if (GlobalUserConfig.guc_database_list)
			(void) do_db_or_ts_check(hga_mainconn, PGOBJ_DATABASE, GlobalUserConfig.guc_database_list, NULL);
		if (GlobalUserConfig.guc_tablespace_list)
			(void) do_db_or_ts_check(hga_mainconn, PGOBJ_TABLESPACE, GlobalUserConfig.guc_tablespace_list, NULL);
		if (GlobalUserConfig.guc_relation_list)
			(void) do_rel_check(hga_mainconn, GlobalUserConfig.guc_relation_list, NULL);

		/* 
		 * Sleep an interval for each loop 
		 */
		pg_usleep(GlobalUserConfig.guc_interval * 1000000);
	}

    return 0;
}

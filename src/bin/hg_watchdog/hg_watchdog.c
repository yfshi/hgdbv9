/*-------------------------------------------------------------------------
 *
 * hg_watchdog.c
 *	  This involves monitoring the Postgres master process and
 *	restarting it if it crashes.
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/bin/hg_tools/hg_watchdog.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>
#include <time.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include "sqliteaudit.h"
#include "getopt_long.h"

#define BUFSZ 1024
#define INSTANCEARRAYNUM 100
/* The default configuration file of hg_watchdog
 * is hg_watchdog.conf
 */
#define HG_WATCHDOG_PATH "/etc/hg_watchdog/hg_watchdog.conf"

typedef struct DatabaseInstance
{
	char	installpath[BUFSZ];
	char	datapath[BUFSZ];
	char	optionarg[BUFSZ];
	int		groupnum;
	bool	installpathflg;
	bool	datapathflg;
}DatabaseInstance;

static bool check_postmaster_pid(const char *pathofpostmasterpid);
static void restart_postgres(const char *startcmd);
static void check_and_restart_postgres(const char *postmasterpidpath, const char *startcmd, char *datapath);
static char *extracting_values_from_str(char *str);

/* main
 *
 * The main function of hg_watchdog monitors
 * the PostgreSQL process every 5 seconds. if
 * the process is not running, it automatically
 * restarts it.
 */
int
main(int argc, char *argv[])
{
#ifndef WIN32
	char	postmasterpidpath[BUFSZ];
	char	startcmd[BUFSZ];
	char	installpath[BUFSZ];
	char	datapath[BUFSZ];
	int		pgnum = 0;
	char	value[BUFSZ];
	DatabaseInstance dbinstance[INSTANCEARRAYNUM];
	int		intancenum = 0;
	struct	stat st;	/* BUG#Z7635 */
#endif
	char	conffilepath[BUFSZ];
	const char   *progname;
	int		c;
	int		option_index;

	/* The parameters accepted by the program */
	static struct option long_options[] = {
		{"file", required_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};

	MemSet(conffilepath, 0, BUFSZ);
	snprintf(conffilepath, BUFSZ, "%s", HG_WATCHDOG_PATH);

	pg_logging_init(argv[0]);
	progname = get_progname(argv[0]);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("hg_watchdog"));

	if (argc > 1)
	{
		/* Startup methods and version display */
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			printf(_("hg_watchdog provides the following help information.\n\n"));
			printf(_("Start can be done in the following ways:\n"));
			printf(_("        %s\n"), progname);
			printf(_("        %s -f file    (file: absolute path to the file)\n"), progname);
			printf(_("        systemctl start hg_watchdog\n\n"));
			printf(_("--version or -V:\n"));
			printf(_("        Display version\n\n"));
			exit(0);
		}
		else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("highgo watchdog " "1.0");
			exit(0);
		}

		/* Chek the provided argumens and return an error
		 * if they do not meet the requirement.
		 */
		while ((c = getopt_long(argc, argv, "f:", long_options, &option_index)) != -1)
		{
			switch (c)
			{
				/* -f is Specify the path to the file. */
				case 'f':
					MemSet(conffilepath, 0, BUFSZ);
					snprintf(conffilepath, BUFSZ, "%s", pg_strdup(optarg));
					break;
				default:
					fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
					exit(1);
			}
		}
	}

#ifndef WIN32

#define HG_WATCHDOG_CLEAR_DADA() \
	do { \
		MemSet(startcmd, 0, BUFSZ); \
		MemSet(postmasterpidpath, 0, BUFSZ); \
	} while (0)

#define HG_WATCHDOG_CLEAR_VALUE() \
	do { \
		MemSet(value, 0, BUFSZ); \
		pgnum = 0; \
	} while (0)

	/* Begin - BUG#Z7635 */
	/* The file must specify an absolute path. */
	if (conffilepath[0] != '/')
	{
		fprintf(stderr, _("%s is not an absolute path.\n"), conffilepath);
		exit(1);
	}

	/* Check if the file exists. */
	if (stat(conffilepath, &st) != 0)
	{
		fprintf(stderr, _("Configure file %s does not exist.\n"), conffilepath);
		exit(1);
	}
	else
	{
		/* Configuration file is ordinary file. */
		if (!S_ISREG(st.st_mode))
		{
			fprintf(stderr, _("%s is not a configuration file.\n"), conffilepath);
			exit(1);
		}
	}

	/* If executed on the frontend, display a notification
	 * information the user that the task can be performed
	 * in the background.
	 */
	if (getpgrp() == tcgetpgrp(STDIN_FILENO))
	{
		printf(_("%d : hg_watchdog is currently runing! \n"), getpid());
		printf(_("You can also run it in background using the command: hg_watchdog -f dir/hg_watchdog.conf &\n"));
	}
	/* End - BUG#Z7635 */

	/* The program runs in an infniite loop to continuously
	 * monitor the process, performing a check every 5
	 * seconds.
	 */
	while (1)
	{
		FILE	*fpopen;
		bool	startflg = false;

		/* reset and clear variables */
		intancenum = 0;
		MemSet(dbinstance, 0, INSTANCEARRAYNUM * sizeof(DatabaseInstance));

		/* Open the configure file */
		fpopen = fopen(conffilepath, "r");

		if (fpopen)
		{
			char	line[BUFSZ];
			int		i;

			/* Read the file one line at a time until reaching
			 * the end of the file(EOF).
			 */
			while (fgets(line, sizeof(line), fpopen) != NULL)
			{
				char	*resultstr = NULL;

				resultstr = extracting_values_from_str(line);
				if (strlen(resultstr) < 1)
					continue;

				HG_WATCHDOG_CLEAR_VALUE();

				/* Lines beginning whith installpath: are install paths,
				 * lines beginning whith datapath: are data directory paths,
				 * lines beginning whith option: treat is as other startup options.
				 */
				if (sscanf(resultstr, "instance%d-installpath:%s", &pgnum, value) == 2)
				{
					/* Array index starts at 0 */
					if (startflg)
						intancenum++;

					if (!startflg)
						startflg = true;

					MemSet(installpath, 0, BUFSZ); \
					snprintf(installpath, BUFSZ, "%s", value);
					if (strlen(installpath) > 0)
					{
						/* If a string ends with a newline character (\n),
						 * covert it to a null terminator(\0).
						 */
						if (installpath[strlen(installpath) - 1] == '\n')
							installpath[strlen(installpath) - 1] = '\0';

						if (strlen(installpath) < 1)
							continue;

						snprintf(dbinstance[intancenum].installpath, BUFSZ, "%s", installpath);
						dbinstance[intancenum].installpathflg = true;
						dbinstance[intancenum].groupnum = pgnum;
					}
				}
				else if (sscanf(resultstr, "instance%d-installdata:%s", &pgnum, value) == 2)
				{
					MemSet(datapath, 0, BUFSZ);
					snprintf(datapath, BUFSZ, "%s", value);

					if (dbinstance[intancenum].groupnum != pgnum)
						continue;

					if (strlen(datapath) > 0)
					{
						/* If a string ends with a newline character (\n),
						 * covert it to a null terminator(\0).
						 */
						if (datapath[strlen(datapath) - 1] == '\n')
							datapath[strlen(datapath) - 1] = '\0';

						if (strlen(datapath) < 1)
							continue;

						snprintf(dbinstance[intancenum].datapath, BUFSZ, "%s", datapath);
						dbinstance[intancenum].datapathflg = true;
					}
				}
				else if (sscanf(resultstr, "instance%d-option:%[^\n]", &pgnum, value) == 2)
				{
					char optionarg[BUFSZ];

					MemSet(optionarg, 0, BUFSZ);
					snprintf(optionarg, BUFSZ, "%s", value);

					if (dbinstance[intancenum].groupnum != pgnum)
						continue;

					if (strlen(optionarg) > 0)
					{
						/* If a string ends with a newline character (\n),
						 * covert it to a null terminator(\0).
						 */
						if (optionarg[strlen(optionarg) - 1] == '\n')
							optionarg[strlen(optionarg) - 1] = '\0';

						if (strlen(optionarg) < 1)
							continue;

						snprintf(dbinstance[intancenum].optionarg, BUFSZ, "%s", optionarg);
					}
				}
			}

			/* Ignore duplicates and keep the
			 * last group as the final one.
			 */
			for (i = 0; i < intancenum + 1; i++)
			{
				int		j;

				for (j = i + 1; j < intancenum + 1; j++)
				{
					if (dbinstance[j].groupnum == dbinstance[i].groupnum)
					{
						dbinstance[i].installpathflg = false;
						dbinstance[i].datapathflg = false;
					}
				}
			}

			for (i = 0; i < intancenum + 1; i++)
			{
				/* The last group of monitored service in the configuration
				 * file, if there is no option, execute directly when the
				 * condition is met.
				 */
				if (dbinstance[i].installpathflg && dbinstance[i].datapathflg)
				{
					HG_WATCHDOG_CLEAR_DADA();
					/* Build the command that needs to be checked
					 * and excuted.
					 */
					snprintf(postmasterpidpath, BUFSZ, "%s/postmaster.pid", dbinstance[i].datapath);
					if (strlen(dbinstance[i].optionarg) > 0)
						snprintf(startcmd, BUFSZ, "%s/bin/pg_ctl -D %s %s start", dbinstance[i].installpath, dbinstance[i].datapath, dbinstance[i].optionarg);
					else
						snprintf(startcmd, BUFSZ, "%s/bin/pg_ctl -D %s start", dbinstance[i].installpath, dbinstance[i].datapath);

					/* Run the commands to check the process and
					 * restart it if necessary
					 */
					check_and_restart_postgres(postmasterpidpath, startcmd, datapath);
				}
			}

			/* close file */
			fclose(fpopen);
		}

		/* Perform a check every 5 seconds. */
		sleep(5);
	}
#endif

	exit(EXIT_SUCCESS);
}

#ifndef WIN32
/* check_postmaster_pid
 *
 * Check if the postmaster.pid file exists, if it does not,
 * if it does not, return false, if it does, check whether
 * the PID contained within the file exists under /proc.
 */
static bool
check_postmaster_pid(const char *pathofpostmasterpid)
{
	struct stat st;
	struct stat st1;

	if (stat(pathofpostmasterpid, &st) == 0)
	{
		FILE   *fp;

		/* Open the configure file */
		fp = fopen(pathofpostmasterpid, "r");
		if (fp)
		{
			char	line[BUFSZ];
			char	path[BUFSZ];

			MemSet(line, 0, BUFSZ);

			/* Read only the first line in the postmaster.pid file,
			 * the first line contains the process ID(PID).
			 */
			if (fgets(line, sizeof(line), fp) != NULL)
			{
				if (strlen(line) > 0)
				{
					/* If a string ends with a newline character (\n),
					 * covert it to a null terminator(\0).
					 */
					if (line[strlen(line) - 1] == '\n')
						line[strlen(line) - 1] = '\0';

					if (strlen(line) > 0)
					{
						MemSet(path, 0, BUFSZ);
						snprintf(path, BUFSZ, "/proc/%s", line);

						if (stat(path, &st1) == 0)
						{
							/* close file */
							fclose(fp);

							return true;
						}
					}
				}
			}

			/* close file */
			fclose(fp);
		}
	}

	return false;
}

/* extracting_values_from_str
 *
 * Remove leading whitespace from a string.
 */
static char *
extracting_values_from_str(char *str)
{
	while (isspace((unsigned char)(*str)))
	{
		str++;
	}

	return str;
}

/* restart_postgres
 *
 * Restart the specified database.
 */
static void
restart_postgres(const char *startcmd)
{
	FILE   *fp;

	if((fp = popen(startcmd,"r")) == NULL)
		fprintf(stderr, _("Faild to execute the database startup command: %s.\n"), startcmd);

	pclose(fp);
}

/* check_and_restart_postgres
 *
 * Verify the existence of the specified database
 * service. if it is not running, automatically
 * restart the database.
 */
static void
check_and_restart_postgres(const char *postmasterpidpath, const char *startcmd, char *datapath)
{
	if (!check_postmaster_pid(postmasterpidpath))
	{
		restart_postgres(startcmd);

#if defined (ENABLE_DKY_BETA_FEATURE) && defined (USE_SQLITE)
		audit_tools(datapath, "hg_watchdog", 'f', 'y', "POSTMASTER PROCESS MONITORRING", "Found the postmaster hanging, restarted it");
#endif
	}
}
#endif

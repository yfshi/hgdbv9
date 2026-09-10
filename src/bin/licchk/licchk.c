/*------------------------------------------------------------------
 *
 * File: licchk.c
 *
 * Abstract:
 * 	  licchk tool checks and displays the content of the license.dat file..
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/bin/licchk/licchk.c
 *
 * Requirement description:
 * 		ReqID:SRS-MISC-LICENSE
 * 		ReqID:SRS-MISC-LICENSE-ClientTool
 *		ReqID:SRS-MISC-LICENSE-ENCODING
 *		ReqID:SRS-MISC-LICENSE-TECC
 *		ReqID:SRS-MISC-LICENSE-STYLECONFIG
 *		ReqID:SRS-MISC-LICENSE-MultiMode
 *		ReqID:SRS-MISC-LICENSE-MysqlDatabases
 *------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "c.h"
#include "port.h"
#include "common/fe_memutils.h"
#include "getopt_long.h" /* fix bug#z642 */

#include "license.h"

#define true	1
#define false	0

static char *license_dat = NULL;
static bool  is_list = false;	/* list enabled features */
static bool  is_all  = false;	/* all features */
static bool  is_view = false;	/* view all features */
static char *hardware_code_file = NULL;

static bool  is_get_name = false;	/* view all features */
static bool  is_get_port = false;	/* view all features */
static bool  is_ignore_check_hardware = false;	/* ignore hardware code check */
/* Begin - fix Bug#M0000321 */
static bool  is_ignore_md5_checksum = false;	/* ignore md5 checksum value */
/* End - fix Bug#M0000321 */
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
static bool	 is_ignore_datetime_check = false;	/* whether check the validity date of the license or not */
static bool  is_client_dat = false;				/* Is it the file that loads the license_cli.dat */
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
/* Begin - fix Bug#M0000329 */
static bool  is_get_max_connections = false;
/* End - fix Bug#M0000329 */
/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
static bool  is_specify_encoding = false; 	/* Command line specifies encoding flag */
static bool  is_specify_language = false;	/* Command line specifies language flag */
/* End - ReqID:SRS-MISC-LICENSE-ENCODING */
/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
static bool  is_get_start_time = false;		/* acquire the start time */
static bool  is_get_final_time = false;		/* acquire the deadline time */
/* End - ReqID:SRS-MISC-LICENSE-TECC */

/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
static bool  is_get_highgo_version = false;	// acquire the highgo version
static bool  is_get_license_version = false;
static bool  is_get_license_serial = false;
static bool  is_get_license_status = false;
static bool  is_get_user_info = false;
static bool  is_get_auth_type = false;
static bool  is_get_used_type = false;
static bool  is_get_project_info = false;
static bool  is_get_max_cpu_cores = false;
static bool  is_get_dbname = false;
static bool  is_get_hardware_code = false;
/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */

static void  printLicenseData(void);
static void  _printFuncList(void);
static void  gen_hardware_code_file(const char *);

static char *lang = NULL;
static bool  is_chinese = false;
/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
static char *cli_encoding = NULL;	/* Command line specified encoding */
static char *cli_language = NULL;	/* Command line specified language */
/* End - ReqID:SRS-MISC-LICENSE-ENCODING */

static void  print_funclist_title_head(bool is_chinese, bool is_view);
static void  print_funclist_content(bool is_chinese, bool is_list, bool is_all, bool is_view);
static void  print_funclist_title_tail(bool is_chinese, bool is_view);

static void  usage(const char *progname);

/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
static void  commands_specified_language_or_encoding(void);	/* Whether to specify encoding and language parameters on the command line */
/* End - ReqID:SRS-MISC-LICENSE-ENCODING */

/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
static bool  is_get_extended_fields_all = false;
static bool  is_get_extended_fields_offset = false;
static int   extended_fields_offset = 0;
/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */

#ifdef WIN32
static char *lic_win32_langinfo(const char *ctype);
#endif

int main(int argc, char **argv)
{
	int		c;
	int		option_index;
	int		iRet;
#ifdef WIN32
	char	*name;
	char	*sys;
#endif

	static struct option long_options[] = {
		{ "list"    , no_argument      , NULL, 'l' },
		{ "all"     , no_argument      , NULL, 'a' },
		{ "view"    , no_argument      , NULL, 'v' },
		{ "filename", required_argument, NULL, 'f' },
		{ "check"   , required_argument, NULL, 'c' },
		{ "username", no_argument      , NULL, 'U' },
		{ "port"    , no_argument      , NULL, 'P' },
		{ "ignore-hardware-code", no_argument, NULL, 'i' },
		/* Begin - fix Bug#M0000321 */
		{ "ignore-md5-checksum", no_argument, NULL, 'I' },
		/* End - fix Bug#M0000321 */
		/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
		{ "client-license", no_argument, NULL, 'C' },
		{ "server-license", no_argument, NULL, 'S' },
		{ "ignore-datetime-check", no_argument, NULL, 'd' },
		/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
		/* Begin - fix Bug#M0000329 */
		{ "max-connections", no_argument, NULL, 'm' },
		/* End - fix Bug#M0000329 */
		/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
		{ "set-encoding", required_argument, NULL, 's' },
		{ "set-language", required_argument, NULL, 'e' },
		/* End - ReqID:SRS-MISC-LICENSE-ENCODING */
		/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
		{ "get-start-time", no_argument, NULL, 't' },
		{ "get-final-time", no_argument, NULL, 'T' },
		/* End - ReqID:SRS-MISC-LICENSE-TECC */
		/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		{ "license-version", no_argument, NULL, 1 },
		{ "license-serial", no_argument, NULL, 2 },
		{ "license-status", no_argument, NULL, 3 },
		{ "user-info", no_argument, NULL, 4 },
		{ "auth-type", no_argument, NULL, 5 },
		{ "used-type", no_argument, NULL, 6 },
		{ "project-info", no_argument, NULL, 7 },
		{ "get-highgo-version", no_argument, NULL, 'H' },
		{ "max-cpu-cores", no_argument, NULL, 'M' },
		{ "dbname", no_argument, NULL, 'D' },
		{ "hardware-code", no_argument, NULL, 'W' },
		/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		{ "extended-fields-all", no_argument, NULL, 'E' },
		{ "extended-fields", required_argument, NULL, 'F' },
		/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		{ "version" , no_argument      , NULL, 'V' },
		{ "help"    , no_argument      , NULL, '?' },
		{  NULL     , 0                , NULL,  0  }
	};

	/* for multiple languages support(zh_CN only for now) */
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("licchk"));
	
#ifdef WIN32
	name = setlocale(LC_CTYPE, "");
	if (!name)
		return -1;
		
	sys = lic_win32_langinfo(name);
	if (!sys)
		return -1;
		
	if (pg_strncasecmp(sys, "CP936", strlen("CP936")) == 0)
	{
		is_chinese = true;
		hg_set_encoding("GBK");		
	}
	else if (pg_strncasecmp(sys, "CP65001", strlen("CP65001")) == 0)
	{
		is_chinese = true;
	}
#endif
	
	if (getenv("LANG"))
	{
		lang = strdup(getenv("LANG"));
		if (pg_strncasecmp(lang, "zh_CN.GB18030", strlen("zh_CN.GB18030")) == 0)
		{
			is_chinese = true;
			hg_set_encoding("GB18030");
		}
		else if (pg_strncasecmp(lang, "zh_CN.GBK", strlen("zh_CN.GBK")) == 0)
		{
			is_chinese = true;
			hg_set_encoding("GBK");
		}
		else if (pg_strncasecmp(lang, "zh_CN.GB2312", strlen("zh_CN.GB2312")) == 0)
		{
			is_chinese = true;
			hg_set_encoding("GB2312");
		}
		else if (pg_strncasecmp(lang, "zh_CN.UTF-8", strlen("zh_CN.UTF-8")) == 0)
		{
			is_chinese = true;
		}
		else if (pg_strncasecmp(lang, "en_US.UTF-8", strlen("en_US.UTF-8")) == 0)
		{
			is_chinese = false;
		}
		free(lang);
	}

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage(argv[0]);
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			printf("licchk DB V%s\n", LICENSE_VERSION);
			exit(0);
		}
	}

	while ((c = getopt_long(argc, argv, "lavf:c:UPiICSdms:e:TtHDMWEF:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'f':
				license_dat = strdup(optarg);
				break;

			case 'l':
				is_list = true;
				break;

			case 'a':
				is_all = true;
				break;

			case 'c':
				hardware_code_file = strdup(optarg);
				break;
			
			case 'U':
				is_get_name = true;
				break;

			case 'P':
				is_get_port = true;
				break;

			case 'i':
				is_ignore_check_hardware = true;
				break;
			/* Begin - fix Bug#M0000321 */
			case 'I':
				is_ignore_md5_checksum = true;
				break;
			/* End - fix Bug#M0000321 */
			/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
			case 'C':
				is_client_dat = true;
				break;

			case 'S':
				is_client_dat = false;
				break;

			case 'd':
				is_ignore_datetime_check = true;
				break;
			/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
			/* Begin - fix Bug#M0000329 */
			case 'm':
				is_get_max_connections = true;
				break;
			/* End - fix Bug#M0000329 */

			/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
			case 's':
				is_specify_encoding = true;
				cli_encoding = strdup(optarg);
				break;

			case 'e':
				is_specify_language = true;
				cli_language = strdup(optarg);
				break;
			/* End - ReqID:SRS-MISC-LICENSE-ENCODING */
			/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
			case 'E':
				is_get_extended_fields_all = true;
				break;

			case 'F':
				is_get_extended_fields_offset = true;
				extended_fields_offset = atoi(optarg);
				break;
			/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */

			/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
			case 't':
				is_get_start_time = true;
				break;

			case 'T':
				is_get_final_time = true;
				break;
			/* End - ReqID:SRS-MISC-LICENSE-TECC */

			/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
			case 'H':
				is_get_highgo_version = true;
				break;

			case 1:
				is_get_license_version = true;
				break;

			case 2:
				is_get_license_serial = true;
				break;

			case 3:
				is_get_license_status = true;
				break;

			case 4:
				is_get_user_info = true;
				break;

			case 5:
				is_get_auth_type = true;
				break;

			case 6:
				is_get_used_type = true;
				break;

			case 7:
				is_get_project_info = true;
				break;

			case 'M':
				is_get_max_cpu_cores = true;
				break;

			case 'D':
				is_get_dbname = true;
				break;

			case 'W':
				is_get_hardware_code = true;
				break;
			/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */

			case 'v':
				is_view = true;
				break;

			default:
				fprintf(stderr, "Try \"%s --help\" for more information.\n",
						argv[0]);
				exit(1);
		}
	}

	if (hardware_code_file != NULL)
	{
		printf("hardware_code:%s\n", hg_get_calc_hardware_code());
		gen_hardware_code_file(hardware_code_file);
		free(hardware_code_file);
		exit(0);
	}

	/* load license.dat or license_cli.dat */
	if (license_dat == NULL)
	{
		hg_set_is_client_dat(is_client_dat); // ReqID:SRS-MISC-LICENSE-ClientTool
		hg_set_license_abspath(argv[0]);
		hg_set_is_ignore_md5_checksum(is_ignore_md5_checksum); // fix Bug#M0000321
		hg_set_is_ignore_hardware_check(is_ignore_check_hardware); // ReqID:SRS-MISC-LICENSE-ClientTool
		hg_set_is_ignore_datetime_check(is_ignore_datetime_check); // ReqID:SRS-MISC-LICENSE-ClientTool
		hg_set_is_chinese_title(is_chinese);

		commands_specified_language_or_encoding(); // ReqID::SRS-MISC-LICENSE-ENCODING

		iRet = hg_load_license(NULL);
		if (iRet != 0)
		{
			printf(_("license validate fail.\n"));
			return -1;
		}

		if (is_get_name)
		{
			printf("%s\n", hg_get_defaultDBuser());
		}
		else if (is_get_port)
		{
			printf("%s\n", hg_get_defaultDBport());
		}
		/* Begin - fix Bug#M0000329 */
		else if (is_get_max_connections)
		{
			printf("%s\n", hg_get_maxConnection());
		}
		/* End - fix Bug#M0000329 */
		/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
		else if (is_get_start_time)
		{
			printf("%s\n", hg_get_createTime());
		}
		else if (is_get_final_time)
		{
			printf("%s\n", hg_get_finalTime());
		}
		/* End - ReqID:SRS-MISC-LICENSE-TECC */
		/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		else if (is_get_highgo_version)
		{
			printf("%s\n", hg_get_highgoVersion());
		}
		else if (is_get_license_version)
		{
			printf("%s\n", hg_get_licver());
		}
		else if (is_get_license_serial)
		{
			printf("%s\n", hg_get_licSerial());
		}
		else if (is_get_license_status)
		{
			printf("%s\n", hg_get_license_status());
		}
		else if (is_get_user_info)
		{
			printf("%s\n", hg_get_userInfo());
		}
		else if (is_get_auth_type)
		{
			printf("%s\n", hg_get_authType());
		}
		else if (is_get_used_type)
		{
			printf("%s\n", hg_get_usedType());
		}
		else if (is_get_project_info)
		{
			printf("%s\n", hg_get_projectInfo());
		}
		else if (is_get_max_cpu_cores)
		{
			printf("%s\n", hg_get_maxCpuCores());
		}
		else if (is_get_dbname)
		{
			printf("%s\n", hg_get_defaultDBname());
		}
		else if (is_get_hardware_code)
		{
			printf("%s\n", hg_get_hardware_code());
		}
		/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		else if (is_get_extended_fields_all && !is_get_extended_fields_offset)
		{
			bool old_state;

			old_state = hg_set_is_with_title(true);
			if (hg_get_extended_fields(LICENSE_VERSION, 0))
				printf("%s\n", hg_get_extended_fields(LICENSE_VERSION, 0));
			hg_set_is_with_title(old_state);
		}
		else if (is_get_extended_fields_all && is_get_extended_fields_offset)
		{
			if (hg_get_extended_fields(LICENSE_VERSION, extended_fields_offset))
			printf("%s\n", hg_get_extended_fields(LICENSE_VERSION, extended_fields_offset));
		}
		/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		else
		{
			printLicenseData();
		}
		exit(0);
	}
	else if (license_dat != NULL && license_dat[0] != 0)
	{
		hg_set_is_ignore_md5_checksum(is_ignore_md5_checksum);	// fix Bug#M0000321
		hg_set_is_ignore_hardware_check(is_ignore_check_hardware); // ReqID:SRS-MISC-LICENSE-ClientTool
		hg_set_is_ignore_datetime_check(is_ignore_datetime_check); // ReqID:SRS-MISC-LICENSE-ClientTool
		hg_set_is_chinese_title(is_chinese);

		commands_specified_language_or_encoding(); // ReqID::SRS-MISC-LICENSE-ENCODING
	
		iRet = hg_load_license(license_dat);
		if (iRet != 0)
		{
			printf(_("license validate fail.\n"));
			return -1;
		}

		if (is_get_name)
		{
			printf("%s\n", hg_get_defaultDBuser());
		}
		else if (is_get_port)
		{
			printf("%s\n", hg_get_defaultDBport());
		}
		/* Begin - fix Bug#M0000334 */
		else if (is_get_max_connections)
		{
			printf("%s\n", hg_get_maxConnection());
		}
		/* End - fix Bug#M0000334 */
		/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
		else if (is_get_start_time)
		{
			printf("%s\n", hg_get_createTime());
		}
		else if (is_get_final_time)
		{
			printf("%s\n", hg_get_finalTime());
		}
		/* End - ReqID:SRS-MISC-LICENSE-TECC */
		/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		else if (is_get_highgo_version)
		{
			printf("%s\n", hg_get_highgoVersion());
		}
		else if (is_get_license_version)
		{
			printf("%s\n", hg_get_licver());
		}
		else if (is_get_license_serial)
		{
			printf("%s\n", hg_get_licSerial());
		}
		else if (is_get_license_status)
		{
			printf("%s\n", hg_get_license_status());
		}
		else if (is_get_user_info)
		{
			printf("%s\n", hg_get_userInfo());
		}
		else if (is_get_auth_type)
		{
			printf("%s\n", hg_get_authType());
		}
		else if (is_get_used_type)
		{
			printf("%s\n", hg_get_usedType());
		}
		else if (is_get_project_info)
		{
			printf("%s\n", hg_get_projectInfo());
		}
		else if (is_get_max_cpu_cores)
		{
			printf("%s\n", hg_get_maxCpuCores());
		}
		else if (is_get_dbname)
		{
			printf("%s\n", hg_get_defaultDBname());
		}
		else if (is_get_hardware_code)
		{
			printf("%s\n", hg_get_hardware_code());
		}
		/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
		/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		else if (is_get_extended_fields_all && !is_get_extended_fields_offset)
		{
			bool old_state;

			old_state = hg_set_is_with_title(true);
			if (hg_get_extended_fields(LICENSE_VERSION, 0))
				printf("%s\n", hg_get_extended_fields(LICENSE_VERSION, 0));
			hg_set_is_with_title(old_state);
		}
		else if (is_get_extended_fields_all && is_get_extended_fields_offset)
		{
			if (hg_get_extended_fields(LICENSE_VERSION, extended_fields_offset))
				printf("%s\n", hg_get_extended_fields(LICENSE_VERSION, extended_fields_offset));
		}
		/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
		else
		{
			printLicenseData();
		}

		exit(0);
	}

	return 0;
}

/* print license.dat contents */
static void 
printLicenseData(void)
{
	hg_set_is_with_title(true);
	
	printf("%s\n", hg_get_licver());
	printf("%s\n", hg_get_licSerial());
	printf("%s\n", hg_get_license_status());
	printf("%s\n", hg_get_userInfo());
	printf("%s\n", hg_get_authType());
	printf("%s\n", hg_get_usedType());
	printf("%s\n", hg_get_createTime());
	printf("%s\n", hg_get_finalTime());
	printf("%s\n", hg_get_highgoVersion());
	printf("%s\n", hg_get_projectInfo());
	printf("%s\n", hg_get_maxConnection());
	printf("%s\n", hg_get_maxCpuCores());
	printf("%s\n", hg_get_defaultDBname());
	printf("%s\n", hg_get_defaultDBuser());
	printf("%s\n", hg_get_defaultDBport());
	printf("%s\n", hg_get_hardware_code());
	
	if (is_list || is_all || is_view)
		_printFuncList();
}

static void 
_printFuncList(void)
{
	print_funclist_title_head(is_chinese, is_view);

	print_funclist_content(is_chinese, is_list, is_all, is_view);

	print_funclist_title_tail(is_chinese, is_view);
}

static void
gen_hardware_code_file(const char *filename)
{
	FILE *fp;

	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf("file %s open error.\n", filename);
		exit(-1);
	}

	fprintf(fp, "%s", hg_get_calc_hardware_code());

	fflush(fp);
	fclose(fp);
}

static void
print_funclist_title_head(bool is_chinese, bool is_view)
{
	if (is_view)
	{
		printf("%s", hg_get_funclist_title_head(is_view, is_chinese ? 64 : 84));
	}
	else
	{
		printf("%s", hg_get_funclist_title_head(is_view, is_chinese ? 39 : 56));
	}
}

static void
print_funclist_content(bool is_chinese, bool is_list, bool is_all, bool is_view)
{
	// The is_chinese parameter will be used for control character output width in the future
	printf("%s", hg_get_funclist_content(is_list, is_all, is_view, 18));
}

static void
print_funclist_title_tail(bool is_chinese, bool is_view)
{
	if (is_view)
	{
		printf("%s", hg_get_funclist_title_tail(is_chinese ? 64 : 84));
	}
	else
	{
		printf("%s", hg_get_funclist_title_tail(is_chinese ? 39 : 56));
	}
}

static void 
usage(const char *progname)
{
	printf(_("Usage:\n"));
	printf(_("  %s [OPTION]... [DATADIR]\n"), progname);
	printf(_("\nOptions:\n"));
	printf(_("  -l, --list                  list enabled function items\n"));
	printf(_("  -a, --all                   all functional items\n"));
	printf(_("  -v, --view                  show desc information\n"));
	printf(_("  -f, --filename=xxx.dat      set license filename\n"));
	printf(_("  -c, --check=xxx.txt         check hardware code, then exit.\n"));
	/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
	printf(_("  -C, --client-information    show client client_info.dat information.\n"));
	printf(_("  -S, --server-license        show server license.dat information.\n"));
	printf(_("  -d, --ignore-datetime-check ignore validity date of the license.\n"));
	/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
	printf(_("  -m, --max-connections       show max connections.\n"));
	printf(_("  -U, --username              obtain the username from the license file.\n"));
	printf(_("  -P, --port                  obtain the port from the license file.\n"));
	/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
	printf(_("  -s, --set-encoding          set license character encoding.\n"));
	printf(_("  -e, --set-language          set display language(ch or en).\n"));
	/* End - ReqID:SRS-MISC-LICENSE-ENCODING */
	/* Begin - ReqID:SRS-MISC-LICENSE-TECC */
	printf(_("  -t, --get-start-time        get authorization start time.\n"));
	printf(_("  -T, --get-final-time        get authorization final time.\n"));
	/* End - ReqID:SRS-MISC-LICENSE-TECC */
	/* Begin - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
	printf(_("  -H, --get-highgo-version    get highgo database version.\n"));
	printf(_("  -M, --max-cpu-cores         get max cpu cores.\n"));
	printf(_("  -D, --dbname                get database default name.\n"));
	printf(_("  -W, --hardware-code         get hardware code.\n"));
	printf(_("      --license-version       get license version.\n"));
	printf(_("      --license-serial        get license serial.\n"));
	printf(_("      --license-status        get license status.\n"));
	printf(_("      --user-info             get user information.\n"));
	printf(_("      --auth-type             get authorization type.\n"));
	printf(_("      --used-type             get used type.\n"));
	printf(_("      --project-info          get project information.\n"));
	/* End - ReqID:SRS-MISC-LICENSE-STYLECONFIG */
	/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
	printf(_("  -E, --extend-fields-all     output all extended parameters, then exit\n"));
	printf(_("  -F, --extend-fields-offset  output extended parameter values by offset, then exit\n"));
	/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
	printf(_("  -V, --version               output version information, then exit\n"));
	printf(_("  -?, --help                  show this help, then exit\n"));
}

#ifdef WIN32
static char *
lic_win32_langinfo(const char *ctype)
{
	char	   *r = NULL;

#if defined(_MSC_VER) && (_MSC_VER < 1900)
	_locale_t	loct = NULL;

	loct = _create_locale(LC_CTYPE, ctype);
	if (loct != NULL)
	{
		r = malloc(16);			/* excess */
		if (r != NULL)
			//sprintf(r, "CP%u", loct->locinfo->lc_codepage);
			sprintf_s(r, 16, "CP%u", loct->locinfo->lc_codepage);
		_free_locale(loct);
	}
#else
	char	   *codepage;

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
	uint32		cp;
	WCHAR		wctype[LOCALE_NAME_MAX_LENGTH];

	memset(wctype, 0, sizeof(wctype));
	MultiByteToWideChar(CP_ACP, 0, ctype, -1, wctype, LOCALE_NAME_MAX_LENGTH);

	if (GetLocaleInfoEx(wctype,
		LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
		(LPWSTR)&cp, sizeof(cp) / sizeof(WCHAR)) > 0)
	{
		r = malloc(16);			/* excess */
		if (r != NULL)
		{
			/*
			* If the return value is CP_ACP that means no ANSI code page is
			* available, so only Unicode can be used for the locale.
			*/
			if (cp == CP_ACP)
				strcpy(r, "utf8");
			else
				sprintf(r, "CP%u", cp);
		}
	}
	else
#endif
	{
		/*
		* Locale format on Win32 is <Language>_<Country>.<CodePage>.  For
		* example, English_United States.1252.  If we see digits after the
		* last dot, assume it's a codepage number.  Otherwise, we might be
		* dealing with a Unix-style locale string; Windows' setlocale() will
		* take those even though GetLocaleInfoEx() won't, so we end up here.
		* In that case, just return what's after the last dot and hope we can
		* find it in our table.
		*/
		codepage = strrchr(ctype, '.');
		if (codepage != NULL)
		{
			size_t		ln;

			codepage++;
			ln = strlen(codepage);
			r = malloc(ln + 3);
			if (r != NULL)
			{
				if (strspn(codepage, "0123456789") == ln)
					sprintf(r, "CP%s", codepage);
				else
					strcpy(r, codepage);
			}
		}

	}
#endif

	return r;
}
#endif

/* Begin - ReqID:SRS-MISC-LICENSE-ENCODING */
static void
commands_specified_language_or_encoding(void)
{
	if (is_specify_encoding)
	{
		hg_set_is_command_specified(true);
		hg_set_encoding(cli_encoding);
	}

	if (is_specify_language)
	{
		hg_set_is_command_specified(true);
		if (pg_strncasecmp(cli_language, "ch", strlen("ch")) == 0)
		{
			hg_set_is_chinese_title(true);
			hg_set_is_chinese_err_desc(true);
		}
		else if (pg_strncasecmp(cli_language, "en", strlen("en")) == 0)
		{
			hg_set_is_chinese_title(false);
			hg_set_is_chinese_err_desc(false);
		}
		else
		{
			printf("-e parameater error\n");
			exit(-1);
		}
	}
}
/* End - ReqID:SRS-MISC-LICENSE-ENCODING */
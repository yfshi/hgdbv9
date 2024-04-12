/*-------------------------------------------------------------------------
 *
 * Command line option processing facilities for frontend code
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/fe_utils/option_utils.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "fe_utils/option_utils.h"
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
#ifdef USE_LICENSE
#include "license.h"
#endif
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
/*
 * Provide strictly harmonized handling of --help and --version
 * options.
 */
void
handle_help_version_opts(int argc, char *argv[],
						 const char *fixed_progname, help_handler hlp)
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			hlp(get_progname(argv[0]));
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			printf("%s (PostgreSQL) " PG_VERSION "\n", fixed_progname);
			exit(0);
		}
/* Begin - ReqID:SRS-MISC-LICENSE-ClientTool */
#ifdef USE_LICENSE
		if (strcmp(argv[1], "--highgo-version") == 0 || strcmp(argv[1], "-G") == 0)
		{
			hg_load_client_license(argv[0]);
			printf("%s %s\n", fixed_progname, hg_get_highgoVersion());
			exit(0);
		}
#endif
/* End - ReqID:SRS-MISC-LICENSE-ClientTool */
	}
}

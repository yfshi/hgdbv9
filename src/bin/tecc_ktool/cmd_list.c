/*
 * cmd_list.c — tecc_ktool list command
 *
 * src/bin/tecc_ktool/cmd_list.c
 */

#include "tecc_ktool_common.h"

int
cmd_list(int argc, char *argv[])
{
	const char *dir_flag = NULL;	/* from --dir <path> or positional <path> */
	const char *dir_path;
	DIR		   *dir;
	struct dirent *entry;
	int			count = 0;
	int			ch;

	static struct option long_opts[] = {
		{"dir", required_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};

	/* Reset so repeated command dispatch in one process re-scans argv */
	optind = 1;

	while ((ch = getopt_long(argc, argv, "d:", long_opts, NULL)) != -1)
	{
		switch (ch)
		{
			case 'd':
				dir_flag = optarg;
				break;
			default:
				fprintf(stderr, "Usage: tecc_ktool list [--dir <path>]\n");
				return CMD_USAGE_ERROR;
		}
	}

	/* A bare positional directory argument is also accepted. */
	if (optind < argc)
	{
		if (dir_flag != NULL)
		{
			fprintf(stderr, "ERROR: both --dir and a directory argument given\n");
			return CMD_USAGE_ERROR;
		}
		dir_flag = argv[optind++];
		if (optind < argc)
		{
			fprintf(stderr, "ERROR: too many arguments\n");
			return CMD_USAGE_ERROR;
		}
	}

	dir_path = dir_flag ? dir_flag : ".";
	dir = opendir(dir_path);
	if (!dir)
	{
		fprintf(stderr, "ERROR: cannot open directory '%s': %s\n",
				dir_path, strerror(errno));
		return CMD_ERROR;
	}

	/*
	 * Table header + dash separator.  All columns are left-justified so the
	 * values sit in the same column as their header (size used to be
	 * right-justified, pushing values away from the "Size" label).  The dash
	 * line is derived from the header width and spans the Modified column
	 * (including the 10-char date), instead of a fixed short run.
	 */
	{
		char		header[160];
		int			i;
		int			hlen;

		hlen = snprintf(header, sizeof(header), "%-40s %-6s %-8s %-10s\n",
						"Name", "Type", "Size", "Modified");
		fputs(header, stdout);
		for (i = 0; i < hlen - 1; i++)	/* hlen includes the '\n' */
			putchar('-');
		putchar('\n');
	}

	while ((entry = readdir(dir)) != NULL)
	{
		char		full_path[1024];
		struct stat st;
		const char *ext;
		const char *type_str = "";
		time_t		mtime;

		if (entry->d_name[0] == '.')
			continue;

		ext = strrchr(entry->d_name, '.');
		if (!ext)
			continue;

		if (strcmp(ext, ".key") == 0)
			type_str = "sym";
		else if (strcmp(ext, ".priv") == 0)
			type_str = "asym";
		else
			continue;

		snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

		if (stat(full_path, &st) != 0)
			continue;

		mtime = st.st_mtime;
		{
			char timebuf[32];
			struct tm tm_res;
			char namebuf[256];
			size_t	baselen;

			/* Show the key name (file base name) rather than the extension */
			baselen = (size_t)(ext - entry->d_name);
			if (baselen > sizeof(namebuf) - 1)
				baselen = sizeof(namebuf) - 1;
			memcpy(namebuf, entry->d_name, baselen);
			namebuf[baselen] = '\0';

			strftime(timebuf, sizeof(timebuf), "%Y-%m-%d",
					 localtime_r(&mtime, &tm_res));
			printf("%-40s %-6s %-8zu %s\n",
				   namebuf, type_str, (size_t)st.st_size, timebuf);
		}
		count++;
	}

	closedir(dir);
	printf("\nTotal: %d KEK file(s)\n", count);
	return CMD_OK;
}

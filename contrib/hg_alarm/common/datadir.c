/*-------------------------------------------------------------------------
 *
 * datadir.c
 *	  routines about datadir limit
 *
 *	  contrib/hg_alarm/datadir.c
 *-------------------------------------------------------------------------
 */
#include "hg_alarm.h"
#include "common/file_utils.h"
#include <sys/stat.h>

/*-----------------------
 *	DataDir limit 
 *-----------------------
 */

static int64	dir_total_size;

static void
walkdir_action(const char *fname, bool isdir, int elevel)
{
	struct 	stat fst;

	if (stat(fname, &fst) < 0)
	{
		if (errno == ENOENT)
			return;
		else
			hga_log_warning("could not stat file \"%s\": %m", fname);
	}

	dir_total_size += fst.st_size;
}

static void
walkdir(const char *path,
		void (*action) (const char *fname, bool isdir, int elevel),
		bool process_symlinks,
		int elevel)
{
	DIR		   *dir;
	struct dirent *de;

	dir = opendir(path);
	if (!dir)
		return;

	while ((de = readdir(dir)) != NULL)
	{
		char		subpath[MAXPGPATH * 2];

		if (strcmp(de->d_name, ".") == 0 ||
			strcmp(de->d_name, "..") == 0)
			continue;

		snprintf(subpath, sizeof(subpath), "%s/%s", path, de->d_name);

		switch (get_dirent_type(subpath, de, process_symlinks, elevel))
		{
			case PGFILETYPE_REG:
				(*action) (subpath, false, elevel);
				break;
			case PGFILETYPE_DIR:
				walkdir(subpath, action, false, elevel);
				break;
			default:
				break;
		}
	}

	closedir(dir);

	if (dir)
		(*action) (path, true, elevel);
}

/*
 * get datadir size currently used.
 *	walk the all data dir to stat each file and get the sum of each file size
 */
int64
get_datadir_size(PGconn *conn)
{
	char   *sql = "SHOW data_directory";
	char   *dirpath;

	PGresult 	*res;

	if (!conn)
		return -1;

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		hga_log_warning("DATADIR: can not fetch the data_directory that the server is running");
		PQclear(res);
		return -2;
	}
	
	dirpath = PQgetvalue(res, 0, 0);

	/* No need to check existence of data_directory. */
	dir_total_size = 0;
	walkdir(dirpath, walkdir_action, false, 0);

	PQclear(res);

	return dir_total_size;
}

/*
 * do datadir checking.
 *	return:
 *		true: current datadir size used <= datadir limit
 *		false: any error occured or current datadir size used > datadir limit
 */
bool
do_datadir_check(PGconn *conn, int32 threshold)
{
	int32	dirsize_mb;

	if (threshold == UNLIMITED_INT)
		return true;

	dirsize_mb = get_datadir_size(conn) / (1024 * 1024);

	if (dirsize_mb > threshold)
	{
		hga_log_warning("DATADIR: datadir size[current: %dMB] exceeds limit[%dMB]",
			 dirsize_mb, threshold);
		return false;
	}

	return true; 
}

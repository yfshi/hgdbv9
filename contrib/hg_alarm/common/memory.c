/*-------------------------------------------------------------------------
 *
 * memory.c
 *	  routines about memory limit
 *
 *	  contrib/hg_alarm/memory.c
 *-------------------------------------------------------------------------
 */
#include "hg_alarm.h"

#include <unistd.h>
#include <fcntl.h>

#define MAXINT8LEN		20
#define pagetok(x)	((x) * sysconf(_SC_PAGESIZE) >> 10)

#define GET_NEXT_VALUE(p, q, value, length, msg, delim, tofree) \
        if ((q = strchr(p, delim)) == NULL) \
        { \
            hga_log_warning(msg); \
			pfree(tofree); \
			return -3; \
        } \
        length = q - p; \
        strncpy(value, p, length); \
        value[length] = '\0'; \
        p = q + 1;

#define SKIP_TOKEN(p) \
		/* Skipping leading white space. */ \
		while (isspace(*p)) \
			p++; \
		/* Skip token. */ \
		while (*p && !isspace(*p)) \
			p++; \
		/* Skipping trailing white space. */ \
		while (isspace(*p)) \
			p++;

/*-----------------------
 *	memory limit
 *-----------------------
 */

/*
 * get current memory size used.
 * 	1. query the pg_stat_acrivity to get all pids of the server
 *  2. parse /proc/pid/stat file to get the memory used by a certain process
 *  3. return the sum of each process' memory used
 */
int64
get_memory_size(PGconn *conn)
{
	int			 i, nproc;
	pid_t		*pids;
	int64		 total_rss = 0;
	char 		*sql = "SELECT pid FROM pg_stat_activity";

	PGresult 	*res;

	if (!conn)
		return -1;

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		hga_log_warning("MEMORY: fetch pid(s) from pg_stat_activity failed, %d", PQresultStatus(res));
		PQclear(res);
		return -2;
	}
	
	nproc = PQntuples(res);
	pids = palloc0(nproc * sizeof(pid_t));
	
	for (i = 0; i < nproc; i++)
		pids[i] = atoi(PQgetvalue(res, i, PQfnumber(res, "pid")));

	PQclear(res);

	for (i = 0; i < nproc; i++)
	{
		int		fd;
		char 	path[MAXPGPATH] = {0};
		char	buffer[4096] = {0};
		char   *p, *q;
		int		len;
		char    tmp[MAXINT8LEN + 1] = {0};
		int64	rss;

		snprintf(path, MAXPGPATH, "%s/%d/stat", PROCFS, pids[i]);
		fd = open(path, O_RDONLY);
		if (fd == -1)
		{
			/* Maybe the process has already exited, ignore it */
			continue;
		}

		if (read(fd, buffer, sizeof(buffer) - 1) < 0)
		{
			hga_log_error("MEMORY: cannot read \"%s\": %m", path);
			close(fd);
			continue;
		}
		close(fd);
		/*
		 * 23009 (postgres) S 1 23009 23009 0 -1 1077944320 17182 917618 0 0 3 
		 * 94 218 973 20 0 1 0 2164982 286154752 3402 18446744073709551615 4194304
		 * 14086028 140730924372688 140730924371848 139845140790051 0 0 24129536 84487
		 * 18446744071581351733 0 0 17 0 0 0 0 0 0 16186816 16253000 34906112 
		 * 140730924377504 140730924377560 140730924377560 140730924380107 0 
		 */
		p = buffer;
		SKIP_TOKEN(p); /* pid */
		SKIP_TOKEN(p); /* (postgres) */
		SKIP_TOKEN(p); /* S */
		SKIP_TOKEN(p); /* ppid */
		SKIP_TOKEN(p); /* prgp */
		SKIP_TOKEN(p); /* session */
		SKIP_TOKEN(p); /* tty_nr */
		SKIP_TOKEN(p); /* tpgid */
		SKIP_TOKEN(p); /* flags */
		SKIP_TOKEN(p); /* minflt */
		SKIP_TOKEN(p); /* cminflt */
		SKIP_TOKEN(p); /* majflt */
		SKIP_TOKEN(p); /* cmajflt */
		SKIP_TOKEN(p); /* utime */
		SKIP_TOKEN(p); /* stime */
		SKIP_TOKEN(p); /* cutime */
		SKIP_TOKEN(p); /* cstime */
		SKIP_TOKEN(p); /* priority */
		SKIP_TOKEN(p); /* nice */
		SKIP_TOKEN(p); /* num_threads */
		SKIP_TOKEN(p); /* itrealvalue */
		SKIP_TOKEN(p); /* starttime */
		SKIP_TOKEN(p); /* vsize */
		GET_NEXT_VALUE(p, q, tmp, len, "MEMORY: ERROR reading rss(unknown proc file format)", ' ', pids);
		snprintf(tmp, sizeof(tmp) -1, "%lld",
			pagetok(atoll(tmp))); /* (in KBs) */
		SKIP_TOKEN(p); /* rlim */
		SKIP_TOKEN(p); /* startcode */
		SKIP_TOKEN(p); /* endcode */
		SKIP_TOKEN(p); /* startstack */
		SKIP_TOKEN(p); /* kstkesp */
		SKIP_TOKEN(p); /* kstkeip */
		SKIP_TOKEN(p); /* signal (obsolete) */
		SKIP_TOKEN(p); /* blocked (obsolete) */
		SKIP_TOKEN(p); /* sigignore (obsolete) */
		SKIP_TOKEN(p); /* sigcatch (obsolete) */
		SKIP_TOKEN(p); /* wchan */
		SKIP_TOKEN(p); /* nswap (place holder) */
		SKIP_TOKEN(p); /* cnswap (place holder) */
		SKIP_TOKEN(p); /* exit_signal */
		SKIP_TOKEN(p); /* processor */
		SKIP_TOKEN(p); /* rt_priority */
		SKIP_TOKEN(p); /* policy */
		SKIP_TOKEN(p); /* delayacct_blkio_ticks */

		rss = atol(tmp);
		total_rss += rss;
	}
	
	pfree(pids);
	return total_rss;
}

/*
 * do memory checking.
 *	return:
 *		true: current memory used <= memory limit
 *		false: any error occured or current memory used > memory limit
 */
bool
do_memory_check(PGconn *conn, int32 threshold)
{
	int64	total_rss_kb;
	int32	total_rss_mb;
	
	if (threshold == UNLIMITED_INT)
		return true;

	total_rss_kb = get_memory_size(conn) /* KB */;

	if (total_rss_kb < 0)
		return false;

	total_rss_mb = total_rss_kb / 1024;

	if (total_rss_mb > threshold)
	{
		hga_log_warning("MEMORY: memory used[current: %dMB] exceeds[limit: %dMB]",
						 total_rss_mb, threshold);
		return false;
	}

	return true;
}
#include "hg_alarm.h"
#include "hga_log.h"

#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define 	DEFAULT_LOGDIR		"hg_alarm_log"
#define		MAXLOGLEN			(4096)

typedef struct Logger
{
	char   *logfile;	/* current file name */
	int32	segsize;	/* max segment size */
	int32	segno;		/* current segment number */
	bool	verbose;	/* always true */
	FILE   *fp;			/* file stream handle */
	char   *email_user;
	bool	send_email;
} Logger;

#define LOGFILE_FMT		"hg_alarm-%Y-%m-%d_%H%M%S.log"
#define LOGPREFIX		"%Y-%m-%d_%H%M%S"
#define LOGFILE_MODE	(S_IRUSR | S_IWUSR)

static Logger   logger;

static char *
logfile_getname(time_t timestamp, char *logdir)
{
	char	   *filename;
	int			len;

	filename = pg_malloc(MAXPGPATH);

	snprintf(filename, MAXPGPATH, "%s/", logdir);

	len = strlen(filename);

	/* treat Log_filename as a strftime pattern */
	strftime(filename + len, MAXPGPATH - len, LOGFILE_FMT, localtime(&timestamp));

	return filename;
}

static FILE *
logfile_open(const char *filename, const char *mode)
{
	FILE	   *fh;
	mode_t		oumask;

	/*
	 * Note we do not let Log_file_mode disable IWUSR, since we certainly want
	 * to be able to write the files ourselves.
	 */
	oumask = umask((mode_t) ((~(LOGFILE_MODE | S_IWUSR)) & (S_IRWXU | S_IRWXG | S_IRWXO)));
	fh = fopen(filename, mode);
	umask(oumask);

	if (fh)
	{
		setvbuf(fh, NULL, PG_IOLBF, 0);
	}
	else
	{
		pg_log_error("could not open log file \"%s\": %m",
						filename);
		return NULL;
	}

	return fh;
}

bool
CreateLogger(char *logdir, int segsize, char *mailuser)
{
    char        path[MAXPGPATH] = "";
    char       *logfile;
    FILE       *fh;
    struct  stat    sb;

    memset(&logger, 0, sizeof(logger));

	if (!logdir || strlen(logdir) == 0)
		logdir = DEFAULT_LOGDIR;
	
    /* make absolute control file path */
   if (is_absolute_path(logdir))
		strlcpy(path, logdir, MAXPGPATH);
	else
	{
		char        cwd[MAXPGPATH];

		if (getcwd(cwd, MAXPGPATH) == NULL)
		{
			pg_log_error("cannot read current directory: ");
			return false;
		}
		join_path_components(path, cwd, logdir);
	}
	canonicalize_path(path);

    if (stat(path, &sb) == 0)
    {
        if (!S_ISDIR(sb.st_mode))
        {
            pg_log_error("\"%s\" exists and is not a dir.", path);
            return false;
        }
    }
    else
    {
        if (mkdir(path, S_IRWXU) < 0)
        {
            pg_log_error("cannot create log directory \"%s\"", path);
            return false;
        }
    }

    logfile = logfile_getname(time(NULL), path);

    fh = logfile_open(logfile, "w");
    if (!fh)
    {
        pg_log_error("cannot create log file \"%s\"", logfile);
        return false;
    }

    logger.logfile = pg_strdup(logfile);
	logger.segsize = segsize;
    logger.fp = fh;
	logger.send_email = false;
	logger.email_user = mailuser;

    return true;
}

void
LoggerLog(int elevel, const char *fmt,...)
{
	va_list		args;
	char		prefix[MAXPGPATH] = "";
	char		msg[MAXLOGLEN] = "";
	int32		msg_len = 0;
	time_t		now;
	struct stat		sb;

	if (!logger.fp)
		return;		/* logger is not ready */

	if (fstat(fileno(logger.fp), &sb) < 0)
	{
		pg_log_error("could not stat file \"%s\"", logger.logfile);
		return;
	}

	now = time(NULL);
	strftime(prefix, MAXPGPATH, LOGPREFIX, localtime(&now));

	if (fseek(logger.fp, 0, SEEK_END) != 0)
	{
		pg_log_error("could not seek loader log file \"%s\": %m",
						logger.logfile);
		return;
	}

	sprintf(msg, "%s ", prefix);
	va_start(args, fmt);
	vsprintf(msg + strlen(msg), fmt, args);
	va_end(args);
	sprintf(msg + strlen(msg), "\n");

	msg_len = strlen(msg);

	/*
	 * Switch to the next log file.
	 */
	if (msg_len + sb.st_size > logger.segsize * 1024 * 1024)
	{
		char	path[MAXPGPATH] = "";

		logger.segno ++;
		fclose(logger.fp);

		snprintf(path, MAXPGPATH, "%s.%d", logger.logfile, logger.segno);
		logger.fp = logfile_open(path, "w");
		if (!logger.fp)
		{
			pg_log_error("cannot create log file \"%s\"", path);
			return;
		}
	}

	fprintf(logger.fp, "%s ", msg);

	if (fflush(logger.fp))
	{
		pg_log_error("could not write loader log file \"%s\": %m",
						logger.logfile);
		return;
	}

	if (!logger.send_email && logger.email_user)
	{
		char	mail_msg[1024] = {0};
		int		ret;

		snprintf(mail_msg, 1024, 
				 "mail -s \"Warning happened on HighgoDB \" %s <<< \'check log file \"%s\" for more details.\'",
				 logger.email_user, logger.logfile);
			
		ret = system(mail_msg);
		if (-1 != ret && WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
			pg_log_info("send a mail to %s", logger.email_user);
		else
			pg_log_error("failed to send mail to %s", logger.email_user);

		logger.send_email = true;
	}

#if false
	if (elevel >= PG_LOG_ERROR || (logger.verbose && elevel >= PG_LOG_WARNING))
	{
		char   *buf;

		buf = palloc(len + 1);

		va_start(args, fmt);
		vsnprintf(buf, len + 1, fmt, args);
		va_end(args);

		while (len > 0 && isspace((unsigned char) buf[len - 1]))
			len--;
		buf[len] = '\0';

		// pg_log_generic(elevel, "%s", elevel);
		pfree(buf);
	}
#endif
}

void
LoggerClose(void)
{
	if (logger.fp != NULL && fclose(logger.fp) < 0)
		pg_log_error("could not close loader log file \"%s\": %m",
						logger.logfile);

	if (logger.logfile != NULL)
		pg_free(logger.logfile);

	memset(&logger, 0, sizeof(logger));
}
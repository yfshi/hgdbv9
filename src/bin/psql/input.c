/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright (c) 2000-2021, PostgreSQL Global Development Group
 *
 * src/bin/psql/input.c
 */
#include "postgres_fe.h"

#ifndef WIN32
#include <unistd.h>
#include <regex.h>
#endif
#include <fcntl.h>
#include <limits.h>

#include "common.h"
#include "common/logging.h"
#include "input.h"
#include "settings.h"
#include "tab-complete.h"

#ifndef WIN32
#define PSQLHISTORY ".psql_history"
#else
#define PSQLHISTORY "psql_history"
#endif

/* Runtime options for turning off readline and history */
/* (of course there is no runtime command for doing that :) */
#ifdef USE_READLINE
static bool useReadline;
static bool useHistory;

static char *psql_history = NULL;

static int history_lines_added;

/*
 *	Preserve newlines in saved queries by mapping '\n' to NL_IN_HISTORY
 *
 *	It is assumed NL_IN_HISTORY will never be entered by the user
 *	nor appear inside a multi-byte string.  0x00 is not properly
 *	handled by the readline routines so it can not be used
 *	for this purpose.
 */
#define NL_IN_HISTORY 0x01
#endif

static void finishInput(void);

/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
static void
finishInput_no_save(void);

#ifndef WIN32
static int
is_tag_char(char c)
{
	return isalnum((unsigned char)c) || c == '_';
}

static int
find_single_quoted_end(const char *str)
{
	const char *p = str + 1;
	while (*p)
	{
		if (*p == '\'')
		{
			if (*(p + 1) == '\'')
			{
				p += 2;
			}
			else
			{
				return (int)(p - str + 1);
			}
		}
		else
		{
			p++;
		}
	}
	return -1;
}

static int
find_escape_quoted_end(const char *str)
{
	const char *p = str + 1;
	while (*p)
	{
		if (*p == '\\' && (*(p + 1) == '\'' || *(p + 1) == '\\'))
		{
			p += 2;
		}
		else if (*p == '\'')
		{
			return (int)(p - str + 1);
		}
		else
		{
			p++;
		}
	}
	return -1;
}

static int
find_dollar_quoted_end(const char *str)
{
	const char *p = str + 1;
	char tag[64] = {0};
	int tag_len = 0;
	char end_tag[70];
	const char *q = NULL;
	size_t end_tag_len = 0;

	if (*str != '$')
		return -1;

	while (is_tag_char(*p) && tag_len < (int)(sizeof(tag) - 1))
	{
		tag[tag_len++] = *p++;
	}
	if (*p != '$')
		return -1;
	p++;

	snprintf(end_tag, sizeof(end_tag), "$%s$", tag);
	end_tag_len = strlen(end_tag);
	q = p;
	while (*q)
	{
		if (*q == '$' && strncmp(q, end_tag, end_tag_len) == 0)
		{
			return (int)(q - str + end_tag_len);
		}
		q++;
	}
	return -1;
}

static bool
hide_password(const char *input, char *output, size_t out_size)
{
	regex_t regex;
	const char *cursor = input;
	size_t out_pos = 0;
	regmatch_t pmatch[2];
	const char *replacement = "'******'";

	int m0_s = 0;
	int g1_s = 0;
	int g1_e = 0;

	const char *pattern = "password[[:space:]]+((E|e)?['$])";
	if (regcomp(&regex, pattern, REG_ICASE | REG_EXTENDED) != 0)
	{
		fprintf(stderr, "Regex compilation failed\n");
		return false;
	}

	while (*cursor)
	{
		int first_len;
		int has_e_prefix = 0;
		const char *g1_start = NULL;
		const char *pw_start = NULL;
		const char *lit_end = NULL;
		size_t prefix_len = 0;
		size_t rep_len = 0;

		int ret = regexec(&regex, cursor, 2, pmatch, 0);
		if (ret == REG_NOMATCH)
		{
			size_t remain = strlen(cursor);
			if (out_pos + remain >= out_size)
			{
				regfree(&regex);
				return false;
			}
			memcpy(output + out_pos, cursor, remain);
			out_pos += remain;
			break;
		}
		else if (ret != 0)
		{
			regfree(&regex);
			return false;
		}

		m0_s = pmatch[0].rm_so;
		g1_s = pmatch[1].rm_so;
		g1_e = pmatch[1].rm_eo;

		if (out_pos + m0_s >= out_size)
		{
			regfree(&regex);
			return false;
		}
		memcpy(output + out_pos, cursor, m0_s);
		out_pos += m0_s;

		has_e_prefix = 0;
		g1_start = cursor + g1_s;
		if (g1_e - g1_s >= 2 && (g1_start[0] == 'E' || g1_start[0] == 'e'))
		{
			has_e_prefix = 1;
			pw_start = g1_start + 1;
		}
		else
		{
			pw_start = g1_start;
		}

		if (*pw_start == '\'')
		{
			first_len = has_e_prefix
							? find_escape_quoted_end(pw_start)
							: find_single_quoted_end(pw_start);
		}
		else
		{
			first_len = find_dollar_quoted_end(pw_start);
		}
		if (first_len < 0)
		{
			regfree(&regex);
			return false;
		}

		lit_end = pw_start + first_len;
		while (1)
		{
			int next_len = -1;
			const char *t = lit_end;
			while (*t && isspace((unsigned char)*t))
				t++;
			if ((t[0] == 'E' || t[0] == 'e') && t[1] == '\'')
			{
				next_len = find_escape_quoted_end(t);
			}
			else if (t[0] == '\'')
			{
				next_len = find_single_quoted_end(t);
			}
			else if (t[0] == '$')
			{
				next_len = find_dollar_quoted_end(t);
			}
			else
			{
				break;
			}
			if (next_len < 0)
			{
				regfree(&regex);
				return false;
			}
			lit_end = t + next_len;
		}

		prefix_len = (pw_start - (cursor + m0_s));
		if (out_pos + prefix_len >= out_size)
		{
			regfree(&regex);
			return false;
		}
		memcpy(output + out_pos, cursor + m0_s, prefix_len);
		out_pos += prefix_len;

		rep_len = strlen(replacement);
		if (out_pos + rep_len >= out_size)
		{
			regfree(&regex);
			return false;
		}
		memcpy(output + out_pos, replacement, rep_len);
		out_pos += rep_len;

		cursor = lit_end;
	}

	output[out_pos] = '\0';
	regfree(&regex);
	return true;
}
#else
static bool
hide_password(const char *input, char *output, size_t out_size)
{
	return false;
}
#endif
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

/*
 * gets_interactive()
 *
 * Gets a line of interactive input, using readline if desired.
 *
 * prompt: the prompt string to be used
 * query_buf: buffer containing lines already read in the current command
 * (query_buf is not modified here, but may be consulted for tab completion)
 *
 * The result is a malloc'd string.
 *
 * Caller *must* have set up sigint_interrupt_jmp before calling.
 */
char *
gets_interactive(const char *prompt, PQExpBuffer query_buf)
{
#ifdef USE_READLINE
	if (useReadline)
	{
		char *result;

		/*
		 * Some versions of readline don't notice SIGWINCH signals that arrive
		 * when not actively reading input.  The simplest fix is to always
		 * re-read the terminal size.  This leaves a window for SIGWINCH to be
		 * missed between here and where readline() enables libreadline's
		 * signal handler, but that's probably short enough to be ignored.
		 */
#ifdef HAVE_RL_RESET_SCREEN_SIZE
		rl_reset_screen_size();
#endif

		/* Make current query_buf available to tab completion callback */
		tab_completion_query_buf = query_buf;

		/* Enable SIGINT to longjmp to sigint_interrupt_jmp */
		sigint_interrupt_enabled = true;

		/* On some platforms, readline is declared as readline(char *) */
		result = readline((char *)prompt);

		/* Disable SIGINT again */
		sigint_interrupt_enabled = false;

		/* Pure neatnik-ism */
		tab_completion_query_buf = NULL;

		return result;
	}
#endif

	fputs(prompt, stdout);
	fflush(stdout);
	return gets_fromFile(stdin);
}

/*
 * Append the line to the history buffer, making sure there is a trailing '\n'
 */
void pg_append_history(const char *s, PQExpBuffer history_buf)
{
#ifdef USE_READLINE
	if (useHistory && s)
	{
		appendPQExpBufferStr(history_buf, s);
		if (!s[0] || s[strlen(s) - 1] != '\n')
			appendPQExpBufferChar(history_buf, '\n');
	}
#endif
}

/*
 * Emit accumulated history entry to readline's history mechanism,
 * then reset the buffer to empty.
 *
 * Note: we write nothing if history_buf is empty, so extra calls to this
 * function don't hurt.  There must have been at least one line added by
 * pg_append_history before we'll do anything.
 */
void pg_send_history(PQExpBuffer history_buf)
{
#ifdef USE_READLINE
	static char *prev_hist = NULL;

	char *s = history_buf->data;
	int i;

	/* Trim any trailing \n's (OK to scribble on history_buf) */
	for (i = strlen(s) - 1; i >= 0 && s[i] == '\n'; i--)
		;
	s[i + 1] = '\0';

	if (useHistory && s[0])
	{
		if (((pset.histcontrol & hctl_ignorespace) &&
			 s[0] == ' ') ||
			((pset.histcontrol & hctl_ignoredups) &&
			 prev_hist && strcmp(s, prev_hist) == 0))
		{
			/* Ignore this line as far as history is concerned */
		}
		else
		{
			char *tmp = NULL;

			/* Save each previous line for ignoredups processing */
			if (prev_hist)
				free(prev_hist);
			prev_hist = pg_strdup(s);
			/* And send it to readline */
			/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
			tmp = pg_malloc(strlen(s) + 7);
			if (hide_password(s, tmp, strlen(s) + 7))
			{
				add_history(tmp);
			}
			else
			{
				add_history(s);
			}
			if (tmp)
				pfree(tmp);
			/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */
			/* Count lines added to history for use later */
			history_lines_added++;
		}
	}

	resetPQExpBuffer(history_buf);
#endif
}

/*
 * gets_fromFile
 *
 * Gets a line of noninteractive input from a file (which could be stdin).
 * The result is a malloc'd string, or NULL on EOF or input error.
 *
 * Caller *must* have set up sigint_interrupt_jmp before calling.
 *
 * Note: we re-use a static PQExpBuffer for each call.  This is to avoid
 * leaking memory if interrupted by SIGINT.
 */
char *
gets_fromFile(FILE *source)
{
	static PQExpBuffer buffer = NULL;

	char line[1024];

	if (buffer == NULL) /* first time through? */
		buffer = createPQExpBuffer();
	else
		resetPQExpBuffer(buffer);

	for (;;)
	{
		char *result;

		/* Enable SIGINT to longjmp to sigint_interrupt_jmp */
		sigint_interrupt_enabled = true;

		/* Get some data */
		result = fgets(line, sizeof(line), source);

		/* Disable SIGINT again */
		sigint_interrupt_enabled = false;

		/* EOF or error? */
		if (result == NULL)
		{
			if (ferror(source))
			{
				pg_log_error("could not read from input file: %m");
				return NULL;
			}
			break;
		}

		appendPQExpBufferStr(buffer, line);

		if (PQExpBufferBroken(buffer))
		{
			pg_log_error("out of memory");
			return NULL;
		}

		/* EOL? */
		if (buffer->len > 0 && buffer->data[buffer->len - 1] == '\n')
		{
			buffer->data[buffer->len - 1] = '\0';
			return pg_strdup(buffer->data);
		}
	}

	if (buffer->len > 0) /* EOF after reading some bufferload(s) */
		return pg_strdup(buffer->data);

	/* EOF, so return null */
	return NULL;
}

#ifdef USE_READLINE

/*
 * Macros to iterate over each element of the history list in order
 *
 * You would think this would be simple enough, but in its inimitable fashion
 * libedit has managed to break it: in libreadline we must use next_history()
 * to go from oldest to newest, but in libedit we must use previous_history().
 * To detect what to do, we make a trial call of previous_history(): if it
 * fails, then either next_history() is what to use, or there's zero or one
 * history entry so that it doesn't matter which direction we go.
 *
 * In case that wasn't disgusting enough: the code below is not as obvious as
 * it might appear.  In some libedit releases history_set_pos(0) fails until
 * at least one add_history() call has been done.  This is not an issue for
 * printHistory() or encode_history(), which cannot be invoked before that has
 * happened.  In decode_history(), that's not so, and what actually happens is
 * that we are sitting on the newest entry to start with, previous_history()
 * fails, and we iterate over all the entries using next_history().  So the
 * decode_history() loop iterates over the entries in the wrong order when
 * using such a libedit release, and if there were another attempt to use
 * BEGIN_ITERATE_HISTORY() before some add_history() call had happened, it
 * wouldn't work.  Fortunately we don't care about either of those things.
 *
 * Usage pattern is:
 *
 *		BEGIN_ITERATE_HISTORY(varname);
 *		{
 *			loop body referencing varname->line;
 *		}
 *		END_ITERATE_HISTORY();
 */
#define BEGIN_ITERATE_HISTORY(VARNAME)                                  \
	do                                                                  \
	{                                                                   \
		HIST_ENTRY *VARNAME;                                            \
		bool use_prev_;                                                 \
                                                                        \
		history_set_pos(0);                                             \
		use_prev_ = (previous_history() != NULL);                       \
		history_set_pos(0);                                             \
		for (VARNAME = current_history(); VARNAME != NULL;              \
			 VARNAME = use_prev_ ? previous_history() : next_history()) \
		{                                                               \
			(void)0

#define END_ITERATE_HISTORY() \
	}                         \
	}                         \
	while (0)

/*
 * Convert newlines to NL_IN_HISTORY for safe saving in readline history file
 */
static void
encode_history(void)
{
	BEGIN_ITERATE_HISTORY(cur_hist);
	{
		char *cur_ptr;

		/* some platforms declare HIST_ENTRY.line as const char * */
		for (cur_ptr = (char *)cur_hist->line; *cur_ptr; cur_ptr++)
		{
			if (*cur_ptr == '\n')
				*cur_ptr = NL_IN_HISTORY;
		}
	}
	END_ITERATE_HISTORY();
}

/*
 * Reverse the above encoding
 */
static void
decode_history(void)
{
	BEGIN_ITERATE_HISTORY(cur_hist);
	{
		char *cur_ptr;

		/* some platforms declare HIST_ENTRY.line as const char * */
		for (cur_ptr = (char *)cur_hist->line; *cur_ptr; cur_ptr++)
		{
			if (*cur_ptr == NL_IN_HISTORY)
				*cur_ptr = '\n';
		}
	}
	END_ITERATE_HISTORY();
}
#endif /* USE_READLINE */

/*
 * Put any startup stuff related to input in here. It's good to maintain
 * abstraction this way.
 *
 * The only "flag" right now is 1 for use readline & history.
 */
void initializeInput(int flags, int save_flags) // ReqID:SRS-SECURITY-IDENTITYAUTH
{
#ifdef USE_READLINE
	if (flags & 1)
	{
		const char *histfile;
		char home[MAXPGPATH];

		useReadline = true;

		/* these two things must be done in this order: */
		initialize_readline();
		rl_initialize();

		useHistory = true;
		using_history();
		history_lines_added = 0;

		histfile = GetVariable(pset.vars, "HISTFILE");

		if (histfile == NULL)
		{
			char *envhist;

			envhist = getenv("PSQL_HISTORY");
			if (envhist != NULL && strlen(envhist) > 0)
				histfile = envhist;
		}

		if (histfile == NULL)
		{
			if (get_home_path(home))
				psql_history = psprintf("%s/%s", home, PSQLHISTORY);
		}
		else
		{
			psql_history = pg_strdup(histfile);
			expand_tilde(&psql_history);
		}

		if (psql_history)
		{
			read_history(psql_history);
			decode_history();
		}
	}
#endif

	/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
	if (save_flags & 1)
		atexit(finishInput);
	else
		atexit(finishInput_no_save);
	/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */
}

/*
 * This function saves the readline history when psql exits.
 *
 * fname: pathname of history file.  (Should really be "const char *",
 * but some ancient versions of readline omit the const-decoration.)
 *
 * max_lines: if >= 0, limit history file to that many entries.
 */
#ifdef USE_READLINE
static bool
saveHistory(char *fname, int max_lines)
{
	int errnum;

	/*
	 * Suppressing the write attempt when HISTFILE is set to /dev/null may
	 * look like a negligible optimization, but it's necessary on e.g. macOS,
	 * where write_history will fail because it tries to chmod the target
	 * file.
	 */
	if (strcmp(fname, DEVNULL) != 0)
	{
		/*
		 * Encode \n, since otherwise readline will reload multiline history
		 * entries as separate lines.  (libedit doesn't really need this, but
		 * we do it anyway since it's too hard to tell which implementation we
		 * are using.)
		 */
		encode_history();

		/*
		 * On newer versions of libreadline, truncate the history file as
		 * needed and then append what we've added.  This avoids overwriting
		 * history from other concurrent sessions (although there are still
		 * race conditions when two sessions exit at about the same time). If
		 * we don't have those functions, fall back to write_history().
		 */
#if defined(HAVE_HISTORY_TRUNCATE_FILE) && defined(HAVE_APPEND_HISTORY)
		{
			int nlines;
			int fd;

			/* truncate previous entries if needed */
			if (max_lines >= 0)
			{
				nlines = Max(max_lines - history_lines_added, 0);
				(void)history_truncate_file(fname, nlines);
			}
			/* append_history fails if file doesn't already exist :-( */
			fd = open(fname, O_CREAT | O_WRONLY | PG_BINARY, 0600);
			if (fd >= 0)
				close(fd);
			/* append the appropriate number of lines */
			if (max_lines >= 0)
				nlines = Min(max_lines, history_lines_added);
			else
				nlines = history_lines_added;
			errnum = append_history(nlines, fname);
			if (errnum == 0)
				return true;
		}
#else /* don't have append support */
		{
			/* truncate what we have ... */
			if (max_lines >= 0)
				stifle_history(max_lines);
			/* ... and overwrite file.  Tough luck for concurrent sessions. */
			errnum = write_history(fname);
			if (errnum == 0)
				return true;
		}
#endif

		pg_log_error("could not save history to file \"%s\": %m", fname);
	}
	return false;
}
#endif

/*
 * Print history to the specified file, or to the console if fname is NULL
 * (psql \s command)
 *
 * We used to use saveHistory() for this purpose, but that doesn't permit
 * use of a pager; moreover libedit's implementation behaves incompatibly
 * (preferring to encode its output) and may fail outright when the target
 * file is specified as /dev/tty.
 */
bool printHistory(const char *fname, unsigned short int pager)
{
#ifdef USE_READLINE
	FILE *output;
	bool is_pager;

	if (!useHistory)
		return false;

	if (fname == NULL)
	{
		/* use pager, if enabled, when printing to console */
		output = PageOutput(INT_MAX, pager ? &(pset.popt.topt) : NULL);
		is_pager = true;
	}
	else
	{
		output = fopen(fname, "w");
		if (output == NULL)
		{
			pg_log_error("could not save history to file \"%s\": %m", fname);
			return false;
		}
		is_pager = false;
	}

	BEGIN_ITERATE_HISTORY(cur_hist);
	{
		fprintf(output, "%s\n", cur_hist->line);
	}
	END_ITERATE_HISTORY();

	if (is_pager)
		ClosePager(output);
	else
		fclose(output);

	return true;
#else
	pg_log_error("history is not supported by this installation");
	return false;
#endif
}

static void
finishInput(void)
{
#ifdef USE_READLINE
	if (useHistory && psql_history)
	{
		(void)saveHistory(psql_history, pset.histsize);
		free(psql_history);
		psql_history = NULL;
	}
#endif
}

static void
finishInput_no_save(void)
{
#ifdef USE_READLINE
	if (useHistory && psql_history)
	{
		free(psql_history);
		psql_history = NULL;
	}
#endif
}

/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
/*-------------------------------------------------------------------------
 *
 * wrap.c
 *	  Frontend PL/SQL source wrapping utility.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <ctype.h>
#include <sys/stat.h>

#include "common/plsqlwrap_crypto.h"
#include "common/logging.h"
#include "getopt_long.h"
#include "lib/stringinfo.h"
#include "port.h"

#define WRAP_MAX_INPUT_SIZE ((size_t) 256 * 1024 * 1024)

typedef struct WrapOptions
{
	const char *input;			/* input file name */
	const char *output;			/* output file name, or NULL for default */
	const char *algorithm;		/* normalized cipher name, defaulting to AES */
	bool		keep_comments;	/* preserve comments outside wrapped units */
	bool		keep_comments_seen; /* reject duplicate comment options */
} WrapOptions;

/* Read a named input file into one allocation sized from fstat(). */
static char *
read_file(const char *path)
{
	FILE	   *file;
	char	   *data;
	struct stat file_stat;
	size_t		file_size;

	file = fopen(path, PG_BINARY_R);
	if (file == NULL)
	{
		pg_log_error("could not open input file \"%s\": %m", path);
		return NULL;
	}
	if (fstat(fileno(file), &file_stat) != 0)
	{
		pg_log_error("could not stat input file \"%s\": %m", path);
		fclose(file);
		return NULL;
	}
	if (file_stat.st_size < 0 ||
		(uint64) file_stat.st_size > (uint64) WRAP_MAX_INPUT_SIZE)
	{
		pg_log_error("input file \"%s\" exceeds the maximum size of 256 MB",
					 path);
		fclose(file);
		return NULL;
	}
	if (file_stat.st_size == 0)
	{
		pg_log_error("input file \"%s\" is empty", path);
		fclose(file);
		return NULL;
	}

	file_size = (size_t) file_stat.st_size;
	data = (char *) pg_malloc(file_size + 1);
	if (data == NULL)
	{
		pg_log_error("out of memory");
		fclose(file);
		return NULL;
	}
	if (fread(data, 1, file_size, file) != file_size)
	{
		pg_log_error("could not read input file \"%s\": %m", path);
		pg_free(data);
		fclose(file);
		return NULL;
	}

	data[file_size] = '\0';
	if (fclose(file) != 0)
	{
		pg_log_error("could not close input file \"%s\": %m", path);
		pg_free(data);
		return NULL;
	}
	return data;
}

/* Write to a named output file. */
static bool
write_file(const char *path, const char *data)
{
	FILE	   *file;
	size_t		len = strlen(data);

	file = fopen(path, PG_BINARY_W);
	if (file == NULL)
	{
		pg_log_error("could not open output file \"%s\": %m", path);
		return false;
	}
	if (fwrite(data, 1, len, file) != len)
	{
		pg_log_error("could not write output file \"%s\": %m", path);
		fclose(file);
		return false;
	}
	if (fclose(file) != 0)
	{
		pg_log_error("could not close output file \"%s\": %m", path);
		return false;
	}
	return true;
}

/* Return true if path names a file with an explicit basename extension. */
static bool
path_has_extension(const char *path)
{
	const char *basename;
	const char *dot;

	basename = last_dir_separator(path);
	basename = basename == NULL ? path : basename + 1;
	dot = strrchr(basename, '.');
	return dot != NULL && dot != basename;
}

/* Resolve Oracle wrap's default .sql extension for extensionless input names. */
static char *
resolve_input_path(const char *input)
{
	size_t		length;
	char	   *path;

	if (path_has_extension(input))
		return pg_strdup(input);

	length = strlen(input);
	path = (char *) pg_malloc(length + strlen(".sql") + 1);
	if (path == NULL)
	{
		pg_log_error("out of memory");
		return NULL;
	}
	memcpy(path, input, length);
	strcpy(path + length, ".sql");
	return path;
}

/* Derive Oracle wrap's default output name by replacing the input extension. */
static char *
default_output_path(const char *input)
{
	const char *basename;
	const char *dot;
	size_t		prefix_length;
	char	   *path;

	basename = last_dir_separator(input);
	basename = basename == NULL ? input : basename + 1;
	dot = strrchr(basename, '.');
	prefix_length = dot == NULL || dot == basename ? strlen(input) : dot - input;
	path = (char *) pg_malloc(prefix_length + strlen(".plb") + 1);
	if (path == NULL)
	{
		pg_log_error("out of memory");
		return NULL;
	}
	memcpy(path, input, prefix_length);
	strcpy(path + prefix_length, ".plb");
	return path;
}

/* Detect an existing wrapped payload so wrapping remains idempotent. */
static bool
source_is_wrapped(const char *src)
{
	if (src == NULL)
		return false;

	while (isspace((unsigned char) *src))
		src++;
	return strncmp(src, PG_PLSQL_WRAP_MAGIC,
				   strlen(PG_PLSQL_WRAP_MAGIC)) == 0;
}

/* Dispatch source wrapping to the shared frontend/backend crypto module. */
static char *
wrap_source(const char *src, const char *algorithm)
{
	PgPlsqlWrapCipher cipher_type;
	const char *error_message;
	char	   *wrapped;

	if (src == NULL || algorithm == NULL)
	{
		pg_log_error("PL/SQL source and algorithm must not be null");
		return NULL;
	}

	if (source_is_wrapped(src))
		return pg_strdup(src);

	if (strcmp(algorithm, "aes") == 0)
		cipher_type = PG_PLSQL_WRAP_CIPHER_AES;
	else if (strcmp(algorithm, "sm4") == 0)
		cipher_type = PG_PLSQL_WRAP_CIPHER_SM4;
	else
	{
		pg_log_error("invalid PL/SQL source wrapping algorithm");
		return NULL;
	}

	wrapped = pg_plsql_wrap_encrypt(src, cipher_type, &error_message);
	if (wrapped == NULL)
		pg_log_error("%s", error_message);

	return wrapped;
}

/* Find the source text immediately following the outer PL/SQL IS or AS clause. */
static const char *
find_body_start(const char *ddl)
{
	const char *p = ddl;

	while (*p != '\0')
	{
		while (*p != '\0' && isspace((unsigned char) *p))
			p++;

		if ((pg_strncasecmp(p, "is", 2) == 0 &&
			 !isalnum((unsigned char) p[2]) && p[2] != '_') ||
			(pg_strncasecmp(p, "as", 2) == 0 &&
			 !isalnum((unsigned char) p[2]) && p[2] != '_'))
		{
			const char *q = p + 2;

			while (*q != '\0' && isspace((unsigned char) *q))
				q++;
			return q;
		}

		while (*p != '\0' && !isspace((unsigned char) *p))
			p++;
	}

	return NULL;
}

static char *strip_comments(const char *src);

/*
 * Replace the IS/AS body with an executable Oracle-style WRAPPED statement
 * while retaining the readable CREATE header.
 */
static char *
wrap_ddl(const char *ddl, const char *algorithm)
{
	char	   *commentless_ddl;
	const char *body;
	const char *body_end;
	char	   *body_source;
	char	   *wrapped_body;
	char	   *quoted_body;
	char	   *result;
	const char *header_end;
	size_t		head_len;
	size_t		body_len;

	if (ddl == NULL || algorithm == NULL)
	{
		pg_log_error("PL/SQL DDL and algorithm must not be null");
		return NULL;
	}

	commentless_ddl = strip_comments(ddl);
	body = find_body_start(commentless_ddl);
	if (body == NULL)
	{
		pg_log_error("could not locate PL/SQL source body to wrap");
		pg_free(commentless_ddl);
		return NULL;
	}

	/* Remove IS/AS and retain only the readable CREATE header. */
	header_end = body;
	while (header_end > commentless_ddl && isspace((unsigned char) header_end[-1]))
		header_end--;
	if (header_end - commentless_ddl < 2 ||
		(pg_strncasecmp(header_end - 2, "is", 2) != 0 &&
		 pg_strncasecmp(header_end - 2, "as", 2) != 0))
	{
		pg_log_error("could not locate PL/SQL source clause to wrap");
		pg_free(commentless_ddl);
		return NULL;
	}
	header_end -= 2;

	body_end = commentless_ddl + strlen(commentless_ddl);
	while (body_end > body && isspace((unsigned char) body_end[-1]))
		body_end--;
	if (body_end > body && body_end[-1] == ';')
		body_end--;
	while (body_end > body && isspace((unsigned char) body_end[-1]))
		body_end--;

	body_len = body_end - body;
	body_source = (char *) pg_malloc(body_len + 1);
	if (body_source == NULL)
	{
		pg_log_error("out of memory");
		pg_free(commentless_ddl);
		return NULL;
	}
	memcpy(body_source, body, body_len);
	body_source[body_len] = '\0';

	wrapped_body = wrap_source(body_source, algorithm);
	if (wrapped_body == NULL)
	{
		pg_free(body_source);
		pg_free(commentless_ddl);
		return NULL;
	}
	/* Emit the same quoted payload form as the backend DBMS_DDL wrapper. */
	quoted_body = pg_plsql_wrap_quote_payload(wrapped_body);
	if (quoted_body == NULL)
	{
		pg_log_error("invalid wrapped PL/SQL source");
		pg_free(body_source);
		pg_free(wrapped_body);
		pg_free(commentless_ddl);
		return NULL;
	}
	head_len = header_end - commentless_ddl;
	result = (char *) pg_malloc(head_len + strlen("WRAPPED\n") +
						 strlen(quoted_body) + 2);
	if (result == NULL)
	{
		pg_log_error("out of memory");
		pg_free(body_source);
		pg_free(wrapped_body);
		pg_free(quoted_body);
		pg_free(commentless_ddl);
		return NULL;
	}
	memcpy(result, commentless_ddl, head_len);
	strcpy(result + head_len, "WRAPPED\n");
	strcat(result, quoted_body);
	strcat(result, ";");

	pg_free(body_source);
	pg_free(wrapped_body);
	pg_free(quoted_body);
	pg_free(commentless_ddl);
	return result;
}

/* Return true when a keyword starts at a word boundary in a SQL script. */
static bool
keyword_at(const char *ptr, const char *keyword)
{
	size_t		length;

	if (ptr == NULL || keyword == NULL)
		return false;

	length = strlen(keyword);

	return pg_strncasecmp(ptr, keyword, length) == 0 &&
		!isalnum((unsigned char) ptr[length]) && ptr[length] != '_';
}

/* Skip a single-quoted literal, including doubled quote escapes. */
static const char *
skip_single_quoted(const char *ptr)
{
	ptr++;
	while (*ptr != '\0')
	{
		if (*ptr == '\'')
		{
			if (ptr[1] == '\'')
				ptr += 2;
			else
				return ptr + 1;
		}
		else
			ptr++;
	}
	return ptr;
}

/* Skip a double-quoted identifier, including doubled quote escapes. */
static const char *
skip_double_quoted(const char *ptr)
{
	ptr++;
	while (*ptr != '\0')
	{
		if (*ptr == '"')
		{
			if (ptr[1] == '"')
				ptr += 2;
			else
				return ptr + 1;
		}
		else
			ptr++;
	}
	return ptr;
}

/* Skip an Oracle q-quoted literal when ptr points at its q or Q prefix. */
static const char *
skip_q_quoted(const char *ptr)
{
	char		opening;
	char		closing;

	if ((ptr[0] != 'q' && ptr[0] != 'Q') || ptr[1] != '\'' || ptr[2] == '\0')
		return ptr;

	opening = ptr[2];
	if (opening == '[')
		closing = ']';
	else if (opening == '{')
		closing = '}';
	else if (opening == '(')
		closing = ')';
	else if (opening == '<')
		closing = '>';
	else
		closing = opening;

	ptr += 3;
	while (*ptr != '\0')
	{
		if (ptr[0] == closing && ptr[1] == '\'')
			return ptr + 2;
		ptr++;
	}
	return ptr;
}

/* Remove SQL comments while retaining newlines and quoted literal contents. */
static char *
strip_comments(const char *src)
{
	StringInfoData output;
	const char *ptr = src;

	initStringInfo(&output);
	while (*ptr != '\0')
	{
		if (ptr[0] == '-' && ptr[1] == '-')
		{
			ptr += 2;
			while (*ptr != '\0' && *ptr != '\n')
				ptr++;
			continue;
		}
		if (ptr[0] == '/' && ptr[1] == '*')
		{
			ptr += 2;
			while (*ptr != '\0' && !(ptr[0] == '*' && ptr[1] == '/'))
			{
				if (*ptr == '\n')
					appendStringInfoChar(&output, *ptr);
				ptr++;
			}
			if (*ptr != '\0')
				ptr += 2;
			continue;
		}
		if (*ptr == '\'')
		{
			const char *end = skip_single_quoted(ptr);

			appendBinaryStringInfo(&output, ptr, end - ptr);
			ptr = end;
			continue;
		}
		if (*ptr == '"')
		{
			const char *end = skip_double_quoted(ptr);

			appendBinaryStringInfo(&output, ptr, end - ptr);
			ptr = end;
			continue;
		}
		if ((ptr[0] == 'q' || ptr[0] == 'Q') && ptr[1] == '\'')
		{
			const char *end = skip_q_quoted(ptr);

			appendBinaryStringInfo(&output, ptr, end - ptr);
			ptr = end;
			continue;
		}
		appendStringInfoChar(&output, *ptr);
		ptr++;
	}
	return output.data;
}

/* Append a script fragment, removing comments unless keep_comments is set. */
static void
append_script_text(StringInfo output, const char *src, Size length,
				   bool keep_comments)
{
	char	   *fragment;
	char	   *commentless;

	if (keep_comments)
	{
		appendBinaryStringInfo(output, src, length);
		return;
	}
	fragment = pnstrdup(src, length);
	commentless = strip_comments(fragment);
	appendStringInfoString(output, commentless);
	pg_free(fragment);
	pg_free(commentless);
}

/* Skip whitespace and comments while parsing a CREATE statement header. */
static const char *
skip_space_and_comments(const char *ptr)
{
	for (;;)
	{
		while (isspace((unsigned char) *ptr))
			ptr++;
		if (ptr[0] == '-' && ptr[1] == '-')
		{
			ptr += 2;
			while (*ptr != '\0' && *ptr != '\n')
				ptr++;
			continue;
		}
		if (ptr[0] == '/' && ptr[1] == '*')
		{
			ptr += 2;
			while (*ptr != '\0' && !(ptr[0] == '*' && ptr[1] == '/'))
				ptr++;
			if (*ptr != '\0')
				ptr += 2;
			continue;
		}
		return ptr;
	}
}

/* Identify the PL/SQL CREATE forms accepted by the wrapping utility. */
static bool
supported_create_at(const char *ptr)
{
	const char *next;

	if (ptr == NULL)
		return false;

	if (!keyword_at(ptr, "create"))
		return false;
	next = skip_space_and_comments(ptr + strlen("create"));
	if (keyword_at(next, "or"))
	{
		next = skip_space_and_comments(next + strlen("or"));
		if (!keyword_at(next, "replace"))
			return false;
		next = skip_space_and_comments(next + strlen("replace"));
	}
	if (keyword_at(next, "function") || keyword_at(next, "procedure"))
		return true;
	if (!keyword_at(next, "package"))
		return false;
	return true;
}

/* Detect a SQL*Plus slash line without treating a slash in source as a delimiter. */
static bool
is_sqlplus_slash_line(const char *ptr)
{
	while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r')
		ptr++;
	if (*ptr != '/')
		return false;
	ptr++;
	while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r')
		ptr++;
	return *ptr == '\n' || *ptr == '\0';
}

/* Find the SQL*Plus slash delimiter while respecting SQL lexical contexts. */
static const char *
find_sqlplus_delimiter(const char *ptr)
{
	bool		at_line_start = true;

	while (*ptr != '\0')
	{
		if (at_line_start && is_sqlplus_slash_line(ptr))
			return ptr;
		if (ptr[0] == '-' && ptr[1] == '-')
		{
			while (*ptr != '\0' && *ptr != '\n')
				ptr++;
			continue;
		}
		if (ptr[0] == '/' && ptr[1] == '*')
		{
			ptr += 2;
			while (*ptr != '\0' && !(ptr[0] == '*' && ptr[1] == '/'))
				ptr++;
			if (*ptr != '\0')
				ptr += 2;
			at_line_start = false;
			continue;
		}
		if (*ptr == '\'')
		{
			ptr = skip_single_quoted(ptr);
			at_line_start = false;
			continue;
		}
		if (*ptr == '"')
		{
			ptr = skip_double_quoted(ptr);
			at_line_start = false;
			continue;
		}
		if ((ptr[0] == 'q' || ptr[0] == 'Q') && ptr[1] == '\'')
		{
			ptr = skip_q_quoted(ptr);
			at_line_start = false;
			continue;
		}
		at_line_start = *ptr == '\n';
		ptr++;
	}
	return NULL;
}

/* Check whether the CREATE header already contains the WRAPPED marker. */
static bool
statement_is_wrapped(const char *ptr, const char *end)
{
	while (ptr < end && *ptr != '\0')
	{
		if (isspace((unsigned char) *ptr))
		{
			ptr++;
			continue;
		}
		if (ptr[0] == '-' && ptr[1] == '-')
		{
			ptr += 2;
			while (ptr < end && *ptr != '\n')
				ptr++;
			continue;
		}
		if (ptr[0] == '/' && ptr[1] == '*')
		{
			ptr += 2;
			while (ptr < end && !(ptr[0] == '*' && ptr[1] == '/'))
				ptr++;
			if (ptr < end)
				ptr += 2;
			continue;
		}
		if (*ptr == '\'')
		{
			ptr = skip_single_quoted(ptr);
			continue;
		}
		if (*ptr == '"')
		{
			ptr = skip_double_quoted(ptr);
			continue;
		}
		if (isalpha((unsigned char) *ptr) || *ptr == '_')
		{
			const char *word = ptr;

			while (isalnum((unsigned char) *ptr) || *ptr == '_')
				ptr++;
			if (keyword_at(word, "wrapped"))
				return true;
			if (keyword_at(word, "is") || keyword_at(word, "as"))
				return false;
			continue;
		}
		ptr++;
	}
	return false;
}

/* Wrap supported PL/SQL units and preserve external comments on request. */
static char *
wrap_script(const char *script, const char *algorithm, bool keep_comments)
{
	StringInfoData output;
	const char *copy_start;
	const char *ptr;

	if (script == NULL || algorithm == NULL)
	{
		pg_log_error("SQL script and algorithm must not be null");
		return NULL;
	}

	copy_start = script;
	ptr = script;

	initStringInfo(&output);
	while (*ptr != '\0')
	{
		/* skip - / \ " q Q */
		if (ptr[0] == '-' && ptr[1] == '-')
		{
			while (*ptr != '\0' && *ptr != '\n')
				ptr++;
			continue;
		}
		if (ptr[0] == '/' && ptr[1] == '*')
		{
			ptr += 2;
			while (*ptr != '\0' && !(ptr[0] == '*' && ptr[1] == '/'))
				ptr++;
			if (*ptr != '\0')
				ptr += 2;
			continue;
		}
		if (*ptr == '\'')
		{
			ptr = skip_single_quoted(ptr);
			continue;
		}
		if (*ptr == '"')
		{
			ptr = skip_double_quoted(ptr);
			continue;
		}
		if ((ptr[0] == 'q' || ptr[0] == 'Q') && ptr[1] == '\'')
		{
			ptr = skip_q_quoted(ptr);
			continue;
		}

		/* check function/ procedure/ package */
		if ((ptr == script || (!isalnum((unsigned char) ptr[-1]) && ptr[-1] != '_')) &&
			supported_create_at(ptr))
		{
			const char *delimiter = find_sqlplus_delimiter(ptr); /* find \ (finish) */
			const char *statement_end = delimiter == NULL ? script + strlen(script) : delimiter;
			const char *ddl_end = statement_end;
			char	   *statement;
			char	   *wrapped;

			while (ddl_end > ptr && isspace((unsigned char) ddl_end[-1]))
				ddl_end--;

			/* append not need wrapped text */
			append_script_text(&output, copy_start, ptr - copy_start,
						   keep_comments);
			if (statement_is_wrapped(ptr, statement_end))
			{
				appendBinaryStringInfo(&output, ptr, statement_end - ptr);
			}
			else
			{
				statement = pnstrdup(ptr, ddl_end - ptr);
				wrapped = wrap_ddl(statement, algorithm);
				if (wrapped == NULL)
				{
					pg_free(statement);
					pg_free(output.data);
					return NULL;
				}
				appendStringInfoString(&output, wrapped);
				appendBinaryStringInfo(&output, ddl_end, statement_end - ddl_end);
				pg_free(statement);
				pg_free(wrapped);
			}
			copy_start = statement_end;
			ptr = statement_end;
			continue;
		}
		ptr++;
	}
	append_script_text(&output, copy_start, strlen(copy_start), keep_comments);
	return output.data;
}

/* Print Oracle-compatible command-line forms and IvorySQL cipher extension. */
static void
usage(const char *progname)
{
	printf("%s wraps PL/SQL source text.\n\n", progname);
	printf("Usage:\n");
	printf("  %s [OPTION]... -i INPUT [-o OUTPUT]\n", progname);
	printf("  %s iname=input_file [oname=output_file] [keep_comments=yes] [algorithm=aes|sm4]\n", progname);
	printf("  %s input_file [output_file] [keep_comments=yes] [algorithm=aes|sm4]\n", progname);
	printf("\nInput and output options:\n");
	printf("  -i, --iname=FILE          input file\n");
	printf("  -o, --oname=FILE          output file\n");
	printf("  -a, --algorithm=NAME      wrapping algorithm: aes (default) or sm4\n");
	printf("  -k                        preserve comments outside wrapped units\n");
	printf("      --keep_comments=BOOL  preserve comments: yes or no (default)\n");
	printf("  -?, --help                show this help, then exit\n");
	printf("\nIf INPUT has no extension, .sql is assumed.\n");
	printf("If OUTPUT is omitted, the output extension defaults to .plb.\n");
}

static void
hint_try_help(const char *progname)
{
	pg_log_info("Try \"%s --help\" for more information.", progname);
}

/* Assign an input or output option and reject ambiguous duplicate values. */
static bool
set_file_option(WrapOptions *options, bool input, const char *value)
{
	if (*value == '\0')
	{
		pg_log_error("%s file name must not be empty", input ? "input" : "output");
		return false;
	}
	if (input)
	{
		if (options->input != NULL)
		{
			pg_log_error("input file specified more than once");
			return false;
		}
		options->input = value;
	}
	else
	{
		if (options->output != NULL)
		{
			pg_log_error("output file specified more than once");
			return false;
		}
		options->output = value;
	}
	return true;
}

static bool
set_algorithm_option(WrapOptions *options, const char *value)
{
	if (options->algorithm != NULL)
	{
		pg_log_error("algorithm specified more than once");
		return false;
	}
	if (*value == '\0')
	{
		pg_log_error("algorithm must not be empty");
		return false;
	}
	options->algorithm = value;
	return true;
}

static bool
set_keep_comments_option(WrapOptions *options, const char *name,
						 const char *value)
{
	if (options->keep_comments_seen)
	{
		pg_log_error("keep_comments specified more than once");
		return false;
	}
	if (pg_strcasecmp(value, "yes") == 0)
		options->keep_comments = true;
	else if (pg_strcasecmp(value, "no") == 0)
		options->keep_comments = false;
	else
	{
		pg_log_error("%s must be yes or no", name);
		return false;
	}
	options->keep_comments_seen = true;
	return true;
}

/* Parse one Oracle-compatible name=value argument, if recognized. */
static int
parse_oracle_option(WrapOptions *options, const char *argument)
{
	const char *equals = strchr(argument, '=');
	size_t		name_length;
	const char *value;

	if (equals == NULL)
		return 0;
	name_length = equals - argument;
	value = equals + 1;

	if (name_length == strlen("iname") &&
		pg_strncasecmp(argument, "iname", name_length) == 0)
		return set_file_option(options, true, value) ? 1 : -1;
	if (name_length == strlen("oname") &&
		pg_strncasecmp(argument, "oname", name_length) == 0)
		return set_file_option(options, false, value) ? 1 : -1;
	if (name_length == strlen("algorithm") &&
		pg_strncasecmp(argument, "algorithm", name_length) == 0)
		return set_algorithm_option(options, value) ? 1 : -1;
	if (name_length == strlen("keep_comments") &&
		pg_strncasecmp(argument, "keep_comments", name_length) == 0)
		return set_keep_comments_option(options, "keep_comments", value) ? 1 : -1;

	return 0;
}

/* Determine whether two path names identify the same filesystem object. */
static bool
paths_are_same(const char *input, const char *output, bool *same)
{
	char	   *absolute_input;
	char	   *absolute_output;
	struct stat input_stat;
	struct stat output_stat;

	if (input == NULL || output == NULL || same == NULL)
		return false;

	absolute_input = make_absolute_path(input);
	absolute_output = make_absolute_path(output);
	if (absolute_input == NULL || absolute_output == NULL)
	{
		free(absolute_input);
		free(absolute_output);
		return false;
	}
#ifdef WIN32
	*same = pg_strcasecmp(absolute_input, absolute_output) == 0;
#else
	*same = strcmp(absolute_input, absolute_output) == 0;
#endif
	free(absolute_input);
	free(absolute_output);

	if (!*same && stat(input, &input_stat) == 0 && stat(output, &output_stat) == 0)
		*same = input_stat.st_dev == output_stat.st_dev &&
			input_stat.st_ino == output_stat.st_ino;
	return true;
}

/* Parse PostgreSQL-style options plus the retained Oracle-compatible syntax. */
static bool
parse_options(int argc, char **argv, const char *progname, WrapOptions *options)
{
	static struct option long_options[] = {
		{"iname", required_argument, NULL, 'i'},
		{"oname", required_argument, NULL, 'o'},
		{"algorithm", required_argument, NULL, 'a'},
		{"keep_comments", required_argument, NULL, 1},
		{"help", no_argument, NULL, 2},
		{NULL, 0, NULL, 0}
	};
	const char *positionals[2];
	int			positional_count = 0;
	int			option;
	int			i;

	while ((option = getopt_long(argc, argv, "i:o:a:k", long_options, NULL)) != -1)
	{
		switch (option)
		{
			case 'i':
				if (!set_file_option(options, true, optarg))
					return false;
				break;
			case 'o':
				if (!set_file_option(options, false, optarg))
					return false;
				break;
			case 'a':
				if (!set_algorithm_option(options, optarg))
					return false;
				break;
			case 'k':
				if (options->keep_comments_seen)
				{
					pg_log_error("keep_comments specified more than once");
					return false;
				}
				options->keep_comments = true;
				options->keep_comments_seen = true;
				break;
			case 1:
				if (!set_keep_comments_option(options, "keep_comments", optarg))
					return false;
				break;
			case 2:
				usage(progname);
				exit(EXIT_SUCCESS);
			default:
				return false;
		}
	}

	for (i = optind; i < argc; i++)
	{
		int			oracle_option = parse_oracle_option(options, argv[i]);

		if (oracle_option < 0)
			return false;
		if (oracle_option > 0)
			continue;
		if (positional_count >= lengthof(positionals))
		{
			pg_log_error("too many command-line arguments (first is \"%s\")", argv[i]);
			return false;
		}
		positionals[positional_count++] = argv[i];
	}

	if ((options->input != NULL || options->output != NULL) &&
		positional_count > 0)
	{
		pg_log_error("cannot mix positional and named input or output files");
		return false;
	}
	if (positional_count > 0)
	{
		options->input = positionals[0];
		if (positional_count > 1)
			options->output = positionals[1];
	}
	if (options->input == NULL)
	{
		pg_log_error("no input file specified");
		return false;
	}
	if (*options->input == '\0')
	{
		pg_log_error("input file name must not be empty");
		return false;
	}
	if (strcmp(options->input, "-") == 0)
	{
		pg_log_error("standard input is not supported");
		return false;
	}
	if (options->output != NULL && strcmp(options->output, "-") == 0)
	{
		pg_log_error("standard output is not supported");
		return false;
	}

	if (options->algorithm == NULL)
		options->algorithm = "aes";
	else if (pg_strcasecmp(options->algorithm, "aes") == 0)
		options->algorithm = "aes";
	else if (pg_strcasecmp(options->algorithm, "sm4") == 0)
		options->algorithm = "sm4";
	else
	{
		pg_log_error("algorithm must be aes or sm4");
		return false;
	}
	return true;
}

/* Parse arguments, wrap every supported statement, and write the result. */
int
main(int argc, char **argv)
{
	const char *progname;
	WrapOptions options = {0};
	char	   *input_path = NULL;
	char	   *output_path = NULL;
	char	   *ddl;
	char	   *wrapped;
	int			exit_code = EXIT_FAILURE;
	bool		same_path;

	pg_logging_init(argv[0]);
	pg_logging_set_level(PG_LOG_WARNING);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("postgres"));
	progname = get_progname(argv[0]);

	if (argc > 1 && (strcmp(argv[1], "--help") == 0 ||
				   strcmp(argv[1], "-?") == 0))
	{
		usage(progname);
		return EXIT_SUCCESS;
	}
	if (!parse_options(argc, argv, progname, &options))
	{
		hint_try_help(progname);
		return EXIT_FAILURE;
	}

	input_path = resolve_input_path(options.input);
	if (input_path == NULL)
		goto cleanup_paths;
	output_path = options.output != NULL ? pg_strdup(options.output) :
		default_output_path(input_path);
	if (output_path == NULL)
		goto cleanup_paths;
	if (!paths_are_same(input_path, output_path, &same_path))
		goto cleanup_paths;
	if (same_path)
	{
		pg_log_error("input and output files must be different");
		goto cleanup_paths;
	}
	ddl = read_file(input_path);
	if (ddl == NULL)
		goto cleanup_paths;
	wrapped = wrap_script(ddl, options.algorithm, options.keep_comments);
	if (wrapped == NULL)
	{
		pg_free(ddl);
		goto cleanup_paths;
	}
	if (write_file(output_path, wrapped))
		exit_code = EXIT_SUCCESS;

	pg_free(ddl);
	pg_free(wrapped);
cleanup_paths:
	pg_free(input_path);
	pg_free(output_path);
	return exit_code;
}
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/

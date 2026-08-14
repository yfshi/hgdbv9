/*-------------------------------------------------------------------------
 *
 * hg_integrity_check.c
 *
 * Authored by xuji@highgo.com, 20260210.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_integrity_check/hg_integrity_check.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-EAL4-WAL_MD5 */
#include "postgres.h"

#include <unistd.h>

#include "access/heapam.h"
#include "access/xlog.h"
#include "access/xlog_internal.h"
#include "common/md5.h"
/* Begin - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
#include "common/hmac.h"
#include "common/sha2.h"
#include "common/sm3.h"
/* End - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
#include "common/cryptohash.h"
#include "catalog/indexing.h"
#include "postmaster/pgarch.h"
#include "utils/builtins.h"
#include "miscadmin.h"
#include "se_features_interface.h"

/* Begin - ReqID:DKY-INTEGRITY-CHECKLOG */
PG_MODULE_MAGIC;

#define CHECKFILE "checklog.txt"
#define MD5_DATALEN 64
#define CHECK_REL_NUM 5

/* Begin - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
#ifdef USE_GMSSL
	#define PG_HMAC_DIGEST_LENGTH			PG_SM3_DIGEST_LENGTH
	#define PG_HMAC_DIGEST_STRING_LENGTH	PG_SM3_DIGEST_STRING_LENGTH
#else
	#define PG_HMAC_DIGEST_LENGTH			PG_SHA256_DIGEST_LENGTH
	#define PG_HMAC_DIGEST_STRING_LENGTH	PG_SHA256_DIGEST_STRING_LENGTH
#endif
/* End - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */


typedef enum FileHmacSumStatus /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
{
	HMAC_FILE_NOT_EXIST,
	HMAC_FILE_NOT_READ,
	HMAC_SUM_OTHER_FAILED,
	HMAC_NULL
} FileHmacSumStatus;

static const char* err_info[] = {
	"The file does not exist.",
	"The file cannot be opened.",
	"The system other error."
};

static bool wal_integrity_enable = false;

typedef struct
{
	char fname[MAXPGPATH];
	/* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
	char checksum[PG_HMAC_DIGEST_STRING_LENGTH]; /* 32 hex + null */
} ChecksumRecord;

Datum pg_wal_md5_verify(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_wal_md5_verify);

/* Begin - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
Datum pg_wal_checksum_verify(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_wal_checksum_verify);
/* End - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */

Datum pg_wal_init_checklog(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_wal_init_checklog);

void		_PG_init(void);
void		_PG_fini(void);

bool pg_file_md5_checksum(const char *filename, char *md5_checksum, FileHmacSumStatus *errnumber);
bool pg_file_hmac_checksum(const char *filename, char *hmac_checksum, FileHmacSumStatus *errnumber); /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
static void parse_archivelog_path(char *archive_path, char *archive_command);
static char* trim_whitespace(char *str);
static bool xlog_checksum_w(char * filename, char *archive_command); /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
static bool set_wal_integrity_check(bool *newval, void **extra, GucSource source);
/* ----------------------------------------------------------------
 * parse_archivelog_path: 
 * Analyze the GUC parameters of archive_command to obtain the full path of archive logs
 */
static void
parse_archivelog_path(char *archive_path, char *archive_command)
{
	const char *p = archive_command;
	const char *arg3_start;
	const char *arg3_end;
	const char *last_sep = NULL;
	size_t len = 0;

	// Skip leading whitespace
	while (isspace(*p))
		p++;

	// --- Parse the first argument (command name, ignored) ---
	if (*p == '"' || *p == '\'')
	{
		char quote = *p++;
		while (*p && *p != quote)
			p++;
		if (*p == quote)
			p++;
	}
	else
	{
		while (*p && !isspace(*p))
			p++;
	}

	// Skip whitespace to reach the second argument (source file)
	while (isspace(*p))
		p++;
	if (*p == '\0')
		return; // Missing second argument

	// --- Parse the second argument (source file, ignored) ---
	if (*p == '"' || *p == '\'')
	{
		char quote = *p++;
		while (*p && *p != quote)
			p++;
		if (*p == quote)
			p++;
	}
	else
	{
		while (*p && !isspace(*p))
			p++;
	}

	// Skip whitespace to reach the third argument (target path)
	while (isspace(*p))
		p++;
	if (*p == '\0')
		return; // Missing third argument

	// --- Parse the third argument (target path) ---
	if (*p == '"' || *p == '\'')
	{
		char quote = *p++;
		arg3_start = p;
		while (*p && *p != quote)
			p++;
		arg3_end = p;
		if (*p == quote)
			p++;
	}
	else
	{
		arg3_start = p;
		while (*p && !isspace(*p))
			p++;
		arg3_end = p;
	}

	if (arg3_start >= arg3_end)
		return; // Empty content

	// Find the last '/' or '\'
	for (const char *q = arg3_start; q < arg3_end; q++)
	{
		if (*q == '/' || *q == '\\')
			last_sep = q;
	}
	if (!last_sep)
		return; // No separator found

	// Extract the part from start to last_sep (including the separator)
	len = last_sep - arg3_start + 1;

	memcpy(archive_path, arg3_start, len);
	archive_path[len] = '\0';
	return;
}

/* ----------------------------------------------------------------
 * Trim leading/trailing whitespace (in-place)
 * ----------------------------------------------------------------
 */
static char*
trim_whitespace(char *str)
{
	char *start = str;
	char *end;
	/* leading whitespace */
	while (isspace((unsigned char)*start))
		start++;
	if (*start == '\0')
	{
		*str = '\0';
		return str;
	}
	/* trailing whitespace */
	end = start + strlen(start) - 1;
	while (end > start && isspace((unsigned char)*end))
		end--;
	/* move cleaned string forward */
	memmove(str, start, end - start + 1);
	str[end - start + 1] = '\0';

	return str;
}

/* Begin - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */

/* ----------------------------------------------------------------
 * Main function: scan pg_wal, verify MD5 of completed segments
 * ----------------------------------------------------------------
 */
Datum pg_wal_md5_verify(PG_FUNCTION_ARGS)
{
	return pg_wal_checksum_verify(fcinfo);
}

/* ----------------------------------------------------------------
 * Main function: scan pg_wal, verify checksum of completed segments
 * ----------------------------------------------------------------
 */
Datum pg_wal_checksum_verify(PG_FUNCTION_ARGS)
{
	ChecksumRecord *records = NULL;
	FILE *rfp = NULL;
	int nrecords = 0;
	int maxrecords = 0;
	/* ----- Filesystem variables ----- */
	ReturnSetInfo *rsinfo = NULL;
	TupleDesc tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	/* Parameter handling: if no argument or NULL, use default $PGDATA/pg_wal/checkfile */
	char *record_file;
	char default_path[MAXPGPATH];

	if (!IS_SYSSSO(GetUserId()))
	{
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			errmsg("permission denied")));
	}

	if (!wal_integrity_enable)
	{
		ereport(ERROR,
			(errmsg("hg_wal_integrity.enable is off")));
	}

	if (PG_NARGS() == 0 || PG_ARGISNULL(0))
	{
		snprintf(default_path, sizeof(default_path), "%s/pg_wal/%s", DataDir, CHECKFILE);
		record_file = default_path;
	}
	else
	{
		text *record_file_text = PG_GETARG_TEXT_PP(0);
		record_file = text_to_cstring(record_file_text);
	}
	rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;

	/* ----- Result set construction ----- */
	if (!rsinfo || !IsA(rsinfo, ReturnSetInfo) ||
		(rsinfo->allowedModes & SFRM_Materialize) == 0)
	{
		ereport(ERROR,
			(errmsg("set-valued function called in context that cannot accept a set")));
	}
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);
	/* Define result set columns */
	tupdesc = CreateTemplateTupleDesc(CHECK_REL_NUM);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "wal_filename", TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "expected_checksum", TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "actual_checksum", TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "is_match", BOOLOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 5, "info", TEXTOID, -1, 0);
	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	/* ----- 1. Read record file (comma separated: filename, checksum) ----- */
	rfp = AllocateFile(record_file, "r");
	if (!rfp)
	{
		/* Cannot open record file, still proceed; all completed files will be marked "not found in record" */
		ereport(ERROR,
			(errmsg("Could not open WAL check record file, Please trigger wal archive.")));
	}
	else
	{
		char linebuf[MAXPGPATH];
		maxrecords = PG_HMAC_DIGEST_STRING_LENGTH;
		records = (ChecksumRecord *) palloc(maxrecords * sizeof(ChecksumRecord));

		/* ----- Record file cache: filename -> expected checksum ----- */
		while (fgets(linebuf, sizeof(linebuf), rfp))
		{
			char exp_fname[MAXPGPATH] = {0};
			char exp_checksum[PG_HMAC_DIGEST_STRING_LENGTH] = {0}; /* 32 hex + null */
			/* Expand record array */
			if (nrecords >= maxrecords)
			{
				maxrecords = maxrecords * 2;
				records = (ChecksumRecord *)repalloc(records, maxrecords * sizeof(ChecksumRecord));
			}

			if (sscanf(linebuf, "%[^,],%[^\n]", exp_fname, exp_checksum) != 2)
			{
				ereport(WARNING, (errmsg("walfile info err:[%s]", linebuf)));
				continue;
			}

			strlcpy(records[nrecords].fname, trim_whitespace(exp_fname), MAXPGPATH);
			strlcpy(records[nrecords].checksum, trim_whitespace(exp_checksum), PG_HMAC_DIGEST_STRING_LENGTH);
			nrecords++;
		}
		FreeFile(rfp);
	}

	/* ----- 2. Scan pg_wal file ----- */
	for (int i = 0; i < nrecords; i++)
	{
		Datum values[CHECK_REL_NUM];
		FileHmacSumStatus errnumber = HMAC_NULL;
		bool nulls[CHECK_REL_NUM] = {false, false, false, false, false};
		char *fname = records[i].fname;
		char act_checksum[PG_HMAC_DIGEST_STRING_LENGTH] = {0};
		bool match;
		bool result;

		/* ----- file get sm3 or sha256 or md5 ----- */
		if (strlen(records[i].checksum) == (PG_HMAC_DIGEST_STRING_LENGTH - 1))
			result = pg_file_hmac_checksum(fname, act_checksum, &errnumber);
		else
			result = pg_file_md5_checksum(fname, act_checksum, &errnumber);

		if (result)
		{
			/* Normal SUCCESSFULLY */
			match = (strcmp(act_checksum, records[i].checksum) == 0);
			values[0] = PointerGetDatum(cstring_to_text(fname));
			values[1] = PointerGetDatum(cstring_to_text(records[i].checksum));
			values[2] = PointerGetDatum(cstring_to_text(act_checksum));
			values[3] = BoolGetDatum(match);
			values[4] = match ? PointerGetDatum(cstring_to_text("Wal files integrity check SUCCESSFULLY")) :
						PointerGetDatum(cstring_to_text("Wal files integrity check FAILED"));

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}
		else
		{
			values[0] = PointerGetDatum(cstring_to_text(fname));
			values[1] = PointerGetDatum(cstring_to_text(records[i].checksum));
			values[2] = PointerGetDatum(cstring_to_text("NULL"));
			values[3] = BoolGetDatum(false);
			values[4] = PointerGetDatum(cstring_to_text( err_info[errnumber]));
			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}
	}

	if (records)
		pfree(records);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_VOID();
}

/* ----------------------------------------------------------------
 * xlog_checksum_w: set check wal filename
 * ----------------------------------------------------------------
 */
static bool
xlog_checksum_w(char * filename, char *archive_command)
{
	FILE *file;
	FileHmacSumStatus errnumber = HMAC_NULL;
	char checkSumValue[PG_HMAC_DIGEST_STRING_LENGTH] = {0};
	char checklog_path[MAXPGPATH] = {0};
	char checklog_text[MAXPGPATH] = {0};
	char archive_path[MAXPGPATH] = {0};
	char archive_log_path[MAXPGPATH] = {0};

	if (!wal_integrity_enable)
		return false;

	/* If it is the first file, end it early */
	if (filename == NULL)
		return false;

	/* get archive path */
	parse_archivelog_path(archive_path, archive_command);

	/* filename = archive path + wal filename */
	if (strlen(archive_path) + strlen(filename) > MAXPGPATH)
	{
		ereport(WARNING, (errmsg("wal path too long")));
		return false;
	}
	snprintf(archive_log_path, MAXPGPATH, "%s%s", archive_path, filename);
	if(!pg_file_hmac_checksum(archive_log_path, checkSumValue, &errnumber))
	{
		ereport(WARNING, (errmsg("wal file get checksum failed")));
		return false;
	}

	/* Generate checksum value and store it in the checklog.txt file */
	memset(checklog_path, 0, MAXPGPATH);
	snprintf(checklog_path, MAXPGPATH, XLOGDIR "/%s", CHECKFILE);

	if ((file = AllocateFile(checklog_path, "aw+")) == NULL)
	{
		ereport(WARNING, (errmsg("cannot open file %s", checklog_path)));
		return false;
	}

	while ((fgets(checklog_text, MAXPGPATH, file)) != NULL)
	{
		if (strncmp(checklog_text, archive_log_path, strlen(archive_log_path)) == 0)
		{
			/* CHECKFILE exist old wal filename info ,return false */
			memset(checklog_text, 0, MAXPGPATH);
			FreeFile(file);
			return false;
		}
		memset(checklog_text, 0, MAXPGPATH);
	}

	fprintf(file, "%s,%s\n", archive_log_path, checkSumValue);
	FreeFile(file);
	return true;
}
/* End - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */

/* ----------------------------------------------------------------
 * pg_wal_init_checklog: init checklog file 
 * ----------------------------------------------------------------
 */
Datum
pg_wal_init_checklog(PG_FUNCTION_ARGS)
{
	char checklog_path[MAXPGPATH] = {0};
	int fd = 0;

	if (!IS_SYSSSO(GetUserId()))
	{
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			errmsg("permission denied")));
	}

	snprintf(checklog_path, MAXPGPATH, XLOGDIR "/%s", CHECKFILE);

	fd = BasicOpenFile(checklog_path, O_RDONLY);
	if (fd >= 0)
	{
		close(fd);
		durable_unlink(checklog_path, LOG);
	}

	PG_RETURN_BOOL(1);
}

/* ----------------------------------------------------------------
 * bytesToHex: bytes to hex 
 * ----------------------------------------------------------------
 */
static void
bytesToHex(uint8 b[16], char *s)
{
	static const char *hex = "0123456789abcdef";
	int			q,
				w;

	for (q = 0, w = 0; q < 16; q++)
	{
		s[w++] = hex[(b[q] >> 4) & 0x0F];
		s[w++] = hex[b[q] & 0x0F];
	}
	s[w] = '\0';
}

/* ----------------------------------------------------------------
 * pg_file_md5_checksum: read the file and get MD5 checksum
 * ----------------------------------------------------------------
 */
bool
pg_file_md5_checksum(const char *filename, char *md5_checksum, FileHmacSumStatus *errnumber)
{
	char checkSumValue[MD5_DATALEN] = {0};
	uint8 buffer[XLOG_BLCKSZ] = {0};
	int bytes_read = 0;
	FILE *file;
	uint8 sum[MD5_DIGEST_LENGTH];
	pg_cryptohash_ctx *ctx;

	if ((file = AllocateFile(filename, "rb")) == NULL)
	{
		if (errno == ENOENT)
		{
			*errnumber = HMAC_FILE_NOT_EXIST; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
		}
		else
		{
			*errnumber = HMAC_FILE_NOT_READ; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
		}
		return false;
	}

	ctx = pg_cryptohash_create(PG_MD5);
	if (pg_cryptohash_init(ctx) < 0)
	{
		FreeFile(file);
		pg_cryptohash_free(ctx);
		*errnumber = HMAC_SUM_OTHER_FAILED; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
		return false;
	}

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
	{
		if (pg_cryptohash_update(ctx, buffer, bytes_read) < 0 )
		{
			FreeFile(file);
			pg_cryptohash_free(ctx);
			*errnumber = HMAC_SUM_OTHER_FAILED; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
			return false;
		}
	}

	if (pg_cryptohash_final(ctx, sum, sizeof(sum)) < 0)
	{
		FreeFile(file);
		pg_cryptohash_free(ctx);
		*errnumber = HMAC_SUM_OTHER_FAILED; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
		return false;
	}

	bytesToHex(sum, checkSumValue);
	strncpy(md5_checksum, checkSumValue, 32);

	FreeFile(file);
	pg_cryptohash_free(ctx);
	return true;
}

/* Begin - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
/* ----------------------------------------------------------------
 * pg_file_hmac_checksum: read the file and get sha256 or sm3 checksum
 * ----------------------------------------------------------------
 */
bool
pg_file_hmac_checksum(const char *filename, char *hmac_checksum, FileHmacSumStatus *errnumber)
{
	const unsigned char key[16] = {
		0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b,
		0x1c, 0x1d, 0x1e, 0x1f
	};
	const unsigned char salt[16] = {
		0x01, 0x02, 0x03, 0x04,
		0x05, 0x06, 0x07, 0x08,
		0x09, 0x0a, 0x0b, 0x0c,
		0x0d, 0x0e, 0x0f, 0x10
	};
	uint8		buffer[XLOG_BLCKSZ];
	uint8		sum[PG_HMAC_DIGEST_LENGTH];
	size_t		bytes_read;
	FILE	   *file;
	pg_hmac_ctx *ctx;

	file = AllocateFile(filename, "rb");
	if (file == NULL)
	{
		if (errno == ENOENT)
			*errnumber = HMAC_FILE_NOT_EXIST;
		else
			*errnumber = HMAC_FILE_NOT_READ;

		return false;
	}

#ifdef USE_GMSSL
	ctx = pg_hmac_create(PG_SM3);
#else
	ctx = pg_hmac_create(PG_SHA256);
#endif

	if (ctx == NULL)
	{
		FreeFile(file);
		*errnumber = HMAC_SUM_OTHER_FAILED;
		return false;
	}

	/*
	 * Compute HMAC-SHA256(key, salt || file_contents).
	 */
	if (pg_hmac_init(ctx, key, sizeof(key)) < 0 ||
		pg_hmac_update(ctx, salt, sizeof(salt)) < 0)
	{
		FreeFile(file);
		pg_hmac_free(ctx);
		*errnumber = HMAC_SUM_OTHER_FAILED;
		return false;
	}

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
	{
		if (pg_hmac_update(ctx, buffer, bytes_read) < 0)
		{
			FreeFile(file);
			pg_hmac_free(ctx);
			*errnumber = HMAC_SUM_OTHER_FAILED;
			return false;
		}
	}

	if (ferror(file))
	{
		FreeFile(file);
		pg_hmac_free(ctx);
		*errnumber = HMAC_FILE_NOT_READ;
		return false;
	}

	if (pg_hmac_final(ctx, sum, sizeof(sum)) < 0)
	{
		FreeFile(file);
		pg_hmac_free(ctx);
		*errnumber = HMAC_SUM_OTHER_FAILED;
		return false;
	}

	/*
	 * Convert the 32-byte digest to hexadecimal. hex_encode() does not append
	 * a null ternimator, so ternimate the 64-character result explicitly.
	 */
	hex_encode((const char *)sum, sizeof(sum), hmac_checksum);
	hmac_checksum[PG_HMAC_DIGEST_STRING_LENGTH - 1] = '\0';

	FreeFile(file);
	pg_hmac_free(ctx);

	return true;
}
/* End - ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
/* End - ReqID:DKY-INTEGRITY-CHECKLOG */

static bool
set_wal_integrity_check(bool *newval, void **extra, GucSource source)
{
	if ( *newval == true &&
		(!XLogArchiveCommandSet() || XLogArchiveMode == ARCHIVE_MODE_OFF))
	{
		ereport(WARNING,
			(errcode(ERRCODE_CONFIG_FILE_ERROR),
				errmsg("Parameter `archive_mode` and `archive_command` are not enabled"),
				errhint("Please set the guc parameters of `archive_mode` and `archive_command`!")));
	}
	return true;
}

void
_PG_init(void)
{
	DefineCustomBoolVariable("hg_wal_integrity.enable",
							"turn on or off check wal integrity.",
							NULL,
							&wal_integrity_enable,
							false,
							PGC_SIGHUP,
							0,
							set_wal_integrity_check,
							NULL,
							NULL);

	WalFileUpdateChecksum = xlog_checksum_w; /* ReqID:SRS-SECURITY-AUTHENTICATION-METHOD */
}

void
_PG_fini(void)
{

}
/* End - ReqID: SRS-SECURITY-EAL4-WAL_MD5 */

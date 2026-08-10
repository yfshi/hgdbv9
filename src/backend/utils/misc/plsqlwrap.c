/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
/*-------------------------------------------------------------------------
 *
 * plsqlwrap.c
 *	 PL/SQL source wrapping support.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "common/plsqlwrap_crypto.h"
#include "utils/guc.h"
#include "utils/plsqlwrap.h"

/* Convert the ivorysql.wrap_algo GUC value to the shared cipher type. */
static PgPlsqlWrapCipher
ivy_plsql_wrap_cipher_type(void)
{
	return oracle_wrap_algorithm == ORACLE_WRAP_SM4 ?
		PG_PLSQL_WRAP_CIPHER_SM4 : PG_PLSQL_WRAP_CIPHER_AES;
}

/* Remove the SQL literal quotes used by externally generated wrapped DDL. */
static char *
ivy_plsql_normalize_wrapped_source(const char *src)
{
	const char *wrapped = src;
	const char *payload;
	const char *payload_end;
	Size		magic_len = strlen(PG_PLSQL_WRAP_MAGIC);
	Size		payload_len;
	char	   *normalized;

	/* Locate the marker without altering whitespace in plaintext input. */
	while (isspace((unsigned char) *wrapped))
		wrapped++;

	/* The parser preserves everything after the canonical magic header. */
	payload = wrapped + magic_len;
	payload_end = src + strlen(src);
	/* Accept both interactive input and generated DDL with a trailing ';'. */
	while (payload_end > payload && isspace((unsigned char) payload_end[-1]))
		payload_end--;
	if (payload_end > payload && payload_end[-1] == ';')
		payload_end--;
	while (payload_end > payload && isspace((unsigned char) payload_end[-1]))
		payload_end--;
	/* Legacy unquoted payloads are already in the internal storage format. */
	if (*payload != '\'' || payload_end <= payload + 1 || payload_end[-1] != '\'')
		return pstrdup(src);

	/* Store magic + algorithm prefix + base64 without SQL literal quotes. */
	payload++;
	payload_len = payload_end - payload - 1;
	normalized = (char *) palloc(magic_len + payload_len + 1);
	memcpy(normalized, PG_PLSQL_WRAP_MAGIC, magic_len);
	memcpy(normalized + magic_len, payload, payload_len);
	normalized[magic_len + payload_len] = '\0';
	return normalized;
}

/* Wrap a PL/SQL source body through the shared frontend/backend crypto code. */
char *
ivy_plsql_wrap_source(const char *src)
{
	const char *error_message;
	char	   *wrapped;

	if (src == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("PL/SQL source must not be null")));

	/* Normalize externally quoted payloads instead of encrypting them again. */
	if (ivy_plsql_source_is_wrapped(src))
		return ivy_plsql_normalize_wrapped_source(src);

	wrapped = pg_plsql_wrap_encrypt(src, ivy_plsql_wrap_cipher_type(), &error_message);
	if (wrapped == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("%s", error_message)));

	return wrapped;
}

/* Unwrap a PL/SQL source body through the shared frontend/backend crypto code. */
char *
ivy_plsql_unwrap_source(const char *src)
{
	const char *payload;
	const char *error_message;
	Size		payload_len;
	PgPlsqlWrapCipher cipher_type;
	char	   *unwrapped;

	if (src == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("PL/SQL source must not be null")));

	if (!ivy_plsql_source_is_wrapped(src))
		return pstrdup(src);

	while (isspace((unsigned char) *src))
		src++;
	payload = src + strlen(PG_PLSQL_WRAP_MAGIC);
	payload_len = strlen(payload);

	/* Strip SQL statement punctuation left by Oracle-style input files. */
	while (payload_len > 0 && isspace((unsigned char) payload[payload_len - 1]))
		payload_len--;
	if (payload_len > 0 && payload[payload_len - 1] == ';')
		payload_len--;
	while (payload_len > 0 && isspace((unsigned char)payload[payload_len - 1]))
		payload_len--;

	if (payload_len >= strlen(PG_PLSQL_WRAP_AES_PREFIX) &&
		strncmp(payload, PG_PLSQL_WRAP_AES_PREFIX, strlen(PG_PLSQL_WRAP_AES_PREFIX)) == 0)
	{
		cipher_type = PG_PLSQL_WRAP_CIPHER_AES;
	}
	else if (payload_len >= strlen(PG_PLSQL_WRAP_SM4_PREFIX) &&
		strncmp(payload, PG_PLSQL_WRAP_SM4_PREFIX, strlen(PG_PLSQL_WRAP_SM4_PREFIX)) == 0)
	{
		cipher_type = PG_PLSQL_WRAP_CIPHER_SM4;
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid wrapped PL/SQL source prefix")));

	unwrapped = pg_plsql_wrap_decrypt(payload, payload_len, cipher_type, &error_message);
	if (unwrapped == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("%s", error_message)));

	return unwrapped;
}

/* Find the first source-body token after the outer CREATE statement's IS/AS. */
static const char *
ivy_plsql_find_body_start(const char *ddl)
{
	const char *p = ddl;

	while (*p != '\0')
	{
		while (*p != '\0' && isspace((unsigned char) *p))
			p++;

		if ((pg_strncasecmp(p, "is", 2) == 0 && !isalnum((unsigned char) p[2]) && p[2] != '_') ||
			(pg_strncasecmp(p, "as", 2) == 0 && !isalnum((unsigned char) p[2]) && p[2] != '_'))
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

/* Replace a full PL/SQL DDL body with an Oracle-style WRAPPED payload. 
 *
 * create function fc return number is begin 1; end;
 *                                  |  |           |
 *                                  |  body        |
 *                                  |              body_end
 *                                  header_end
 */
char *
ivy_plsql_wrap_ddl(const char *ddl)
{
	const char *body;
	const char *body_end;
	const char *header_end;
	char	   *body_source;
	char	   *wrapped_body;
	char	   *quoted_body;
	char	   *result;
	Size		head_len;
	Size		body_len;

	if (strstr(ddl, PG_PLSQL_WRAP_MAGIC) != NULL)
		return pstrdup(ddl);

	body = ivy_plsql_find_body_start(ddl);
	if (body == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("could not locate PL/SQL source body to wrap")));

	/* move back over whitespace and is/as */
	header_end = body;
	while (header_end > ddl && isspace((unsigned char) header_end[-1]))
		header_end--;
	if (header_end - ddl < 2 ||
		(pg_strncasecmp(header_end - 2, "is", 2) != 0 &&
		 pg_strncasecmp(header_end - 2, "as", 2) != 0))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("could not locate PL/SQL source clause to wrap")));
	header_end -= 2;

	/* exclude sql statement and whitespace */
	body_end = ddl + strlen(ddl);
	while (body_end > body && isspace((unsigned char) body_end[-1]))
		body_end--;
	if (body_end > body && body_end[-1] == ';')
		body_end--;
	while (body_end > body && isspace((unsigned char) body_end[-1]))
		body_end--;

	/* copy the plaintext */
	body_len = body_end - body;
	body_source = (char *) palloc(body_len + 1);
	memcpy(body_source, body, body_len);
	body_source[body_len] = '\0';

	wrapped_body = ivy_plsql_wrap_source(body_source);
	/* Quote the opaque payload before embedding it in executable SQL text. */
	quoted_body = pg_plsql_wrap_quote_payload(wrapped_body);
	if (quoted_body == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("invalid wrapped PL/SQL source")));
	head_len = header_end - ddl;
	result = (char *) palloc(head_len + strlen("WRAPPED\n") + strlen(quoted_body) + 2);
	memcpy(result, ddl, head_len);
	strcpy(result + head_len, "WRAPPED\n");
	strcat(result, quoted_body);
	strcat(result, ";");
	pfree(body_source);
	pfree(wrapped_body);
	pfree(quoted_body);

	return result;
}
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/

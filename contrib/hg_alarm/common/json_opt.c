#include "hg_alarm.h"

#include "common/jsonapi.h"
#include "mb/pg_wchar.h"
#include "common/logging.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef enum
{
	hga_jps_start,
	hga_jps_toplevel,
    hga_jps_host,
    hga_jps_port,
	hga_jps_database,
	hga_jps_username,
	hga_jps_password,
    hga_jps_interval,
    hga_jps_log_directory,
    hga_jps_log_segsize,
    hga_jps_mail_user,
    hga_jps_memory_limit,
    hga_jps_datadir_limit,

	hga_jps_database_limit,
	hga_jps_database_limit_dbname,
	hga_jps_database_limit_limit,

    hga_jps_tablespace_limit,
	hga_jps_tablespace_limit_tsname,
	hga_jps_tablespace_limit_limit,

	hga_jps_relation_limit,
	hga_jps_relation_limit_dbname,
	hga_jps_relation_limit_schema,
	hga_jps_relation_limit_relname,
	hga_jps_relation_limit_limit,

	hga_jps_end
}hga_jps_type;

typedef struct hga_jps
{
	hga_jps_type	state;

    /* Raw string token values */
    char        *host;
    char        *port;
	char		*database;
	char		*username;
	char		*password;

    char        *interval;
    char        *log_directory;
    char        *log_segsize;
    char        *mail_user;

    char        *memory_limit;
    char        *datadir_limit;

	PGObject	*database_list;
	PGObject	*tablespace_list;
    PGObject    *relation_list;
} hga_jps;


/* 
 * Only 4 kinds of objects now
 *  1. the root object
 *  2. database list
 *  3. tablespace list
 *  4. relation list
 */
static void
hga_json_object_start(void *state)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
		case hga_jps_start:
			ps->state = hga_jps_toplevel;
			break;

		case hga_jps_database_limit:
        case hga_jps_tablespace_limit:
		case hga_jps_relation_limit:
			break;
		
		default:
			hga_log_fatal("unknown json object type: %d", ps->state);
	}
    
	hga_log_debug("object start");
}

static void
hga_json_object_end(void *state)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
		case hga_jps_toplevel:
			ps->state = hga_jps_end;
			break;
		case hga_jps_database_limit:
        case hga_jps_tablespace_limit:
		case hga_jps_relation_limit:
			break;
		default:
			hga_log_fatal("unknown json object type: %d", ps->state);
	}
	hga_log_debug("object end");
}

static void
hga_json_object_field_start(void *state, char *fname, bool isnull)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
		case hga_jps_toplevel:
            if (pg_strcasecmp(fname, "host") == 0)
				ps->state = hga_jps_host;
            else if (pg_strcasecmp(fname, "port") == 0)
				ps->state = hga_jps_port;
			else if (pg_strcasecmp(fname, "database") == 0)
				ps->state = hga_jps_database;
			else if (pg_strcasecmp(fname, "username") == 0)
				ps->state = hga_jps_username;
			else if (pg_strcasecmp(fname, "password") == 0)
				ps->state = hga_jps_password;
            else if (pg_strcasecmp(fname, "interval") == 0)
                ps->state = hga_jps_interval;
            else if (pg_strcasecmp(fname, "log_directory") == 0)
                ps->state = hga_jps_log_directory;
            else if (pg_strcasecmp(fname, "log_segsize") == 0)
                ps->state = hga_jps_log_segsize;
            else if (pg_strcasecmp(fname, "mail_user") == 0)
                ps->state = hga_jps_mail_user;
            else if (pg_strcasecmp(fname, "memory_limit") == 0)
				ps->state = hga_jps_memory_limit;
            else if (pg_strcasecmp(fname, "datadir_limit") == 0)
				ps->state = hga_jps_datadir_limit;
			else if (pg_strcasecmp(fname, "database_limit") == 0)
				ps->state = hga_jps_database_limit;
            else if (pg_strcasecmp(fname, "tablespace_limit") == 0)
				ps->state = hga_jps_tablespace_limit;
			else if (pg_strcasecmp(fname, "relation_limit") == 0)
				ps->state = hga_jps_relation_limit;
            else goto unknown_config;
			break;
		case hga_jps_database_limit:
			if (pg_strcasecmp(fname, "dbname") == 0)
				ps->state = hga_jps_database_limit_dbname;
			else if (pg_strcasecmp(fname, "limit") == 0)
				ps->state = hga_jps_database_limit_limit;
            else goto unknown_config;
			break;
        case hga_jps_tablespace_limit:
			if (pg_strcasecmp(fname, "tsname") == 0)
				ps->state = hga_jps_tablespace_limit_tsname;
			else if (pg_strcasecmp(fname, "limit") == 0)
				ps->state = hga_jps_tablespace_limit_limit;
            else goto unknown_config;
			break;
		case hga_jps_relation_limit:
			if (pg_strcasecmp(fname, "dbname") == 0)
				ps->state = hga_jps_relation_limit_dbname;
			else if (pg_strcasecmp(fname, "schema") == 0)
				ps->state = hga_jps_relation_limit_schema;
			else if (pg_strcasecmp(fname, "relname") == 0)
				ps->state = hga_jps_relation_limit_relname;
			else if (pg_strcasecmp(fname, "limit") == 0)
				ps->state = hga_jps_relation_limit_limit;
            else goto unknown_config;
			break;
		default:
			break;
	}
	hga_log_debug("object field start : %s", fname);
    return;

unknown_config:
    hga_log_fatal("unknown json field: \"%s\"", fname);
}

static void
hga_json_object_field_end(void *state, char *fname, bool isnull)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
        case hga_jps_host:
        case hga_jps_port:
		case hga_jps_database:
		case hga_jps_username:
		case hga_jps_password:
        case hga_jps_interval:
        case hga_jps_log_directory:
        case hga_jps_log_segsize:
        case hga_jps_mail_user:
        case hga_jps_memory_limit:
        case hga_jps_datadir_limit:
		case hga_jps_database_limit:
        case hga_jps_tablespace_limit:
		case hga_jps_relation_limit:
			ps->state = hga_jps_toplevel;
			break;
		case hga_jps_database_limit_dbname:
		case hga_jps_database_limit_limit:
			ps->state = hga_jps_database_limit;
			break;
        case hga_jps_tablespace_limit_tsname:
		case hga_jps_tablespace_limit_limit:
			ps->state = hga_jps_tablespace_limit;
			break;
		case hga_jps_relation_limit_dbname:
		case hga_jps_relation_limit_schema:
		case hga_jps_relation_limit_relname:
		case hga_jps_relation_limit_limit:
			ps->state = hga_jps_relation_limit;
			break;
		default:
			hga_log_info("unknown config: \"%s\"", fname);
            hga_log_fatal("failed hga_json_object_field_start");
			break;
	}
	hga_log_debug("object field end : %s", fname);
}	

static void
hga_json_array_start(void *state)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
		case hga_jps_database_limit:
        case hga_jps_tablespace_limit:
		case hga_jps_relation_limit:
			break;
		default:
			hga_log_fatal("unknown json array type: %d", ps->state);
			break;
	}
    hga_log_debug("array start");
}

static void
hga_json_array_end(void *state)
{
	hga_jps		*ps = (hga_jps *) state;

	switch (ps->state)
	{
		case hga_jps_database_limit:
        case hga_jps_tablespace_limit:
		case hga_jps_relation_limit:
			break;
		default:
			hga_log_fatal("unknown json array type: %d", ps->state);
			break;
	}
    hga_log_debug("array end");
}

static void
hga_json_array_element_start(void *state, bool isnull)
{
	hga_jps		*ps = (hga_jps *) state;
    PGObject	*new;

    /* Skip null array element */
    if (isnull)
        return;

    new = palloc0(sizeof(PGObject));

	switch (ps->state)
	{
		case hga_jps_database_limit:
            new->type = PGOBJ_DATABASE;
            new->next = ps->database_list;
            ps->database_list = new;
			break;
        case hga_jps_tablespace_limit:
            new->type = PGOBJ_TABLESPACE;
            new->next = ps->tablespace_list;
            ps->tablespace_list = new;
			break;
		case hga_jps_relation_limit:
            new->type = PGOBJ_RELATION;
            new->next = ps->relation_list;
            ps->relation_list = new;
			break;
		default:
			hga_log_fatal("unknown json array element type: %d", ps->state);
			break;
	}
    hga_log_debug("array element start");
}

static void
hga_json_array_element_end(void *state, bool isnull)
{
	hga_log_debug("array element end");
}

static void
hga_json_scalar(void *state, char *token, JsonTokenType tokentype)
{
	hga_jps		*ps = (hga_jps *) state;

    #define TOKEN_SET(ptr, token, type) do { \
        if (type == JSON_TOKEN_NULL \
            || strlen(token) == 0) \
            ptr = NULL; \
        else \
            ptr = token; \
    } while(0)

	switch (ps->state)
	{
        case hga_jps_host:
            TOKEN_SET(ps->host, token, tokentype);
            break;
        case hga_jps_port:
            TOKEN_SET(ps->port, token, tokentype);
            break;
		case hga_jps_database:
            TOKEN_SET(ps->database, token, tokentype);
			break;
		case hga_jps_username:
            TOKEN_SET(ps->username, token, tokentype);
			break;
		case hga_jps_password:
            TOKEN_SET(ps->password, token, tokentype);
			break;
        case hga_jps_interval:
            TOKEN_SET(ps->interval, token, tokentype);
            break;
        case hga_jps_log_directory:
            TOKEN_SET(ps->log_directory, token, tokentype);
            break;
        case hga_jps_log_segsize:
            TOKEN_SET(ps->log_segsize, token, tokentype);
            break;
        case hga_jps_mail_user:
            TOKEN_SET(ps->mail_user, token, tokentype);
            break;

        case hga_jps_memory_limit:
            TOKEN_SET(ps->memory_limit, token, tokentype);
            break;
        case hga_jps_datadir_limit:
            TOKEN_SET(ps->datadir_limit, token, tokentype);
            break;

        /* database */
		case hga_jps_database_limit_dbname:
            TOKEN_SET(ps->database_list->object.db.name, token, tokentype);
			break;
		case hga_jps_database_limit_limit:
            TOKEN_SET(ps->database_list->limit, token, tokentype);
			break;

        /* tablespace */
        case hga_jps_tablespace_limit_tsname:
			TOKEN_SET(ps->tablespace_list->object.ts.name, token, tokentype);
			break;
		case hga_jps_tablespace_limit_limit:
			TOKEN_SET(ps->tablespace_list->limit, token, tokentype);
			break;

        /* relation */
		case hga_jps_relation_limit_dbname:
			TOKEN_SET(ps->relation_list->object.rel.dbname, token, tokentype);
			break;
		case hga_jps_relation_limit_schema:
			TOKEN_SET(ps->relation_list->object.rel.schema, token, tokentype);
			break;
		case hga_jps_relation_limit_relname:
			TOKEN_SET(ps->relation_list->object.rel.relname, token, tokentype);
            break;
		case hga_jps_relation_limit_limit:
			TOKEN_SET(ps->relation_list->limit, token, tokentype);
			break;
		default:
			hga_log_fatal("unknown token: %s", token);
	}
    hga_log_debug("scalar : %s", token);
}

bool
parse_json_option(char *jsonfile, GUC *guc)
{
    int		 fd;
    int		 rc;
    struct stat statbuf;
    char	*buffer;
    JsonLexContext *lex;
    JsonSemAction 	sem;
    hga_jps	        *ps;
    JsonParseErrorType json_error;

    if ((fd = open(jsonfile, O_RDONLY | PG_BINARY, 0)) < 0)
    {
        hga_log_fatal("open file failed: \"%s\": %m", jsonfile);
        return false;
    }

    if (fstat(fd, &statbuf) != 0)
    {
        hga_log_fatal("could not stat file \"%s\": %m", jsonfile);
        return false;
    }

    /*
     * Slurp in the whole file.
     *
     * This is not ideal, but there's currently no easy way to get
     * pg_parse_json() to perform incremental parsing.
     */
    buffer = palloc(statbuf.st_size);
    rc = read(fd, buffer, statbuf.st_size);
    if (rc != statbuf.st_size)
    {
        if (rc < 0)
            hga_log_fatal("could not read file \"%s\": %m",
                            jsonfile);
        else
            hga_log_fatal("could not read file \"%s\": read %d of %lld",
                            jsonfile, rc, (long long int) statbuf.st_size);
        return false;
    }

    /* Close the manifest file. */
    close(fd);
    
    lex = makeJsonLexContextCstringLen(buffer, rc, PG_UTF8, true);
    ps = palloc0(sizeof(hga_jps));
    ps->state = hga_jps_start;

    sem.object_start = hga_json_object_start;
    sem.object_end = hga_json_object_end;
    sem.array_start = hga_json_array_start;
    sem.array_end = hga_json_array_end;
    sem.object_field_start = hga_json_object_field_start;
    sem.object_field_end = hga_json_object_field_end;
    sem.array_element_start = hga_json_array_element_start;
    sem.array_element_end = hga_json_array_element_end;
    sem.scalar = hga_json_scalar;
    sem.semstate = (void *) ps;

    json_error = pg_parse_json(lex, &sem);

    if (json_error != JSON_SUCCESS)
    {
        hga_log_fatal("configuration file \"%s\" parsing failed.",
                      jsonfile);
        return false;
    }

    /* Validate integer/string option values */
    do
    {
        int32       val;
        const  char *hintmsg;

        if (ps->host)
            guc->guc_host = ps->host;
        if (ps->port)
            guc->guc_port = ps->port;
        if (ps->database)
            guc->guc_database = ps->database;
        if (ps->username)
            guc->guc_username = ps->username;
        if (ps->password)
            guc->guc_password = ps->password;
        if (ps->log_directory)
            guc->guc_log_directory = ps->log_directory;
        if (ps->log_segsize)
            guc->guc_log_segsize_raw = ps->log_segsize;
        if (ps->mail_user)
            guc->guc_email_user = ps->mail_user;
        if (ps->interval)
            guc->guc_interval_raw = ps->interval;
        /* Valid params */
        if (ps->memory_limit)
        {
            if (!parse_int(ps->memory_limit, &val, GUC_UNIT_MB, &hintmsg))
            {
                if (hintmsg)
                    hga_log_info("%s", hintmsg);
                hga_log_fatal("Invalid value: %s", ps->memory_limit);
            }
            if (val < 0)
                val = UNLIMITED_INT;
            guc->guc_memory_limit = val;
        }
        else guc->guc_memory_limit = UNLIMITED_INT;

        if (ps->datadir_limit)
        {
            if (!parse_int(ps->datadir_limit, &val, GUC_UNIT_MB, &hintmsg))
            {
                if (hintmsg)
                    hga_log_info("%s", hintmsg);
                hga_log_fatal("Invalid value: %s", ps->datadir_limit);
            }
            if (val < 0)
                val = UNLIMITED_INT;
            guc->guc_datadir_limit = val;
        }
        else guc->guc_datadir_limit = UNLIMITED_INT;

        if (ps->database_list)
        {
            PGObject    *po = ps->database_list;

            while (po)
            {
                if (!po->object.db.name)
                    hga_log_fatal("Invalid conf file format: 'dbname' missed for a 'database_limit' object");

                if (po->limit)
                {
                    if (!parse_int(po->limit, &val, GUC_UNIT_MB, &hintmsg))
                    {
                        if (hintmsg)
                            hga_log_info("%s", hintmsg);
                        hga_log_fatal("Invalid value: %s", po->limit);
                    }

                    if (val < 0)
                        val = UNLIMITED_INT;
                    po->object.db.limit = val;
                }
                else po->object.db.limit = UNLIMITED_INT;
                    
                po = po->next;
            }

            guc->guc_database_list = ps->database_list;
        }
        if (ps->tablespace_list)
        {
            PGObject    *po = ps->tablespace_list;

            while (po)
            {
                if (!po->object.ts.name)
                    hga_log_fatal("Invalid conf file format: 'tsname' missed for a 'tablespace_limit' object");

                if (po->limit)
                {
                    if (!parse_int(po->limit, &val, GUC_UNIT_MB, &hintmsg))
                    {
                        if (hintmsg)
                            hga_log_info("%s", hintmsg);
                        hga_log_fatal("Invalid value: %s", po->limit);
                    }

                    if (val < 0)
                        val = UNLIMITED_INT;
                    po->object.ts.limit = val;
                }
                else po->object.ts.limit = UNLIMITED_INT;
                
                po = po->next;
            }

            guc->guc_tablespace_list = ps->tablespace_list;
        }
        if (ps->relation_list)
        {
            PGObject    *po = ps->relation_list;

            while (po)
            {
                if (!po->object.rel.dbname)
                    hga_log_fatal("Invalid conf file format: 'dbname' missed for a 'relation_limit' object");
                if (!po->object.rel.schema)
                    hga_log_fatal("Invalid conf file format: 'schema' missed for a 'relation_limit' object");
                if (!po->object.rel.relname)
                    hga_log_fatal("Invalid conf file format: 'relname' missed for a 'relation_limit' object");

                if (po->limit)
                {
                    if (!parse_int(po->limit, &val, GUC_UNIT_MB, &hintmsg))
                    {
                        if (hintmsg)
                            hga_log_info("%s", hintmsg);
                        hga_log_fatal("Invalid value: %s", po->limit);
                    }

                    if (val < 0)
                        val = UNLIMITED_INT;
                    po->object.rel.limit = val;
                }
                else po->object.rel.limit = UNLIMITED_INT;

                po = po->next;
            }

            guc->guc_relation_list = ps->relation_list;
        }
    } while (0);

    return true;
}


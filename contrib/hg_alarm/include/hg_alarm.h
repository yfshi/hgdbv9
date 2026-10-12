#ifndef hga_FE_H
#define hga_FE_H
#ifdef FRONTED
#include "postgres_fe.h"
#else
#include "postgres.h"
#endif
#include "hga_log.h"
#include "libpq-fe.h"

extern const char   *hga_progname;

#define PROCFS "/proc"

#define MAXSQLLEN       1024
#define MAXINTLEN       12
#define MAXINT8LEN      20

#define UNLIMITED_INT   (-1)

/* json_opt.c */
typedef enum
{
    PGOBJ_DATABASE,
    PGOBJ_TABLESPACE,
    PGOBJ_RELATION
} PGOjbectType;

/* Parser related */
typedef struct PGObject
{
    PGOjbectType      type;
    
    char       *limit;
	
    union object
    {
        struct db
        {
            char    *name;
            int32    limit;
        }       db;

        struct ts
        {
            char    *name;
            int32    limit;
        }       ts;

        struct rel
        {
            char    *dbname;
            char    *schema;
            char    *relname;
            int32    limit;
        }       rel;
    }   object;

    struct PGObject	*next;
    
} PGObject;

/* global josn options */
typedef struct GUC
{
    int32    guc_interval;
    int32    guc_log_segsize;
    char    *guc_interval_raw;
    char    *guc_log_segsize_raw;
    char    *guc_host;
    char    *guc_port;
    char    *guc_log_directory;
    char    *guc_email_user;
    char    *guc_database;
    char    *guc_username;
    char    *guc_password;
    int32    guc_memory_limit;
    int32    guc_datadir_limit;
    PGObject    *guc_database_list;
    PGObject    *guc_tablespace_list;
    PGObject    *guc_relation_list;
} GUC;

extern GUC      GlobalUserConfig;

extern bool parse_json_option(char *jsonfile, GUC *guc);

/* parse_int.c */
#define GUC_UNIT_KB				0x1000	/* value is in kilobytes */
#define GUC_UNIT_BLOCKS			0x2000	/* value is in blocks */
#define GUC_UNIT_XBLOCKS		0x3000	/* value is in xlog blocks */
#define GUC_UNIT_MB				0x4000	/* value is in megabytes */
#define GUC_UNIT_BYTE			0x8000	/* value is in bytes */
#define GUC_UNIT_MEMORY			0xF000	/* mask for size-related units */

#define GUC_UNIT_MS			   0x10000	/* value is in milliseconds */
#define GUC_UNIT_S			   0x20000	/* value is in seconds */
#define GUC_UNIT_MIN		   0x30000	/* value is in minutes */
#define GUC_UNIT_TIME		   0xF0000	/* mask for time-related units */

#define GUC_UNIT				(GUC_UNIT_MEMORY | GUC_UNIT_TIME)

extern bool parse_int(const char *value, int *result, int flags, const char **hintmsg);

/* file.c */
extern FILE *hga_fopen(const char *path, const char *omode);


/* memory.c */
extern int64 get_memory_size(PGconn *conn);
extern bool do_memory_check(PGconn *conn, int32 threshold);

/* datadir.c */
extern int64 get_datadir_size(PGconn *conn);
extern bool do_datadir_check(PGconn *conn, int32 threshold);

/* database.c */
extern int64 get_db_or_ts_size(PGconn *conn, char *objname, int objtype);
extern bool do_db_or_ts_check(PGconn *conn, PGOjbectType objtype, PGObject *objlist, char *objname);

/* relation.c */
extern int64 get_rel_size_and_kind(PGconn *conn, char *dbname, char *nspname, char *relname, 
					  char *relkind);
extern bool do_rel_check(PGconn *conn, PGObject *rellist, char *relname);

/* pqfe.c */
enum trivalue
{
	TRI_DEFAULT,
	TRI_NO,
	TRI_YES
};

/* Parameters needed by connectDatabase/connectMaintenanceDatabase */
typedef struct _connParams
{
	/* These fields record the actual command line parameters */
	const char *dbname;			/* this may be a connstring! */
	const char *pghost;
	const char *pgport;
	const char *pguser;
    char *password;
	enum trivalue prompt_password;
	/* If not NULL, this overrides the dbname obtained from command line */
	/* (but *only* the DB name, not anything else in the connstring) */
	const char *override_dbname;
} ConnParams;

extern PGconn *connectDatabase(ConnParams *cparams, const char *progname,
				bool echo, bool fail_ok, bool save_password, GUC *guc);
extern void disconnectDatabase(PGconn *conn);
extern void apply_default_connection_params(GUC *guc);
extern void valid_connection_param(GUC *guc);
#endif



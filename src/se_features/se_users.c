#include "postgres.h"

#include "executor/spi.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "utils/acl.h"
#include "utils/builtins.h"


#include "utils/ora_compatible.h"

#include "se_features_interface.h"

#define MAX_INIT_SE_MEMBERS 2

typedef struct
{
    char   *role_name;
    char   *attributes;
    bool    is_role;
    char   *members[MAX_INIT_SE_MEMBERS];// just hard-coded
}SE_roles;

static void insert_serole(SE_roles* se_role, Oid dboid);
static void change_dboid_and_name(SE_roles* se_role, Oid dboid, Oid templateid);


void create_db_seroles(Oid dboid, Oid templateid)
{
    SE_roles   *se_role;
    bool        old_val;
    /*be careful the order make sure roles created after users*/
    SE_roles    all_roles[] = {
        {"sys","SUPERUSER CREATEROLE",false,{NULL}},
        {"system","SUPERUSER CREATEROLE",false,{NULL}},
        {"syssso","",false,{NULL}},
        {"syssao","",false,{NULL}},
        {SYSDBA_ROLE,NULL,true,{"sys","system"}},
        {SYSSSO_ROLE,NULL,true,{"syssso",NULL}},
        {SYSSAO_ROLE,NULL,true,{"syssao",NULL}},
        {NULL,NULL,false,{NULL}}
    };

    old_val = se_feature.mode;
    se_feature.mode = false;

    for (se_role = &all_roles[0]; se_role->role_name != NULL; se_role++)
    {
        insert_serole(se_role, dboid);
    }
    /*make sure this is after seroles was created */
    for (se_role = &all_roles[0]; se_role->role_name != NULL; se_role++)
    {
        change_dboid_and_name(se_role, dboid, templateid);
    }
    se_feature.mode = old_val;
}

/*
 * drop all the seroles that belongs to dropping database
 */
void drop_db_seroles(Oid dboid)
{
    StringInfoData  querybuf;

    /*Since only instance-level user can drop db, make an assertion here*/
	Assert(compatible_mode == PG_MODE);

    initStringInfo(&querybuf);

    /*Open SPI context. */
    if (SPI_connect() != SPI_OK_CONNECT)
        elog(ERROR, "SPI_connect failed");

    /*Make a temp name so avoid to violate constraint. */
    appendStringInfo(&querybuf, "delete from pg_authid where rolowndboid = %d", dboid);

    /*Create a new serole with temp name under current db. */
    if (SPI_exec(querybuf.data, 0) != SPI_OK_DELETE)
        elog(ERROR, "SPI_exec failed: %s", querybuf.data);

        /*Close SPI context. */
    if (SPI_finish() != SPI_OK_FINISH)
        elog(ERROR, "SPI_finish failed");

    pfree(querybuf.data);
}

/*
 * insert a new database-level serole to the specified database after 
 * it created
 */
static void
insert_serole(SE_roles* se_role, Oid dboid)
{
    StringInfoData  querybuf;
    char           *rolname = se_role->role_name;

    /*Since only instance-level user can create db, make an assertion here*/
	Assert(compatible_mode == PG_MODE);

    initStringInfo(&querybuf);

    /*Open SPI context. */
    if (SPI_connect() != SPI_OK_CONNECT)
        elog(ERROR, "SPI_connect failed");
    
    /*Choose build role or user. */
    if (se_role->is_role) {
        appendStringInfo(&querybuf, "CREATE ROLE _%d_%s", dboid, rolname);
    }
    else {
        appendStringInfo(&querybuf, "CREATE USER _%d_%s", dboid, rolname);
    }
    /*Add attributes. be careful, keep the whitespace ahead of 'WITH',
     *or it will cause issue */
    if (se_role->attributes != NULL) {
        appendStringInfo(&querybuf, " WITH %s PASSWORD \'Highgo@123\'", se_role->attributes);
    }
    /*Add user as the admin of new role*/
    if (se_role->is_role) 
    {
        int     i;
        char   *member;
        bool    is_first = true;
        appendStringInfo(&querybuf, " ADMIN ");

        for (i = 0; i < MAX_INIT_SE_MEMBERS ; i++)
        {
            member = se_role->members[i];
            if (!member)
                break;
            
            if (is_first)
            {
                appendStringInfo(&querybuf, "_%d_%s ", dboid, member);
                is_first = false;
            }
            else
            {
                appendStringInfo(&querybuf, ",_%d_%s ", dboid, member);
            }
        }

    }

    /*Create a new serole with temp name under current db. */
    if (SPI_exec(querybuf.data, 0) != SPI_OK_UTILITY)
        elog(ERROR, "SPI_exec failed: %s", querybuf.data);

    /*Close SPI context. */
    if (SPI_finish() != SPI_OK_FINISH)
        elog(ERROR, "SPI_finish failed");

    pfree(querybuf.data);
}

static 
void change_dboid_and_name(SE_roles* se_role, Oid dboid, Oid templateid)
{
    char           *role_oid;
    StringInfoData  querybuf;
    StringInfoData  tempname;
    StringInfoData  targetname;
    char           *rolname = se_role->role_name;

    initStringInfo(&querybuf);
    initStringInfo(&tempname);
    initStringInfo(&targetname);

    /*Open SPI context. */
    if (SPI_connect() != SPI_OK_CONNECT)
        elog(ERROR, "SPI_connect failed");

    /*Build name */
    appendStringInfo(&tempname, "_%d_%s", dboid, rolname);
    appendStringInfo(&targetname, "%s", rolname);

    /*Change newrole dboid and rolname then get newrole oid */
    appendStringInfo(&querybuf,
                     "UPDATE pg_authid SET rolowndboid = %d, rolname = \'%s\' "
                     "WHERE rolname = \'%s\' "
                     "RETURNING oid",
                     dboid, targetname.data, tempname.data);
    
    if (SPI_execute(querybuf.data, false, 1) != SPI_OK_UPDATE_RETURNING)
        elog(ERROR, "SPI_exec failed: %s", querybuf.data);
    
    if (SPI_processed != 1)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_ROLE_SPECIFICATION),
                 errmsg("error on create named: %s in database %ld row returned",
                        tempname.data, SPI_processed)
                ));
    role_oid = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);


    /*And copy the password from template db if is a user*/
    if(!se_role->is_role)
    {
        resetStringInfo(&querybuf);
        appendStringInfo(&querybuf,
                        "WITH temp_user AS ("
                        "SELECT rolpassword AS t_p, rolvaliduntil AS t_v "
                        "FROM pg_authid WHERE rolname = \'%s\' and rolowndboid = %d)"
                        "UPDATE pg_authid SET rolpassword = temp_user.t_p, rolvaliduntil = temp_user.t_v "
                        "FROM temp_user WHERE oid = %s",
                        rolname, templateid, role_oid);
        if (SPI_execute(querybuf.data, false, 1) != SPI_OK_UPDATE)
            elog(ERROR, "SPI_exec failed: %s", querybuf.data);
    }

    /*Close SPI context. */
    if (SPI_finish() != SPI_OK_FINISH)
        elog(ERROR, "SPI_finish failed");

    pfree(querybuf.data);
    pfree(tempname.data);
}
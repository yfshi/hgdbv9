/*
 *-------------------------------------------------------------------------
 * logininfo.c
 *	 Routines to handle login information
 *
 * Portions Copyright (c) -2025, Highgo Development Group
 *
 * Copyright:
 * Copyright (c) -2025, HighGo Software Co.Ltd. All right reserved.
 *
 *
 * IDENTIFICATION
 *	 contrib/credcheck/logininfo.c
 *
 * ReqID:SRS-SECURITY-EAL4-LOGIN-DISPLAY
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include <unistd.h>
#include "credcheck.h"
#include "miscadmin.h"
#include "catalog/pg_access_history.h"
#include "catalog/pg_access_history_last.h"
#include "catalog/pg_authid.h"
#include "catalog/indexing.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/xact.h"
#include "common/fe_memutils.h"
#include "common/hashfn.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "storage/lwlock.h"
#include "utils/wait_event.h"
#include "libpq/hba.h"
#ifdef WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "utils/fmgroids.h"

#ifdef ENABLE_DKY_BETA_FEATURE

/* Login information folder name.*/
#define HG_USER_LOGIN_INFO "pg_login_info"
/* Login information folder name and file name length.*/
#define HG_USER_LOGIN_INFO_DIR_LENGTH 1024

static char *get_method(UserAuth auth_method);
static char *get_dir_path(void);
static char *get_file_name(Oid oid, char *dirpath);
static bool ensure_directory(char *dirpath);
static void delete_login_info_from_history(char *username);
static void delete_login_info_from_history_last(char *username);

/* clientauthentication_accesshistory
 *
 * Store login information into loging info file.
 */
void clientauthentication_accesshistory(Port *port, const char *user, const char *application_name,
													 const char *hostinfo, TimestampTz logintime,
													 int action, const int maxfailednumss)
{
	HeapTuple	roleTup;
	Oid 		useroid = 0;
	const char *login_time = NULL;
	char	   *authentication_method = NULL;
	char	   *dirpath = NULL;
	char	   *filepath = NULL;
	FILE	   *fpout;
	int 		rc;
	acessHistoryEntry entry;

	if (action == STATUS_EOF || login_msg_display_storage == 0)
		return;

	/* get user oid */
	if(user != NULL)
	{
		roleTup = SearchSysCache1(AUTHNAME, PointerGetDatum(user));
		if (!HeapTupleIsValid(roleTup))
		{
			/* If user does not exist, report a login failed for user username */
			if (port->hba != NULL)
			{
				switch(port->hba->auth_method)
				{
					case uaPassword:
					case uaMD5:
					case uaSCRAMsha256:
#ifdef USE_GMSSL
					case uaSM3:
					case uaMD5SM3:
					case uaSM3MD5:
					case uaSCRAMsm3:
					case uaSCRAMsha256sm3:
					case uaSCRAMsm3sha256:
#endif
					{
						char *cdetail = NULL;
						char *logdetail = NULL;
						char *errstr = gettext_noop("password authentication failed for user \"%s\"");

						cdetail = psprintf(_("Connection matched pg_hba.conf line %d: \"%s\""),
										   port->hba->linenumber, port->hba->rawline);
						if (logdetail)
							logdetail = psprintf("%s\n%s", logdetail, cdetail);
						else
							logdetail = cdetail;

						ereport(FATAL,
								(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
								 errmsg(errstr, port->user_name),
								 logdetail ? errdetail_log("%s", logdetail) : 0));
					}
					break;

					default:
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
								 errmsg("login failed for user \"%s\"", user)));
					break;
				}
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
							errmsg("login failed for user \"%s\"", user)));
		}

		useroid = ((Form_pg_authid) GETSTRUCT(roleTup))->oid;
		ReleaseSysCache(roleTup);
	}

	login_time = timestamptz_to_str(logintime);

	if (port != NULL && port->hba != NULL)
		authentication_method = get_method(port->hba->auth_method);

	/* Get the login information folder path */
	dirpath = get_dir_path();
	if (dirpath == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("The file path %s cannot be NULL",
				 HG_USER_LOGIN_INFO)));
	}

	/* Ensure that the login information
	 * folder exists, then create the user
	 * longin file.
	 */
	if (ensure_directory(dirpath))
	{
		filepath = get_file_name(useroid, dirpath);
		if (filepath == NULL)
		{
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("The file name %s/%u cannot be NULL",
					 HG_USER_LOGIN_INFO, useroid)));
		}
	}

	/* free dirpath */
	pfree(dirpath);

	entry.roloid = useroid;
	entry.loginfailedsum = 0;
	entry.seqoid = logintime;

	if (action == STATUS_OK)
	{
		entry.login_status = true;
	}
	else
	{
		entry.login_status = false;
	}

	/* Preparing Data: IP address */
	if (hostinfo != NULL)
	{
		memset(entry.login_ip, 0, LOGININFOLEN);
		memcpy(entry.login_ip, hostinfo, strlen(hostinfo));
	}

	/* Preparing Data: application name */
	if (application_name != NULL)
	{
		memset(entry.login_app, 0, LOGININFOLEN);
		memcpy(entry.login_app, application_name, strlen(application_name));
	}

	/* Preparing Data: authentication method */
	if (authentication_method != NULL)
	{
		memset(entry.login_method, 0, LOGININFOLEN);
		memcpy(entry.login_method, authentication_method, strlen(authentication_method));
	}

	/* Preparing Data: login time */
	if (login_time != NULL)
	{
		memset(entry.login_time, 0, LOGININFOLEN);
		memcpy(entry.login_time, login_time, strlen(login_time));
	}

	/* using file lock */
	LWLockAcquire(AutoFileLock, LW_EXCLUSIVE);

	/* open a file save data */
	fpout = AllocateFile(filepath, PG_BINARY_A);
	if (fpout == NULL)
	{
		LWLockRelease(AutoFileLock);
		ereport(LOG,
				 errmsg("could not open file \"%s\"", filepath));
		return;
	}
	rc = fwrite(&entry, sizeof(acessHistoryEntry), 1, fpout);
	(void) rc;
	FreeFile(fpout);
	LWLockRelease(AutoFileLock);

	/* free filepath */
	pfree(filepath);
}

/* construct_a_insert_tuple
 *
 * construct a tuple for table pg_access_history.
 */
static HeapTuple
construct_a_insert_tuple(Oid useroid, NameData userName, Oid seqoid,
								  bool login_status, char *login_time,
								  char *login_app, char *login_method,
								  char *login_ip, Relation relAccessHistory)
{
	Datum			historyvalues[Natts_pg_access_history];
	bool			historynulls[Natts_pg_access_history];
	HeapTuple		tup;

	memset(historynulls, false, sizeof(historynulls));

	/*
	 * Prepare the data, these data is login failed information,
	 * and insert these data to hg_history_access.
	 */
	historyvalues[Anum_pg_access_history_roloid - 1] = ObjectIdGetDatum(useroid);
	historyvalues[Anum_pg_access_history_rolename - 1] = NameGetDatum(&userName);
	historyvalues[Anum_pg_access_history_seqoid - 1] = ObjectIdGetDatum(seqoid);
	historyvalues[Anum_pg_access_history_login_status - 1] = BoolGetDatum(login_status);
	historyvalues[Anum_pg_access_history_login_time - 1] = CStringGetTextDatum(login_time);
	historyvalues[Anum_pg_access_history_login_app - 1] = CStringGetTextDatum(login_app);
	historyvalues[Anum_pg_access_history_login_method - 1] = CStringGetTextDatum(login_method);
	historyvalues[Anum_pg_access_history_login_ip - 1] = CStringGetTextDatum(login_ip);
	tup = heap_form_tuple(RelationGetDescr(relAccessHistory), historyvalues, historynulls);

	return tup;
}

/* insertLoginFailureInformationIntoHistoryTable
 *
 * store the login failure information for the current user in
 * table pg_access_history, and write the failure information for
 * other users to the temporary file hg_user_login_info_for_username.
 * return login failure message.
 */
static acessResEntry *
insertLoginFailureInformationIntoHistoryTable(char *filename, char *currentusername, Relation relAccessHistory)
{
	FILE	   *fpout;
	char	   *username = NULL;	/*login user name*/
	Oid			useroid = InvalidOid;
	Oid			seqoid = InvalidOid;
	bool		login_status = false; /* login status */
	char		*login_time = NULL;
	char		*login_app = NULL;
	char		*login_method = NULL;
	char		*login_ip = NULL;
	Oid			failedseqoid = InvalidOid;
	bool		loginfailed_status = false; /* login status */
	char		*loginfailed_time = NULL;
	char		*loginfailed_app = NULL;
	char		*loginfailed_method = NULL;
	char		*loginfailed_ip = NULL;
	HeapTuple	tup;
	bool		readsuccflg = false;
	bool		readfailedflg = false;
	int			loginfailednum = 0;
	acessHistoryEntry entry;
	acessResEntry *result = NULL;

	/* Open the file hg_user_login_info */
	if ((fpout = AllocateFile(filename, PG_BINARY_R)))
	{
		while (fread(&entry, sizeof(acessHistoryEntry), 1, fpout) == 1)
		{
			NameData	userName;

			username = GetUserNameFromId(entry.roloid, true);
			if (username == NULL)
			{
				break;
			}

			/* Prepare the data, then insert it into
			 * the system table pg_access_history.
			 */
			useroid = entry.roloid;
			login_status = entry.login_status;
			namestrcpy(&userName, username);

			if (entry.login_status)
			{
				readsuccflg = true;
				seqoid = entry.seqoid;
				login_time = entry.login_time;
				login_app = entry.login_app;
				login_method = entry.login_method;
				login_ip = entry.login_ip;

				tup = construct_a_insert_tuple(useroid, userName, seqoid,
										   login_status, login_time,
										   login_app, login_method,
										   login_ip, relAccessHistory);
			}
			else
			{
				readfailedflg = true;
				loginfailednum++;
				failedseqoid = entry.seqoid;
				loginfailed_time = entry.login_time;
				loginfailed_app = entry.login_app;
				loginfailed_method = entry.login_method;
				loginfailed_ip = entry.login_ip;

				tup = construct_a_insert_tuple(useroid, userName, failedseqoid,
										   loginfailed_status, loginfailed_time,
										   loginfailed_app, loginfailed_method,
										   loginfailed_ip, relAccessHistory);
			}

			/* Insert into the system table pg_access_history. */
			CatalogTupleInsert(relAccessHistory, tup);
			/* Free tuple. */
			heap_freetuple(tup);
		}

		FreeFile(fpout);
	}

	if (readsuccflg || readfailedflg)
	{
		result = (acessResEntry *)palloc(sizeof(acessResEntry));

		/* Clear variable. */
		memset(result->login_ip, 0, LOGININFOLEN);
		memset(result->login_app, 0, LOGININFOLEN);
		memset(result->login_method, 0, LOGININFOLEN);
		memset(result->login_time, 0, LOGININFOLEN);
		memset(result->loginfailed_ip, 0, LOGININFOLEN);
		memset(result->loginfailed_app, 0, LOGININFOLEN);
		memset(result->loginfailed_method, 0, LOGININFOLEN);
		memset(result->loginfailed_time, 0, LOGININFOLEN);

		/* Variable assignment. */
		result->roloid = useroid;
		result->allloginfailed = loginfailednum;
		result->seqoid = seqoid;
	}

	/* Successful login information in the file. */
	if (readsuccflg)
	{
		memcpy(result->login_ip, login_ip, strlen(login_ip));
		memcpy(result->login_app, login_app, strlen(login_app));
		memcpy(result->login_method, login_method, strlen(login_method));
		memcpy(result->login_time, login_time, strlen(login_time));
	}

	/* The last failed login information in the file. */
	if (readfailedflg)
	{
		memcpy(result->loginfailed_ip, loginfailed_ip, strlen(loginfailed_ip));
		memcpy(result->loginfailed_app, loginfailed_app, strlen(loginfailed_app));
		memcpy(result->loginfailed_method, loginfailed_method, strlen(loginfailed_method));
		memcpy(result->loginfailed_time, loginfailed_time, strlen(loginfailed_time));
	}

	return result;
}

/* insertLoginInformationIntoHistoryLastTable
 *
 * Update table pg_access_history_last.
 */
static void
insertLoginInformationIntoHistoryLastTable(char *currentusername, acessResEntry *entry)
{
	NameData	userName;
	HeapTuple	tuple;
	HeapTuple	tup;
	Relation	pg_history_last_rel;
	Datum		historylastvalues[Natts_pg_access_history_last];
	bool		historylastnulls[Natts_pg_access_history_last];
	bool		historylastreplace[Natts_pg_access_history_last];

	if (entry == NULL)
		return;

	MemSet(historylastnulls, false, sizeof(historylastnulls));
	MemSet(historylastreplace, false, sizeof(historylastreplace));

	namestrcpy(&userName, currentusername);

	/* Open the table pg_access_history_last */
	pg_history_last_rel = table_open(AccessHistoryLastId, RowExclusiveLock);

	historylastvalues[Anum_pg_access_history_last_rolename - 1] =
		NameGetDatum(&userName);

	/* If ahEntry is not NULL, construct the data for
	 * a successful login, otherwise this data to NULL.
	 */
	if (entry != NULL)
	{
		historylastvalues[Anum_pg_access_history_last_last_success_login_time - 1] =
		CStringGetTextDatum(entry->login_time);
		historylastvalues[Anum_pg_access_history_last_last_success_login_app - 1] =
		CStringGetTextDatum(entry->login_app);
		historylastvalues[Anum_pg_access_history_last_last_success_login_method - 1] =
		CStringGetTextDatum(entry->login_method);
		historylastvalues[Anum_pg_access_history_last_last_success_login_ip - 1] =
		CStringGetTextDatum(entry->login_ip);
		historylastvalues[Anum_pg_access_history_last_fail_sum - 1] =
		Int64GetDatum(entry->allloginfailed);

		if(entry->allloginfailed > 0)
		{
			historylastvalues[Anum_pg_access_history_last_last_fail_login_time - 1] =
			CStringGetTextDatum(entry->loginfailed_time);
			historylastvalues[Anum_pg_access_history_last_last_fail_login_app - 1] =
						CStringGetTextDatum(entry->loginfailed_app);
			historylastvalues[Anum_pg_access_history_last_last_fail_login_method - 1] =
						CStringGetTextDatum(entry->loginfailed_method);
			historylastvalues[Anum_pg_access_history_last_last_fail_login_ip - 1] =
			CStringGetTextDatum(entry->loginfailed_ip);
		}
		else
		{
			historylastnulls[Anum_pg_access_history_last_last_fail_login_time - 1] = true;
			historylastnulls[Anum_pg_access_history_last_last_fail_login_app - 1] = true;
			historylastnulls[Anum_pg_access_history_last_last_fail_login_method - 1] = true;
			historylastnulls[Anum_pg_access_history_last_last_fail_login_ip - 1] = true;
		}
	}

	/* Check is the current user exists in the tbale;
	 * if not, insert the data; if so, update the data.
	 */
	tuple = SearchSysCache1(ACCESSHISTORYLASTROLENAME, NameGetDatum(&userName));
	if (!HeapTupleIsValid(tuple))
	{
		historylastvalues[Anum_pg_access_history_last_fail_sum - 1] = Int64GetDatum(entry->allloginfailed);
		tup = heap_form_tuple(RelationGetDescr(pg_history_last_rel), historylastvalues, historylastnulls);
		CatalogTupleInsert(pg_history_last_rel, tup);
		heap_freetuple(tup);
	}
	else
	{
		int64		fail_sum = 0;
		int64		userallfailsum = 0;
		bool		isnull;
		Datum		datum_fail_sum;
		HeapTuple	new_tuple;

		datum_fail_sum = SysCacheGetAttr(ACCESSHISTORYLASTROLENAME, tuple,
									 Anum_pg_access_history_last_fail_sum, &isnull);
		if (!isnull)
			fail_sum = DatumGetInt64(datum_fail_sum);

		MemSet(historylastreplace, true, sizeof(historylastreplace));
		/* Here, we only need to consider whether there is any failed data. */
		if (entry != NULL && entry->allloginfailed == 0)
		{
			historylastreplace[Anum_pg_access_history_last_last_fail_login_time - 1] = false;
			historylastreplace[Anum_pg_access_history_last_last_fail_login_app - 1] = false;
			historylastreplace[Anum_pg_access_history_last_last_fail_login_method - 1] = false;
			historylastreplace[Anum_pg_access_history_last_last_fail_login_ip - 1] = false;
		}
		userallfailsum = fail_sum + entry->allloginfailed;
		historylastvalues[Anum_pg_access_history_last_fail_sum - 1] = Int64GetDatum(userallfailsum);

		/* construct a tuple and update this record. */
		new_tuple = heap_modify_tuple(tuple, RelationGetDescr(pg_history_last_rel), historylastvalues,
										  historylastnulls, historylastreplace);
		CatalogTupleUpdate(pg_history_last_rel, &new_tuple->t_self, new_tuple);
		ReleaseSysCache(tuple);
	}

	table_close(pg_history_last_rel,RowExclusiveLock);
}

/* updateAccessHistory
 *
 * Flush the login information into the system table.
 */
void updateAccessHistory(char *currentusername)
{
	acessResEntry	   *findentry;
	char			   *filename = NULL;
	char			   *dirpath = NULL;
	Relation			relAccessHistory;
	Oid					useroid = InvalidOid;

	/* If the hash table does not exist, or thepath
	 * switch is not enabled, return.
	 */
	if(login_msg_display_storage == 0)
		return;

	/* get user oid */
	if (currentusername == NULL)
	{
		useroid = GetSessionUserId();
		currentusername = GetUserNameFromId(useroid, true);
	}
	else
	{
		useroid = get_role_oid(currentusername, false);
	}

	/* Construct a temporary filename named
	 * pg_login_info/useroid.
	 */
	dirpath = get_dir_path();
	if (dirpath == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("The file path %s cannot be NULL",
				 HG_USER_LOGIN_INFO)));
	}
	filename = get_file_name(useroid, dirpath);
	if (filename == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("The file name %s/%u cannot be NULL",
				 dirpath, useroid)));
	}

	/* open table. */
	LWLockAcquire(PgAccessHistoryLock, LW_EXCLUSIVE);
	relAccessHistory = table_open(AccessHistoryId, RowExclusiveLock);

	/* using file lock. */
	LWLockAcquire(AutoFileLock, LW_EXCLUSIVE);

	/* Store the login failure information for the current user in
	 * table pg_access_history, and write the failure information for
	 * other users to the temporary file hg_user_login_info_for_username.
	 */
	findentry= insertLoginFailureInformationIntoHistoryTable(filename, currentusername, relAccessHistory);

	/* Upon successful user login, the contents of the login
	 * file are written to able pg_access_history, after
	 * which the login file deleted.
	 */
	unlink(filename);
	LWLockRelease(AutoFileLock);

	/* close table pg_access_history. */
	table_close(relAccessHistory,RowExclusiveLock);

	/* free variable. */
	pfree(dirpath);
	pfree(filename);

	/* Insert the successfull and failed data
	 * into table pg_access_history_last.
	 */
	insertLoginInformationIntoHistoryLastTable(currentusername, findentry);

	LWLockRelease(PgAccessHistoryLock);
}

/* get_login_info_from_login_file
 *
 * get login information from shared memory hash. Retrieve
 * login failure data from file hg_user_login_info.
 */
acessResEntry *
get_login_info_from_login_file(const char *username)
{
	acessResEntry	   *resEntry = NULL;
	FILE			   *fpout;
	acessHistoryEntry	entry;
	char	   *dirpath = NULL;
	char	   *filepath = NULL;
	int64		allloginfailed = 0;
	Oid			useroid = InvalidOid;

	if (login_msg_display_storage == 0)
		return NULL;

	/* Get folder path. */
	dirpath = get_dir_path();
	if (dirpath == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("The login information file path cannot be NULL")));
	}
	/* Get file path. */
	useroid = get_role_oid(username, false);
	filepath = get_file_name(useroid, dirpath);
	if (filepath == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("The file name %s/%u cannot be NULL",
				 dirpath, useroid)));
	}

	/* Retrieve login failure data from file hg_user_login_info. */
	LWLockAcquire(AutoFileLock, LW_EXCLUSIVE);
	if ((fpout = AllocateFile(filepath, PG_BINARY_R)))
	{
		resEntry = (acessResEntry *)palloc0(sizeof(acessResEntry));

		/* Read the data from the file user oid. */
		while (fread(&entry, sizeof(acessHistoryEntry), 1, fpout) == 1)
		{
			resEntry->login_status = true;
			/* Search the log file for entries indicating successful
			 * and failed logins, respectively, and construct a return
			 * struct. */
			if (!entry.login_status)
			{
				allloginfailed++;
				resEntry->login_status = entry.login_status;
				memset(resEntry->loginfailed_ip, 0, LOGININFOLEN);
				memcpy(resEntry->loginfailed_ip, entry.login_ip, strlen(entry.login_ip));
				memset(resEntry->loginfailed_app, 0, LOGININFOLEN);
				memcpy(resEntry->loginfailed_app, entry.login_app, strlen(entry.login_app));
				memset(resEntry->loginfailed_method, 0, LOGININFOLEN);
				memcpy(resEntry->loginfailed_method, entry.login_method, strlen(entry.login_method));
				memset(resEntry->loginfailed_time, 0, LOGININFOLEN);
				memcpy(resEntry->loginfailed_time, entry.login_time, strlen(entry.login_time));
			}
			else
			{
				/* Login Success message. */
				resEntry->roloid = entry.roloid;
				resEntry->seqoid = entry.seqoid;

				memset(resEntry->login_ip, 0, LOGININFOLEN);
				memcpy(resEntry->login_ip, entry.login_ip, strlen(entry.login_ip));
				memset(resEntry->login_app, 0, LOGININFOLEN);
				memcpy(resEntry->login_app, entry.login_app, strlen(entry.login_app));
				memset(resEntry->login_method, 0, LOGININFOLEN);
				memcpy(resEntry->login_method, entry.login_method, strlen(entry.login_method));
				memset(resEntry->login_time, 0, LOGININFOLEN);
				memcpy(resEntry->login_time, entry.login_time, strlen(entry.login_time));
			}
		}

		resEntry->allloginfailed = allloginfailed;

		FreeFile(fpout);
	}
	LWLockRelease(AutoFileLock);

	/* free variable */
	pfree(dirpath);
	pfree(filepath);

	return resEntry;
}

/* get_method
 *
 * Get the name of the login method.
 */
static char *
get_method(UserAuth auth_method)
{
	char	*res = NULL;

	switch(auth_method)
	{
		case uaReject:
			res = "reject";
			break;
		case uaImplicitReject:/* Not a user-visible option */
			res = NULL;
			break;
		case uaTrust:
			res = "trust";
			break;
		case uaIdent:
			res = "ident";
			break;
		case uaPassword:
			res = "password";
			break;
		case uaMD5:
			res = "md5";
			break;
#ifdef USE_GMSSL
		case uaSM3:
			res = "sm3";
			break;
		case uaMD5SM3:
			res = "md5sm3";
			break;
		case uaSM3MD5:
			res = "sm3md5";
			break;
#endif
		case uaSCRAMsha256:
			res = "scram-sha-256";
			break;
#ifdef USE_GMSSL
		case uaSCRAMsm3:
			res = "scram-sm3";
			break;
		case uaSCRAMsm3sha256:
			res = "scram-sm3-sha-256";
			break;
		case uaSCRAMsha256sm3:
			res = "scram-sha-256-sm3";
			break;
#endif
		case uaGSS:
			res = "gss";
			break;
		case uaSSPI:
			res = "sspi";
			break;
		case uaPAM:
			res = "pam";
			break;
		case uaBSD:
			res = "bsd";
			break;
		case uaLDAP:
			res = "ldap";
			break;
		case uaCert:
			res = "cert";
			break;
		case uaRADIUS:
			res = "radius";
			break;
		case uaNative:
			res = "mysql_native_password";
			break;
#ifdef USE_OPENSSL
		case uaCachingSha256:
			res = "caching_sha2_password";
			break;
#endif
		case uaPeer:
			res = "peer";
			break;
		default:
			res = " ";
			break;
	}

	return res;
}

/* get_dir_path
 *
 * Get the path to the login information file.
 */
static char *
get_dir_path(void)
{
	char	fullpath[HG_USER_LOGIN_INFO_DIR_LENGTH];

	/* The login folder is create within
	 * the data directory.
	 */
	if (DataDir == NULL)
	{
		return NULL;
	}

	MemSet(fullpath, 0, HG_USER_LOGIN_INFO_DIR_LENGTH);

	/* Concatenate file paths. */
#ifdef WIN32
	snprintf(fullpath, HG_USER_LOGIN_INFO_DIR_LENGTH, "%s\\%s", DataDir, HG_USER_LOGIN_INFO);
#else
	snprintf(fullpath, HG_USER_LOGIN_INFO_DIR_LENGTH, "%s/%s", DataDir, HG_USER_LOGIN_INFO);
#endif

	return pstrdup(fullpath);
}

/* get_file_name
 *
 * Get the login file for the currently login user.
 */
static char *
get_file_name(Oid oid, char *dirpath)
{
	char	   filename[HG_USER_LOGIN_INFO_DIR_LENGTH];

	if (dirpath == NULL)
	{
		return NULL;
	}

	MemSet(filename, 0, HG_USER_LOGIN_INFO_DIR_LENGTH);
#ifdef WIN32
	snprintf(filename, HG_USER_LOGIN_INFO_DIR_LENGTH, "%s\\%u", dirpath, oid);
#else
	snprintf(filename, HG_USER_LOGIN_INFO_DIR_LENGTH, "%s/%u", dirpath, oid);
#endif

	return pstrdup(filename);
}

/* ensure_directory
 *
 * Ensure that the user's login folder exists,
 * if it does not, create it.
 */
static bool
ensure_directory(char *dirpath)
{
#ifdef WIN32
	DWORD		ftyp;
#else
	struct stat st;
#endif

	if (dirpath == NULL)
	{
		return false;
	}

	/* Check if a folder exists,
	 * if not, create it.
	 */
#ifdef WIN32
	ftyp = GetFileAttributesA(dirpath);
	if (ftyp == (DWORD)-1 || (ftyp & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (!CreateDirectory(dirpath, 0) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return false;
		}
	}
#else
	if (stat(dirpath, &st) != 0 && !S_ISDIR(st.st_mode))
	{
		if (mkdir(dirpath, 0755) != 0)
		{
			return false;
		}
	}
#endif

	return true;
}

/* delete_login_info_from_history
 *
 * When a user is dropped, remove records
 * from table pg_access_history.
 */
static void
delete_login_info_from_history(char *username)
{
	Relation	accesshistory;
	ScanKeyData skey[1];
	SysScanDesc scan;
	HeapTuple	oldtuple;

	/* Prepare data for sequential scan */
	ScanKeyInit(&skey[0],
				Anum_pg_access_history_rolename,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(username));

	/* Open the table pg_access_history */
	accesshistory = table_open(AccessHistoryId, RowExclusiveLock);

	scan= systable_beginscan(accesshistory, InvalidOid, false,
							NULL, 1, skey);

	/* Sequential scan */
	while (HeapTupleIsValid(oldtuple = systable_getnext(scan)))
	{
		/* Drop the records. */
		CatalogTupleDelete(accesshistory, &oldtuple->t_self);
	}

	/* End the sequential scan */
	systable_endscan(scan);
	/* Close the table pg_access_history */
	table_close(accesshistory, RowExclusiveLock);
}

/* delete_login_info_from_history_last
 *
 * When a user is dropped, remove records
 * from table pg_access_history_last.
 */
static void
delete_login_info_from_history_last(char *username)
{
	Relation	accesshistorylast;
	HeapTuple	oldtuple;

	/* Open the table pg_access_history_last */
	accesshistorylast = table_open(AccessHistoryLastId, RowExclusiveLock);
	/* Search the records. */
	oldtuple = SearchSysCache1(ACCESSHISTORYLASTROLENAME, CStringGetDatum(username));
	if (HeapTupleIsValid(oldtuple))
	{
		/* Drop the records. */
		CatalogTupleDelete(accesshistorylast, &oldtuple ->t_self);
		/* Free tuple. */
		ReleaseSysCache(oldtuple);
	}

	/* Close the table pg_access_history_last */
	table_close(accesshistorylast, RowExclusiveLock);
}

/*
 * When a user is dropped, remove the
 * login file.
 */
static void
drop_user_drop_login_info_file(char *username)
{
	Oid			useroid = InvalidOid;
	char	   *dirpath = NULL;
	char	   *filename = NULL;

	/* Find the file path and construct the file name. */
	useroid = get_role_oid(username, false);
	dirpath = get_dir_path();
	if (dirpath == NULL)
	{
		return;
	}

	filename = get_file_name(useroid, dirpath);
	if (filename == NULL)
	{
		pfree(dirpath);
		return;
	}

	/* Using file lock. */
	LWLockAcquire(AutoFileLock, LW_EXCLUSIVE);

	/* Drop Login info file. */
	unlink(filename);

	/* Relase file lock. */
	LWLockRelease(AutoFileLock);

	pfree(dirpath);
	pfree(filename);
}

/* drop_user_delete_login_info
 *
 * When a user is dropped, remove records
 * from table pg_access_history_last and
 * pg_access_history_last.
 */
void
drop_user_delete_login_info(Node *parsetree)
{
	if (parsetree != NULL && nodeTag(parsetree) == T_DropRoleStmt)
	{
		DropRoleStmt *stmt = (DropRoleStmt *)parsetree;
		ListCell *item;

		foreach (item, stmt->roles)
		{
			RoleSpec *rolspec = lfirst(item);

			if (rolspec->rolename != NULL)
			{
				delete_login_info_from_history(rolspec->rolename);
				delete_login_info_from_history_last(rolspec->rolename);
				drop_user_drop_login_info_file(rolspec->rolename);
			}
		}
	}
}
#endif

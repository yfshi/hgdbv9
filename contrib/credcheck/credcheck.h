/*-------------------------------------------------------------------------
 *
 * credcheck.h
 *	  Implenmentation of the login display and storage functionality.
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/credcheck/credcheck.h
 *
 * ReqID:SRS-SECURITY-EAL4-LOGIN-DISPLAY
 *
 *-------------------------------------------------------------------------
 */
#ifndef CREDCHECK_H_
#define CREDCHECK_H_

#include "postgres.h"
#include "libpq/libpq-be.h"
#include "utils/hsearch.h"

#ifdef ENABLE_DKY_BETA_FEATURE

extern int login_msg_display_storage;

#define LOGININFOLEN 64

/* This structure stores the user's login information,
 * and then stores the entire structure in the login
 * file.
 */
typedef struct acessHistoryEntry
{
	Oid roloid;
	int64 loginfailedsum;
	int64 seqoid;
	bool login_status;
	char login_app[LOGININFOLEN];		/* login failed appliction */
	char login_time[LOGININFOLEN];		/* login failed time */
	char login_ip[LOGININFOLEN];		/* login failed ip */
	char login_method[LOGININFOLEN];	/* login failed method, value from pg_hba.conf */
}acessHistoryEntry;

/* This structure stores the user login information
 * and then returns the entire structure for printing.
 */
typedef struct acessResEntry
{
	Oid roloid;
	int64 seqoid;
	int64 allloginfailed;
	bool login_status;						/* last login failed status */
	char login_app[LOGININFOLEN];			/* current login failed appliction */
	char login_time[LOGININFOLEN];			/* current login failed time */
	char login_ip[LOGININFOLEN];			/* current login failed ip */
	char login_method[LOGININFOLEN];		/* current login failed method, value from pg_hba.conf */
	char loginfailed_app[LOGININFOLEN];		/* last login failed appliction */
	char loginfailed_time[LOGININFOLEN];	/* last login failed time */
	char loginfailed_ip[LOGININFOLEN];		/* last login failed ip */
	char loginfailed_method[LOGININFOLEN];	/* last login failed method, value from pg_hba.conf */
}acessResEntry;

/* Store successful login information into
 * the login file.
 */
void clientauthentication_accesshistory(Port *port, const char *user, const char *application_name,
													 const char *hostinfo, TimestampTz logintime,
													 int status, const int maxfailednumss);
/* Flush the login information into the system table */
extern void updateAccessHistory(char *currentusername);
/* get login information from login file */
extern acessResEntry * get_login_info_from_login_file(const char *username);
/* hook for printing and storing login information */
extern void login_info_print_and_stoage(void);
/* table pg_access_history and pg_access_history_last access restrictions */
extern bool tb_pg_access_history_access_restrictions(List *tbnames, bool flg);
void drop_user_delete_login_info(Node *parsetree);
#endif
#endif
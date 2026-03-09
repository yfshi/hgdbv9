/*-------------------------------------------------------------------------
 *
 * postmaster.h
 *	  Exports from postmaster/postmaster.c.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * Authored by wanghaoyan@highgo.com, 20241127.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 *
 * src/include/postmaster/postmaster.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _POSTMASTER_H
#define _POSTMASTER_H

#include "access/printtup.h"
#include "libpq/auth.h"
#include "libpq/libpq.h"

/* GUC options */
extern PGDLLIMPORT bool EnableSSL;		/* ReqID:SRS-SECURITY-IDENTITYAUTH */

extern int	ReservedBackends;
extern PGDLLIMPORT int PostPortNumber;
extern PGDLLIMPORT char *DatabaseName; /* ReqID:SRS-SECURITY-AUDIT */

extern int	Unix_socket_permissions;
extern char *Unix_socket_group;
extern PGDLLIMPORT char *Unix_socket_directories;
extern char *ListenAddresses;
extern bool ClientAuthInProgress;
extern int	PreAuthDelay;
extern int	AuthenticationTimeout;
extern bool Log_connections;
extern bool log_hostname;
extern bool enable_bonjour;
extern char *bonjour_name;
extern bool restart_after_crash;
extern bool remove_temp_files_after_crash;
/* Begin - ReqID:SRS-SECURITY-TDE */
extern int	terminal_fd;
/* End - ReqID:SRS-SECURITY-TDE */

#ifdef WIN32
extern HANDLE PostmasterHandle;
#else
extern int	postmaster_alive_fds[2];

/*
 * Constants that represent which of postmaster_alive_fds is held by
 * postmaster, and which is used in children to check for postmaster death.
 */
#define POSTMASTER_FD_WATCH		0	/* used in children to check for
									 * postmaster death */
#define POSTMASTER_FD_OWN		1	/* kept open by postmaster only */
#endif

extern PGDLLIMPORT const char *progname;

extern void PostmasterMain(int argc, char *argv[]) pg_attribute_noreturn();
extern void ClosePostmasterPorts(bool am_syslogger);
extern void InitProcessGlobals(void);

extern int	MaxLivePostmasterChildren(void);

extern bool PostmasterMarkPIDForWorkerNotify(int);

#ifdef EXEC_BACKEND
extern pid_t postmaster_forkexec(int argc, char *argv[]);
extern void SubPostmasterMain(int argc, char *argv[]) pg_attribute_noreturn();

extern Size ShmemBackendArraySize(void);
extern void ShmemBackendArrayAllocation(void);
#endif

/*
 * Note: MAX_BACKENDS is limited to 2^18-1 because that's the width reserved
 * for buffer references in buf_internals.h.  This limitation could be lifted
 * by using a 64bit state; but it's unlikely to be worthwhile as 2^18-1
 * backends exceed currently realistic configurations. Even if that limitation
 * were removed, we still could not a) exceed 2^23-1 because inval.c stores
 * the backend ID as a 3-byte signed integer, b) INT_MAX/4 because some places
 * compute 4*MaxBackends without any overflow check.  This is rechecked in the
 * relevant GUC check hooks and in RegisterBackgroundWorker().
 */
#define MAX_BACKENDS	0x3FFFF

/* Begin - Req: SRS-MYSQL-PROTOCOL */
#define MAXLISTEN	64

typedef void (*create_server_listen_hook_type) ();
typedef void (*create_mysql_auth_cache_hook_type) ();
typedef ProtocolExtensionConfig *(*get_protocol_config_hook_type) (int family, char *local_port); /* bug#6805 */

extern int StartupPacketInitialize(Port *port);
extern void send_cancel_key(int32 pid, int32 key);
extern void libpq_report_param_status(const char *name, char *val);

extern PGDLLIMPORT pgsocket ListenSocket[MAXLISTEN];
extern PGDLLIMPORT create_server_listen_hook_type server_listen_hook;
extern PGDLLIMPORT get_protocol_config_hook_type get_protocol_config_hook; /* bug#6805 */
/* End - Req: SRS-MYSQL-PROTOCOL */

#endif							/* _POSTMASTER_H */

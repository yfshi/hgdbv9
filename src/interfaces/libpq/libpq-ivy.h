/*-------------------------------------------------------------------------
 *
 * libpq-ivy.h like libpq-fe.h but contain some others ivy functions
 *	  This file contains definitions for structures and
 *	  externs for functions used by frontend postgres applications.
 *
 * Copyright (c) 2009-2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * src/interfaces/libpq/libpq-ivy.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef LIBPQ_IVY_H
#define LIBPQ_IVY_H

#ifdef __cplusplus
extern		"C"
{
#endif

#include <stdio.h>
#include "pg_config.h"
#include "ivy_sema.h"

/*
 * postgres_ext.h defines the backend's externally visible types,
 * such as Oid.
 */
#include "postgres_ext.h"

/* function argument modes */
typedef enum
{
	argmode_in,
	argmode_out,
	argmode_inout
} Ivyargmode;

typedef struct Ivylist
{
	void *value;
	struct Ivylist *next;
} Ivylist;


/* bind out parameter struct */
typedef struct IvyBindOutInfo
{
	struct IvyBindOutInfo *next;
	int position;		/* variable position */
	int	resultcolumnno;	/* the column number in the PGresult */
	void *var;			/* the address to save out parameters return value */
	int *indp;			/* index the value copy status */
	size_t val_size;	/* the size of var */
	unsigned int		dtype;		/* the type of parameter */
	/* Begin - bug#00008129 */
	Ivyargmode			ivy_mode;	/* the bind paramter mode */
	/* End - bug#00008129 */

	/*
	 * these parameter copy from OCI,
	 * it may be used in further
	 */
	char *alenp;
	char *recodep;
	int  maxarr_len;
	int  *curelep;
	int  mode;
} IvyBindOutInfo;
typedef struct IvyBindOutInfo IvyBindInfo;

typedef enum IvyStmtType
{
	IVY_STMT_UNKNOW,
	IVY_STMT_DO,
	IVY_STMT_DOFROMCALL, /* ReqID:SRS-SQL-CALL */
	IVY_STMT_DOHANDLED,
	IVY_STMT_OTHERS
} IvyStmtType;

typedef struct IvyBindOutNameInfo
{
	IvyBindOutInfo **bindinfo;
	void			*var;			/* bindvar address */
	int				*indp;
	size_t			val_size;
	char			*name;			/* placeholdvar name */
	size_t			name_len;		/* placeholdvar name len */
	int				replace;		/* has replace to position */
	/* Begin - BUG:M0000346 */
	unsigned int	dtype;			/* the type of parameter */
	/* End - BUG:M0000346 */
	/*
	 * these parameter copy from OCI
	 * it maybe used in further
	 */
	char *alenp;
	char *recodep;
	int  maxarr_len;
	int  *curelep;
	int  mode;
	struct IvyBindOutNameInfo *next;
} IvyBindOutNameInfo;
typedef struct IvyBindOutNameInfo IvyBindNameInfo;

/* like PGconn */
typedef struct
{
	PGconn *conn;
	PGSemaphoreData self_lock;	/* modify conn should add this lock */
	PGSemaphoreData lock;		/* modify IvystmtHandleList should add this lock */
	Ivylist *IvystmtHandleList;
	/* Begin - bug#00008129 */
	bool	result_first;		/* come from server ivorysql.result_first */
	/* End - bug#00008129 */
} Ivyconn;

/* like PGresult */
typedef struct
{
	PGresult *result;
	int		off;		/* return parameter offset */ /* bug#00008129 */
	/* Begin - bug#00008129 */
	int		noutparams;	/* the number of out parameters */
	/* End - bug#00008129 */
} Ivyresult;

/* similar to jdbc statement handle */
typedef struct
{
	char *stmtName;
	char *query;
	int	query_len;
	int nParams;
	Oid *paramTypes;
	int		mode;
	char	**paramNames;	/* save param names */
	int		language;		/* not nused */

	PGSemaphoreData lock;	/* modify its data should add this lock */
	Ivylist *IvyconnList;
	int	noutvars;
	IvyBindOutInfo *outbind;
	IvyBindOutNameInfo *namebind;	/* binded by name */
	int			name_replace;		/* by name is replaced by position */
	IvyStmtType	stmttype;			/* stmttype */
	char		*do_using_query;	/* Dostmt should rewrite query */
} IvyPreparedStatement;

typedef struct IvyError
{
	size_t err_buf_size;
	char	*error_msg;
} IvyError;

typedef enum IVY_HANDLE_TYPE
{
	IVY_HANDLE_STMT,
	IVY_HANDLE_ERROR,
	IVY_HANDLE_NO
} ivy_handle_type;

/* Begin - ReqID:SRS-CMD-PSQL */
typedef struct HostVariableEntry
{
	char	*name;
	int		position;
} HostVariableEntry;

typedef struct HostVariable
{
	HostVariableEntry *hostvars;
	int		length;
	bool	isdostmt;
	/* Begin - ReqID:SRS-SQL-CALL */
	bool	iscallstmt;
	char	*convertcall;	/* Converted CALL INTO */
	/* End - ReqID:SRS-SQL-CALL */
} HostVariable;
/* End - ReqID:SRS-CMD-PSQL */

extern Ivyconn *Ivyconnectdb(const char *conninfo);
/* Begin - ReqID:SRS-CMD-PSQL */
extern Ivyconn *Ivyconnectdb2(PGconn *conn);
extern void Ivyfinish2(Ivyconn *conn);
/* End - ReqID:SRS-CMD-PSQL */
extern IvyPreparedStatement *IvyCreatePreparedStatement(const char *stmtName,
													const char *query,
													int nParams,
													const Oid *paramTypes);
extern int IvybindOutParameterByPos(IvyPreparedStatement *stmtHandle,
									IvyBindOutInfo **bindinfo,
									int	position,
									void *var,
									size_t	val_size,
									int *indp,
									int	replace_ok,
									char *errmsg,
									size_t size_errmsg);
/* Begin - ReqID:SRS-SQL-LIBPQOUT */
extern int IvyHandleAlloc(const void *parent,
								void **hndlpp,
								ivy_handle_type		type,
								size_t	size,
								void	**usermapp);
extern int IvyStmtPrepare(IvyPreparedStatement * stmt,
						IvyError	*errhp,
						const char *query,
						size_t	query_len,
						int		language,
						int		mode);
extern void IvyFreeHandle(void *handle, ivy_handle_type type);
extern int IvyBindByPos(IvyPreparedStatement *stmtHandle,
									IvyBindInfo **bindinfo,
									IvyError			*errhp,
									int	position,
									void *valuep,
									size_t	value_sz,
									unsigned int dty, /* BUG#M0000346 */
									int *indp,
									char *alenp,
									char *recodep,
									int  maxarr_len,
									int  *curelep,
									int  mode);
extern int IvyBindByName(IvyPreparedStatement *stmtHandle,
									IvyBindInfo **bindinfo,
									IvyError		*errhp,
									const char	*name,
									size_t name_len,
									void *var,
									size_t	val_size,
									unsigned int dty, /* BUG#M0000346 */
									int *indp,
									char *alenp,
									char *recodep,
									int  maxarr_len,
									int  *curelep,
									int  mode);
extern Ivyresult *IvyStmtExecute(Ivyconn *tconn,
									IvyPreparedStatement *stmtHandle,
									IvyError *errhp);

/* Begin - ReqID:SRS-CMD-PSQL */
extern Ivyresult *IvyStmtExecute2(Ivyconn *tconn,
					  				IvyPreparedStatement *stmtHandle,
					  				IvyError *errhp,
					  				HostVariable *host);
/* End - ReqID:SRS-CMD-PSQL */

extern int IvybindOutParameterByName(IvyPreparedStatement *stmtHandle,
									IvyBindOutInfo **bindinfo,
									const char	*name,
									size_t name_len,
									void *var,
									size_t	val_size,
									int *indp,
									int	replace_ok,
									char *errmsg,
									size_t size_errmsg);
/* End - ReqID:SRS-SQL-LIBPQOUT */

/* exec stmt nedd Ivybind function */
extern Ivyresult *IvyexecPreparedStatement(Ivyconn *conn,
										IvyPreparedStatement *stmtHandle,
										int nParams,
										const char *const *paramValues,
										const int *paramLengths,
										const int *paramFormats,
										const Ivyargmode *argmodes,
										int resultFormat,
										char *errmsg,
										size_t size_errmsg);

/* exec stmt don't need Ivybindxxx function */
extern Ivyresult *IvyexecPreparedStatement2(Ivyconn *conn,
										IvyPreparedStatement *stmtHandle,
										int nParams,
										const char *const *paramValues,
										const int *paramLengths,
										const int *paramFormats,
										const Ivyargmode *argmodes,
										IvyBindOutInfo *bindinfos,
										int resultFormat,
										char *errmsg,
										size_t size_errmsg);

extern void IvyFreePreparedStatement(IvyPreparedStatement *stmtHandle);
extern void Ivyclear(Ivyresult *tresult);
extern void Ivyfinish(Ivyconn *conn);

/* some usefull functions */
extern ConnStatusType Ivystatus(const Ivyconn *conn);
extern Ivyresult *Ivyexec(Ivyconn *conn, const char *query);
extern ExecStatusType IvyresultStatus(const Ivyresult *res);
extern int	Ivynfields(const Ivyresult *tresult);
extern char *Ivyfname(const Ivyresult *res, int field_num);
extern int	Ivyntuples(const Ivyresult *res);
extern char *Ivygetvalue(const Ivyresult *res, int tup_num, int field_num);
extern char *IvyerrorMessage(const Ivyconn *conn);

#ifdef __cplusplus
}
#endif

#endif   /* LIBPQ_IVY_H */

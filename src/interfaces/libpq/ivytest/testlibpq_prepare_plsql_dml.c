/*
 * testlibpq_prepare_plsql.c
 * Test the C version of LIBPQ, the POSTGRES frontend library.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "libpq-fe.h"
#include "libpq-ivy.h"
#include <string.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>

const char *anonymous_paramete_str =
"begin"
"  insert into test_msds values(:x,:y);"
"  insert into test_msds values(:m,:n);"
" end;";


static void
exit_nicely(Ivyconn *conn)
{
	Ivyfinish(conn);
	exit(EXIT_SUCCESS);
}

static void
exec_prepare(Ivyconn *conn, int byname)
{
	Ivyresult   *res;
	IvyPreparedStatement *stmthandle = NULL;
	IvyError *errhp = NULL;
	int		x = 23;
	char		y[256] = "xiexie";
	int		m = 24;
	char	n[256] = "welcome to beijing";
	IvyBindInfo *bindinfo[4] = {NULL, NULL, NULL, NULL};
	int index[4] = {0,0,0,0};

	if (!IvyHandleAlloc(NULL, (void **) &stmthandle, IVY_HANDLE_STMT,4, NULL))
	{
		fprintf(stderr, "IvyHandleAlloc prepared stmt failed\n");
		exit_nicely(conn);
	}

	if (!IvyHandleAlloc(NULL, (void **) &errhp, IVY_HANDLE_ERROR, 4, NULL))
	{
		IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
		fprintf(stderr, "IvyHandleAlloc Error handle failed\n");
		exit_nicely(conn);
	}

	if (!IvyStmtPrepare(stmthandle, errhp, anonymous_paramete_str, strlen(anonymous_paramete_str), 0, 0))
	{
		fprintf(stderr, "%s\n", errhp->error_msg);
		IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
		IvyFreeHandle(errhp, IVY_HANDLE_ERROR);
		exit_nicely(conn);
	}
	if (byname)
	{
		IvyBindByName(stmthandle,
					&bindinfo[0],
					errhp,
					":x",
					strlen(":x"),
					&x,
					sizeof(x),
					23,
					&index[0],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByName(stmthandle,
					&bindinfo[1],
					errhp,
					":y",
					strlen(":y"),
					&y,
					strlen(y),
					0,
					&index[1],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByName(stmthandle,
					&bindinfo[2],
					errhp,
					":m",
					strlen(":m"),
					&m,
					sizeof(int),
					23,
					&index[2],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByName(stmthandle,
					&bindinfo[3],
					errhp,
					":n",
					strlen(":n"),
					n,
					strlen(n),
					0,
					&index[3],
					NULL,
					NULL,
					256,
					NULL,
					0);
	}
	else
	{
		IvyBindByPos(stmthandle,
					&bindinfo[0],
					errhp,
					1,
					&x,
					sizeof(int),
					23,
					&index[0],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByPos(stmthandle,
					&bindinfo[1],
					errhp,
					2,
					y,
					strlen(y),
					705,
					&index[1],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByPos(stmthandle,
					&bindinfo[2],
					errhp,
					3,
					&m,
					sizeof(int),
					23,
					&index[2],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByPos(stmthandle,
					&bindinfo[3],
					errhp,
					4,
					n,
					strlen(n),
					0,
					&index[3],
					NULL,
					NULL,
					256,
					NULL,
					0);
	}

	res = IvyStmtExecute(conn, stmthandle, errhp);
	if (IvyresultStatus(res) != PGRES_TUPLES_OK &&
		IvyresultStatus(res) != PGRES_COMMAND_OK &&
		IvyresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
		IvyFreeHandle(errhp, IVY_HANDLE_ERROR);
		fprintf(stderr, "IvyStmtExecute statement didn't return tuples properly\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

	Ivyclear(res);
	IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
	IvyFreeHandle(errhp, IVY_HANDLE_ERROR);

	return;
}

int
main(int args, char **argv)
{
	Ivyconn    *conn;
	Ivyresult   *res;
	int i;
	int nFields;
	int j;
	char conninfo[100]="user=system dbname=template1 port="; /* BUG#M0000075: When the license is enabled, the default library is changed from postgres to highgo. */
	const char *oraport = argv[1];
	if (oraport == NULL)
	{
		fprintf(stderr, "The first parameter, which is the port number of an oracle compatible database, is not specified.\n");
		return -1;
	}
	strcat(conninfo, oraport);

	/* make a connection to the database */
	conn = Ivyconnectdb(conninfo);

	/* check to see that the backend connection was successfully made */
	if (Ivystatus(conn) == CONNECTION_BAD)
	{
		fprintf(stderr, "Connection to database failed.\n");
		fprintf(stderr, "%s", IvyerrorMessage(conn));
		exit_nicely(conn);
	}

	/* start a transaction block */
	res = Ivyexec(conn, "BEGIN");

	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed\n");
		Ivyclear(res);
		exit_nicely(conn);
	}
	Ivyclear(res);

	res = Ivyexec(conn, "create table test_msds(id integer,name varchar2(256))");
	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Create command failed\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

	Ivyclear(res);

	exec_prepare(conn, 1);
	exec_prepare(conn, 0);

	res =  Ivyexec(conn, "select * from test_msds");
	if (IvyresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "errmsg:%s\n", IvyerrorMessage(conn));
		Ivyclear(res);
		exit_nicely(conn);
	}

	/* print the result attribute names */
	nFields = Ivynfields(res);
	for (i = 0; i < nFields; i++)
		printf("%-15s", Ivyfname(res, i));

	printf("\n");

	/* print out the instances */
	for (i = 0; i < Ivyntuples(res); i++)
	{
		for (j = 0; j < nFields; j++)
		{
			printf("%-15s", Ivygetvalue(res, i, j));
			printf("\t");
		}
		printf("\n");
	}
	Ivyclear(res);

	res = Ivyexec(conn, "drop table test_msds");
	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Drop command failed\n");
		Ivyclear(res);
		exit_nicely(conn);
	}
	Ivyclear(res);

	/* end the transaction */
	res = Ivyexec(conn, "END");
	Ivyclear(res);

	Ivyfinish(conn);

	return 0;
}

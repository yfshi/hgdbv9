/*
 * testlibpq.c
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

char *function_str = "create or replace function f1(c1 out sys_refcursor, name varchar, c2 out integer, c3 integer, c4 inout varchar) return sys_refcursor is"
"		rc sys_refcursor;"
"       type cur_type is ref cursor return test%rowtype;"
"       cur_var cur_type;"
"	begin"
"		open rc for select * from test;"
"		open cur_var for select * from test;"
"		c2 := c3 + 23;"
"		c4 := c4 || name || 'xiexie';"
"		c1 := cur_var;"
"		return rc;"
"	end;";

static void
exit_nicely(Ivyconn *conn)
{
	Ivyfinish(conn);
	exit(EXIT_SUCCESS);
}

static void
exec_prepare(Ivyconn *conn, int byname)
{
	int   nFields;
	int   i,j;
	Ivyresult   *res;
	const char *stmt = "select * from f1(:x,:y,:z,:f,:m);";
	IvyPreparedStatement *stmthandle = NULL;
	IvyError *errhp = NULL;
	char ref_cursor[256] = "200";
	char	name[256] = "welcome to beijing";
	int		c2 = 23;
	int		c3 = 24;
	char	c4[256] = "welcome to zhaojiacun";
	IvyBindInfo *bindinfo[5] = {NULL, NULL,NULL,NULL,NULL};
	char fetchstr[1024];
	char ref_cursor1[256];
	int index[5] = {0,0,0,0,0};
	int	run = 0;

	memset(ref_cursor1, 0x00, 256);

	if (!IvyHandleAlloc(NULL, (void **) &stmthandle,IVY_HANDLE_STMT,4, NULL))
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

	if (!IvyStmtPrepare(stmthandle, errhp, stmt, strlen(stmt), 0, 0))
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
					ref_cursor,
					256,
					9043,
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
					name,
					strlen(name),
					1043,
					&index[1],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByName(stmthandle,
					&bindinfo[2],
					errhp,
					":z",
					strlen(":z"),
					&c2,
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
					":f",
					strlen(":f"),
					&c3,
					sizeof(int),
					23,
					&index[3],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByName(stmthandle,
					&bindinfo[4],
					errhp,
					":m",
					strlen(":m"),
					c4,
					256,
					1043,
					&index[4],
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
					ref_cursor,
					256,
					9043,
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
					name,
					strlen(name),
					1043,
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
					&c2,
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
					&c3,
					sizeof(int),
					23,
					&index[3],
					NULL,
					NULL,
					256,
					NULL,
					0);
		IvyBindByPos(stmthandle,
					&bindinfo[4],
					errhp,
					5,
					c4,
					256,
					1043,
					&index[4],
					NULL,
					NULL,
					256,
					NULL,
					0);
	}

	for (run = 0; run < 4; run++)
	{
		printf("######RUN :%d #########\n", run);
		res = IvyStmtExecute(conn, stmthandle, errhp);
		if (IvyresultStatus(res) != PGRES_TUPLES_OK && IvyresultStatus(res) != PGRES_COMMAND_OK &&
			IvyresultStatus(res) != PGRES_EMPTY_QUERY)
		{
			IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
			IvyFreeHandle(errhp, IVY_HANDLE_ERROR);
			fprintf(stderr, "PQexecPrepared statement didn't return tuples properly\n");
			Ivyclear(res);
			exit_nicely(conn);
		}

		/* get result cursor */
		{
			char *retstr = Ivygetvalue(res, 0, 0);
			memcpy(ref_cursor1, retstr, strlen(retstr));
		}
		Ivyclear(res);

		printf("first out parameter:\n");
		printf("%s\n", ref_cursor);
		printf("%d\n", index[0]);

		printf("second out parameter:\n");
		printf("%d\n", c2);
		printf("%d\n", index[2]);

		printf("third out parameter:\n");
		printf("%s\n", c4);
		printf("%d\n", index[4]);
		/* fetch cursor */
		memset(fetchstr, 0x00, 256);
		sprintf(fetchstr, "fetch all in \"%s\"", ref_cursor);
		res = Ivyexec(conn, fetchstr);
		if (IvyresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "PQexec statement didn't return tuples properly\n");
			Ivyclear(res);
			exit_nicely(conn);
		}
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
		memset(fetchstr, 0x00, 256);
		sprintf(fetchstr, "close \"%s\"", ref_cursor);
		res = Ivyexec(conn, fetchstr);
		Ivyclear(res);

		printf("another cursor data\n");
		memset(fetchstr, 0x00, 256);
		sprintf(fetchstr, "fetch all in \"%s\"", ref_cursor1);
		res = Ivyexec(conn, fetchstr);
		if (IvyresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "PQexec statement didn't return tuples properly\n");
			Ivyclear(res);
			exit_nicely(conn);
		}
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
		memset(fetchstr, 0x00, 256);
		sprintf(fetchstr, "close \"%s\"", ref_cursor1);
		res = Ivyexec(conn, fetchstr);
		Ivyclear(res);
	}
	IvyFreeHandle(stmthandle, IVY_HANDLE_STMT);
	IvyFreeHandle(errhp, IVY_HANDLE_ERROR);
	return;
}

int
main(int argc, char **argv)
{
	Ivyconn    *conn;
	Ivyresult   *res;
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
	res = Ivyexec(conn, "create table test(id integer,name varchar(23));");
	Ivyclear(res);
	res = Ivyexec(conn, "insert into test values(23,'xiexie');");
	Ivyclear(res);
	res = Ivyexec(conn,function_str);
	Ivyclear(res);

	/* start a transaction block */
	res = Ivyexec(conn, "BEGIN");

	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed\n");
		Ivyclear(res);
		exit_nicely(conn);
	}
	Ivyclear(res);

	exec_prepare(conn, 1);
	exec_prepare(conn, 0);
	/* end the transaction */
	res = Ivyexec(conn, "END");
	Ivyclear(res);

	res = Ivyexec(conn, "drop function f1(sys_refcursor, varchar, integer,integer, varchar);");
	Ivyclear(res);
	res = Ivyexec(conn, "drop table test");
	Ivyclear(res);

	Ivyfinish(conn);
	return 0;
}

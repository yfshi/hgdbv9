/*
 * testlibpq_prepare_anonlymous_parameter.c
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
#include "anonly.h"

/* should map test_gen_insert.sh max_nargs */
#define MAX_NARGS 2000

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
	IvyBindOutInfo *outinfo[1660];
	int 		outval[1600];
	int		indp[1600];
	int i;
	int nargs = MAX_NARGS;
	Oid param_type[MAX_NARGS];
	const char *param_values[MAX_NARGS];
	int param_formats[MAX_NARGS];
	char errmsg[256];
	int param_lengths[MAX_NARGS];
	Ivyargmode argmodes[MAX_NARGS];


	for (i = 0 ; i  < nargs; i++)
	{
		param_type[i] = 23;
		param_values[i] = "1";
		param_lengths[i] = 1;
		param_formats[i] = 0;
		argmodes[i] = argmode_in;
	}
	stmthandle = IvyCreatePreparedStatement("test_stmt", anonymous_paramete_str, nargs, param_type);

	for (i = 0; i < 1660; i++)
	{
		outinfo[i] = NULL;
		IvybindOutParameterByPos(stmthandle, &outinfo[i],i + 1,
					&outval[i], sizeof(int),
					&indp[i], 1, errmsg, 255);
		argmodes[i] = argmode_out;
	}

	res = IvyexecPreparedStatement(conn, stmthandle, nargs, param_values, param_lengths,
									param_formats, argmodes,
									1, errmsg, 256);

	if (IvyresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "errmsg:%s\n", errmsg);
		Ivyclear(res);
		return;
	}

	Ivyclear(res);

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
	res = Ivyexec(conn, "create table test_stmt(id integer,id2 integer,id3 integer,id4 integer,id5 integer,id6 integer,id7 integer,id8 integer,id9 integer,id10 integer);");
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

	exec_prepare(conn, 0);
	/* end the transaction */
	res = Ivyexec(conn, "END");
	Ivyclear(res);

	{
		int nFields;
		int i;
		int j;

		res = Ivyexec(conn, "select count(*) from test_stmt");
		if (IvyresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "select failed");
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
	}
	res = Ivyexec(conn, "drop table test_stmt");
	Ivyclear(res);

	Ivyfinish(conn);
	return 0;
}

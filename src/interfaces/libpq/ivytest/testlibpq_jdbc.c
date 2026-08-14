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

char *function_str = "create or replace function short_proc(OUT IMAX integer, OUT IMIN integer, OUT INUL integer) return integer as "
" begin "
 "         select max_val into imax from short_tab;"
 "         select min_val into imin from short_tab;"
 "         select null_val into inul from short_tab;"
 "         return 1;"
 "         end;";

char *block_str ="declare var1 integer;"
	" begin "
	" var1 := short_proc(:1,:2,:3);"
	" :4 := var1;"
	" end;";
static void
exit_nicely(Ivyconn *conn)
{
	Ivyfinish(conn);
	exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
	int   nFields;
	int   i,j;
	int   result_format = 1;
	Ivyconn    *conn;
	Ivyresult   *res;
	const char *stmt_name = "test_stmt";
	const char *stmt = "select * from short_proc($1,$2,$3,$4);";
	Oid param_types[4];
	IvyPreparedStatement *stmthandle;
	const char* custid = "200";
	const char* salary = "2000";
	const char* name = "2000";
	const char *ret = "23";
	const char* param_values[4];
	int param_lengths[4];
	int param_formats[4];
	char errmsg[256];
	IvyBindOutInfo *bindinfo[4] = {NULL, NULL, NULL};
	signed int bindvar[4];
	int index[4];
	Ivyargmode modes[4];
	char conninfo[100]="user=system dbname=template1 options='-c ivorysql.result_first=on' port="; /* BUG#M0000075: When the license is enabled, the default library is changed from postgres to highgo. */
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
	res = Ivyexec(conn, "create temp table short_tab ( max_val integer, min_val integer, null_val integer );");
	Ivyclear(res);
	res = Ivyexec(conn, "insert into short_tab values (32767,-32768,null);");
	Ivyclear(res);
	res = Ivyexec(conn,function_str);
	Ivyclear(res);

	res = Ivyexec(conn, "set ivorysql.where_from = 'jdbc'");
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
	/* Step 2, activating prepared statement */
	param_types[0] = 23 |0x80000000;	/* return */
	param_types[1] = 23 | 0x80000000;  /* out */;
	param_types[2] = 23 | 0x80000000;  /* out */;
	param_types[3] = 23 | 0x80000000;  /* out */;

	stmthandle = IvyCreatePreparedStatement(stmt_name, stmt, 4, param_types);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[0],
				1,
				&bindvar[0],
				sizeof (signed int),
				&index[0],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[1],
				2,
				&bindvar[1],
				sizeof(signed int),
				&index[1],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[2],
				3,
				&bindvar[2],
				sizeof(signed int),
				&index[2],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[3],
				4,
				&bindvar[3],
				sizeof(signed int),
				&index[3],
				1,
				errmsg,
				256);


	param_values[0] = ret;
	param_values[1] = custid;
	param_values[2] = salary;
	param_values[3] = name;
	param_lengths[0] = strlen(ret);
	param_lengths[1] = strlen(custid);
	param_lengths[2] = strlen(salary);
	param_lengths[3] = strlen(name);

	param_formats[0] = 0;
	param_formats[1] = 0;
	param_formats[2] = 0;
	param_formats[3] = 0;
	modes[0] = modes[1] = modes[2] = modes[3] = argmode_out;
	res = IvyexecPreparedStatement(conn, stmthandle, 4, param_values, param_lengths,
				param_formats, modes, result_format, errmsg, 256);

	if (IvyresultStatus(res) != PGRES_TUPLES_OK && IvyresultStatus(res) != PGRES_COMMAND_OK &&
		IvyresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		fprintf(stderr, "PQexecPrepared statement didn't return tuples properly\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

	/* print out the attribute names */
	nFields = Ivynfields(res);
	for (i = 0; i < nFields; i++)
		printf("%-15s", Ivyfname(res, i));

	printf("\n");
	printf("bindvar[0]=%d,bindvar[1]=%d,bindvar[2]=%d,bindvar[3]=%d\n",ntohl(bindvar[0]),
		ntohl(bindvar[1]),ntohl(bindvar[2]), ntohl(bindvar[3]));
	printf("%d,%d,%d,%d\n",index[0],index[1],index[2], index[3]);
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
	printf ("again exec\n");

	res = Ivyexec(conn, "set search_path to public,sys");
	Ivyclear(res);

	/* exec agian */
	res = IvyexecPreparedStatement(conn, stmthandle, 4, param_values, param_lengths,
		param_formats, modes, result_format, errmsg, 256);
	if (IvyresultStatus(res) != PGRES_TUPLES_OK && IvyresultStatus(res) != PGRES_COMMAND_OK &&
		IvyresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		fprintf(stderr, "PQexecPrepared statement didn't return tuples properly\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

	nFields = Ivynfields(res);
	for (i = 0; i < nFields; i++)
		printf("%-15s", Ivyfname(res, i));

	printf("\n");
	printf("bindvar[0]=%d,bindvar[1]=%d,bindvar[2]=%d,bindvar[3]=%d\n",ntohl(bindvar[0]),
		ntohl(bindvar[1]),ntohl(bindvar[2]), ntohl(bindvar[3]));
	printf("%d,%d,%d,%d\n",index[0],index[1],index[2], index[3]);
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

	/* free handle */
	IvyFreePreparedStatement(stmthandle);

	/* exec declare block */
	stmthandle = IvyCreatePreparedStatement(stmt_name, block_str, 4, param_types);
	bindinfo[0] = bindinfo[1] = bindinfo[2] = bindinfo[3] = NULL;
	bindvar[0] = bindvar[1] = bindvar[2] = bindvar[3] = 0;
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[0],
				1,
				&bindvar[0],
				sizeof (signed int),
				&index[0],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[1],
				2,
				&bindvar[1],
				sizeof(signed int),
				&index[1],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[2],
				3,
				&bindvar[2],
				sizeof(signed int),
				&index[2],
				1,
				errmsg,
				256);
	IvybindOutParameterByPos(stmthandle,
				&bindinfo[3],
				4,
				&bindvar[3],
				sizeof(signed int),
				&index[3],
				1,
				errmsg,
				256);

	param_values[0] = ret;
	param_values[1] = custid;
	param_values[2] = salary;
	param_values[3] = name;
	param_lengths[0] = strlen(ret);
	param_lengths[1] = strlen(custid);
	param_lengths[2] = strlen(salary);
	param_lengths[3] = strlen(name);

	param_formats[0] = 0;
	param_formats[1] = 0;
	param_formats[2] = 0;
	param_formats[3] = 0;
	modes[0] = modes[1] = modes[2] = modes[3] = argmode_out;
	res = IvyexecPreparedStatement(conn, stmthandle, 4, param_values, param_lengths,
				param_formats, modes, 0, errmsg, 256);

	if (IvyresultStatus(res) != PGRES_TUPLES_OK && IvyresultStatus(res) != PGRES_COMMAND_OK &&
		IvyresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		fprintf(stderr, "PQexecPrepared statement didn't return tuples properly\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

	/* print out the attribute names */
	nFields = Ivynfields(res);
	for (i = 0; i < nFields; i++)
		printf("%-15s", Ivyfname(res, i));

	printf("\n");
	printf("bindvar[0]=%d,bindvar[1]=%d,bindvar[2]=%d,bindvar[3]=%d\n", bindvar[0],
		bindvar[1],bindvar[2], bindvar[3]);
	printf("%d,%d,%d,%d\n",index[0],index[1],index[2], index[3]);
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

	/* free handle */
	IvyFreePreparedStatement(stmthandle);

	/* end the transaction */
	res = Ivyexec(conn, "END");
	Ivyclear(res);

	res = Ivyexec(conn, "drop function short_proc(OUT IMAX integer, OUT IMIN integer, OUT INUL integer);");
	Ivyclear(res);

	Ivyfinish(conn);
	return 0;
}

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

char *function_str =
"declare"
"       id integer;"
"       salary integer;"
"       id1 integer;"
"       id2 varchar(256);"
"begin"
"       id := ?;"
"       ? := id * 2 + 1;"
"       ? := 'this is a test ok';"
"       ? := 23;"
"       SELECT count(1) into id FROM jsonbtest WHERE detail ?@ ?;"
"       ? := id;"
"       SELECT count(1) into id2 FROM jsonbtest WHERE detail ?@ ? = ?;"
"       ? := id2;"
"       end;";



static void
exit_nicely(Ivyconn *conn)
{
	Ivyfinish(conn);
	exit(EXIT_SUCCESS);
}

static void
exec_prepare(Ivyconn *conn)
{
	int   result_format = 0;
	Ivyresult   *res;
	const char *stmt_name = "test_stmt";
	const char *stmt = function_str;
	Oid param_types[9];
	IvyPreparedStatement *stmthandle;
	const char* param_values[9];
	int param_lengths[9];
	int param_formats[9];
	char errmsg[256];
	IvyBindOutInfo *bindinfo[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int prestmt_indp[9];
	Ivyargmode modes[9];
    int prestmt_integer1;
    int prestmt_integer2;
    char prestmt_char1[256];
    int prestmt_integer3;
    char prestmt_char2[256];
    int prestmt_integer4;
    char prestmt_char3[256];
    bool prestmt_bool=true;
    int prestmt_integer5=0;

    const char* value1 = "3";
    const char* value2 = "2000";
    const char* value3 = "welcome to beijing";
    const char* value4 = "2";
    const char* value5 = "a";
    const char* value6 = "0";
    const char* value7 = "a";
    const char* valur8 = "t";
    const char* value9 = "0";

	param_types[0] = 23 | 0xC0000000;  /* inout */
	param_types[1] = 23 | 0xC0000000;  /* inout */
	param_types[2] = 25 | 0xC0000000;  /* inout */
	param_types[3] = 23 | 0xC0000000;  /* inout */
	param_types[4] = 25 | 0xC0000000;  /* inout */
	param_types[5] = 23 | 0xC0000000;  /* inout */
	param_types[6] = 25 | 0xC0000000;  /* inout */
	param_types[7] = 16 | 0xC0000000;  /* inout */
	param_types[8] = 23 | 0xC0000000;  /* inout */
	
	stmthandle = IvyCreatePreparedStatement(stmt_name, stmt, 9, param_types);
	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[0],
								1,
								(void *) &prestmt_integer1,
								sizeof(int),
								&prestmt_indp[0],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

    if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[1],
								2,
								(void *) &prestmt_integer2,
								sizeof(int),
								&prestmt_indp[1],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}
	memset(prestmt_char1,0x00,256);
	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[2],
								3,
								(void *) prestmt_char1,
								256,
								&prestmt_indp[2],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[3],
								4,
								(void *) &prestmt_integer3,
								sizeof(int),
								&prestmt_indp[3],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

	memset(prestmt_char2,0x00,256);
	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[4],
								5,
								(void *) prestmt_char2,
								256,
								&prestmt_indp[4],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}
	
	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[5],
								6,
								(void *) &prestmt_integer4,
								sizeof(int),
								&prestmt_indp[5],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}
	memset(prestmt_char3,0x00,256);
	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[6],
								7,
								(void *) prestmt_char3,
								256,
								&prestmt_indp[6],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[7],
								8,
								(void *) &prestmt_bool,
								sizeof(bool),
								&prestmt_indp[7],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

	if (IvybindOutParameterByPos(stmthandle,
								&bindinfo[8],
								9,
								(void *) &prestmt_integer5,
								sizeof(int),
								&prestmt_indp[8],
								1,
								errmsg,
								255) == 0)
	{
		printf("%s\n",errmsg);
		return;
	}

	param_values[0] = value1;
	param_values[1] = value2;
	param_values[2] = value3;
	param_values[3] = value4;
	param_values[4] = value5;
	param_values[5] = value6;
	param_values[6] = value7;
	param_values[7] = valur8;
	param_values[8] = value9;

	param_lengths[0] = strlen(value1);
	param_lengths[1] = strlen(value2);
	param_lengths[2] = strlen(value3);
	param_lengths[3] = strlen(value4);
	param_lengths[4] = strlen(value5);
	param_lengths[5] = strlen(value6);
	param_lengths[6] = strlen(value7);
	param_lengths[7] = strlen(valur8);
	param_lengths[8] = strlen(value9);

	param_formats[0] = 0;
	param_formats[1] = 0;
	param_formats[2] = 0;
	param_formats[3] = 0;
	param_formats[4] = 0;
	param_formats[5] = 0;
	param_formats[6] = 0;
	param_formats[7] = 0;
	param_formats[8] = 0;

	modes[0] = modes[1] = modes[2] = modes[3] = modes[4] = modes[5] = modes[6] = modes[7] = modes[8] = argmode_inout;

	res = IvyexecPreparedStatement(conn, stmthandle, 9, param_values, param_lengths,
									param_formats, modes, result_format, errmsg, 256);
	if (IvyresultStatus(res) != PGRES_TUPLES_OK && IvyresultStatus(res) != PGRES_COMMAND_OK &&
		IvyresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		fprintf(stderr, "PQexecPrepared statement didn't return tuples properly\n");
		Ivyclear(res);
		exit_nicely(conn);
	}

    printf("anonymous: %d,%d,%s,%d,%s,%d,%s,%s,%d\n", prestmt_integer1, prestmt_integer2, prestmt_char1, prestmt_integer3, prestmt_char2, prestmt_integer4, prestmt_char3, prestmt_bool ? "false" : "true", prestmt_integer5);
	Ivyclear(res);
	IvyFreePreparedStatement(stmthandle);
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
	res = Ivyexec(conn,function_str);
	Ivyclear(res);

	res = Ivyexec(conn,"create table jsonbtest(detail jsonb)");
	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "create table jsonbtest failed\n");
		Ivyclear(res);
	    Ivyfinish(conn);
	    Ivyfinish(conn);
	    exit(EXIT_SUCCESS);
	}
	Ivyclear(res);

	res = Ivyexec(conn,"INSERT INTO jsonbtest (detail) VALUES ('{\"a\": 1}');");
	if (IvyresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "insert into table jsonbtest failed\n");
		Ivyclear(res);
	    Ivyfinish(conn);
	    Ivyfinish(conn);
	    exit(EXIT_SUCCESS);
	}
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

	exec_prepare(conn);
	/* end the transaction */
	res = Ivyexec(conn, "END");
	Ivyclear(res);

	res = Ivyexec(conn, "drop table jsonbtest;");
	Ivyclear(res);

	Ivyfinish(conn);
	return 0;
}

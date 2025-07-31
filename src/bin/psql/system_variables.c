/*-------------------------------------------------------------------------
 *
 * File: system_variables.c
 *
 * Abstract:
 *		compatible Oracle SQL*PLUS system variables.
 *
 * Authored by xuzhuang@highgo.com, 20230304.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/bin/psql/system_variables.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "common/logging.h"
#include "fe_utils/psqlscan_int.h"


/* range array of datatype integer */
int64 system_variables_int[][3] =
{
	/* {disable, minimum, maximum} */
	{-1, 2, 128}
};

static struct system_variable_data system_variables[] =
{
	{
		"NUMWIDTH",
		SVT_INTEGER,
		{-1},			/* XXX disable, not allowed int SQL*PLUS */
		3,				/* must be greater than 0 */
		(void *) &system_variables_int[0][0]
	},

	/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
	{
		"serveroutput",
		SVT_COMPOSITE,
		{0},
		9,				/* must be greater than 0 */
		NULL
	}
	/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */
};

system_variable
GetSysVariable(const char *name)
{
	int	nb_sys_vars = sizeof(system_variables) / sizeof(system_variable_data),
		i = 0,
		len = (int) strlen(name);

	for (; i < nb_sys_vars; ++i)
	{
		/*
		 * matching system variable's name
		 * with a length of at least max(strlen(name), min_name_len)
		 */
		Assert(system_variables[i].min_name_len > 0);
		if (len < system_variables[i].min_name_len)
			continue;
		else if (pg_strncasecmp(name, system_variables[i].name, len) == 0)
			return &system_variables[i];
	}

	/* or not */
	return NULL;
}

bool
ShowSysVariable(sys_var_list *list)
{
	int	i;

	for (i = 0; i < list->length; ++i)
	{
		sys_var		   *sv = list->sysvars[i];
		system_variable	var = GetSysVariable(sv->sv_name);

		/*
		 * none system variable found
		 * XXX no error message here, because it may be a "VariableShowStmt"
		 */
		if (var == NULL)
			return false;

		/* system variable found */
		switch (var->type)
		{
			case SVT_BOOL:
				if (var->value.on)
					printf("%s ON\n", var->name);
				else
					printf("%s OFF\n", var->name);
				break;
			case SVT_INTEGER:
				printf("%s %ld\n", var->name, var->value.ival);
				break;
			case SVT_STRING:
				printf("%s %s\n", var->name, var->value.string);
				break;
			/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
			case SVT_COMPOSITE:
				{
					if (var->value.string)
					{
						csv_node	*csv = (csv_node *) var->value.string;

						switch(csv->tag)
						{
							case CSV_SERVEROUTPUT:
								{
									csv_serveroutput_node	*val = (csv_serveroutput_node *) var->value.string;

									if (val->enable)
									{
										if (val->size == SV_SERVEROUT_BUFSIZE_UNLIMITED)
											printf("%s ON SIZE UNLIMITED\n", var->name);
										else
											printf("%s ON SIZE %d\n", var->name, val->size);
									}
									else
										printf("%s OFF\n", var->name);
									
								}
								break;
							default:
								pg_log_error("Invalid composite system varibale type.");
						}
					}
					else
					{
						/* serveroutput default is off */
						if (pg_strcasecmp(var->name, "serveroutput") == 0)
							printf("%s OFF\n", var->name);
						/* Followed by else if for other SVT_COMPOSITE */
					}
				}
				break;
			/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */
			default:
				/* because var is the one found */
				Assert(SVT_NONE < var->type && var->type <= SVT_STRING);
				break;
		}
	}

	return true;
}

bool
SetSysVariable(sys_var_list *list, bool *serveroutput_set /* ReqID:SRS-PLSQL-DBMS_OUTPUT */)
{
	int	i;

	for (i = 0 ; i < list->length; ++i)
	{
		sys_var		   *sv = list->sysvars[i];
		system_variable	var = GetSysVariable(sv->sv_name);

		/*
		 * none system variable found
		 * XXX no error message here, because it may be a "VariableSetStmt"
		 */
		if (var == NULL)
			return false;
		/* maybe, set system variable with an unexpected datatype */
		else if (var->type != sv->sv_type)
			continue;

		switch (sv->sv_type)
		{
			case SVT_BOOL:
				var->value.on = sv->sv_value.on;
				break;
			case SVT_INTEGER:
				/* {disable, minimum, maximum} presets are necessary */
				Assert(var->extra != NULL &&
						((int64 *) var->extra)[0] < ((int64 *) var->extra)[1] &&
						((int64 *) var->extra)[1] < ((int64 *) var->extra)[2]);

				/* value out of range */
				if (sv->sv_value.ival != ((int64 *) var->extra)[0] &&
						(((int64 *) var->extra)[1] > sv->sv_value.ival ||
							sv->sv_value.ival > ((int64 *) var->extra)[2]))
				{
					pg_log_error("%s option %ld out of range (%ld through %ld)",
								 var->name, sv->sv_value.ival,
								 ((int64 *) var->extra)[1],
								 ((int64 *) var->extra)[2]);
					return true;
				}

				var->value.ival = sv->sv_value.ival;
				break;
			case SVT_STRING:
				var->value.string = pg_strdup(sv->sv_value.string);
				break;
			/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
			case SVT_COMPOSITE:
				{
					csv_node	*csv = (csv_node *) sv->sv_value.string;

					switch(csv->tag)
					{
						case CSV_SERVEROUTPUT:
						{
							csv_serveroutput_node	*val = (csv_serveroutput_node *) sv->sv_value.string;
							csv_serveroutput_node	*new;

							/*
							 * psqlplusparse.y ensures that val->size < 0 in only two cases,
							 * unlimited and without SIZE option.
							 */
							if (val->size >= 0 &&
								(val->size < SV_SERVEROUT_BUFSIZE_MIN ||
								val->size > SV_SERVEROUT_BUFSIZE_MAX))
							{
								pg_log_error("%s option %d out of range (%d through %d)",
											var->name, val->size, SV_SERVEROUT_BUFSIZE_MIN, SV_SERVEROUT_BUFSIZE_MAX);
								if(serveroutput_set)
									*serveroutput_set = false;
								return true;
							}
							new = (csv_serveroutput_node *) pg_malloc0(sizeof(csv_serveroutput_node));
							new->csv_tag = val->csv_tag;
							new->enable = val->enable;

							/*
							 * Compatible Oracle:
							 * if the SIZE option is omitted then use the previous
							 * SET SERVEROUTPUT ON SIZE n or uses default values if
							 * no SET SERVEROUTPUT command was previously issued in
							 * the current connection.
							 */
							if (val->size != SV_SERVEROUT_NO_SIZE_OPTION)	/* exist SIZE option */
								new->size = val->size;
							else
							{
								/* first time to SET SERVEROUTPUT command in the current connection */
								if (!var->value.string)
									new->size = SV_SERVEROUT_BUFSIZE_UNLIMITED;
								else
									/* keep old value of serveroutput size */
									new->size = ((csv_serveroutput_node *) var->value.string)->size;
							}

							if (var->value.string)
								pfree(var->value.string);

							var->value.string = (char *) new;
							pfree(sv->sv_value.string);
							if(serveroutput_set)
								*serveroutput_set = true;
						}
						break;
						default:
							pg_log_error("Invalid composite system varibale type.");
							return true;
					}
				}
				break;
			/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */
			default:
				/* the type of system variable's value must be specified */
				Assert(SVT_NONE < sv->sv_type && sv->sv_type <= SVT_STRING);
		}
	}

	return true;
}
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
	}
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
			default:
				/* because var is the one found */
				Assert(SVT_NONE < var->type && var->type <= SVT_STRING);
				break;
		}
	}

	return true;
}

bool
SetSysVariable(sys_var_list *list)
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
			default:
				/* the type of system variable's value must be specified */
				Assert(SVT_NONE < sv->sv_type && sv->sv_type <= SVT_STRING);
		}
	}

	return true;
}

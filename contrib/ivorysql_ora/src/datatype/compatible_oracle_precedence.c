/*---------------------------------------------------------------------
 * 
 * File: compatible_oracle_precedence.c 
 *
 * Abstract: 
 * 		Compatible with Oracle data type precedence.
 *
 * Authored by zhenmingyang@highgo.com, 20220509.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/datatype/compatible_oracle_precedence.c
 *
 *---------------------------------------------------------------------
 */

#include "postgres.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "utils/lsyscache.h"	/* BUG:0001052 */

#include "../include/ivorysql_ora.h"

/* OracleDataTypePriority array size */
#define	ORACLE_DATATYPE_NUM	28

/*
 * Oracle data type precedence array.
 * Note that the elements of this array must be sorted in
 * the order of OID value.Therefore, we using the OID of a 
 * certain data type - 9000 is the subscript of the array.
 */
static const int OracleDataTypePriority[ORACLE_DATATYPE_NUM][2] =
{
	{ORACHARCHAROID, 1},		/* 9000 */
	{ORACHARBYTEOID, 1},		/* 9001 */
	{ORAVARCHARCHAROID, 2},		/* 9002 */
	{ORAVARCHARBYTEOID, 2},		/* 9003 */
	{InvalidOid, 0},			/* 9004 - ORACHARCHARARRAYOID */
	{InvalidOid, 0},			/* 9005 - ORACHARBYTEARRAYOID */
	{InvalidOid, 0},			/* 9006 - ORAVARCHARCHARARRAYOID */
	{InvalidOid, 0},			/* 9007 - ORAVARCHARBYTEARRAYOID */
	{ORADATEOID, 6},			/* 9008 */
	{InvalidOid, 0},			/* 9009 - ORADATEARRAYOID */
	{ORATIMESTAMPOID, 6},		/* 9010 */
	{InvalidOid, 0},			/* 9011 - ORATIMESTAMPARRAYOID */
	{ORATIMESTAMPTZOID, 6},		/* 9012 */
	{InvalidOid, 0},			/* 9013 - ORATIMESTAMPTZARRAYOID */
	{ORATIMESTAMPLTZOID, 6},	/* 9014 */
	{InvalidOid, 0},			/* 9015 - ORATIMESTAMPLTZARRAYOID */
	{YMINTERVALOID, 6},			/* 9016 */
	{InvalidOid, 0},			/* 9017 - YMINTERVALARRAYOID */
	{DSINTERVALOID, 6},			/* 9018 */
	{InvalidOid, 0},			/* 9019 - DSINTERVALARRAYOID */
	{NUMBEROID, 3},				/* 9020 */
	{InvalidOid, 0},			/* 9021 - NUMBERARRAYOID */
	{BINARY_FLOATOID, 4},		/* 9022 */
	{InvalidOid, 0},			/* 9023 - BINARY_FLOATARRAYOID */
	{BINARY_DOUBLEOID, 5},		/* 9024 */
	{InvalidOid, 0},			/* 9025 - BINARY_DOUBLEARRAYOID */
	{InvalidOid, 0},			/* Used in the future */
	{InvalidOid, 0}				/* Used in the future */
};


/*
 * pg_compatible_oracle_precedence
 *
 * The function is used to compatible with the Oracle data type precedence , 
 * It is the caller's responsibility to ensure that opname_p is not NULL. 
 *
 * When OpernameGetOprid cannot accurately find an operator, this hook provides
 * an opportunity to change the data type of the left and right operands and search
 * again. If the change is successful, it returns true, otherwise it returns false.
 */
bool
pg_compatible_oracle_precedence(Oid arg1, Oid arg2, char *opname_p, Oid *result_arg1, Oid *result_arg2)
{
	bool	arg1_is_oracle = false;
	bool	arg2_is_oracle = false;
	int		arg1_precedence = 0;
	int		arg2_precedence = 0;
	
	/* 
	 * Compatible Oracle text literals properties.
	 * In Postgresql text literals 'abc' is UNKNOWN data type.
	 * In Oracle text literals 'abc' is CHAR data type , except 
	 *  that maximum length is 4000 bytes. 
	 */
	if (arg1 == UNKNOWNOID)
		arg1 = ORACHARCHAROID;
	if (arg2 == UNKNOWNOID)
		arg2 = ORACHARCHAROID;
	
	/* Begin - BUGID:0001052 */
	/* If arg1 or arg2 is a domain, reduce them to their base types */
	arg1 = getBaseType(arg1);
	arg2 = getBaseType(arg2);
	/* End - BUGID:0001052 */

	/* Begin - BUG#M0000296 */
	if (IS_ORACLE_TYPE(arg1))
	{
		int	idx = arg1 - ORACLE_TYPE_OID_INITIAL;

		/* Avoid invalid reads of array and exclude array types */
		arg1_is_oracle = (idx < ORACLE_DATATYPE_NUM && arg1 == (Oid)OracleDataTypePriority[idx][0]) ? true : false;
	}

	if (IS_ORACLE_TYPE(arg2))
	{
		int	idx = arg2 - ORACLE_TYPE_OID_INITIAL;

		/* Avoid invalid reads of array and exclude array types */
		arg2_is_oracle = (idx < ORACLE_DATATYPE_NUM && arg2 == (Oid)OracleDataTypePriority[idx][0]) ? true : false;
	}
	/* End - BUG#M0000296 */
	
	/* Compatible Oracle
	 * If the left operand or right operand is compatible with oracle's data type and 
	 * and the other operand is the data type of POSTGREQSL. We convert to compatible 
	 * oracle data types.
	 */
	if (arg1_is_oracle && arg2_is_oracle)
	{
		/* Both arg1 and arg2 are Oracle data type, do nothing */
	}
	else if (arg1_is_oracle)
	{
		switch(arg2)
		{
			case BPCHAROID:
				arg2 = ORACHARCHAROID;
				break;
				
			case VARCHAROID:
				arg2 = ORAVARCHARCHAROID;
				break;
				
			case NUMERICOID:
			case INT2OID:
			case INT4OID:
			case INT8OID:
				arg2 = NUMBEROID;
				break;
				
			case FLOAT4OID:
				arg2 = BINARY_FLOATOID;
				break;
				
			case FLOAT8OID:
				arg2 = BINARY_DOUBLEOID;
				break;
				
			case DATEOID:
				arg2 = ORADATEOID;
				break;
				
			case TIMESTAMPOID:
				arg2 = ORATIMESTAMPOID;
				break;
				
			case TIMESTAMPTZOID:
				arg2 = ORATIMESTAMPTZOID;
				break;

			default:
				break;	/* do nothing */
		}
	}
	else if (arg2_is_oracle)
	{
		switch(arg1)
		{
			case BPCHAROID:
				arg1 = ORACHARCHAROID;
				break;
				
			case VARCHAROID:
				arg1 = ORAVARCHARCHAROID;
				break;
				
			case NUMERICOID:
			case INT2OID:
			case INT4OID:
			case INT8OID:
				arg1 = NUMBEROID;
				break;
				
			case FLOAT4OID:
				arg1 = BINARY_FLOATOID;
				break;
				
			case FLOAT8OID:
				arg1 = BINARY_DOUBLEOID;
				break;
				
			case DATEOID:
				arg1 = ORADATEOID;
				break;
				
			case TIMESTAMPOID:
				arg1 = ORATIMESTAMPOID;
				break;
				
			case TIMESTAMPTZOID:
				arg1 = ORATIMESTAMPTZOID;
				break;

			default:
				break;	/* do nothing */
		}
	}
	else
	{
		/* Both arg1 and arg2 are Postgresql data type, return false */
		return false;
	}

	/* Begin - BUG#M0000296 */
	/*
	 * Security check, a little paranoid?
	 * Avoid not being in array OracleDataTypePriority after
	 * converting from postgres data type to oracle data type.
	 */
	if (IS_ORACLE_TYPE(arg1))
	{
		int	idx = arg1 - ORACLE_TYPE_OID_INITIAL;

		/* Avoid invalid reads of array and exclude array types */
		arg1_is_oracle = (idx < ORACLE_DATATYPE_NUM && arg1 == (Oid)OracleDataTypePriority[idx][0]) ? true : false;
	}

	if (IS_ORACLE_TYPE(arg2))
	{
		int	idx = arg2 - ORACLE_TYPE_OID_INITIAL;

		/* Avoid invalid reads of array and exclude array types */
		arg2_is_oracle = (idx < ORACLE_DATATYPE_NUM && arg2 == (Oid)OracleDataTypePriority[idx][0]) ? true : false;
	}
	/* End - BUG#M0000296 */

	if (arg1_is_oracle && arg2_is_oracle)
	{
		/* Begin - BUG#M0000296 */
		arg1_precedence = OracleDataTypePriority[arg1 - ORACLE_TYPE_OID_INITIAL][1];
		arg2_precedence = OracleDataTypePriority[arg2 - ORACLE_TYPE_OID_INITIAL][1];
		/* End - BUG#M0000296 */
		
		/* Compatible Oracle implicit conversion
		 * In arithmetic operations between CHAR/VARCHAR2 and NCHAR/NVARCHAR2,  
		 * Oracle converts CHAR/VARCHAR2 to a NUMBER.
		 */
		if ((arg1_precedence == 1 || arg1_precedence == 2) &&
			(arg2_precedence == 1 || arg2_precedence == 2) &&
			(strcmp(opname_p, "+") == 0 ||
			 strcmp(opname_p, "-") == 0 ||
			 strcmp(opname_p, "*") == 0 ||
			 strcmp(opname_p, "/") == 0))
		{
			*result_arg1 = NUMBEROID;
			*result_arg2 = NUMBEROID;
			return true;
		}
		
		/* Compatible Oracle 
		 * If the operational is Datetime/Interval Arithmetic,  
		 * in this case, we do not adhere to the priority policy. 
		 */
		if ((arg1_precedence == 6 && 
			 (arg2_precedence == 3 || 
			  arg2_precedence == 4 || 
			  arg2_precedence == 5)) &&
			(strcmp(opname_p, "+") == 0 ||
			 strcmp(opname_p, "-") == 0 ||
			 strcmp(opname_p, "*") == 0 ||
			 strcmp(opname_p, "/") == 0))
		{
			/* Compatible Oracle 
			 * Oracle implicitly converts BINARY_FLOAT and BINARY_DOUBLE 
			 * operands to NUMBER in Datetime/Interval Arithmetic.   
			 */
			if (arg2_precedence == 4 || arg2_precedence == 5)
			{
				*result_arg2 = NUMBEROID;
				*result_arg1 = arg1;
				return true;
			}
			else
			{
				*result_arg2 = arg2;
				*result_arg1 = arg1;
				return true;
			}	
		}
		else if ((arg2_precedence == 6 && 
				 (arg1_precedence == 3 || 
				  arg1_precedence == 4 || 
				  arg1_precedence == 5)) &&
				(strcmp(opname_p, "+") == 0 ||
				 strcmp(opname_p, "-") == 0 ||
				 strcmp(opname_p, "*") == 0 ||
				 strcmp(opname_p, "/") == 0))
		{
			/* Compatible Oracle 
			 * Oracle implicitly converts BINARY_FLOAT and BINARY_DOUBLE 
			 * operands to NUMBER in Datetime/Interval Arithmetic.   
			 */
			if (arg1_precedence == 4 || arg1_precedence == 5)
			{
				*result_arg1 = NUMBEROID;
				*result_arg2 = arg2;
				return true;
			}
			else
			{
				*result_arg2 = arg2;
				*result_arg1 = arg1;
				return true;
			}	
		}		
		
		if (arg1_precedence == arg2_precedence)
		{
			*result_arg1 = arg1;
			*result_arg2 = arg2;
		}
		else if (arg1_precedence > arg2_precedence)
		{
			*result_arg1 = arg1;
			*result_arg2 = arg1;
		}
		else
		{
			*result_arg1 = arg2;
			*result_arg2 = arg2;
		}

		/* Begin - BUG#M0000234 */
		/*
		 * Compatible with Oracle's concatenation operator ||,this operatoris in
		 * SYS mode and should have only one. The left and right operands are of
		 * VARCHAR2 type. All Oracle-compatible data types use this unique || operator.
		 *
		 * This || operator may also be used when mixing Oracle data types and postgres
		 * data types. Because we may implicitly convert certain postgres data types to
		 * oracle data types.
		 */
		if (strcmp(opname_p, "||") == 0)
		{
			*result_arg1 = ORAVARCHARCHAROID;
			*result_arg2 = ORAVARCHARCHAROID;
			return true;
		}
		/* End - BUG#M0000234 */
	}
	else
	{
		/* 
		 * Begin - Bug#Z471 & SRS-MISC-COMPATDM-SQL
		 * Cause text if a prefered datatype,like operator tend to find text like text,
		 * So we need to let it go through oravarbyte like oravarbyte.
		 */
		/* This is for compate DM in situation orachar || other. */
		if(strcmp(opname_p,"||") == 0 && (arg1 == ORACHARCHAROID || arg1 == ORACHARBYTEOID || arg2 == ORACHARCHAROID || arg2 == ORACHARBYTEOID))
		{
			*result_arg1 = ORAVARCHARCHAROID;
			*result_arg2 = ORAVARCHARCHAROID;
			PG_RETURN_BOOL(true);
		}

		if(strcmp(opname_p,"~~") == 0 || strcmp(opname_p,"!~~") == 0)
		{
			if(arg1 == ORACHARCHAROID || arg1 == ORACHARBYTEOID)
			{
				*result_arg1 = ORAVARCHARBYTEOID;
				*result_arg2 = ORAVARCHARBYTEOID;
				PG_RETURN_BOOL(true);
			}
		}
		/* End - Bug#Z471 & SRS-MISC-COMPATDM-SQL */
		
		/* arg1 and arg2 is postgresql data type */
		*result_arg1 = InvalidOid;
		*result_arg2 = InvalidOid;
		return false;
	}

	return true;
}



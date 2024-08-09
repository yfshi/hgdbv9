#ifdef nlssort_0
Datum 
nlssort_0(PG_FUNCTION_ARGS) 
{
    /**
     * SELECT nlssort() FROM dual;                  ==> ORA-00938: not enough arguments for function
     * SELECT * FROM tbl order by nlssort();        ==> the same as `SELECT * FROM tbl' and 'SELECT * FROM tbl order by null`
     * SELECT * FROM tbl order by length(nlssort()) ==> the same as `SELECT * FROM tbl order by length(nlssort(NULL))`
     *
     * Call `nlssort` without any argument in `order by` statement is legal, other statement is illegal
     * we realize it in function `sortClause_filter` on ora_gram.y
     */
    ereport(ERROR,
		 errmsg("IVY-00938: not enough arguments for function"));
    PG_RETURN_NULL();
}
#endif

#ifdef nlssort_1
Datum
nlssort_1(PG_FUNCTION_ARGS) 
{
    int32_t len;
    char    *src = NULL;
    char    *collname = NULL;
    bytea   *result = NULL;

    /**
     * SELECT nlssort(null) FROM dual; ==> nlssort() will return NULL
     */
    if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

    #if   defined (NLS_INPUT_IS_TEXT)
        src = VARDATA_ANY(PG_GETARG_TEXT_P(0));
        len = VARSIZE_ANY_EXHDR(PG_GETARG_TEXT_P(0));
    #elif defined (NLS_INPUT_IS_CSTRING)
        src = PG_GETARG_CSTRING(0);
        len = strlen(src);
    #elif defined (NLS_INPUT_IS_RAW)
        src = rawout(PG_GETARG_TEXT_P(0));
        len = strlen(src);
    #endif
    
    if(len == 0)
        PG_RETURN_NULL();

    collname = nls_sort_str;

    #if   defined (NLS_NEED_TRIM)
        result = nlssort_action_choose(src, len, "NLS_SORT", collname, true);
    #elif defined (NLS_NOT_TRIM)
        result = nlssort_action_choose(src, len, "NLS_SORT", collname, false);
    #endif

    #if defined (NLS_INPUT_IS_RAW)        
        pfree(src);
    #endif

    PG_RETURN_BYTEA_P(result);
}
#endif

#ifdef nlssort_2
Datum 
nlssort_2(PG_FUNCTION_ARGS) 
{
    int32_t len;
    char    *src = NULL;
    char    *nlssort_arg = NULL;
    List    *l = NULL;
    bytea   *result = NULL;

    #if   defined (NLS_INPUT_IS_TEXT)
        src = VARDATA_ANY(PG_GETARG_TEXT_P(0));
        len = VARSIZE_ANY_EXHDR(PG_GETARG_TEXT_P(0));
    #elif defined (NLS_INPUT_IS_CSTRING)
        src = PG_GETARG_CSTRING(0);
        len = strlen(src);
    #elif defined (NLS_INPUT_IS_RAW)
        src = rawout(PG_GETARG_TEXT_P(0));
        len = strlen(src);
    #endif
    
    if(len == 0)
        PG_RETURN_NULL();

    nlssort_arg = pstrdup(PG_GETARG_CSTRING(1));
    
    if (!SplitIdentifierString(nlssort_arg, '=', &l) || list_length(l) != 2) {
        list_free(l);
        pfree(nlssort_arg);
        ereport(ERROR,
            errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));
    }

    #if   defined (NLS_NEED_TRIM)
        result = nlssort_action_choose(src, len, (char *)linitial(l), (char *)lsecond(l), true);
    #elif defined (NLS_NOT_TRIM)
        result = nlssort_action_choose(src, len, (char *)linitial(l), (char *)lsecond(l), false);
    #endif

    #if defined (NLS_INPUT_IS_RAW)        
        pfree(src);
    #endif

    PG_RETURN_BYTEA_P(result);
}
#endif

#ifdef nlssort_0
#undef nlssort_0
#endif

#ifdef nlssort_1
#undef nlssort_1
#endif

#ifdef nlssort_2
#undef nlssort_2
#endif

#ifdef NLS_NOT_TRIM
#undef NLS_NOT_TRIM
#endif

#ifdef NLS_NEED_TRIM
#undef NLS_NEED_TRIM
#endif

#ifdef NLS_INPUT_IS_TEXT
#undef NLS_INPUT_IS_TEXT
#endif

#ifdef NLS_INPUT_IS_RAW
#undef NLS_INPUT_IS_RAW
#endif

#ifdef NLS_INPUT_IS_CSTRING
#undef NLS_INPUT_IS_CSTRING
#endif

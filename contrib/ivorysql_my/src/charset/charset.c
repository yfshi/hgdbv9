#include "postgres.h"

#include "catalog/pg_collation.h"
#include "catalog/pg_namespace.h"
#include "ctype.h"
#include "funcapi.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "regex/regex.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"

#define SHOW_CHARSET_RESULTSET_NATTR        4
#define SHOW_COLLATION_RESULTSET_NATTR      6

PG_FUNCTION_INFO_V1(show_mysql_charsets);
PG_FUNCTION_INFO_V1(show_mysql_collations);
/* Begin - BUG#7845 */
PG_FUNCTION_INFO_V1(mysql_text_regexeq);
PG_FUNCTION_INFO_V1(mysql_text_regexne);
/* End - BUG#7845 */
typedef struct {
    Oid              coll;
    CHARSET_INFO    *cs;
} MysqlCharsetInfoHashEntry;

static List *mys_charset_list;
static HTAB *MysqlCharsetInfoHash;

static CHARSET_INFO *all_cs[] = {
    &my_charset_big5_chinese_ci,                // 1
    &my_charset_koi8r_general_ci,               // 7
    &my_charset_latin1,                         // 8
    &my_charset_latin2_general_ci,              // 9
    &my_charset_ascii_general_ci,               // 11
    &my_charset_ujis_japanese_ci,               // 12
    &my_charset_sjis_japanese_ci,               // 13
    &my_charset_hebrew_general_ci,              // 16
    &my_charset_euckr_korean_ci,                // 19
    &my_charset_koi8u_general_ci,               // 22
    &my_charset_gb2312_chinese_ci,              // 24
    &my_charset_greek_general_ci,               // 25
    &my_charset_cp1250_general_ci,              // 26
    &my_charset_gbk_chinese_ci,                 // 28
    &my_charset_latin5_turkish_ci,              // 30
    &my_charset_latin1_german2_ci,              // 31
    &my_charset_utf8_general_ci,                // 33
    &my_charset_utf8mb3_general_ci,             // 33
    &my_charset_cp866_general_ci,               // 36
    &my_charset_latin7_general_ci,              // 41
    &my_charset_utf8mb4_general_ci,             // 45
    &my_charset_utf8mb4_bin,                    // 46
    &my_charset_latin1_bin,                     // 47
    &my_charset_cp1251_bin,                     // 50
    &my_charset_cp1251_general_ci,              // 51
    &my_charset_cp1256_general_ci,              // 57
    &my_charset_cp1257_bin,                     // 58
    &my_charset_cp1257_general_ci,              // 59
    &my_charset_bin,                            // 63
    &my_charset_ascii_bin,                      // 65
    &my_charset_cp1250_bin,                     // 66
    &my_charset_cp1256_bin,                     // 67
    &my_charset_cp866_bin,                      // 68
    &my_charset_greek_bin,                      // 70
    &my_charset_hebrew_bin,                     // 71
    &my_charset_koi8r_bin,                      // 74
    &my_charset_koi8u_bin,                      // 75
    &my_charset_utf8_tolower_ci,                // 76
    &my_charset_utf8mb3_tolower_ci,             // 76
    &my_charset_latin2_bin,                     // 77
    &my_charset_latin5_bin,                     // 78
    &my_charset_latin7_bin,                     // 79
    &my_charset_utf8_bin,                       // 83
    &my_charset_utf8mb3_bin,                    // 83
    &my_charset_big5_bin,                       // 84
    &my_charset_euckr_bin,                      // 85
    &my_charset_gb2312_bin,                     // 86
    &my_charset_gbk_bin,                        // 87
    &my_charset_sjis_bin,                       // 88
    &my_charset_ujis_bin,                       // 91
    &my_charset_cp932_japanese_ci,              // 95
    &my_charset_cp932_bin,                      // 96
    &my_charset_eucjpms_japanese_ci,            // 97
    &my_charset_eucjpms_bin,                    // 98
    &my_charset_utf8_unicode_ci,                // 192
    &my_charset_utf8mb3_unicode_ci,             // 192
    &my_charset_utf8_esperanto_uca_ci,          // 209
    &my_charset_utf8_general_mysql500_ci,       // 223
    &my_charset_utf8mb3_general_mysql500_ci,    // 223
    &my_charset_utf8mb4_unicode_ci,             // 224
    &my_charset_gb18030_chinese_ci,             // 248
    &my_charset_gb18030_bin,                    // 249
    &my_charset_gb18030_unicode_520_ci,         // 250
    &my_charset_utf8mb4_0900_ai_ci,             // 255
};

static bool
has_suffix(const char *str, const char *suffix)
{
    int str_len = strlen(str);
    int suf_len = strlen(suffix);

    return memcmp(str + str_len - suf_len, suffix, suf_len) == 0;
}

static inline void
add_compiled_collation(const CHARSET_INFO *cs)
{
    int          collencoding = pg_valid_server_encoding(cs->csname);
    Oid          objectOwner = get_namespace_owner(PG_CATALOG_NAMESPACE);
    const char  *collname = cs->coll_name;
    const char  *collcollate;

    /* Begin - BUG#7845 */
    if (strcmp(cs->csname, "binary") == 0)
    {
        collencoding = -1;
    }
    /* End - BUG#7845 */

    // Encoding is -1 or collate name is end of '_bin'
    if (collencoding == -1 || has_suffix(collname, "_bin"))
        collcollate = "C";
    else 
        collcollate = collname;
    
    CollationCreate(collname,             /* collname             */
                    PG_CATALOG_NAMESPACE, /* collnamespace        */
                    objectOwner,          /* collowner            */
                    COLLPROVIDER_MYSQL,   /* collprovider         */
                    true,                 /* collisdeterministic  */
                    collencoding,         /* collencoding         */
                    collcollate,          /* collcollate          */
                    collname,             /* collctype            */ /* must be same as collname, bug#8474 */
                    "5.7",                /* collversion          */
                    true,                 /* if_not_exists        */
                    true);                /* quiet                */
}

/* Begin - SRS-MYSQL-CHARSET-INTRODUCER */
/*
 * Search MySQL CHARSET_INFO FROM ALL_CS by MySQL collation ID.
 *
 * cs_number is CHARSET_INFO.number, not pg_collation OID.
 */
CHARSET_INFO *
search_charset_info_by_cs_number(int cs_number)
{
    CHARSET_INFO *charset_info = NULL;
    /*
     * Search MySQL static CHARSET_INFO table directly.
     */
    for (int i = 0; i < lengthof(all_cs); i++)
    {
        if (all_cs[i]->number == cs_number)
        {
            charset_info = all_cs[i];
            break;
        }
    }

    if (charset_info == NULL)
    {
        ereport(ERROR,
                errmsg("Cannot found MySQL collation(%d)", cs_number));
    }

    return charset_info;
}
/* End - SRS-MYSQL-CHARSET-INTRODUCER */

static CHARSET_INFO *
search_charset_info_by_coll_id(Oid coll, bool missing_ok)
{
    char *collname = NULL;
    char *collctype = NULL; /* bug#8474 */
    MysqlCharsetInfoHashEntry *entry;

    if (MysqlCharsetInfoHash == NULL)
    {
        HASHCTL		hash_ctl;

	    hash_ctl.entrysize = sizeof(MysqlCharsetInfoHashEntry);
        hash_ctl.keysize = sizeof(int32);

        MysqlCharsetInfoHash = hash_create("Mysql Collation", 100, &hash_ctl,
                                            HASH_ELEM | HASH_BLOBS);
    }

    entry = hash_search(MysqlCharsetInfoHash, &coll, HASH_FIND, NULL);
    collname = get_collation_name(coll);
    
    /* Begin - bug#8474 */
    collctype = get_collation_ctype(coll);

    if (!collctype)
    {
        ereport(ERROR,
                errmsg("collctype of MySQL collation(%s) cannot be NULL", collname));
    }
    /* End - bug#8474 */

    if (collname && entry == NULL)
    {
        /**
         * Begin - bug#8474
         * If the collname and collctype of the are not equal, we consider
         * this collation is created with `CREATE COLLATION FROM`, based on
         * the collation, whose collname is the same as collctype.
         */
        if (strcmp(collname, collctype))
        {
            pfree(collname);
            collname = collctype;
            collctype  = NULL;	/* OA-2721 */
        }
        /* End - bug#8474 */
        
        entry = hash_search(MysqlCharsetInfoHash, &coll, HASH_ENTER, NULL);
        entry->cs = NULL;

        for (int i = 0; i < lengthof(all_cs); i++)
        {
            if (!strcmp(collname, all_cs[i]->coll_name))
            {
                entry->cs = all_cs[i];
                break;
            }
        }

        entry = hash_search(MysqlCharsetInfoHash, &coll, HASH_FIND, NULL);
    }

    assert(entry);

	if (entry->cs == NULL)
    {
        if (!missing_ok)
            ereport(ERROR,
                    errmsg("The collation \"%s\" is not supported in MySQL mode.", collname));
    }

	if (collname)
		pfree(collname);
	/* Begin - OA-2721 */
	if (collctype)
		pfree(collctype);
	/* End - OA-2721 */

    return entry->cs;
}

int
do_mysql_strcoll(unsigned char *arg1, int len1, unsigned char *arg2, int len2, Oid coll, bool trim)
{
    CHARSET_INFO   *cs = search_charset_info_by_coll_id(coll, false);

    return cs->coll->strnncoll(cs, arg1, len1, arg2, len2, trim);
}

size_t
do_mysql_strnxfrm(unsigned char *dst, size_t dstlen, uint32 nweights,
						 const unsigned char *src, size_t srclen,
						 uint32 flags, Oid coll)
{
	CHARSET_INFO   *cs = search_charset_info_by_coll_id(coll, false);

	return cs->coll->strnxfrm(cs, dst, dstlen, nweights, src, srclen, flags);
}

size_t
do_mysql_strnxfrmlen(size_t srclen, uint32 nweights, Oid coll)
{
	CHARSET_INFO	*cs = search_charset_info_by_coll_id(coll, false);

	return cs->coll->strnxfrmlen(cs, Max(srclen, nweights) * cs->mbmaxlen);
}

Datum
do_mysql_hash(const unsigned char *key, int32_t len, Oid coll)
{
    uint64          nr1 = 1;
	uint64          nr2 = 4;
    CHARSET_INFO   *cs = search_charset_info_by_coll_id(coll, false);

    assert(cs);

    cs->coll->hash_sort(cs, key, len, &nr1, &nr2);

    return UInt32GetDatum((uint32) nr1);
}

void
do_import_mysql_collation()
{
    for (int i = 0; i < lengthof(all_cs); i++)
    {
        add_compiled_collation(all_cs[i]);
    }
}

static void
show_charset_first_call(PG_FUNCTION_ARGS)
{
    FuncCallContext  *fctx = SRF_FIRSTCALL_INIT();
    MemoryContext     oldContext = MemoryContextSwitchTo(fctx->multi_call_memory_ctx);
	TupleDesc         tupdesc = CreateTemplateTupleDesc(SHOW_CHARSET_RESULTSET_NATTR);

    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "Charset", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 2, "Description", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 3, "Default collation", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 4, "Maxlen", INT8OID, -1, 0);
    
    /**
     * The list will be free before doing next function call.
     */
    mys_charset_list = NIL;

    /**
     * Show mysql supported charset only.
     * mys_charset_list will be free when first call.
     */
    for (int i = 0; i < pg_encname_tbl_size; i++)
    {
        const pg_encname *charset = &pg_encname_tbl[i];

        if (charset->description && charset->default_collation)
            mys_charset_list = lappend(mys_charset_list, (void *) charset);
    }

    fctx->tuple_desc = BlessTupleDesc(tupdesc);
    fctx->max_calls = list_length(mys_charset_list);
    fctx->call_cntr = 0;
    
    MemoryContextSwitchTo(oldContext);
}

Datum
show_mysql_charsets(PG_FUNCTION_ARGS)
{
    FuncCallContext *fctx;
    
    if (SRF_IS_FIRSTCALL())
        show_charset_first_call(fcinfo);

    fctx = SRF_PERCALL_SETUP();

    if (fctx->call_cntr < fctx->max_calls)
    {
        HeapTuple         returnTuple;
        Datum             values[SHOW_CHARSET_RESULTSET_NATTR];
        bool              nulls[SHOW_CHARSET_RESULTSET_NATTR];
        const pg_encname *charset = (pg_encname *) list_nth(mys_charset_list, fctx->call_cntr);

        if (!charset->description)
            SRF_RETURN_NEXT_NULL(fctx);
        
        memset(nulls, 0, sizeof(nulls));

        values[0] = CStringGetTextDatum(charset->name);
        values[1] = CStringGetTextDatum(charset->description);
        values[2] = CStringGetTextDatum(charset->default_collation);
        values[3] = Int64GetDatum(pg_encoding_max_length(charset->encoding));
        
        returnTuple = heap_form_tuple(fctx->tuple_desc, values, nulls);

        SRF_RETURN_NEXT(fctx, HeapTupleGetDatum(returnTuple));
    }
    
    SRF_RETURN_DONE(fctx);
}

static void
show_collation_first_call(PG_FUNCTION_ARGS)
{
    FuncCallContext  *fctx = SRF_FIRSTCALL_INIT();
    MemoryContext     oldContext = MemoryContextSwitchTo(fctx->multi_call_memory_ctx);
	TupleDesc         tupdesc = CreateTemplateTupleDesc(SHOW_COLLATION_RESULTSET_NATTR);

    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "Collation", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 2, "Charset", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 3, "Id", INT8OID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 4, "Default", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 5, "Compiled", VARCHAROID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 6, "Sortlen", INT8OID, -1, 0);
    
    fctx->tuple_desc = BlessTupleDesc(tupdesc);
    fctx->max_calls = lengthof(all_cs);
    fctx->call_cntr = 0;
    
    MemoryContextSwitchTo(oldContext);
}

Datum
show_mysql_collations(PG_FUNCTION_ARGS)
{
    FuncCallContext *fctx;
    
    if (SRF_IS_FIRSTCALL())
        show_collation_first_call(fcinfo);

    fctx = SRF_PERCALL_SETUP();

    if (fctx->call_cntr < fctx->max_calls)
    {
        HeapTuple         returnTuple;
        Datum             values[SHOW_COLLATION_RESULTSET_NATTR];
        bool              nulls[SHOW_COLLATION_RESULTSET_NATTR];
        CHARSET_INFO     *cs = all_cs[fctx->call_cntr];
        const char       *csname = cs->csname;
        const char       *coll_name = cs->coll_name;
        const char       *Compiled, *Default;
        
        memset(nulls, 0, sizeof(nulls));

        Compiled = (cs->state & MY_CS_COMPILED) ? "Yes" : "";
        
        Default = "";

        for (int i = 0; i < pg_encname_tbl_size; i++)
        {
            const pg_encname *charset = &pg_encname_tbl[i];

            if (charset->description && charset->default_collation &&
                !strcmp(charset->default_collation, coll_name))
            {
                Default = "Yes";
                break;
            }
        }
        
        values[0] = CStringGetTextDatum(coll_name);     // Collation
        values[1] = CStringGetTextDatum(csname);        // Charset
        values[2] = Int64GetDatum(cs->number);          // Id
        values[3] = CStringGetTextDatum(Default);       // Default
        values[4] = CStringGetTextDatum(Compiled);      // Compiled
        values[5] = Int64GetDatum(cs->sort_len);        // Sortlen

        returnTuple = heap_form_tuple(fctx->tuple_desc, values, nulls);

        SRF_RETURN_NEXT(fctx, HeapTupleGetDatum(returnTuple));
    }

    SRF_RETURN_DONE(fctx);
}

/* Begin - BUG#7845 */
Datum
mysql_text_regexeq(PG_FUNCTION_ARGS)
{
    text *s = PG_GETARG_TEXT_PP(0);
    text *p = PG_GETARG_TEXT_PP(1);
    int regex_case_flag = REG_BASIC;
    CHARSET_INFO *cs = search_charset_info_by_coll_id(PG_GET_COLLATION(), true);

    if (cs && !(cs->state & (MY_CS_BINSORT | MY_CS_CSSORT)))
    {
        regex_case_flag = REG_ICASE;
    }

    PG_RETURN_BOOL(RE_compile_and_execute(p,
                                          VARDATA_ANY(s),
                                          VARSIZE_ANY_EXHDR(s),
                                          REG_ADVANCED | regex_case_flag,
                                          PG_GET_COLLATION(),
                                          0, NULL));
}

Datum
mysql_text_regexne(PG_FUNCTION_ARGS)
{
    text *s = PG_GETARG_TEXT_PP(0);
    text *p = PG_GETARG_TEXT_PP(1);
    int regex_case_flag = REG_BASIC;
    CHARSET_INFO *cs = search_charset_info_by_coll_id(PG_GET_COLLATION(), true);

    if (cs && !(cs->state & (MY_CS_BINSORT | MY_CS_CSSORT)))
    {
        regex_case_flag = REG_ICASE;
    }

    PG_RETURN_BOOL(!RE_compile_and_execute(p,
                                           VARDATA_ANY(s),
                                           VARSIZE_ANY_EXHDR(s),
                                           REG_ADVANCED | regex_case_flag,
                                           PG_GET_COLLATION(),
                                           0, NULL));
}
/* End - BUG#7845 */

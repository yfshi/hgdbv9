#include "postgres.h"

#include <assert.h>
#include <wctype.h>

#include "catalog/namespace.h"
#include "catalog/pg_collation.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "mb/pg_wchar.h"
#include "nodes/pg_list.h"
#include "parser/parse_coerce.h"
#include "utils/builtins.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h"
#include "utils/varlena.h"

#include "../include/nlssort.h"

PG_FUNCTION_INFO_V1(nlssort_0);
PG_FUNCTION_INFO_V1(nlssort_1);
PG_FUNCTION_INFO_V1(nlssort_2);
PG_FUNCTION_INFO_V1(nlssort_1_raw);
PG_FUNCTION_INFO_V1(nlssort_2_raw);
PG_FUNCTION_INFO_V1(nlssort_1_char);
PG_FUNCTION_INFO_V1(nlssort_2_char);
PG_FUNCTION_INFO_V1(nlssort_1_blob);
PG_FUNCTION_INFO_V1(nlssort_2_blob);
PG_FUNCTION_INFO_V1(nlssort_1_unknown);
PG_FUNCTION_INFO_V1(nlssort_2_unknown);
PG_FUNCTION_INFO_V1(nls_collation_id);
PG_FUNCTION_INFO_V1(nls_collation_name);

nlssort_hook_type nlssort_hook_pre = NULL;
ora_raw_input_hook_type ora_raw_input_hook_pre = NULL;

typedef enum nlsparam {
    NLS_SORT,
    NLS_LANG,
    NLS_LANGUAGE,
    _NLS_LAST_ARGS
} nlsparam;

typedef enum nlssort_collation {
    BINARY,
    BINARY_AI,
    BINARY_CI,
    GENERIC_M_CI,
    SCHINESE_PINYIN_M,
    SCHINESE_RADICAL_M,
    SCHINESE_STROKE_M,
    UCA0700_SCHINESE,   // UCA pinyin
    UCA0700_SCHINESE1,  // UCA radical
    UCA0700_SCHINESE2,  // UCA stroke
    USING_NLS_SORT_CS,
    USING_NLS_SORT_AI,
    USING_NLS_SORT_CI,
    USING_NLS_SORT,
    USING_NLS_COMP,
    _ORA_LAST_COLLATION
} nlssort_collation;

typedef struct {
	uint32_t code;
} nlssort_mp_data_t;

typedef struct {
    uint32_t code;
    uint8_t  val[8];
} nlssort_binary_mp_data_t;

typedef struct {
    uint32_t    code;
    int32_t     val1;
    int16_t     val2;
    int16_t     val3;
} nlssort_schinese_mp_data_t;

typedef struct {
    uint32_t    code;
    int32_t     val1;
    int8_t      val2;
} nlssort_generic_mp_data_t;

typedef struct {
    uint32_t    code;
    int16_t     nindex;
    int16_t     indexs[7];
} nlssort_uca_norm_mp_t;

typedef struct {
    uint32_t    code;
    uint32_t    nfkd[8];
} nlssort_schinese_norm_mp_t;

typedef struct {
    uint32_t    code;
    int32_t     val1;
    int16_t     val2;
    int8_t      val3;
    int32_t     val4;
} nlssort_uca_mp_data_t;

typedef struct nlssort_mp_s nlssort_mp_t;
typedef struct nlssort_trans_s nlssort_trans_t;

typedef bytea *args_callback(const char*, int32_t, const char*);
typedef bytea *collation_callback(nlssort_trans_t *, const char *, int32_t);
typedef void  *mp_find_callback(const void *key, const void *base, size_t num, size_t size);
typedef int32_t mp_format_callback(void *, StringInfoData *);
typedef void normalize_callback(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest);

struct nlssort_mp_s {
	pg_enc              base;
	int32_t             size;
    int32_t             sec_size;
    mp_find_callback    *mp_find;
	mp_format_callback  *mp_format;
	nlssort_mp_data_t   *data;
    normalize_callback  *normalize;
};

struct nlssort_trans_s {
    nlssort_collation   coll;
    pg_enc              enc;
	const char          *enc_name; // `pg_convert` need target encoding's name
    nlssort_mp_t        *map;
};

typedef struct {
    Oid                 collid;
    Oid                 ora_id;    // get from NLS_COLLATION_ID() function
    const char*         collname;
    const char*         longname;
    collation_callback  *collate;
    nlssort_trans_t     *default_trans;
    nlssort_trans_t     *trans[_PG_LAST_ENCODING_];
} ora_collation_t;

static bytea *do_nls_sort(const char *src, int32_t len, const char *collname);
static bytea *do_nls_lang(const char *src, int32_t len, const char *collname);
static bytea *do_nls_language(const char *src, int32_t len, const char *collname);
static inline int32_t remove_trailing_spaces(const char *src, int32_t len) ;
static inline int32_t nls_find_format(nlssort_trans_t *trans, uint32_t code, StringInfoData *dest);
static void nls_find_format_multi_char(nlssort_trans_t *trans, const char *src, int32_t len, StringInfoData *dest);
static int32_t nls_find_format_single_char(nlssort_trans_t *trans, const char *src, int32_t len, StringInfoData *dest);
static void *nls_bsearch_find(const void *key, const void *base, size_t num, size_t size);
static inline uint32_t nls_get_code_number(const char *src, int32_t len);

#include "nlssort_binary.c"
#include "nlssort_schinese.c"
#include "nlssort_generic_ci.c"
#include "nlssort_uca.c"

struct {
    const char *argname;
    args_callback *action;
} nls_args [] = {
    { "NLS_SORT", do_nls_sort },
    { "NLS_LANG", do_nls_lang },
    { "NLS_LANGUAGE", do_nls_language }
};

static ora_collation_t ora_collations [] = {
    { ORA_COLL_OID(BINARY),              16383,  "BINARY",               NULL,     binary_collate,         NULL },
    { ORA_COLL_OID(BINARY_AI),           81919,  "BINARY_AI",            NULL,     binary_ci_collate,      &nls_binary_ai_utf8 },
    { ORA_COLL_OID(BINARY_CI),           147455, "BINARY_CI",            NULL,     binary_ci_collate,      &nls_binary_ci_utf8 },
    { ORA_COLL_OID(GENERIC_M_CI),        135169, "GENERIC_M_CI",         NULL,     generic_m_ci_collate,   &nls_generic_m_ci_utf8 },
    { ORA_COLL_OID(SCHINESE_PINYIN_M),   4160,   "SCHINESE_PINYIN_M",    NULL,     schinese_collate,       &nls_pinyin_utf8 },
    { ORA_COLL_OID(SCHINESE_RADICAL_M),  4163,   "SCHINESE_RADICAL_M",   NULL,     schinese_collate,       &nls_radical_utf8 },
    { ORA_COLL_OID(SCHINESE_STROKE_M),   4161,   "SCHINESE_STROKE_M",    NULL,     schinese_collate,       &nls_stroke_utf8 },
    { ORA_COLL_OID(UCA0700_SCHINESE),    208941, "UCA0700_SCHINESE",     "UCA0700_SCHINESE_S4_VS_BN_NY_EN_FN_HN_DN_MN",     uca_schinese_collate,   &nls_uca_schinese_utf8 },
    { ORA_COLL_OID(UCA0700_SCHINESE1),   208942, "UCA0700_SCHINESE1",    "UCA0700_SCHINESE1_S4_VS_BN_NY_EN_FN_HN_DN_MN",    uca_schinese_collate,   &nls_uca_schinese1_utf8 },
    { ORA_COLL_OID(UCA0700_SCHINESE2),   208943, "UCA0700_SCHINESE2",    "UCA0700_SCHINESE2_S4_VS_BN_NY_EN_FN_HN_DN_MN",    uca_schinese_collate,   &nls_uca_schinese2_utf8 },
    // pseudo-collation
    { ORA_COLL_OID(USING_NLS_SORT_CS),   16378,  "USING_NLS_SORT_CS",    NULL,     NULL,   NULL },
    { ORA_COLL_OID(USING_NLS_SORT_AI),   16379,  "USING_NLS_SORT_AI",    NULL,     NULL,   NULL },
    { ORA_COLL_OID(USING_NLS_SORT_CI),   16380,  "USING_NLS_SORT_CI",    NULL,     NULL,   NULL },
    { ORA_COLL_OID(USING_NLS_SORT),      16381,  "USING_NLS_SORT",       NULL,     NULL,   NULL },
    { ORA_COLL_OID(USING_NLS_COMP),      16382,  "USING_NLS_COMP",       NULL,     NULL,   NULL },
};

static inline int32_t
remove_trailing_spaces(const char *src, int32_t len) 
{
    const char *pos = src + len - 1;

    while(pos != src && *pos == ' ') {
        pos--;
        len--;
    }

    return len;
}

static int32_t
nls_mp_compare(const void *p1, const void *p2)
{
    uint32 v1, v2;

    v1 = *(const uint32 *)p1;
    v2 = ((const nlssort_mp_data_t *)p2)->code;

    return (v1 > v2) ? 1 : ((v1 == v2) ? 0 : -1);
}

static void *
nls_bsearch_find(const void *key, const void *base, size_t num, size_t size)
{
    return bsearch(key, base, num, size, nls_mp_compare);
}

static inline int32_t
nls_find_format(nlssort_trans_t *trans, uint32_t code, StringInfoData *dest) 
{
    void *find = NULL;
    nlssort_mp_t *map = trans->map;

    find = map->mp_find(&code, map->data, map->size, map->sec_size);

    if(find == NULL)
        return 0;

    assert(map->mp_format != NULL);
    
    return map->mp_format(find, dest);
}

static inline uint32_t
nls_get_code_number(const char *src, int32_t len)
{
    uint32_t code = 0;

    for(int32_t i = len - 1; i >= 0; --i, src++)
        code |= *((uint8_t*)src) << (8 * i);
    
    return code;
}

static inline int32_t
nls_find_format_single_char(nlssort_trans_t *trans, const char *src, int32_t len, StringInfoData *dest)
{
    int32_t ret = 0;
    uint32_t code = 0;

    if(src == NULL || len == 0)
        return 0;

    code = nls_get_code_number(src, len);

    ret = nls_find_format(trans, code, dest);

    return ret;
}

/**
 * The encoding of input paramater in `normalize()` must be UTF8,
 * because the map file used by UCA0700_SCHINESE* collation is based on UTF8.
 * 
 * `normalize()` of SCHINESE* and GENERIC_M_CI collations will call `unicode_normalize_func()`, it's based on UNICODE Standard;
 * `normalize()` of UCA070* collations implemented with map file.
 */
static void
nls_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest)
{
    const char *utf8 = src;

    if(!trans->map || !trans->map->normalize) 
        return;

    if(trans->enc != PG_UTF8) {
        text *ret;
        char raw[8] = { 0 };

        /**
         * must copy it, because `src` is not end with `\0`
         */
        memcpy(raw, src, len);

        ret = DatumGetTextP(
                        DirectFunctionCall3(pg_convert,
                                    CStringGetTextDatum(raw), 
                                    CStringGetDatum(trans->enc_name), 
                                    CStringGetDatum("utf8")));

        utf8 = VARDATA_ANY(ret);
        len = VARSIZE_ANY_EXHDR(ret);
    }
    
    trans->map->normalize(trans, utf8, len, dest);
}

static void
nls_find_format_multi_char(nlssort_trans_t *trans, const char *src, int32_t srclen, StringInfoData *dest)
{
    for(int32_t total = srclen, len = 0, append = 0; total > 0; total -= len, src += len) {
        len = pg_encoding_mblen(trans->enc, src);

        /**
         * Unicode  with a maximum of 4 bytes;
         * GBK      with a maximum of 2 bytes;
         * GB18030  with a maximum of 4 bytes;
         * UTF8     with a maximum of 6 bytes, however, accoding to `RFC 3629`, UTF8 values with 5-6 bytes is illegal !!!
         */
        assert(len <= 4);

        append = nls_find_format_single_char(trans, src, len, dest);

        /**
         * may be the character is not support,
         * or the character is combinaton char, just normalize it !!!
         */
        if(append == 0)
            nls_normalize(trans, src, len, dest);
    }
}

/**
 * Ignoring excessively long data to save memory
 */
static int32_t
nls_truncate_pre(const char *src, int32_t len, Oid collid)
{
    #define NLSSORT_INPUT_LIMIT 32768

    int32_t cnt = 0;
    
    if(collid == ORA_COLL_OID(BINARY)) {
        cnt = len;
    } else {
        while(cnt < len && cnt < NLSSORT_INPUT_LIMIT)
            cnt += pg_encoding_mblen(GetDatabaseEncoding(), src);
    }

    if(cnt < NLSSORT_INPUT_LIMIT)
        return len;
    
    return NLSSORT_INPUT_LIMIT;
}

/**
 * According to Oracle's document:
 *   1. If MAX_STRING_SIZE = STANDARD, then the maximum length of the return value is 2000  bytes.
 *   2. If MAX_STRING_SIZE = EXTENDED, then the maximum length of the return value is 32767 bytes. 
 *   3. If the collation key exceeds this limit, then the function fails with the error "ORA-12742: unable to create the collation key".
 */
static void
nls_truncate_final(bytea *result)
{
    // Check from `MAX_STRING_SIZE` to ensure truncate length
    int32_t limit = (max_string_size == ORA_MAX_STRING_SIZE_STANDARD) ? 2000 : 32767;

    if(VARSIZE_ANY_EXHDR(result) > limit) {
        if(max_string_size == ORA_MAX_STRING_SIZE_STANDARD)
            SET_VARSIZE(result, limit + VARHDRSZ);

        /*
         * there will effect INSERT, if table is partition-table, 
         * or it has a index. 
         */
        #if 0
        else
            ereport(ERROR,
                    errmsg("IVY-12742: unable to create the collation key"));
        #endif
    }
}

static ora_collation_t *
pick_collation_with_oid(Oid collid)
{
    int32_t i;

    for(i = 0; i != _ORA_LAST_COLLATION && ora_collations[i].collid != collid; i++) {}

    if(i == _ORA_LAST_COLLATION)
        return NULL;

    return &ora_collations[i];
}

static ora_collation_t *
pick_collation_with_ora_id(Oid ora_id)
{
    int32_t i;

    for(i = 0; i != _ORA_LAST_COLLATION && ora_collations[i].ora_id != ora_id; i++) {}

    if(i == _ORA_LAST_COLLATION)
        return NULL;

    return &ora_collations[i];
}

static ora_collation_t *
pick_collation_with_collname(const char *collname, int len)
{
    int32_t i = 0;
    
    for(i = 0; i != _ORA_LAST_COLLATION; i++) {
        const char *shortname = ora_collations[i].collname;
        const char *longname = ora_collations[i].longname;

        if ((len == strlen(shortname) && !pg_strncasecmp(collname, shortname, len)) || 
            (longname && len == strlen(longname) && !pg_strncasecmp(collname, longname, len)))
            break;

        if (!pg_strcasecmp(collname, shortname) || 
            (longname && !pg_strcasecmp(collname, longname))) 
            break;
    }

    if(i == _ORA_LAST_COLLATION)
        return NULL;
    
    return &ora_collations[i];
}

static bytea *
do_nls_sort_internal(const char *src, int32_t len, ora_collation_t *coll)
{
    bytea           *result = NULL;
    nlssort_trans_t *trans = NULL;
    pg_enc enc = GetDatabaseEncoding();

    len = nls_truncate_pre(src, len, coll->collid);

    trans = coll->trans[enc];
    if(trans == NULL && coll->default_trans && coll->default_trans->enc != GetDatabaseEncoding()) {
        Datum converted;
        char *p = (char *) palloc(len + VARHDRSZ);

        memcpy(p + VARHDRSZ, src, len);
        SET_VARSIZE(p, len + VARHDRSZ);
        
        trans = coll->default_trans;

        converted = DirectFunctionCall3(pg_convert, 
                                    PointerGetDatum(p),
                                    CStringGetDatum(GetDatabaseEncodingName()), 
                                    CStringGetDatum(trans->enc_name));

        src = text_to_cstring(DatumGetTextP(converted));
    }

    result = coll->collate(trans, src, len);

    nls_truncate_final(result);

    return result;
}

static bytea *
do_nls_sort(const char *src, int32_t len, const char *collname)
{
    ora_collation_t *coll = NULL;
    
    coll = pick_collation_with_collname(collname, strlen(collname));

    if(coll == NULL)
        ereport(ERROR,
                errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));

    // this is pseudo-collation
    if(coll->collate == NULL)
        ereport(ERROR,
                errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));

    return do_nls_sort_internal(src, len, coll);
}

static bytea *
do_nls_lang(const char *src, int32_t len, const char *abbreviation)
{
    if(pg_strcasecmp(abbreviation, "ZHS") == 0 || pg_strcasecmp(abbreviation, "US") == 0) 
        return do_nls_sort(src, len, "BINARY");
    
    ereport(ERROR,
            errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));

    return NULL; // never execute, just add it for windows compile warning
}

/**
 * Language and it's default collation:
 *      https://docs.oracle.com/en/database/oracle/oracle-database/19/nlspg/appendix-A-locale-data.html#GUID-D2FCFD55-EDC3-473F-9832-AAB564457830
 *
 * Oracle provide `SIMPLIFIED CHINESE`, but it don't work in 19C
 */
static bytea *
do_nls_language(const char *src, int32_t len, const char *language)
{
    if(pg_strcasecmp(language, "AMERICAN") == 0) 
        return do_nls_sort(src, len, "BINARY");

    ereport(ERROR,
            errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));


    return NULL; // never execute, just add it for windows compile warning
}

static bytea *
nlssort_action_choose(const char *src, int32_t len, char *key, char *val, bool trim)
{
    int32_t i;
    bytea   *result = NULL;

    if(trim)
        len = remove_trailing_spaces(src, len);

    for(i = NLS_SORT; i != _NLS_LAST_ARGS; ++i) {
        if(pg_strcasecmp(key, nls_args[i].argname) == 0) {
            assert(nls_args[i].action != NULL);
            result = nls_args[i].action(src, len, val);
            break;
        }
    }

    if (i == _NLS_LAST_ARGS)
        ereport(ERROR,
                errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));


    return result;
}

static bytea *
nlssort_internal_with_pseudo_collation(const char *src, int32_t len, Oid collid, bool trim)
{    
    ora_collation_t *coll;
    
    if(trim)
        len = remove_trailing_spaces(src, len);

    if (IsPseudoCollation(collid))
        collid = PseudoCollationReplace3(collid);
    
    coll = pick_collation_with_oid(collid);

    if (coll == NULL)
        ereport(ERROR,
                errmsg("IVY-12702: invalid NLS parameter string used in SQL function"));

    return do_nls_sort_internal(src, len, coll);
}

/* START BUG#M0000285 */
static Datum
rawin(PG_FUNCTION_ARGS)
{
	int 		len;
	int			bc;
	char	   *r = NULL;
	uint8_t	   *w = NULL;
	bytea	   *result = NULL;
    Oid         targetTypeId = PG_GETARG_OID(0);

	static const uint8_t hexlookup[128] = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};

    if(targetTypeId != RAWOID && targetTypeId != BLOBOID)
      PG_RETURN_NULL();  

	r = PG_GETARG_CSTRING(1);

	if(r == NULL)
		len = 0;
	else 
		len = strlen(r);
	
    // compatible PG's bytea input style: '\xfe'
    if(len > 2 && r[0] == '\\' && r[1] == 'x') {
        len -= 2;
        r += 2;
    }

    bc = (len + 1) / 2 + VARHDRSZ;

	for(int i = 0; i < len; ++i) {
		if(hexlookup[(int8_t)r[i]] == (uint8_t)-1) {
            ereport(ERROR,
                    errmsg("IVY-01465: invalid hex number"));
		}
	}

    result = palloc(bc);
	w = (uint8_t *)VARDATA(result);

	if(len & 1) {
		*(w++) = hexlookup[(int8_t)*(r++)];
	}
	
	for(; r && *r; r += 2) {
		*(w++) = hexlookup[(int8_t)*r] << 4 | hexlookup[(int8_t)*(r + 1)];
	}

	SET_VARSIZE(result, bc);
	
	PG_RETURN_BYTEA_P(result);
}
/* END BUG#M0000285 */

void
nlssort_init(void)
{
    for(int32_t i=SCHINESE_STROKE_M; i != _ORA_LAST_COLLATION; i++) {
        memset(ora_collations[i].trans, 0, _PG_LAST_ENCODING_ * sizeof(nlssort_trans_t *));
    }

    #define SET_COLLATION_TRANS(COLL, ENCODE, TRANS) \
        ora_collations[COLL].trans[ENCODE] = TRANS;

    // init `BINARY_AI`
    SET_COLLATION_TRANS(BINARY_AI, PG_UTF8, &nls_binary_ai_utf8);
    SET_COLLATION_TRANS(BINARY_AI, PG_GBK, &nls_binary_ai_gbk);
    SET_COLLATION_TRANS(BINARY_AI, PG_GB18030, &nls_binary_ai_gb18030);

    // init `BINARY_CI`
    SET_COLLATION_TRANS(BINARY_CI, PG_UTF8, &nls_binary_ci_utf8);
    SET_COLLATION_TRANS(BINARY_CI, PG_GBK, &nls_binary_ci_gbk);
    SET_COLLATION_TRANS(BINARY_CI, PG_GB18030, &nls_binary_ci_gb18030);
    
    // init `GENERIC_M_CI` collation groups according server encoding
    SET_COLLATION_TRANS(GENERIC_M_CI, PG_UTF8, &nls_generic_m_ci_utf8);
    SET_COLLATION_TRANS(GENERIC_M_CI, PG_GBK, &nls_generic_m_ci_gbk);
    SET_COLLATION_TRANS(GENERIC_M_CI, PG_GB18030, &nls_generic_m_ci_gb18030);

    // init `SCHINESE_PINYIN_M` collation groups according server encoding
    SET_COLLATION_TRANS(SCHINESE_PINYIN_M, PG_UTF8, &nls_pinyin_utf8);
    SET_COLLATION_TRANS(SCHINESE_PINYIN_M, PG_GBK, &nls_pinyin_gbk);
    SET_COLLATION_TRANS(SCHINESE_PINYIN_M, PG_GB18030, &nls_pinyin_gb18030);

    // init `SCHINESE_RADICAL_M` collation groups according server encoding
    SET_COLLATION_TRANS(SCHINESE_RADICAL_M, PG_UTF8, &nls_radical_utf8);
    SET_COLLATION_TRANS(SCHINESE_RADICAL_M, PG_GBK, &nls_radical_gbk);
    SET_COLLATION_TRANS(SCHINESE_RADICAL_M, PG_GB18030, &nls_radical_gb18030);

    // init `SCHINESE_STROKE_M` collation groups according server encoding
    SET_COLLATION_TRANS(SCHINESE_STROKE_M, PG_UTF8, &nls_stroke_utf8);
    SET_COLLATION_TRANS(SCHINESE_STROKE_M, PG_GBK, &nls_stroke_gbk);
    SET_COLLATION_TRANS(SCHINESE_STROKE_M, PG_GB18030, &nls_stroke_gb18030);

    // init `UCA0700_SCHINESE` collation groups according server encoding
    SET_COLLATION_TRANS(UCA0700_SCHINESE, PG_UTF8, &nls_uca_schinese_utf8);
    SET_COLLATION_TRANS(UCA0700_SCHINESE, PG_GBK, &nls_uca_schinese_gbk);
    SET_COLLATION_TRANS(UCA0700_SCHINESE, PG_GB18030, &nls_uca_schinese_gb18030);

    // init `UCA0700_SCHINESE1` collation groups according server encoding
    SET_COLLATION_TRANS(UCA0700_SCHINESE1, PG_UTF8, &nls_uca_schinese1_utf8);
    SET_COLLATION_TRANS(UCA0700_SCHINESE1, PG_GBK, &nls_uca_schinese1_gbk);
    SET_COLLATION_TRANS(UCA0700_SCHINESE1, PG_GB18030, &nls_uca_schinese1_gb18030);
    
    // init `UCA0700_SCHINESE2` collation groups according server encoding
    SET_COLLATION_TRANS(UCA0700_SCHINESE2, PG_UTF8, &nls_uca_schinese2_utf8);
    SET_COLLATION_TRANS(UCA0700_SCHINESE2, PG_GBK, &nls_uca_schinese2_gbk);
    SET_COLLATION_TRANS(UCA0700_SCHINESE2, PG_GB18030, &nls_uca_schinese2_gb18030);

    // init uca normaize
    uca_normalize_init();

    // init nlssort hook for oracle `collate` keyword
    nlssort_hook_pre = nlssort_hook;
    nlssort_hook = nlssort_internal_with_pseudo_collation;

    ora_raw_input_hook_pre = ora_raw_input_hook;
    ora_raw_input_hook = rawin;
}

void
nlssort_fini(void)
{
    nlssort_hook = nlssort_hook_pre;
}

static char *
rawout(bytea *src)
{
	char	   *result;
	int32_t		len;

	result = palloc(VARSIZE_ANY_EXHDR(src) * 2 + 1);
	len = hex_encode(VARDATA_ANY(src), VARSIZE_ANY_EXHDR(src), result);

	*(result + len) = '\0';

	while(len--) {
		result[len] = toupper(result[len]);
	}

	return result;
}

Datum
nls_collation_id(PG_FUNCTION_ARGS) 
{
    char       *input = VARDATA_ANY(PG_GETARG_TEXT_P(0));
    int32_t     inputlen = VARSIZE_ANY_EXHDR(PG_GETARG_TEXT_P(0));
    ora_collation_t *coll = NULL;

    coll = pick_collation_with_collname(input, inputlen);

    if (coll == NULL)
        PG_RETURN_NULL();

    PG_RETURN_OID(coll->ora_id);
}

Datum
nls_collation_name(PG_FUNCTION_ARGS) 
{
    Oid	            ora_id = PG_GETARG_OID(0);
    const char      *ret;
    ora_collation_t *coll;
    char            type = 'L';
    
    if (PG_NARGS() == 2) {
        type = PG_GETARG_CHAR(1);

        if (type != 's' && type != 'S' &&
            type != 'l' && type != 'L' )
            ereport(ERROR,
                    errmsg("IVY-01760: illegal argument for function"));
    }

    coll = pick_collation_with_ora_id(ora_id);

    if(coll == NULL)
        PG_RETURN_NULL();

    if ((type == 'l' || type == 'L') && coll->longname)
        ret = coll->longname;
    else
        ret = coll->collname;

    PG_RETURN_TEXT_P(CStringGetTextDatum(ret));
}

#define   nlssort_0(PG_FUNCTION_ARGS) nlssort_0(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_1(PG_FUNCTION_ARGS) nlssort_1(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_2(PG_FUNCTION_ARGS) nlssort_2(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_RAW
#define   nlssort_1(PG_FUNCTION_ARGS) nlssort_1_raw(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_RAW
#define   nlssort_2(PG_FUNCTION_ARGS) nlssort_2_raw(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NEED_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_1(PG_FUNCTION_ARGS) nlssort_1_char(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NEED_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_2(PG_FUNCTION_ARGS) nlssort_2_char(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_1(PG_FUNCTION_ARGS) nlssort_1_blob(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NOT_TRIM
#define   NLS_INPUT_IS_TEXT
#define   nlssort_2(PG_FUNCTION_ARGS) nlssort_2_blob(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NEED_TRIM
#define   NLS_INPUT_IS_CSTRING
#define   nlssort_1(PG_FUNCTION_ARGS) nlssort_1_unknown(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

#define   NLS_NEED_TRIM
#define   NLS_INPUT_IS_CSTRING
#define   nlssort_2(PG_FUNCTION_ARGS) nlssort_2_unknown(PG_FUNCTION_ARGS)
#include "nlssort_template.c"

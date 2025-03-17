#ifndef NLSSORT_BINARY_C
#define NLSSORT_BINARY_C

static int32_t
binary_format(void *find, StringInfoData *dest)
{
    return 0;
}

static int32_t
binary_accent_insensitive_format(void *find, StringInfoData *dest)
{
    int32_t ret = 0;

    nlssort_binary_mp_data_t *item = (nlssort_binary_mp_data_t *)find;

    for (int32_t i = 0; item->val[i] != 0; ++i) 
    {
        pq_sendint8(dest, item->val[i]);
        ret++;
    }

    return ret;
}

static inline uint32_t
binary_case_insensitive(uint32_t code) 
{
    #define EQUAL(val, V) ((val == V))
    #define BETWEEN(val, L, R) ((val >= L && val <= R))

    if (EQUAL(code, 0xd081))
        code += 0x0110;
    else if (BETWEEN(code, 0x41, 0x5A))
        code += 0x20;
    else if (BETWEEN(code, 0xce91, 0xce9f))
        code += 0x20;
    else if (BETWEEN(code, 0xcea0, 0xcea1))
        code += 0xe0;
    else if (BETWEEN(code, 0xcea3, 0xcea9))
        code += 0xe0;
    else if (BETWEEN(code, 0xd090, 0xd09f))
        code += 0x20;
    else if (BETWEEN(code, 0xd0a0, 0xd0af))
        code += 0xe0;
    else if (BETWEEN(code, 0xe285a0, 0xe285ab))
        code += 0x10;
    else if (BETWEEN(code, 0xefbca1, 0xefbcba))
        code += 0xe0;

    return code;
}

static void 
binary_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest)
{
    uint32_t code = nls_get_code_number(src, len);
    
    for (uint32_t i = len - 1, v = 0xff << i * 8; v > 0; i--, v >>= 8) 
    {
        pq_sendint8(dest, (code & v) >> (i * 8));
    }
}

static void 
binary_case_insensitive_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest)
{
    uint32_t code = nls_get_code_number(src, len);
    
    code = binary_case_insensitive(code);

    for (uint32_t i = len - 1, v = 0xff << i * 8; v > 0; i--, v >>= 8) 
    {
        pq_sendint8(dest, (code & v) >> (i * 8));
    }
}

#include "binary/binary_ai_utf8.map"
#include "binary/binary_ci_utf8.map"

static bytea *
binary_collate(nlssort_trans_t *trans, const char *src, int32_t len)
{
    StringInfoData result;

    pq_begintypsend(&result);

    for (int32_t i=0; i<len; ++i) 
    {
        pq_sendint8(&result, *(uint8_t *)(src + i));
    }

    pq_sendint8(&result, 0x00);
    
    return pq_endtypsend(&result);
}

static bytea *
binary_ci_collate(nlssort_trans_t *trans, const char *src, int32_t len)
{
    StringInfoData result;

    pq_begintypsend(&result);

    nls_find_format_multi_char(trans, src, len, &result);

    pq_sendint8(&result, 0x00);
    
    return pq_endtypsend(&result);
}

nlssort_trans_t nls_binary_ai_utf8 = {
    .coll = BINARY_AI,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &binary_ai_utf8,
};

nlssort_trans_t nls_binary_ai_gbk = {
    .coll = BINARY_AI,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &binary_ai_gbk,
};

nlssort_trans_t nls_binary_ai_gb18030 = {
    .coll = BINARY_AI,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &binary_ai_gb18030,
};

nlssort_trans_t nls_binary_ci_utf8 = {
    .coll = BINARY_CI,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &binary_ci_utf8,
};

nlssort_trans_t nls_binary_ci_gbk = {
    .coll = BINARY_CI,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &binary_ci_gbk,
};

nlssort_trans_t nls_binary_ci_gb18030 = {
    .coll = BINARY_CI,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &binary_ci_gb18030,
};

#endif
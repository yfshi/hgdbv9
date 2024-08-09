#ifndef NLSSORT_GENERIC_CI_C
#define NLSSORT_GENERIC_CI_C

static inline int32_t
format_generic_map1(int32_t val, StringInfoData *dest)
{
    uint32_t low_16  = (val & 0x0000ffff);
    uint32_t high_16 = (val & 0xffff0000) >> 16;

    if(high_16 == 0x0000) {
        pq_sendint16(dest, low_16);
        return 2;
    }

    if(high_16 == 0xffff) {
        pq_sendint32(dest, val);
        return 4;
    }
    
    pq_sendint32(dest, 0xffff0000 | high_16);
    pq_sendint32(dest, 0xffff0000 | low_16);
    
	return 8;
}

static inline int32_t
format_generic_map2(int8_t val, StringInfoData *dest)
{
    pq_sendint8(dest, val & 0xff);
	return 1;
}

static int32_t
generic_find_format(void *find, StringInfoData *dest)
{
    int len1 = 0, 
        len2 = 0;

    nlssort_generic_mp_data_t *src = (nlssort_generic_mp_data_t *)find;

    if(src->val1)
        len1 = format_generic_map1(src->val1, &dest[0]);

    if(src->val2)
        len2 = format_generic_map2(src->val2, &dest[1]);

    return len1 + len2;
}

#include "generic_m_ci/generic_utf8.map"
#include "generic_m_ci/generic_gbk.map"
#include "generic_m_ci/generic_gb18030.map"

static bytea *
generic_m_ci_collate(nlssort_trans_t *trans, const char *src, int32_t len)
{
    #define GENERIC_MAP_NUMBER 2

    StringInfoData data[GENERIC_MAP_NUMBER + 1];
    StringInfoData *result = &data[GENERIC_MAP_NUMBER];

    for(int i = 0; i < GENERIC_MAP_NUMBER + 1; ++i)
        pq_begintypsend(&data[i]);

    nls_find_format_multi_char(trans, src, len, data);

    pq_sendbytes(result, data[0].data + VARHDRSZ, data[0].len - VARHDRSZ);
    pq_sendint16(result, 0x0000);
    pq_sendbytes(result, data[1].data + VARHDRSZ, data[1].len - VARHDRSZ);
    
    for(int i = 0; i < GENERIC_MAP_NUMBER; ++i)
        pfree(data[i].data);

    return pq_endtypsend(result);
}

nlssort_trans_t nls_generic_m_ci_utf8 = {
    .coll = GENERIC_M_CI,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &generic_utf8
};

nlssort_trans_t nls_generic_m_ci_gbk = {
    .coll = GENERIC_M_CI,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &generic_gbk
};

nlssort_trans_t nls_generic_m_ci_gb18030 = {
    .coll = GENERIC_M_CI,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &generic_gb18030
};

#endif
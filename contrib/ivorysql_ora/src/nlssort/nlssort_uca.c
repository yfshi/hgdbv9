#ifndef NLSSORT_UCA_C
#define NLSSORT_UCA_C

static void uca_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest);

static inline int32_t
uca_schinese_format_map1_map4(int32_t val, StringInfoData *dest)
{
    uint32_t low_16  = (val & 0x0000ffff);
    uint32_t high_16 = (val & 0xffff0000) >> 16;

    if (high_16 == 0) 
    {
        pq_sendint16(dest, low_16);
        return 2;
    }

    pq_sendint32(dest, val);
    return 1;
}

static inline int32_t
uca_schinese_format_map2(int16_t val, StringInfoData *dest)
{
    pq_sendint16(dest, val & 0xffff);
    return 2;
}

static inline int32_t
uca_schinese_format_map3(int8_t val, StringInfoData *dest)
{
    pq_sendint8(dest, val & 0xff);
    return 1;
}

static int32_t
uca_find_format(void *find, StringInfoData *dest) 
{
    int len1 = 0, 
        len2 = 0, 
        len3 = 0, 
        len4 = 0;

    nlssort_uca_mp_data_t *src = (nlssort_uca_mp_data_t *)find;

    if (src->val1)
        len1 = uca_schinese_format_map1_map4(src->val1, &dest[0]);

    if (src->val2)
        len2 = uca_schinese_format_map2(src->val2, &dest[1]);

    if (src->val3)
        len3 = uca_schinese_format_map3(src->val3, &dest[2]);

    if (src->val4)
        len4 = uca_schinese_format_map1_map4(src->val4, &dest[3]);

    return len1 + len2 + len3 + len4;
}

#include "normalize/uca_normalize.map"
#include "uca/uca_schinese_utf8.map"
#include "uca/uca_schinese_gbk.map"
#include "uca/uca_schinese_gb18030.map"
#include "uca/uca_schinese1_utf8.map"
#include "uca/uca_schinese1_gbk.map"
#include "uca/uca_schinese1_gb18030.map"
#include "uca/uca_schinese2_utf8.map"
#include "uca/uca_schinese2_gbk.map"
#include "uca/uca_schinese2_gb18030.map"

static nlssort_mp_t *uca_norm_mp_list[_ORA_LAST_COLLATION];
static nlssort_uca_mp_data_t *uca_norm_data_list[_ORA_LAST_COLLATION];

static void
uca_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest)
{
    uint32_t code = 0;
    int16_t *indexs, nindex;
    nlssort_uca_norm_mp_t *find = NULL;
    nlssort_mp_t *map = uca_norm_mp_list[trans->coll];
    nlssort_uca_mp_data_t *uca_mp = uca_norm_data_list[trans->coll];

    code = nls_get_code_number(src, len);

    if (map)
        find = (nlssort_uca_norm_mp_t *)map->mp_find(&code, map->data, map->size, map->sec_size);

    if (!find) 
    {
        schinese_normalize(trans, src, len, dest);
        return;
    }

    nindex = find->nindex;
    indexs = find->indexs;

    for (int i = 0; i < nindex; ++i) 
    {
        if (uca_mp)
            uca_find_format((nlssort_mp_data_t *)(uca_mp + indexs[i]), dest);
    }
}

static bytea *
uca_schinese_collate(nlssort_trans_t *trans, const char *src, int32_t len)
{
    #define UCA_MAP_NUMBER 4

    StringInfoData data[UCA_MAP_NUMBER + 1];
    StringInfoData *result = &data[UCA_MAP_NUMBER];

    for (int i = 0; i < UCA_MAP_NUMBER + 1; ++i)
        pq_begintypsend(&data[i]);

    nls_find_format_multi_char(trans, src, len, data);

    pq_sendbytes(result, data[0].data + VARHDRSZ, data[0].len - VARHDRSZ);
    pq_sendint16(result, 0x0000);
    pq_sendbytes(result, data[1].data + VARHDRSZ, data[1].len - VARHDRSZ);
    pq_sendint16(result, 0x0000);
    pq_sendbytes(result, data[2].data + VARHDRSZ, data[2].len - VARHDRSZ);
    pq_sendint8(result, 0x00);
    pq_sendbytes(result, data[3].data + VARHDRSZ, data[3].len - VARHDRSZ);

    for (int i = 0; i < UCA_MAP_NUMBER; ++i)
        pfree(data[i].data);

    return pq_endtypsend(result);
}

static void
uca_normalize_init()
{
    uca_norm_data_list[UCA0700_SCHINESE]  = uca_schinese_norm;
    uca_norm_data_list[UCA0700_SCHINESE1] = uca_schinese1_norm;
    uca_norm_data_list[UCA0700_SCHINESE2] = uca_schinese2_norm;

    uca_norm_mp_list[UCA0700_SCHINESE]  = &uca_norm_schinese_mp;
    uca_norm_mp_list[UCA0700_SCHINESE1] = &uca_norm_schinese1_mp;
    uca_norm_mp_list[UCA0700_SCHINESE2] = &uca_norm_schinese2_mp;
}

nlssort_trans_t nls_uca_schinese_utf8 = {
    .coll = UCA0700_SCHINESE,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &uca_schinese_utf8
};

nlssort_trans_t nls_uca_schinese_gbk = {
    .coll = UCA0700_SCHINESE,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &uca_schinese_gbk
};

nlssort_trans_t nls_uca_schinese_gb18030 = {
    .coll = UCA0700_SCHINESE,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &uca_schinese_gb18030
};

nlssort_trans_t nls_uca_schinese1_utf8 = {
    .coll = UCA0700_SCHINESE1,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &uca_schinese1_utf8
};

nlssort_trans_t nls_uca_schinese1_gbk = {
    .coll = UCA0700_SCHINESE1,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &uca_schinese1_gbk
};

nlssort_trans_t nls_uca_schinese1_gb18030 = {
    .coll = UCA0700_SCHINESE1,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &uca_schinese1_gb18030
};

nlssort_trans_t nls_uca_schinese2_utf8 = {
    .coll = UCA0700_SCHINESE2,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &uca_schinese2_utf8
};

nlssort_trans_t nls_uca_schinese2_gbk = {
    .coll = UCA0700_SCHINESE2,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &uca_schinese2_gbk
};

nlssort_trans_t nls_uca_schinese2_gb18030 = {
    .coll = UCA0700_SCHINESE2,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &uca_schinese2_gb18030
};

#endif
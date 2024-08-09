#ifndef NLSSORT_SCHINESE_C
#define NLSSORT_SCHINESE_C

static void schinese_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest);

static inline int32_t
schinese_format_map1(int32_t val, StringInfoData *dest) 
{
    uint32_t low_16  = (val & 0x0000ffff);
    uint32_t high_16 = (val & 0xffff0000) >> 16;

	if(high_16 == 0) {
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
schinese_format_map2(int16_t val, StringInfoData *dest) 
{
    pq_sendint8(dest, val & 0xff);
	return 1;
}

static inline int32_t
schinese_format_map3(int16_t val, StringInfoData *dest) 
{
    pq_sendint8(dest, val & 0xff);
	return 1;
}

static int32_t
schinese_find_format(void *find, StringInfoData *dest) {
    int len1 = 0, 
        len2 = 0, 
        len3 = 0;

    nlssort_schinese_mp_data_t *src = (nlssort_schinese_mp_data_t *)find;

    if(src->val1)
        len1 = schinese_format_map1(src->val1, &dest[0]);

    if(src->val2)
        len2 = schinese_format_map2(src->val2, &dest[1]);

    if(src->val3)
        len3 = schinese_format_map3(src->val3, &dest[2]);

    return len1 + len2 + len3;
}

#include "normalize/schinese_normalize.map"
#include "schinese/pinyin_utf8.map"
#include "schinese/pinyin_gbk.map"
#include "schinese/pinyin_gb18030.map"
#include "schinese/radical_utf8.map"
#include "schinese/radical_gbk.map"
#include "schinese/radical_gb18030.map"
#include "schinese/stroke_utf8.map"
#include "schinese/stroke_gbk.map"
#include "schinese/stroke_gb18030.map"

static void
schinese_normalize(nlssort_trans_t *trans, const char *src, int len, StringInfoData *dest) {
    uint32_t code = 0;
    
    nlssort_mp_t *map = &schinese_norm_mp;
    nlssort_schinese_norm_mp_t *find = NULL;

    code = nls_get_code_number(src, len);
    
    find = (nlssort_schinese_norm_mp_t *)map->mp_find(&code, map->data, map->size, map->sec_size);

    if(!find)
        return;
    
    for(int32_t i = 0; find->nfkd[i] != 0; i++) {
        nls_find_format(trans, find->nfkd[i], dest);
    }
}

static bytea *
schinese_collate(nlssort_trans_t *trans, const char *src, int32_t len)
{
    #define SCHINESE_MAP_NUMBER 3
    StringInfoData data[SCHINESE_MAP_NUMBER + 1];
    StringInfoData *result = &data[SCHINESE_MAP_NUMBER];

    for(int i = 0; i < SCHINESE_MAP_NUMBER + 1; ++i)
        pq_begintypsend(&data[i]);

    nls_find_format_multi_char(trans, src, len, data);

    pq_sendbytes(result, data[0].data + VARHDRSZ, data[0].len - VARHDRSZ);
    pq_sendint16(result, 0x0000);
    pq_sendbytes(result, data[1].data + VARHDRSZ, data[1].len - VARHDRSZ);
    pq_sendint8(result, 0x00);
    pq_sendbytes(result, data[2].data + VARHDRSZ, data[2].len - VARHDRSZ);

    for(int i = 0; i < SCHINESE_MAP_NUMBER; ++i)
        pfree(data[i].data);

    return pq_endtypsend(result);
}

nlssort_trans_t nls_pinyin_utf8 = {
    .coll = SCHINESE_PINYIN_M,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &pinyin_utf8,
};

nlssort_trans_t nls_pinyin_gbk = {
    .coll = SCHINESE_PINYIN_M,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &pinyin_gbk,
};

nlssort_trans_t nls_pinyin_gb18030 = {
    .coll = SCHINESE_PINYIN_M,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &pinyin_gb18030,
};

nlssort_trans_t nls_radical_utf8 = {
    .coll = SCHINESE_RADICAL_M,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &radical_utf8,
};

nlssort_trans_t nls_radical_gbk = {
    .coll = SCHINESE_RADICAL_M,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &radical_gbk,
};

nlssort_trans_t nls_radical_gb18030 = {
    .coll = SCHINESE_RADICAL_M,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &radical_gb18030,
};

nlssort_trans_t nls_stroke_utf8 = {
    .coll = SCHINESE_STROKE_M,
    .enc = PG_UTF8,
	.enc_name = "utf8",
    .map = &stroke_utf8,
};

nlssort_trans_t nls_stroke_gbk = {
    .coll = SCHINESE_STROKE_M,
    .enc = PG_GBK,
	.enc_name = "gbk",
    .map = &stroke_gbk,
};

nlssort_trans_t nls_stroke_gb18030 = {
    .coll = SCHINESE_STROKE_M,
    .enc = PG_GB18030,
	.enc_name = "gb18030",
    .map = &stroke_gb18030,
};

#endif
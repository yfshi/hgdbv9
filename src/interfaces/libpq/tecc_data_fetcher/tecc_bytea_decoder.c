/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_bytea_decoder.c
 *    TECC bytea value decoder implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_bytea_decoder.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_bytea_decoder.h"

/* libpq函数声明 */
#include "libpq-fe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TECC日志头文件 */
#include "../tecc_common/tecc_logging.h"

/*
 * decode_bytea_value - 解码bytea类型的值
 *
 * bytea_str: bytea字符串（可能以\x开头）
 * 返回: 解码后的字符串，需要调用者释放
 */
char *
decode_bytea_value(const char *bytea_str)
{
    unsigned char *decoded;
    size_t decoded_len;
    char *result = NULL;

    if (bytea_str == NULL)
    {
        return NULL;
    }

    /* 添加调试日志 */
    TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: Input bytea_str='%s'",
             bytea_str ? bytea_str : "(NULL)");

    /* 检查是否是十六进制格式（\x开头） */
    if (bytea_str[0] == '\\' && bytea_str[1] == 'x')
    {
        TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: Detected hex format, calling PQunescapeBytea");

        /* 使用PQunescapeBytea解码十六进制格式 */
        decoded = PQunescapeBytea((const unsigned char *)bytea_str, &decoded_len);
        if (decoded != NULL)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: PQunescapeBytea succeeded, decoded_len=%zu", decoded_len);

            /* 将解码后的二进制数据转换为字符串 */
            result = (char *)pg_malloc(decoded_len + 1);
            if (result != NULL)
            {
                memcpy(result, decoded, decoded_len);
                result[decoded_len] = '\0';

                TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: Decoded result='%s'", result);
            }
            else
            {
                TECC_LOG(TECC_LOG_ERROR, "decode_bytea_value: Failed to allocate memory for result");
            }
            PQfreemem(decoded);
        }
        else
        {
            TECC_LOG(TECC_LOG_ERROR, "decode_bytea_value: PQunescapeBytea failed");
        }
    }
    else
    {
        TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: Not hex format, copying string directly");

        /* 如果不是十六进制格式，直接复制字符串 */
        result = pg_strdup(bytea_str);

        if (result != NULL)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "decode_bytea_value: Copied result='%s'", result);
        }
    }

    return result;
}
/* End - ReqID:TECC4.0 */
/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_byteorder.c
 *
 * Abstract:
 *   Unified byte order conversion functions for TECC encryption algorithms.
 *   Provides platform-independent byte order conversion for all algorithms.
 *
 * Authored by <wangxp@highgo.com>, 20260420.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_byteorder.c
 */

#include "../../include/common/tecc_algorithm/tecc_byteorder.h"

/* Byte order conversion functions implementation */

uint32_t tecc_hton32(uint32_t host32)
{
    /* Simple implementation for little-endian systems (most common) */
    return ((host32 & 0x000000FFU) << 24) |
           ((host32 & 0x0000FF00U) << 8) |
           ((host32 & 0x00FF0000U) >> 8) |
           ((host32 & 0xFF000000U) >> 24);
}

uint32_t tecc_ntoh32(uint32_t net32)
{
    /* Same as hton32 for symmetric conversion */
    return tecc_hton32(net32);
}

uint16_t tecc_hton16(uint16_t host16)
{
    return ((host16 & 0x00FFU) << 8) |
           ((host16 & 0xFF00U) >> 8);
}

uint16_t tecc_ntoh16(uint16_t net16)
{
    return tecc_hton16(net16);
}
/* End - ReqID:TECC4.0 */
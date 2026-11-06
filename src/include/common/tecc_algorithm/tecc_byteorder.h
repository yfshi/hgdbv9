/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_byteorder.h
 *
 * Abstract:
 *   Unified byte order conversion functions for TECC encryption algorithms.
 *   Provides platform-independent byte order conversion for all algorithms.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/include/common/tecc_algorithm/tecc_byteorder.h
 */

#ifndef TECC_BYTEORDER_H
#define TECC_BYTEORDER_H

#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/*
 * Byte order conversion functions
 * These functions convert between host byte order and network byte order (big-endian).
 * Network byte order is used for data storage and transmission.
 */

/* Get current time in microseconds */
#ifdef _WIN32
static __inline uint64_t get_time_us(void)
{
    FILETIME ft;
    ULARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    /* Convert from 100-nanosecond intervals since 1601-01-01 to microseconds since 1970-01-01 */
    return (uint64_t)((li.QuadPart - 116444736000000000ULL) / 10);
}
#else
static __inline uint64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}
#endif

/* Byte order conversion functions */
extern uint32_t tecc_hton32(uint32_t host32);
extern uint32_t tecc_ntoh32(uint32_t net32);
extern uint16_t tecc_hton16(uint16_t host16);
extern uint16_t tecc_ntoh16(uint16_t net16);







#endif /* TECC_BYTEORDER_H */
/* End - ReqID:TECC4.0 */
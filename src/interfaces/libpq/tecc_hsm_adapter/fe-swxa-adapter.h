/*-------------------------------------------------------------------------
 *
 * fe-swxa-adapter.h
 *	  三未信安 (SWXA) HSM adapter interface for TECC
 *
 * Portions Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef FE_SWXA_ADAPTER_H
#define FE_SWXA_ADAPTER_H

#include "fe-hsm-adapter.h"

/* 获取三未信安 SWXA 适配器实例 */
extern HsmAdapterInterface *get_swxa_adapter(void);

#endif /* FE_SWXA_ADAPTER_H */

/*-------------------------------------------------------------------------
 *
 * File: license.h
 *	  API for IvorySQL Pro to access license.dat.
 *
 * Copyright (c) 2023, Highgo Software Co.,Ltd. All rights reserved.
 *
 * src/libsrc/interface/license.h
 * 
 * Requirement description:
 * 		ReqID:SRS-MISC-LICENSE
 *-------------------------------------------------------------------------
 */

#ifndef __LICENSE_H__
#define __LICENSE_H__

#include <stdint.h>
#include <stdbool.h>
#include "func_code.h"

#ifdef WIN32
	#define LICENSEEXPORT	__declspec(dllexport)
#else
	#define LICENSEEXPORT
#endif

extern LICENSEEXPORT const char * hg_get_productName(void);			// Product name
extern LICENSEEXPORT const char * hg_get_majorVersion(void);		// Major version
extern LICENSEEXPORT const char * hg_get_minVersion(void);			// Min version
extern LICENSEEXPORT const char * hg_get_productType(void);			// Product type
extern LICENSEEXPORT const char * hg_get_highgoVersion(void);		// Highgo version
extern LICENSEEXPORT const char * hg_get_defaultDBname(void);		// Default database name
extern LICENSEEXPORT const char * hg_get_defaultDBuser(void);		// Default database user
extern LICENSEEXPORT const char * hg_get_defaultDBport(void);		// Default database port

extern LICENSEEXPORT const char * hg_get_licver(void);				// License version
extern LICENSEEXPORT const char * hg_get_licSerial(void);			// License serial
extern LICENSEEXPORT const char * hg_get_authType(void);			// Authorization type
extern LICENSEEXPORT const char * hg_get_usedType(void);			// Use type
extern LICENSEEXPORT const char * hg_get_userInfo(void);			// User information
extern LICENSEEXPORT const char * hg_get_projectInfo(void);			// Project information
extern LICENSEEXPORT const char * hg_get_createTime(void);			// Create time
extern LICENSEEXPORT const char * hg_get_finalTime(void);			// Final time
extern LICENSEEXPORT const char * hg_get_maxConnection(void);		// max_connection
extern LICENSEEXPORT const char * hg_get_maxCpuCores(void);			// CPU cores

extern LICENSEEXPORT bool     	  hg_get_productBitMapByCode(PRODUCT_FUNC_CODE); // Query function bitmap
extern LICENSEEXPORT int		  hg_load_license(const char *);	// Load license authorization file
extern LICENSEEXPORT const char * hg_get_license_status(void);		// Get license file status
extern LICENSEEXPORT bool		  hg_validate_license_status(void);	// Verify the status of the license file
extern LICENSEEXPORT const char * hg_get_calc_hardware_code(void);		// get calculated hardware code
extern LICENSEEXPORT const char * hg_get_hardware_code_by_macstr(const char *str);
extern LICENSEEXPORT const char * hg_get_hardware_code(void);			// get hardware code from license file

extern LICENSEEXPORT uint32_t     hg_get_enumLen(void);							// Obtain the number of function codes
extern LICENSEEXPORT const char * hg_get_productFuncNameCh(PRODUCT_FUNC_CODE);	// Obtain Name (in Chinese)
extern LICENSEEXPORT const char * hg_get_productFuncNameEn(PRODUCT_FUNC_CODE);	// Obtain Name (in English)
extern LICENSEEXPORT const char * hg_get_productFuncDescCh(PRODUCT_FUNC_CODE);	// Obtain Description (in Chinese)
extern LICENSEEXPORT const char * hg_get_productFuncDescEn(PRODUCT_FUNC_CODE);	// Obtain Description (in English)

extern LICENSEEXPORT void		  hg_set_encoding(const char *encoding);		// Set encoding
extern LICENSEEXPORT const char * hg_get_encoding(void);						// Get encoding
extern LICENSEEXPORT void		  hg_set_is_with_title(bool);					// is Output Title
extern LICENSEEXPORT void		  hg_set_is_chinese_title(bool);				// Is it a Chinese character
extern LICENSEEXPORT void		  hg_set_license_abspath(const char *);

extern LICENSEEXPORT void		  hg_get_convert_string(const char * from, char * to);

extern LICENSEEXPORT const char * hg_get_funclist_title_head(bool is_desc, int width);
extern LICENSEEXPORT const char * hg_get_funclist_content(bool is_list, bool is_all, bool is_desc, int width);
extern LICENSEEXPORT const char * hg_get_funclist_title_tail(int width);

extern LICENSEEXPORT const char * hg_get_startup_info(int *log_lvl);
extern LICENSEEXPORT void         hg_set_is_ignore_hardware_check(bool flag);	// is check hardware code
extern LICENSEEXPORT const char * hg_get_mac_string(void);						// get mac string
extern LICENSEEXPORT const char * hg_get_service_hotline(void);					// get service hotline
extern LICENSEEXPORT const char * hg_get_technical_support_url(void);			// get technical support

#endif // end __LICENSE_H__

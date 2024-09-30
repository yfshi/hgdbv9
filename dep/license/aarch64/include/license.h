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

extern const char * hg_get_productName(void);	// Product name
extern const char * hg_get_majorVersion(void);	// Major version
extern const char * hg_get_minVersion(void);	// Min version
extern const char * hg_get_productType(void);   // Product type
extern const char * hg_get_highgoVersion(void);	// Highgo version
extern const char * hg_get_defaultDBname(void);	// Default database name
extern const char * hg_get_defaultDBuser(void);	// Default database user
extern const char * hg_get_defaultDBport(void);	// Default database port

extern const char * hg_get_licver(void);		// License version
extern const char * hg_get_licSerial(void);		// License serial
extern const char * hg_get_authType(void);		// Authorization type
extern const char * hg_get_usedType(void);		// Use type
extern const char * hg_get_userInfo(void);		// User information
extern const char * hg_get_projectInfo(void);	// Project information
extern const char * hg_get_createTime(void);	// Create time
extern const char * hg_get_finalTime(void);		// Final time
extern const char * hg_get_maxConnection(void);	// max_connection
extern const char * hg_get_maxCpuCores(void);	// CPU cores

extern bool     	hg_get_productBitMapByCode(PRODUCT_FUNC_CODE);  // Query function bitmap
extern int		    hg_load_license(const char *);	                // Load license authorization file
extern const char * hg_get_license_status(void);	        // Get license file status
extern bool		    hg_validate_license_status(void);		// Verify the status of the license file
extern const char * hg_get_calc_hardware_code(void);		// get calculated hardware code
extern const char * hg_get_hardware_code_by_macstr(const char *str);
extern const char * hg_get_hardware_code(void);			    // get hardware code from license file

extern uint32_t     hg_get_productTemplateLen(void);		// get the number of function template length
extern bool         hg_get_is_classifyTitle(uint32_t);	    // is classify title?
extern const char * hg_get_productFuncName(uint32_t);	    // get function name
extern const char * hg_get_productFuncCodeStatus(uint32_t);	// get function status
extern const char * hg_get_productFuncVersion(uint32_t);	// get function versions
extern bool         hg_set_productFunclistIsChinese(bool);
extern uint32_t     hg_get_productFuncId(int);
extern uint32_t     hg_get_productMaxFuncId(void);

extern void		    hg_set_encoding(const char *encoding);	// Set encoding
extern const char * hg_get_encoding(void);					// Get encoding
extern bool		    hg_set_is_with_title(bool);				// is Output Title
extern bool		    hg_set_is_chinese_title(bool);			// Is it a Chinese character
extern void		    hg_set_license_abspath(const char *);

extern void		    hg_get_convert_string(const char * from, char * to);

extern const char * hg_get_funclist_title_head(bool is_desc, int width);
extern const char * hg_get_funclist_content(bool is_list, bool is_all, bool is_desc, int width);
extern const char * hg_get_funclist_title_tail(int width);

extern const char * hg_get_startup_info(int *log_lvl);
extern bool         hg_set_is_ignore_hardware_check(bool flag);	// is check hardware code
extern bool         hg_set_is_ignore_md5_checksum(bool flag);	// is check md5 checksum value
extern bool         hg_set_is_ignore_datetime_check(bool);		// is check the validity date of the license
extern bool         hg_set_is_client_dat(bool);                 /* 
                                                                 * Decide whether to load the client license file or the server license file. 
                                                                 * If not set, the default is the server-side license file. 
                                                                 * This setting will affect the selection of which license file (license.dat or client_info.dat) to read by the hg_load_license() function
                                                                 */
extern const char * hg_get_mac_string(void);					// get mac string
extern const char * hg_get_service_hotline(void);				// get service hotline
extern const char * hg_get_technical_support_url(void);			// get technical support
extern int		    hg_load_client_license(const char *);	    // Load client license authorization file
extern int		    hg_load_server_license(const char *);	    // Load server license authorization file

#endif // end __LICENSE_H__

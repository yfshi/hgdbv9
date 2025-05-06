/* ---------------------------------------------------------------------
 *
 * File: mac.h
 *
 * Abstract:
 * 		Obtain the mac address of the network card for encryption and decryption of the pgpass file.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 		src/include/common/mac.h
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#ifndef PG_MAC_H
#define PG_MAC_H

extern bool pg_get_mac_address(const char *interface_name, char *mac_address);

#endif /* PG_MAC_H */
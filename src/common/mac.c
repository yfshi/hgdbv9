/* ---------------------------------------------------------------------
 *
 * File: mac.c
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
 * 		src/common/mac.c
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#include "postgres.h"
#include "common/mac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#endif

bool pg_get_mac_address(const char *interface_name, char *mac_address)
{

#ifdef _WIN32
    ULONG buffer_size = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &buffer_size);
    IP_ADAPTER_ADDRESSES *pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(buffer_size);

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &buffer_size) == NO_ERROR)
    {
        for (IP_ADAPTER_ADDRESSES *pCurrent = pAddresses; pCurrent != NULL; pCurrent = pCurrent->Next)
        {

            if (pCurrent->PhysicalAddressLength == 6 &&
                (interface_name == NULL || strcmp(pCurrent->AdapterName, interface_name) == 0))
            {
                snprintf(mac_address, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                         pCurrent->PhysicalAddress[0],
                         pCurrent->PhysicalAddress[1],
                         pCurrent->PhysicalAddress[2],
                         pCurrent->PhysicalAddress[3],
                         pCurrent->PhysicalAddress[4],
                         pCurrent->PhysicalAddress[5]);
                free(pAddresses);
                return true;
            }
        }
    }
    free(pAddresses);
    return false;
#else
    int sockfd;
    struct ifreq ifr;
    unsigned char *mac = NULL;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return false;
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1)
    {
        perror("ioctl");
        close(sockfd);
        return false;
    }

    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

    sprintf(mac_address, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    close(sockfd);
#endif

    return true;
}
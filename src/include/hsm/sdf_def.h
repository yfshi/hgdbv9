#ifndef SDF_DEF_H
#define SDF_DEF_H


#include <openssl/opensslconf.h>
#ifndef OPENSSL_NO_SDF

#include <stdio.h>

#ifdef ECCMAXBITS256
#define ECCref_MAX_BITS			256 
#else
#define ECCref_MAX_BITS			512 
#endif
#define ECCref_MAX_LEN			((ECCref_MAX_BITS+7) / 8)

#define LiteRSAref_MAX_BITS    4096
#define LiteRSAref_MAX_LEN     ((LiteRSAref_MAX_BITS + 7) / 8)
#define LiteRSAref_MAX_PBITS   ((LiteRSAref_MAX_BITS + 1) / 2)
#define LiteRSAref_MAX_PLEN    ((LiteRSAref_MAX_PBITS + 7)/ 8)

#if defined(SGD_RSA_MAX_BITS) && (SGD_RSA_MAX_BITS > LiteRSAref_MAX_BITS)
#define RSAref_MAX_BITS    ExRSAref_MAX_BITS
#define RSAref_MAX_LEN     ExRSAref_MAX_LEN
#define RSAref_MAX_PBITS   ExRSAref_MAX_PBITS
#define RSAref_MAX_PLEN    ExRSAref_MAX_PLEN

#else
#define RSAref_MAX_BITS    LiteRSAref_MAX_BITS
#define RSAref_MAX_LEN     LiteRSAref_MAX_LEN
#define RSAref_MAX_PBITS   LiteRSAref_MAX_PBITS
#define RSAref_MAX_PLEN    LiteRSAref_MAX_PLEN

#endif


#define SDR_OK				0x0
#define SDR_BASE			0x01000000
#define SDR_UNKNOWERR		(SDR_BASE + 0x00000001)
#define SDR_NOTSUPPORT		(SDR_BASE + 0x00000002)
#define SDR_COMMFAIL		(SDR_BASE + 0x00000003)
#define SDR_HARDFAIL		(SDR_BASE + 0x00000004)
#define SDR_OPENDEVICE		(SDR_BASE + 0x00000005)
#define SDR_OPENSESSION		(SDR_BASE + 0x00000006)
#define SDR_PARDENY			(SDR_BASE + 0x00000007)
#define SDR_KEYNOTEXIST		(SDR_BASE + 0x00000008)
#define SDR_ALGNOTSUPPORT	(SDR_BASE + 0x00000009)
#define SDR_ALGMODNOTSUPPORT	(SDR_BASE + 0x0000000A)
#define SDR_PKOPERR			(SDR_BASE + 0x0000000B)
#define SDR_SKOPERR			(SDR_BASE + 0x0000000C)
#define SDR_SIGNERR			(SDR_BASE + 0x0000000D)
#define SDR_VERIFYERR		(SDR_BASE + 0x0000000E)
#define SDR_SYMOPERR		(SDR_BASE + 0x0000000F)
#define SDR_STEPERR			(SDR_BASE + 0x00000010)
#define SDR_FILESIZEERR		(SDR_BASE + 0x00000011)
#define SDR_FILENOEXIST		(SDR_BASE + 0x00000012)
#define SDR_FILEOFSERR		(SDR_BASE + 0x00000013)
#define SDR_KEYTYPEERR		(SDR_BASE + 0x00000014)
#define SDR_KEYERR			(SDR_BASE + 0x00000015)
#define SDR_ENCDATAERR		(SDR_BASE + 0x00000016)
#define SDR_RANDERR			(SDR_BASE + 0x00000017)
#define SDR_PRKRERR			(SDR_BASE + 0x00000018)
#define SDR_MACERR			(SDR_BASE + 0x00000019)
#define SDR_FILEEXSITS		(SDR_BASE + 0x0000001A)
#define SDR_FILEWERR		(SDR_BASE + 0x0000001B)
#define SDR_NOBUFFER		(SDR_BASE + 0x0000001C)
#define SDR_INARGERR		(SDR_BASE + 0x0000001D)
#define SDR_OUTARGERR		(SDR_BASE + 0x0000001E)


#define SGD_SM1_ECB		0x00000101
#define SGD_SM1_CBC		0x00000102
#define SGD_SM1_CFB		0x00000104
#define SGD_SM1_OFB		0x00000108
#define SGD_SM1_MAC		0x00000110

#define SGD_SM4_ECB		0x00000401
#define SGD_SM4_CBC		0x00000402
#define SGD_SM4_CFB		0x00000404
#define SGD_SM4_OFB		0x00000408
#define SGD_SM4_MAC		0x00000410

#define SGD_SM3			0x00000001
#define SGD_SHA1		0x00000002
#define SGD_SHA256		0x00000004
#define SGD_SHA384		0x00000008 //���̶��� �ٷ�����δ�ҵ�
#define SGD_SHA512		0x00000010 //���̶��� �ٷ�����δ�ҵ�
#define SGD_SHA224		0x00000020 //���̶��� �ٷ�����δ�ҵ�
#define SGD_MD5			0x00000040 //���̶��� �ٷ�����δ�ҵ�

#define SGD_SM2			0x00020100
#define SGD_SM2_1		0x00020200
#define SGD_SM2_2		0x00020400
#define SGD_SM2_3		0x00020800

/* RSA algorithm identifier */
#define SGD_RSA			0x00010000

/* SM4 algorithm identifier (basic, without mode) */
#define SGD_SM4			0x00000400

#endif
#endif	//SDF_DEF_H

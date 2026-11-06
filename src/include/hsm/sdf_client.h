#ifndef SDF_CLIENT_H_
#define SDF_CLIENT_H_

#ifdef	_WINDOWS
#ifdef  SDFCLIENT_EXPORTS
#define SDFCLIENT_API __declspec(dllexport)
#else
#define SDFCLIENT_API __declspec(dllimport)
#endif
#define SDF_CALL __stdcall
#else
#define	SDFCLIENT_API
#define SDF_CALL
#endif

#include "sdf_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(1)
	typedef struct DeviceInfo_st {
		unsigned char IssuerName[40];
		unsigned char DeviceName[16];
		/* 
		* 8-char date
		* 3-char batch num
		* 5-char serial num
		*/
		unsigned char DeviceSerial[16];
		unsigned int DeviceVersion;
		unsigned int StandardVersion;
		/*
		* AsymAlgAbility[0] = algors
		* AsymAlgAbility[1] = modulus lens
		*/
		unsigned int AsymAlgAbility[2];	
		unsigned int SymAlgAbility;
		unsigned int HashAlgAbility;
		unsigned int BufferSize;
	} DEVICEINFO;

	typedef struct RSArefPublicKey_st {
		unsigned int bits;
		unsigned char m[RSAref_MAX_LEN];
		unsigned char e[RSAref_MAX_LEN];
	} RSArefPublicKey;

	typedef struct RSArefPrivateKey_st {
		unsigned int bits;
		unsigned char m[RSAref_MAX_LEN];
		unsigned char e[RSAref_MAX_LEN];
		unsigned char d[RSAref_MAX_LEN];
		unsigned char prime[2][RSAref_MAX_PLEN];
		unsigned char pexp[2][RSAref_MAX_PLEN];
		unsigned char coef[RSAref_MAX_PLEN];
	} RSArefPrivateKey;

	typedef struct ECCrefPublicKey_st {
		unsigned int bits;
		unsigned char x[ECCref_MAX_LEN];
		unsigned char y[ECCref_MAX_LEN];
	} ECCrefPublicKey;

	typedef struct ECCrefPrivateKey_st {
		unsigned int  bits;
		unsigned char K[ECCref_MAX_LEN];
	} ECCrefPrivateKey;

	typedef struct ECCCipher_st {
		unsigned char x[ECCref_MAX_LEN];
		unsigned char y[ECCref_MAX_LEN];
		unsigned char M[32];
		unsigned int L;
		unsigned char C[1];
	} ECCCipher;

	typedef struct ECCSignature_st {
		unsigned char r[ECCref_MAX_LEN];
		unsigned char s[ECCref_MAX_LEN];
	} ECCSignature;

	typedef struct SDF_ENVELOPEDKEYBLOB {
		unsigned long Version;
		unsigned long ulSymmAlgID;
		ECCCipher ECCCipehrBlob;
		ECCrefPublicKey PubKey;
		unsigned char cbEncryptedPrivKey[64];
	} EnvelopedKeyBlob, *PEnvelopedKeyBlob;

#pragma pack()

	//满负载测试专用
	SDFCLIENT_API int SDF_OpenDevice2 (char *ip,unsigned int port,long long *phDeviceHandle);
	SDFCLIENT_API int SDF_OpenSession2(long long hDeviceHandle,char *ip,unsigned int port,long long *phSessionHandle);

	SDFCLIENT_API int SDF_OpenDevice(
		long long *phDeviceHandle);

	SDFCLIENT_API int SDF_CloseDevice(
		long long hDeviceHandle);

	SDFCLIENT_API int SDF_OpenSession(
		long long  hDeviceHandle,
		long long *phSessionHandle);

	SDFCLIENT_API int SDF_CloseSession(
		long long hSessionHandle);

	SDFCLIENT_API int SDF_GetDeviceInfo(
		long long hSessionHandle,
		DEVICEINFO *pstDeviceInfo);

	SDFCLIENT_API int SDF_GenerateRandom(
		long long hSessionHandle,
		unsigned int uiLength,
		unsigned char *pucRandom);

	SDFCLIENT_API int SDF_GetPrivateKeyAccessRight(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		unsigned char *pucPassword,
		unsigned int uiPwdLength);

	SDFCLIENT_API int SDF_ReleasePrivateKeyAccessRight(
		long long hSessionHandle,
		unsigned int uiKeyIndex);

	SDFCLIENT_API int SDF_ExportSignPublicKey_RSA(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		RSArefPublicKey *pucPublicKey);

	SDFCLIENT_API int SDF_ExportEncPublicKey_RSA(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		RSArefPublicKey *pucPublicKey);

	SDFCLIENT_API int SDF_GenerateKeyPair_RSA(
		long long hSessionHandle,
		unsigned int uiKeyBits,
		RSArefPublicKey *pucPublicKey,
		RSArefPrivateKey *pucPrivateKey);

	SDFCLIENT_API int SDF_GenerateKeyWithIPK_RSA(
		long long hSessionHandle,
		unsigned int uiIPKIndex,
		unsigned int uiKeyBits,
		unsigned char *pucKey,
		unsigned int *puiKeyLength,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_GenerateKeyWithEPK_RSA(
		long long hSessionHandle,
		unsigned int uiKeyBits,
		RSArefPublicKey *pucPublicKey,
		unsigned char *pucKey,
		unsigned int *puiKeyLength,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ImportKeyWithISK_RSA(
		long long hSessionHandle,
		unsigned int uiISKIndex,
		unsigned char *pucKey,
		unsigned int uiKeyLength,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ExchangeDigitEnvelopeBaseOnRSA(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		RSArefPublicKey *pucPublicKey,
		unsigned char *pucDEInput,
		unsigned int uiDELength,
		unsigned char *pucDEOutput,
		unsigned int *puiDELength);

	SDFCLIENT_API int SDF_ExportSignPublicKey_ECC(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		ECCrefPublicKey *pucPublicKey);

	SDFCLIENT_API int SDF_ExportEncPublicKey_ECC(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		ECCrefPublicKey *pucPublicKey);

	SDFCLIENT_API int SDF_GenerateKeyPair_ECC(
		long long hSessionHandle,
		unsigned int uiAlgID,
		unsigned int  uiKeyBits,
		ECCrefPublicKey *pucPublicKey,
		ECCrefPrivateKey *pucPrivateKey);

	SDFCLIENT_API int SDF_GenerateKeyWithIPK_ECC(
		long long hSessionHandle,
		unsigned int uiIPKIndex,
		unsigned int uiKeyBits,
		ECCCipher *pucKey,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_GenerateKeyWithEPK_ECC(
		long long hSessionHandle,
		unsigned int uiKeyBits,
		unsigned int uiAlgID,
		ECCrefPublicKey *pucPublicKey,
		ECCCipher *pucKey,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ImportKeyWithISK_ECC(
		long long hSessionHandle,
		unsigned int uiISKIndex,
		ECCCipher *pucKey,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_GenerateAgreementDataWithECC(
		long long hSessionHandle,
		unsigned int uiISKIndex,
		unsigned int uiKeyBits,
		unsigned char *pucSponsorID,
		unsigned int uiSponsorIDLength,
		ECCrefPublicKey *pucSponsorPublicKey,
		ECCrefPublicKey *pucSponsorTmpPublicKey,
		long long *phAgreementHandle);

	SDFCLIENT_API int SDF_GenerateKeyWithECC(
		long long hSessionHandle,
		unsigned char *pucResponseID,
		unsigned int uiResponseIDLength,
		ECCrefPublicKey *pucResponsePublicKey,
		ECCrefPublicKey *pucResponseTmpPublicKey,
		long long hAgreementHandle,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_GenerateAgreementDataAndKeyWithECC(
		long long hSessionHandle,
		unsigned int uiISKIndex,
		unsigned int uiKeyBits,
		unsigned char *pucResponseID,
		unsigned int uiResponseIDLength,
		unsigned char *pucSponsorID,
		unsigned int uiSponsorIDLength,
		ECCrefPublicKey *pucSponsorPublicKey,
		ECCrefPublicKey *pucSponsorTmpPublicKey,
		ECCrefPublicKey *pucResponsePublicKey,
		ECCrefPublicKey *pucResponseTmpPublicKey,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ExchangeDigitEnvelopeBaseOnECC(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		unsigned int uiAlgID,
		ECCrefPublicKey *pucPublicKey,
		ECCCipher *pucEncDataIn,
		ECCCipher *pucEncDataOut);

	SDFCLIENT_API int SDF_GenerateKeyWithKEK(
		long long hSessionHandle,
		unsigned int uiKeyBits,
		unsigned int uiAlgID,
		unsigned int uiKEKIndex,
		unsigned char *pucKey,
		unsigned int *puiKeyLength,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ImportKeyWithKEK(
		long long hSessionHandle,
		unsigned int uiAlgID,
		unsigned int uiKEKIndex,
		unsigned char *pucKey,
		unsigned int uiKeyLength,
		long long *phKeyHandle);

	SDFCLIENT_API int SDF_ImportKey
		(
		long long hSessionHandle,
		unsigned char *pucKey,
		unsigned int uiKeyLength,
		long long *phKeyHandle
		);

	SDFCLIENT_API int SDF_DestroyKey(
		long long hSessionHandle,
		long long hKeyHandle);

	SDFCLIENT_API int SDF_ExternalPublicKeyOperation_RSA(
		long long hSessionHandle,
		RSArefPublicKey *pucPublicKey,
		unsigned char *pucDataInput,
		unsigned int uiInputLength,
		unsigned char *pucDataOutput,
		unsigned int *puiOutputLength);

	SDFCLIENT_API int SDF_ExternalPrivateKeyOperation_RSA(
		long long hSessionHandle,
		RSArefPrivateKey *pucPrivateKey,
		unsigned char *pucDataInput,
		unsigned int uiInputLength,
		unsigned char *pucDataOutput,
		unsigned int *puiOutputLength);

	SDFCLIENT_API int SDF_InternalPublicKeyOperation_RSA(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		unsigned char *pucDataInput,
		unsigned int uiInputLength,
		unsigned char *pucDataOutput,
		unsigned int *puiOutputLength);

	SDFCLIENT_API int SDF_InternalPrivateKeyOperation_RSA(
		long long hSessionHandle,
		unsigned int uiKeyIndex,
		unsigned char *pucDataInput,
		unsigned int uiInputLength,
		unsigned char *pucDataOutput,
		unsigned int *puiOutputLength);

	SDFCLIENT_API int SDF_ExternalSign_ECC(
		long long hSessionHandle,
		unsigned int uiAlgID,
		ECCrefPrivateKey *pucPrivateKey,
		unsigned char *pucData,
		unsigned int  uiDataLength,
		ECCSignature *pucSignature);

	SDFCLIENT_API int SDF_ExternalVerify_ECC(
		long long hSessionHandle,
		unsigned int uiAlgID,
		ECCrefPublicKey *pucPublicKey,
		unsigned char *pucDataInput,
		unsigned int uiInputLength,
		ECCSignature *pucSignature);

	SDFCLIENT_API int SDF_InternalSign_ECC(
		long long hSessionHandle,
		unsigned int uiISKIndex,
		unsigned char *pucData,
		unsigned int uiDataLength,
		ECCSignature *pucSignature);

	SDFCLIENT_API int SDF_InternalVerify_ECC(
		long long hSessionHandle,
		unsigned int  uiIPKIndex,
		unsigned char *pucData,
		unsigned int uiDataLength,
		ECCSignature *pucSignature);

	SDFCLIENT_API int SDF_ExternalEncrypt_ECC(
		long long hSessionHandle,
		unsigned int uiAlgID,
		ECCrefPublicKey *pucPublicKey,
		unsigned char *pucData,
		unsigned int uiDataLength,
		ECCCipher *pucEncData);

	SDFCLIENT_API int SDF_ExternalDecrypt_ECC(
		long long hSessionHandle,
		unsigned int uiAlgID,
		ECCrefPrivateKey *pucPrivateKey,
		ECCCipher *pucEncData,
		unsigned char *pucData,
		unsigned int *puiDataLength);

	SDFCLIENT_API int SDF_InternalEncrypt_ECC(
		long long hSessionHandle,
		unsigned int uiIPKIndex,
		unsigned int uiAlgID,
		unsigned char *pucData,
		unsigned int uiDataLength,
		ECCCipher *pucEncData);

	SDFCLIENT_API int SDF_InternalDecrypt_ECC(
		long long    hSessionHandle,
		unsigned int uiISKIndex,
		unsigned int uiAlgID,
		ECCCipher     *pucEncData,
		unsigned char *pucData,
		unsigned int  *uiDataLength);

	SDFCLIENT_API int SDF_Encrypt(
		long long hSessionHandle,
		long long hKeyHandle,
		unsigned int uiAlgID,
		unsigned char *pucIV,
		unsigned char *pucData,
		unsigned int   uiDataLength,
		unsigned char *pucEncData,
		unsigned int  *puiEncDataLength);

	SDFCLIENT_API int SDF_Decrypt(
		long long hSessionHandle,
		long long hKeyHandle,
		unsigned int uiAlgID,
		unsigned char *pucIV,
		unsigned char *pucEncData,
		unsigned int uiEncDataLength,
		unsigned char *pucData,
		unsigned int *puiDataLength);

	SDFCLIENT_API int SDF_CalculateMAC(
		long long hSessionHandle,
		long long hKeyHandle,
		unsigned int uiAlgID,
		unsigned char *pucIV,
		unsigned char *pucData,
		unsigned int uiDataLength,
		unsigned char *pucMAC,
		unsigned int *puiMACLength);

	SDFCLIENT_API int SDF_HashInit(
		long long hSessionHandle,
		unsigned int uiAlgID,
		ECCrefPublicKey *pucPublicKey,
		unsigned char *pucID,
		unsigned int uiIDLength);

	SDFCLIENT_API int SDF_HashUpdate(
		long long hSessionHandle,
		unsigned char *pucData,
		unsigned int uiDataLength);

	SDFCLIENT_API int SDF_HashFinal(
		long long hSessionHandle,
		unsigned char *pucHash,
		unsigned int *puiHashLength);

	SDFCLIENT_API int SDF_CreateFile(
		long long hSessionHandle,
		unsigned char *pucFileName,
		unsigned int uiFileNameLen, /* max 128-byte */
		unsigned int uiFileSize);

	SDFCLIENT_API int SDF_ReadFile(
		long long hSessionHandle,
		unsigned char *pucFileName,
		unsigned int uiNameLen,
		unsigned int uiOffset,
		unsigned int *puiReadLength,
		unsigned char *pucBuffer);

	SDFCLIENT_API int SDF_WriteFile(
		long long hSessionHandle,
		unsigned char* pucFileName,
		unsigned int uiNameLen,
		unsigned int uiOffset,
		unsigned int uiWriteLength,
		unsigned char* pucBuffer);

	SDFCLIENT_API int SDF_DeleteFile(
		long long hSessionHandle,
		unsigned char *pucFileName,
		unsigned int uiNameLen);

	SDFCLIENT_API int SDF_GenerateKeywithIKE(
		long long hSessionHandle,
		unsigned char* pucSponsorNonce,
		unsigned int uiSponsorNonceLength,
		unsigned char* pucResponseNonce,
		unsigned int uiResponseNonceLength,
		unsigned char* pucSponsorCookie,
		unsigned int uiSponsorCookieLength,
		unsigned char* pucResponseCookie,
		unsigned int uiResponseCookieLength,
		unsigned int uiPrfAlgID,
		unsigned int uiKeyBitsD,
		long long* phKeyHandleD,
		unsigned int uiKeyBitsA,
		long long* phKeyHandleA,
		unsigned int uiKeyBitsE,
		long long* phKeyHandleE);

	SDFCLIENT_API int SDF_GenerateKeywithEPK_IKE(
		long long hSessionHandle,
		unsigned char* pucSponsorNonce,
		unsigned int uiSponsorNonceLength,
		unsigned char * pucResponseNonce,
		unsigned int uiResponseNonceLength,
		unsigned char* pucSponsorCookie,
		unsigned int uiSponsorCookieLength,
		unsigned char* pucResponseCookie,
		unsigned int uiResponseCookieLength,
		unsigned int uiPrfAlgID,
		unsigned int uiEccAlgID,
		ECCrefPublicKey* pucPublicKey,
		unsigned int uiKeyBitsD,
		ECCCipher* pucKeyD,
		long long* phKeyHandleD,
		unsigned int uiKeyBitsA,
		ECCCipher* pucKeyA,
		long long* phKeyHandleA,
		unsigned int uiKeyBitsE,
		ECCCipher* pucKeyE,
		long long* phKeyHandleE);

	SDFCLIENT_API int SDF_GenerateKeywithIPSEC(
		long long hSessionHandle,
		unsigned char* pucProtocollD,
		unsigned int uiProtocolDLenxth,
		unsigned char* pucSpi,
		unsigned int uiSpiLength,
		unsigned char* pucSponsorNonce,
		unsigned int uiSponsorNonceLength,
		unsigned char* pucResponseNonce,
		unsigned int uiResponseNonceLength,
		long long hKeyHandleD,
		unsigned int uiPrfAlglD,
		unsigned int uiKeyBitsEnc,
		long long * phKeyHandleEnc,
		unsigned int uiKeyBitsMac,
		long long* phKeyHandleMac);

	SDFCLIENT_API int SDF_GenerateKeywithEPK_IPSEC(
		long long hSessionHandle,
		unsigned char* pucProtocolID,
		unsigned int uiProtocolIDLength,
		unsigned char* pucSpi,
		unsigned int  uiSpiLength,
		unsigned char* pucSponsorNonce,
		unsigned int uiSponsorNonceLength,
		unsigned char* pucResponseNonce,
		unsigned int uiResponseNonceLength,
		long long hKeyHandle,
		unsigned int uiPrfAlgID,
		unsigned int uiEccAlglD,
		ECCrefPublicKey* pucPublicKey,
		unsigned int uiKeyBitsEnc,
		ECCCipher* pucKeyEnc,
		long long* phKeyHandleEnc,
		unsigned int uiKeyBitsMac,
		ECCCipher* pucKeyMac,
		long long* phKeyHandleMac);

	SDFCLIENT_API int SDF_GenerateKeywithSSL(
		long long hSessionHandle,
		unsigned char* pucKeyPreMaster,
		unsigned int uiKeyPreMasterLength,
		unsigned char* pucClientRandom,
		unsigned int uiClientRandomLength,
		unsigned char* pucServerRandom,
		unsigned int uiServerRandomLength,
		unsigned int uiPrfAlgID,
		unsigned int uiKeyBitsClientMac,
		long long* phKeyHandleClientMac,
		unsigned int uiKeyBitsServerMac,
		long long* phKeyHandleServerMac,
		unsigned int uiKeyBitsClientEnc,
		long long* phKeyHandleClientEnc,
		unsigned int uiKeyBitsServerEnc,
		long long* phKeyHandleServerEnc);

	SDFCLIENT_API int SDF_GenerateKeywithEPK_SSL(
		long long hSessionHandle,
		unsigned char* pucKeyPreMaster,
		unsigned int uiKeyPreMasterLength,
		unsigned char* pucClientRandom,
		unsigned int uiClientRandomLength,
		unsigned char* pucServerRandom,
		unsigned int uiServerRandomLength,
		unsigned int uiPrfAlgID,
		unsigned int uiEccAlgID,
		ECCrefPublicKey* pucPublicKey,
		unsigned int uiKeyBitsClientMac,
		ECCCipher* pucKeyClientMac,
		long long* phKeyHandleClientMac,
		unsigned int uiKeyBitsServerMac,
		ECCCipher* pucKeyServerMac,
		long long* phKeyHandleServerMac,
		unsigned int uiKeyBitsClientEnc,
		ECCCipher* pucKeyClientEnc,
		long long* phKeyHandleClientEnc,
		unsigned int uiKeyBitsServerEnc,
		ECCCipher* pucKeyServerEnc,
		long long* phKeyHandleServerEnc);

	SDFCLIENT_API int SDF_GenerateKeywithECDHE_SSL(
		long long hSessionHandle,
		long long hKeyHandlePreMaster,
		unsigned char* pucClientRandom,
		unsigned int uiClientRandomLength,
		unsigned char* pucServerRandom,
		unsigned int uiServerRandomLength,
		unsigned int uiPrfAlgID,
		unsigned int uiKeyBitsClientMac,
		long long* phKeyHandleClientMac,
		unsigned int uiKeyBitsServerMac,
		long long* phKeyHandleServerMac,
		unsigned int uiKeyBitsClientEnc,
		long long* phKeyHandleClientEnc,
		unsigned int uiKeyBitsServerEnc,
		long long* phKeyHandleServerEnc);

	SDFCLIENT_API int SDF_GenerateKeywithEPK_ECDHE_SSL(
		long long hSessionHandle,
		long long hKeyHandlePreMaster,
		unsigned char*pucClientRandom,
		unsigned int uiClientRandomLength,
		unsigned char* pucServerRandom,
		unsigned int uiServerRandomLength,
		unsigned int uiPrfAlgID,
		unsigned int uiEccAlgID,
		ECCrefPublicKey* pucPublicKey,
		unsigned int uiKeyBitsClientMac,
		ECCCipher* pucKeyClientMac,
		long long* phKeyHandleClientMac,
		unsigned int uiKeyBitsServerMac,
		ECCCipher* pucKeyServerMac,
		long long* phKeyHandleServerMac,
		unsigned int uiKeyBitsClientEnc,
		ECCCipher * pucKeyClientEnc,
		long long * phKeyHandleClientEnc,
		unsigned int uiKeyBitsServerEnc,
		ECCCipher* pucKeyServerEnc,
		long long* phKeyHandleServerEnc);

#ifdef __cplusplus
}
#endif
#endif	//SDF_CLIENT_H_
/*
 *  Copyright 2014-2024 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gmssl/mem.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4_cbc_mac.h>
#include <gmssl/rand.h>
#include <gmssl/error.h>
#include "sdf.h"


#define SDR_GMSSLERR	(SDR_BASE + 0x00000100)

static const uint8_t zeros[ECCref_MAX_LEN - 32] = {0};


#define SOFTSDF_MAX_KEY_SIZE	64

struct SOFTSDF_KEY {
	uint8_t key[SOFTSDF_MAX_KEY_SIZE];
	size_t key_size;
	struct SOFTSDF_KEY *next;
};

typedef struct SOFTSDF_KEY SOFTSDF_KEY;


struct SOFTSDF_CONTAINER {
	unsigned int key_index;
	SM2_KEY sign_key;
	SM2_KEY enc_key;
	struct SOFTSDF_CONTAINER *next;
};
typedef struct SOFTSDF_CONTAINER SOFTSDF_CONTAINER;

struct SOFTSDF_SESSION {
	SOFTSDF_CONTAINER *container_list;
	SOFTSDF_KEY *key_list;
	SM3_CTX sm3_ctx;
	struct SOFTSDF_SESSION *next;
};
typedef struct SOFTSDF_SESSION SOFTSDF_SESSION;

struct SOFTSDF_DEVICE {
	SOFTSDF_SESSION *session_list;
};
typedef struct SOFTSDF_DEVICE SOFTSDF_DEVICE;

SOFTSDF_DEVICE *deviceHandle = NULL;



#define FILENAME_MAX_LEN 256

int SDF_OpenDevice(
	void **phDeviceHandle)
{
	if (phDeviceHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (deviceHandle != NULL) {
		error_print();
		return SDR_OPENDEVICE;
	}

	deviceHandle = (SOFTSDF_DEVICE *)malloc(sizeof(SOFTSDF_DEVICE));
	if (deviceHandle == NULL) {
		error_print();
		return SDR_OPENDEVICE;
	}
	memset(deviceHandle, 0, sizeof(SOFTSDF_DEVICE));

	*phDeviceHandle = deviceHandle;
	return SDR_OK;
}

/* ReqID:TECC4.0 - swxa 厂商扩展：按配置文件路径打开设备。
 * 真实 swxa 加密机用 swsds.ini 配置设备，SoftSDF 为软件 stub 无需配置，
 * 忽略 pcCfgfile 直接落到标准 SDF_OpenDevice。
 * swxa 适配器模型中设备在进程内只打开一次（disconnect 只关 session 不关
 * device），故此处幂等：已打开时直接返回现有句柄。 */
int SDF_OpenDeviceWithPathAndName(
	char *pcCfgfile,
	void **phDeviceHandle)
{
	(void)pcCfgfile;	/* SoftSDF 无需配置文件 */

	if (deviceHandle != NULL) {
		if (phDeviceHandle != NULL)
			*phDeviceHandle = deviceHandle;
		return SDR_OK;
	}

	return SDF_OpenDevice(phDeviceHandle);
}

int SDF_CloseDevice(
	void *hDeviceHandle)
{
	if (hDeviceHandle != deviceHandle) {
		error_print();
		return SDR_INARGERR;
	}

	if (deviceHandle != NULL) {
		while (deviceHandle->session_list) {
			if (SDF_CloseSession(deviceHandle->session_list) != SDR_OK) {
				error_print();
			}
		}
	}

	memset(deviceHandle, 0, sizeof(SOFTSDF_DEVICE));
	free(deviceHandle);
	deviceHandle = NULL;

	return SDR_OK;
}


int SDF_OpenSession(
	void *hDeviceHandle,
	void **phSessionHandle)
{
	SOFTSDF_SESSION *session;

	if (hDeviceHandle == NULL || hDeviceHandle != deviceHandle) {
		error_print();
		return SDR_INARGERR;
	}

	if (phSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (!(session = (SOFTSDF_SESSION *)malloc(sizeof(*session)))) {
		error_print();
		return SDR_GMSSLERR;
	}
	memset(session, 0, sizeof(*session));

	// append session to session_list
	if (deviceHandle->session_list == NULL) {
		deviceHandle->session_list = session;
	} else {
		SOFTSDF_SESSION *current = deviceHandle->session_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = session;
	}

	*phSessionHandle = session;
	return SDR_OK;
}

int SDF_CloseSession(
	void *hSessionHandle)
{
	SOFTSDF_SESSION *current_session;
	SOFTSDF_SESSION *prev_session;
	SOFTSDF_CONTAINER *current_container;
	SOFTSDF_CONTAINER *next_container;
	SOFTSDF_KEY *current_key;
	SOFTSDF_KEY *next_key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// find hSessionHandle in session_list
	current_session = deviceHandle->session_list;
	prev_session = NULL;
	while (current_session != NULL && current_session != hSessionHandle) {
		prev_session = current_session;
		current_session = current_session->next;
	}
	if (current_session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// free container_list
	current_container = current_session->container_list;
	while (current_container != NULL) {
		next_container = current_container->next;
		memset(current_container, 0, sizeof(*current_container));
		free(current_container);
		current_container = next_container;
	}

	// free key_list
	current_key = current_session->key_list;
	while (current_key != NULL) {
		next_key = current_key->next;
		memset(current_key, 0, sizeof(*current_key));
		free(current_key);
		current_key = next_key;
	}

	// delete current_session from session_list
	if (prev_session == NULL) {
		deviceHandle->session_list = current_session->next;
	} else {
		prev_session->next = current_session->next;
	}
	memset(current_session, 0, sizeof(*current_session));
	free(current_session);

	return SDR_OK;
}

#define SOFTSDF_DEV_DATE	"20240622"
#define SOFTSDF_DEV_BATCH_NUM	"001" // as version.major
#define SOFTSDF_DEV_SERIAL_NUM	"0200" // as version.minor
#define SOFTSDF_DEV_SERIAL	SOFTSDF_DEV_DATE \
				SOFTSDF_DEV_BATCH_NUM \
				SOFTSDF_DEV_SERIAL_NUM

int SDF_GetDeviceInfo(
	void *hSessionHandle,
	DEVICEINFO *pstDeviceInfo)
{
	SOFTSDF_SESSION *session;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pstDeviceInfo == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	memset(pstDeviceInfo, 0, sizeof(*pstDeviceInfo));
	strncpy((char *)pstDeviceInfo->IssuerName, "GmSSL Project (http://gmssl.org)",
		sizeof(pstDeviceInfo->IssuerName));
	strncpy((char *)pstDeviceInfo->DeviceName, "Soft SDF",
		sizeof(pstDeviceInfo->DeviceName));
	strncpy((char *)pstDeviceInfo->DeviceSerial, SOFTSDF_DEV_SERIAL,
		sizeof(pstDeviceInfo->DeviceSerial));
	pstDeviceInfo->DeviceVersion = 1;
	pstDeviceInfo->StandardVersion = 1;
	pstDeviceInfo->AsymAlgAbility[0] = SGD_SM2_1|SGD_SM2_3;
	pstDeviceInfo->AsymAlgAbility[1] = 256;
	pstDeviceInfo->SymAlgAbility = SGD_SM4|SGD_CBC|SGD_MAC;
#if ENABLE_SM4_ECB
	pstDeviceInfo->SymAlgAbility |= SGD_ECB;
#endif
#if ENABLE_SM4_CFB
	pstDeviceInfo->SymAlgAbility |= SGD_CFB;
#endif
#if ENABLE_SM4_OFB
	pstDeviceInfo->SymAlgAbility |= SGD_OFB;
#endif
	pstDeviceInfo->HashAlgAbility = SGD_SM3;
	pstDeviceInfo->BufferSize = 256*1024;

	return SDR_OK;
}

int SDF_GenerateRandom(
	void *hSessionHandle,
	unsigned int uiLength,
	unsigned char *pucRandom)
{
	SOFTSDF_SESSION *session;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucRandom == NULL || uiLength == 0) {
		error_puts("Invalid output buffer or length");
		return SDR_INARGERR;
	}
	if (uiLength > RAND_BYTES_MAX_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (rand_bytes(pucRandom, uiLength) != 1) {
		error_print();
		return SDR_GMSSLERR;
	}

	return SDR_OK;
}

/*
 * softsdf_load_sm2_private_key - 按 PEM 的实际形态加载 SM2 私钥。
 *
 * ReqID:TECC4.0 - 密码机内的设备密钥可以不带访问口令，teccsdfinit 省略
 * -pass 时生成的即是非加密 PKCS#8 PEM。按 PEM 头部区分加载方式：
 *   -----BEGIN ENCRYPTED PRIVATE KEY-----  带口令，用 pass 解密加载
 *   -----BEGIN PRIVATE KEY-----            无口令，直接解析
 * 先判别再加载，避免"先试错再回退"在 stderr 上留下误导性报错。
 */
static int softsdf_load_sm2_private_key(SM2_KEY *key, const char *pass, FILE *file)
{
	char line[128];
	int encrypted = 0;

	if (fgets(line, sizeof(line), file) != NULL &&
		strstr(line, "ENCRYPTED PRIVATE KEY") != NULL) {
		encrypted = 1;
	}
	rewind(file);

	if (!encrypted) {
		return sm2_private_key_info_from_pem(key, file);
	}
	if (pass == NULL || pass[0] == '\0') {
		error_puts("Private key is password protected but no password given");
		return 0;
	}
	return sm2_private_key_info_decrypt_from_pem(key, pass, file);
}

int SDF_GetPrivateKeyAccessRight(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	unsigned char *pucPassword,
	unsigned int uiPwdLength)
{
	int ret = SDR_OK;
	SOFTSDF_SESSION *session;
	SOFTSDF_CONTAINER *container = NULL;
	char *pass = NULL;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	/* ReqID:TECC4.0 - 允许空口令授权。真机密码机内的设备密钥可以不带访问
	 * 口令，这种密钥同样必须先调用本接口（把私钥加载进设备会话容器）才能
	 * 用于 SDF_InternalDecrypt_ECC。原实现直接拒绝空口令，使"无口令设备
	 * 密钥"这一真机形态无法授权、SM2 信封解密必然失败。 */
	pass = (char *)malloc(uiPwdLength + 1);
	if (pass == NULL) {
		error_print();
		return SDR_NOBUFFER;
	}
	if (uiPwdLength > 0 && pucPassword != NULL) {
		memcpy(pass, pucPassword, uiPwdLength);
	}
	pass[uiPwdLength] = 0;
	if (uiPwdLength > 0 && strlen(pass) != uiPwdLength) {
		error_print();
		ret = SDR_INARGERR;
		goto end;
	}

	/* ReqID:TECC4.0 - 真机形态：私钥访问授权索引必须是「设备密钥索引 + 10000」。
	 * 真机实测传裸索引返回 SWR_KEYNOTEXIST(0x01000408)，传 +10000 才成功
	 * （厂商 demo demo/ecc.c:529/572 亦如此，见 libpq fe-swxa-adapter.c 的
	 * SWXA_AUTH_KEY_INDEX_OFFSET）。软件模拟器按同一形态解析，使"漏加偏移"
	 * 这类只在真机上暴露的缺陷能在 SoftSDF 回归里就被拦住；容器按归一化后的
	 * 索引登记，与后续 SDF_InternalDecrypt_ECC 用的裸索引一致。 */
	if (uiKeyIndex < 10000) {
		error_puts("Private key access index must be device key index + 10000"
				   " (the real device returns SWR_KEYNOTEXIST for the bare index)");
		ret = SDR_KEYNOTEXIST;
		goto end;
	}
	uiKeyIndex -= 10000;

	// create container
	container = (SOFTSDF_CONTAINER *)malloc(sizeof(*container));
	if (container == NULL) {
		error_print();
		ret = SDR_NOBUFFER;
		goto end;
	}
	memset(container, 0, sizeof(*container));
	container->key_index = uiKeyIndex;

	// load sign_key
	snprintf(filename, FILENAME_MAX_LEN, "sm2sign-%u.pem", uiKeyIndex);
	file = fopen(filename, "r");
	if (file == NULL) {
		perror("Error opening file");
		fprintf(stderr, "open failure %s\n", filename);
		ret = SDR_KEYNOTEXIST;
		goto end;
	}
	if (softsdf_load_sm2_private_key(&container->sign_key, pass, file) != 1) {
		error_print();
		ret = SDR_GMSSLERR;
		goto end;
	}
	fclose(file);
	file = NULL;

	// load enc_key
	snprintf(filename, FILENAME_MAX_LEN, "sm2enc-%u.pem", uiKeyIndex);
	file = fopen(filename, "r");
	if (file == NULL) {
		perror("Error opening file");
		ret = SDR_KEYNOTEXIST;
		goto end;
	}
	if (softsdf_load_sm2_private_key(&container->enc_key, pass, file) != 1) {
		error_print();
		ret = SDR_GMSSLERR;
		goto end;
	}

	// append container to container_list
	if (session->container_list == NULL) {
		session->container_list = container;
	} else {
		SOFTSDF_CONTAINER *current = session->container_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = container;
	}

	container = NULL;
	ret = SDR_OK;
end:
	if (container) {
		memset(container, 0, sizeof(*container));
		free(container);
	}
	if (pass) {
		memset(pass, 0, uiPwdLength);
		free(pass);
	}
	if (file) fclose(file);
	return ret;
}

int SDF_ReleasePrivateKeyAccessRight(
	void *hSessionHandle,
	unsigned int uiKeyIndex)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_CONTAINER *current_container;
	SOFTSDF_CONTAINER *prev_container;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// delete container in container_list with uiKeyIndex
	/* ReqID:TECC4.0 - 与 SDF_GetPrivateKeyAccessRight 对称：厂商形态的授权索引是
	 * 「设备密钥索引 + 10000」，而容器是按**归一化后的裸索引**登记的（见授权函数
	 * 里的说明），故这里必须做同样的归一化——否则调用方按真机规范传 +10000 索引时，
	 * 释放永远找不到容器（真机上取/放用同一个 +10000 索引，两次都成功）。 */
	if (uiKeyIndex >= 10000) {
		uiKeyIndex -= 10000;
	}

	current_container = session->container_list;
	prev_container = NULL;

	while (current_container != NULL && current_container->key_index != uiKeyIndex) {
		prev_container = current_container;
		current_container = current_container->next;
	}
	if (current_container == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (prev_container == NULL) {
		session->container_list = current_container->next;
	} else {
		prev_container->next = current_container->next;
	}
	memset(current_container, 0, sizeof(*current_container));
	free(current_container);

	return SDR_OK;
}

int SDF_ExportSignPublicKey_RSA(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	RSArefPublicKey *pucPublicKey)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExportEncPublicKey_RSA(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	RSArefPublicKey *pucPublicKey)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_GenerateKeyPair_RSA(
	void *hSessionHandle,
	unsigned int uiKeyBits,
	RSArefPublicKey *pucPublicKey,
	RSArefPrivateKey *pucPrivateKey)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_GenerateKeyWithIPK_RSA(
	void *hSessionHandle,
	unsigned int uiIPKIndex,
	unsigned int uiKeyBits,
	unsigned char *pucKey,
	unsigned int *puiKeyLength,
	void **phKeyHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_GenerateKeyWithEPK_RSA(
	void *hSessionHandle,
	unsigned int uiKeyBits,
	RSArefPublicKey *pucPublicKey,
	unsigned char *pucKey,
	unsigned int *puiKeyLength,
	void **phKeyHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ImportKeyWithISK_RSA(
	void *hSessionHandle,
	unsigned int uiISKIndex,
	unsigned char *pucKey,
	unsigned int uiKeyLength,
	void **phKeyHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExchangeDigitEnvelopeBaseOnRSA(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	RSArefPublicKey *pucPublicKey,
	unsigned char *pucDEInput,
	unsigned int uiDELength,
	unsigned char *pucDEOutput,
	unsigned int *puiDELength)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExportSignPublicKey_ECC(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	ECCrefPublicKey *pucPublicKey)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	SM2_KEY sm2_key;
	SM2_POINT point;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	snprintf(filename, FILENAME_MAX_LEN, "sm2signpub-%u.pem", uiKeyIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (sm2_public_key_info_from_pem(&sm2_key, file) != 1) {
		error_print();
		fclose(file);
		return SDR_KEYNOTEXIST;
	}
	fclose(file);

	if (pucPublicKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	sm2_z256_point_to_bytes(&sm2_key.public_key, (uint8_t *)&point);

	pucPublicKey->bits = 256;
	memset(pucPublicKey->x, 0, ECCref_MAX_LEN - 32);
	memcpy(pucPublicKey->x + ECCref_MAX_LEN - 32, point.x, 32);
	memset(pucPublicKey->y, 0, ECCref_MAX_LEN - 32);
	memcpy(pucPublicKey->y + ECCref_MAX_LEN - 32, point.y, 32);

	return SDR_OK;
}

int SDF_ExportEncPublicKey_ECC(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	ECCrefPublicKey *pucPublicKey)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	SM2_KEY sm2_key;
	SM2_POINT point;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	snprintf(filename, FILENAME_MAX_LEN, "sm2encpub-%u.pem", uiKeyIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (sm2_public_key_info_from_pem(&sm2_key, file) != 1) {
		error_print();
		fclose(file);
		return SDR_KEYNOTEXIST;
	}
	fclose(file);

	if (pucPublicKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	sm2_z256_point_to_bytes(&sm2_key.public_key, (uint8_t *)&point);

	pucPublicKey->bits = 256;
	memset(pucPublicKey->x, 0, ECCref_MAX_LEN - 32);
	memcpy(pucPublicKey->x + ECCref_MAX_LEN - 32, point.x, 32);
	memset(pucPublicKey->y, 0, ECCref_MAX_LEN - 32);
	memcpy(pucPublicKey->y + ECCref_MAX_LEN - 32, point.y, 32);

	return SDR_OK;
}

int SDF_GenerateKeyPair_ECC(
	void *hSessionHandle,
	unsigned int uiAlgID,
	unsigned int uiKeyBits,
	ECCrefPublicKey *pucPublicKey,
	ECCrefPrivateKey *pucPrivateKey)
{
	SOFTSDF_SESSION *session;
	SM2_KEY sm2_key;
	SM2_POINT public_key;
	uint8_t private_key[32];

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM2_1 && uiAlgID != SGD_SM2_3) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiKeyBits != 256) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey == NULL || pucPrivateKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (sm2_key_generate(&sm2_key) != 1) {
		error_print();
		return SDR_GMSSLERR;
	}

	sm2_z256_to_bytes(sm2_key.private_key, private_key);
	sm2_z256_point_to_bytes(&sm2_key.public_key, (uint8_t *)&public_key);

	memset(pucPublicKey, 0, sizeof(*pucPublicKey));
	pucPublicKey->bits = 256;
	memcpy(pucPublicKey->x + ECCref_MAX_LEN - 32, public_key.x, 32);
	memcpy(pucPublicKey->y + ECCref_MAX_LEN - 32, public_key.y, 32);

	memset(pucPrivateKey, 0, sizeof(*pucPrivateKey));
	pucPrivateKey->bits = 256;
	memcpy(pucPrivateKey->K + ECCref_MAX_LEN - 32, private_key, 32);

	memset(&sm2_key, 0, sizeof(sm2_key));
	memset(private_key, 0, sizeof(private_key));
	return SDR_OK;
}

int SDF_GenerateKeyWithIPK_ECC(
	void *hSessionHandle,
	unsigned int uiIPKIndex,
	unsigned int uiKeyBits,
	ECCCipher *pucKey,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file;
	SM2_KEY sm2_key;
	SOFTSDF_KEY *key;
	SM2_CIPHERTEXT ctxt;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	snprintf(filename, FILENAME_MAX_LEN, "sm2encpub-%u.pem", uiIPKIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (sm2_public_key_info_from_pem(&sm2_key, file) != 1) {
		error_print();
		fclose(file);
		return SDR_KEYNOTEXIST;
	}
	fclose(file);

	if (uiKeyBits%8 != 0 || uiKeyBits/8 > SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// generate key
	key = (SOFTSDF_KEY *)malloc(sizeof(*key));
	if (key == NULL) {
		error_print();
		return SDR_NOBUFFER;
	}
	memset(key, 0, sizeof(*key));
	if (rand_bytes(key->key, uiKeyBits/8) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}
	key->key_size = uiKeyBits/8;

	// encrypt key with container
	if (sm2_do_encrypt(&sm2_key, key->key, key->key_size, &ctxt) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}
	memset(pucKey, 0, sizeof(*pucKey));
	memcpy(pucKey->x + ECCref_MAX_LEN - 32, ctxt.point.x, 32);
	memcpy(pucKey->y + ECCref_MAX_LEN - 32, ctxt.point.y, 32);
	memcpy(pucKey->M, ctxt.hash, 32);
	pucKey->L = ctxt.ciphertext_size;
	memcpy(pucKey->C, ctxt.ciphertext, ctxt.ciphertext_size);

	// append key to key_list
	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

int SDF_GenerateKeyWithEPK_ECC(
	void *hSessionHandle,
	unsigned int uiKeyBits,
	unsigned int uiAlgID,
	ECCrefPublicKey *pucPublicKey,
	ECCCipher *pucKey,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	SM2_POINT point;
	SM2_Z256_POINT public_key;
	SM2_KEY sm2_key;
	SOFTSDF_KEY *key;
	SM2_CIPHERTEXT ctxt;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiKeyBits%8 != 0 || uiKeyBits/8 > SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM2_3) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey == NULL || pucKey == NULL || phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// load public key
	memset(&point, 0, sizeof(point));
	memcpy(point.x, pucPublicKey->x + ECCref_MAX_LEN - 32, 32);
	memcpy(point.y, pucPublicKey->y + ECCref_MAX_LEN - 32, 32);
	if (sm2_z256_point_from_bytes(&public_key, (uint8_t *)&point) != 1) {
		error_print();
		return SDR_INARGERR;
	}
	if (sm2_key_set_public_key(&sm2_key, &public_key) != 1) {
		error_print();
		return SDR_INARGERR;
	}

	// generate key
	key = (SOFTSDF_KEY *)malloc(sizeof(*key));
	if (key == NULL) {
		error_print();
		return SDR_NOBUFFER;
	}
	memset(key, 0, sizeof(*key));
	if (rand_bytes(key->key, uiKeyBits/8) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}
	key->key_size = uiKeyBits/8;

	// encrypt key with external public key
	if (sm2_do_encrypt(&sm2_key, key->key, key->key_size, &ctxt) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}
	memset(pucKey, 0, sizeof(*pucKey));
	memcpy(pucKey->x + ECCref_MAX_LEN - 32, ctxt.point.x, 32);
	memcpy(pucKey->y + ECCref_MAX_LEN - 32, ctxt.point.y, 32);
	memcpy(pucKey->M, ctxt.hash, 32);
	pucKey->L = ctxt.ciphertext_size;
	memcpy(pucKey->C, ctxt.ciphertext, ctxt.ciphertext_size);




	// append key to key_list
	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

int SDF_ImportKeyWithISK_ECC(
	void *hSessionHandle,
	unsigned int uiISKIndex,
	ECCCipher *pucKey,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_CONTAINER *container;
	SM2_CIPHERTEXT ctxt;
	SOFTSDF_KEY *key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_puts("Invalid session handle");
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	container = session->container_list;
	while (container != NULL && container->key_index != uiISKIndex) {
		container = container->next;
	}
	if (container == NULL) {
		error_puts("ISK not loaded, call GetPrivateKeyAccess before use ISK\n");
		return SDR_INARGERR;
	}

	if (pucKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (pucKey->L > SM2_MAX_PLAINTEXT_SIZE) {
		error_print();
		return SDR_INARGERR;
	}
	if (pucKey->L > SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// create key
	key = (SOFTSDF_KEY *)malloc(sizeof(*key));
	if (key == NULL) {
		error_print();
		return SDR_NOBUFFER;
	}
	memset(key, 0, sizeof(*key));

	// decrypt key
	memset(&ctxt, 0, sizeof(ctxt));
	memcpy(ctxt.point.x, pucKey->x + ECCref_MAX_LEN - 32, 32);
	memcpy(ctxt.point.y, pucKey->y + ECCref_MAX_LEN - 32, 32);
	memcpy(ctxt.hash, pucKey->M, 32);
	memcpy(ctxt.ciphertext, pucKey->C, pucKey->L);
	ctxt.ciphertext_size = pucKey->L;

	if (sm2_do_decrypt(&container->enc_key, &ctxt, key->key, &key->key_size) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}

	// append key to key_list
	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

int SDF_GenerateAgreementDataWithECC(
	void *hSessionHandle,
	unsigned int uiISKIndex,
	unsigned int uiKeyBits,
	unsigned char *pucSponsorID,
	unsigned int uiSponsorIDLength,
	ECCrefPublicKey *pucSponsorPublicKey,
	ECCrefPublicKey *pucSponsorTmpPublicKey,
	void **phAgreementHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_GenerateKeyWithECC(
	void *hSessionHandle,
	unsigned char *pucResponseID,
	unsigned int uiResponseIDLength,
	ECCrefPublicKey *pucResponsePublicKey,
	ECCrefPublicKey *pucResponseTmpPublicKey,
	void *hAgreementHandle,
	void **phKeyHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_GenerateAgreementDataAndKeyWithECC(
	void *hSessionHandle,
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
	void **phKeyHandle)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExchangeDigitEnvelopeBaseOnECC(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	unsigned int uiAlgID,
	ECCrefPublicKey *pucPublicKey,
	ECCCipher *pucEncDataIn,
	ECCCipher *pucEncDataOut)
{
	error_print();
	return SDR_NOTSUPPORT;
}

// XXX: `SDF_GenerateKeyWithKEK` use CBC-Padding, so the `pucKey` can not be decrypted by `SDF_Decrypt`
int SDF_GenerateKeyWithKEK(
	void *hSessionHandle,
	unsigned int uiKeyBits,
	unsigned int uiAlgID,
	unsigned int uiKEKIndex,
	unsigned char *pucKey,
	unsigned int *puiKeyLength,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file;
	uint8_t kek[16];
	SM4_KEY sm4_key;
	uint8_t *iv;
	uint8_t *enced;
	size_t enced_len;
	SOFTSDF_KEY *key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiKeyBits % 8 != 0 || uiKeyBits/8 > SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM4_CBC) {
		error_print();
		return SDR_INARGERR;
	}

	// load KEK file with index
	snprintf(filename, FILENAME_MAX_LEN, "kek-%u.key", uiKEKIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "open file: %s\n", filename);
		error_print();
		return SDR_KEYNOTEXIST;
	}

	size_t rlen;
	if ((rlen = fread(kek, 1, sizeof(kek), file)) != sizeof(kek)) {


		printf("rlen = %zu\n", rlen);
		perror("fread");
		error_print();
		fclose(file);
		return SDR_INARGERR;
	}
	fclose(file);

	if (pucKey == NULL || puiKeyLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// generate key
	key = (SOFTSDF_KEY *)malloc(sizeof(SOFTSDF_KEY));
	if (key == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	memset(key, 0, sizeof(*key));

	iv = pucKey;
	enced = pucKey + SM4_BLOCK_SIZE;

	if (rand_bytes(iv, SM4_BLOCK_SIZE) != 1) {
		error_print();
		return SDR_GMSSLERR;
	}

	key->key_size = uiKeyBits/8;
	if (rand_bytes(key->key, key->key_size) != 1) {
		error_print();
		free(key);
		return SDR_GMSSLERR;
	}

	sm4_set_encrypt_key(&sm4_key, kek);
	if (sm4_cbc_padding_encrypt(&sm4_key, iv, key->key, key->key_size, enced, &enced_len) != 1) {
		error_print();
		memset(&sm4_key, 0, sizeof(sm4_key));
		free(key);
		return SDR_GMSSLERR;
	}
	memset(&sm4_key, 0, sizeof(sm4_key));
	*puiKeyLength = 16 + enced_len;

	// append key to key_list
	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

int SDF_ImportKeyWithKEK(
	void *hSessionHandle,
	unsigned int uiAlgID,
	unsigned int uiKEKIndex,
	unsigned char *pucKey,
	unsigned int uiKeyLength,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file;
	uint8_t kek[16];
	SM4_KEY sm4_key;
	const uint8_t *iv;
	const uint8_t *enced;
	size_t enced_len;
	SOFTSDF_KEY *key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM4_CBC) {
		error_print();
		return SDR_INARGERR;
	}

	// load KEK file with index
	snprintf(filename, FILENAME_MAX_LEN, "kek-%u.key", uiKEKIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (fread(kek, 1, sizeof(kek), file) != sizeof(kek)) {
		error_print();
		fclose(file);
		return SDR_INARGERR;
	}
	fclose(file);


	// decrypt SM4-CBC encrypted pucKey
	if (pucKey == NULL || uiKeyLength <= SM4_BLOCK_SIZE) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiKeyLength > SM4_BLOCK_SIZE + SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	key = (SOFTSDF_KEY *)malloc(sizeof(SOFTSDF_KEY));
	if (key == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	memset(key, 0, sizeof(*key));

	iv = pucKey;
	enced = pucKey + SM4_BLOCK_SIZE;
	enced_len = uiKeyLength - SM4_BLOCK_SIZE;

	sm4_set_decrypt_key(&sm4_key, kek);
	if (sm4_cbc_padding_decrypt(&sm4_key, iv, enced, enced_len, key->key, &key->key_size) != 1) {
		error_print();
		memset(&sm4_key, 0, sizeof(sm4_key));
		free(key);
		return SDR_GMSSLERR;
	}
	memset(&sm4_key, 0, sizeof(sm4_key));

	// append key to key_list
	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

/* ReqID:TECC4.0 - 裸导入明文会话密钥（厂商扩展接口，非标准 GM/T 0018；标准为 SDF_ImportKeyWithKEK）。
 * 工程 fe-hsm-adapter.c 的 sm4_encrypt/sm4_decrypt 用调用方传入的 DEK 明文做 SM4 加解密，
 * 故需支持明文导入会话密钥并返回句柄（不经 KEK 解密）。 */
int SDF_ImportKey(
	void *hSessionHandle,
	unsigned char *pucKey,
	unsigned int uiKeyLength,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL || pucKey == NULL || phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiKeyLength == 0 || uiKeyLength > SOFTSDF_MAX_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	key = (SOFTSDF_KEY *)malloc(sizeof(SOFTSDF_KEY));
	if (key == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	memset(key, 0, sizeof(*key));
	memcpy(key->key, pucKey, uiKeyLength);
	key->key_size = uiKeyLength;

	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

/* ReqID:TECC4.0 - swxa 厂商扩展：按索引获取加密机内预置对称 KEK。
 * 真实 swxa 加密机的 KEK 由管理员预先注入，客户端按 key_index 引用。
 * SoftSDF 对应：读 kek-<index>.key（teccsdfinit -kek <index> 生成），
 * 加载到会话 key_list 返回句柄，与 SDF_GenerateKeyWithKEK 的 KEK 机制一致。 */
int SDF_GetSymmKeyHandle(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	void **phKeyHandle)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *key;
	char filename[FILENAME_MAX_LEN];
	uint8_t kek[16];
	FILE *file;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL || phKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	/* load KEK file with index */
	snprintf(filename, FILENAME_MAX_LEN, "kek-%u.key", uiKeyIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "open file: %s\n", filename);
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (fread(kek, 1, sizeof(kek), file) != sizeof(kek)) {
		fclose(file);
		error_print();
		return SDR_INARGERR;
	}
	fclose(file);

	key = (SOFTSDF_KEY *)malloc(sizeof(SOFTSDF_KEY));
	if (key == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	memset(key, 0, sizeof(*key));
	memcpy(key->key, kek, sizeof(kek));
	key->key_size = sizeof(kek);

	if (session->key_list == NULL) {
		session->key_list = key;
	} else {
		SOFTSDF_KEY *current = session->key_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = key;
	}

	*phKeyHandle = key;
	return SDR_OK;
}

int SDF_DestroyKey(
	void *hSessionHandle,
	void *hKeyHandle)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *current;
	SOFTSDF_KEY *prev;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (hKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	current = session->key_list;
	{
		assert(current != NULL);
	}
	prev = NULL;
	while (current != NULL && current != (SOFTSDF_KEY *)hKeyHandle) {
		prev = current;
		current = current->next;
	}
	if (current == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (prev == NULL) {
		session->key_list = current->next;
	} else {
		prev->next = current->next;
	}
	memset(current, 0, sizeof(SOFTSDF_KEY));
	free(current);

	return SDR_OK;
}

int SDF_ExternalPublicKeyOperation_RSA(
	void *hSessionHandle,
	RSArefPublicKey *pucPublicKey,
	unsigned char *pucDataInput,
	unsigned int uiInputLength,
	unsigned char *pucDataOutput,
	unsigned int *puiOutputLength)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExternalPrivateKeyOperation_RSA(
	void *hSessionHandle,
	RSArefPrivateKey *pucPrivateKey,
	unsigned char *pucDataInput,
	unsigned int uiInputLength,
	unsigned char *pucDataOutput,
	unsigned int *puiOutputLength)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_InternalPrivateKeyOperation_RSA(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	unsigned char *pucDataInput,
	unsigned int uiInputLength,
	unsigned char *pucDataOutput,
	unsigned int *puiOutputLength)
{
	error_print();
	return SDR_NOTSUPPORT;
}

int SDF_ExternalVerify_ECC(
	void *hSessionHandle,
	unsigned int uiAlgID,
	ECCrefPublicKey *pucPublicKey,
	unsigned char *pucDataInput,
	unsigned int uiInputLength,
	ECCSignature *pucSignature)
{
	SOFTSDF_SESSION *session;
	SM2_POINT point;
	SM2_Z256_POINT public_key;
	SM2_KEY sm2_key;
	SM2_SIGNATURE sig;
	unsigned int i;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM2_1) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey->bits != 256) {
		error_print();
		return SDR_INARGERR;
	}

	// load public key
	memset(&point, 0, sizeof(point));
	memcpy(point.x, pucPublicKey->x + ECCref_MAX_LEN - 32, 32);
	memcpy(point.y, pucPublicKey->y + ECCref_MAX_LEN - 32, 32);
	if (sm2_z256_point_from_bytes(&public_key, (uint8_t *)&point) != 1) {
		error_print();
		return -1;
	}
	if (sm2_key_set_public_key(&sm2_key, &public_key) != 1) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucDataInput == NULL || uiInputLength != 32) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucSignature == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucSignature->r[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucSignature->s[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}

	memcpy(sig.r, pucSignature->r + ECCref_MAX_LEN - 32, 32);
	memcpy(sig.s, pucSignature->s + ECCref_MAX_LEN - 32, 32);

	if (sm2_do_verify(&sm2_key, pucDataInput, &sig) != 1) {
		error_print();
		return SDR_VERIFYERR;
	}

	return SDR_OK;
}

int SDF_InternalSign_ECC(
	void *hSessionHandle,
	unsigned int uiISKIndex,
	unsigned char *pucData,
	unsigned int uiDataLength,
	ECCSignature *pucSignature)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_CONTAINER *container;
	SM2_SIGNATURE sig;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// find container with key index
	container = session->container_list;
	while (container != NULL && container->key_index != uiISKIndex) {
		container = container->next;
	}
	if (container == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucData == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiDataLength != SM3_DIGEST_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucSignature == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (sm2_do_sign(&container->sign_key, pucData, &sig) != 1) {
		error_print();
		return SDR_GMSSLERR;
	}

	memset(pucSignature, 0, sizeof(*pucSignature));
	memcpy(pucSignature->r + ECCref_MAX_LEN - 32, sig.r, 32);
	memcpy(pucSignature->s + ECCref_MAX_LEN - 32, sig.s, 32);

	return SDR_OK;
}

int SDF_InternalVerify_ECC(
	void *hSessionHandle,
	unsigned int uiIPKIndex,
	unsigned char *pucData,
	unsigned int uiDataLength,
	ECCSignature *pucSignature)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	SM2_KEY sm2_key;
	SM2_SIGNATURE sig;
	unsigned int i;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// load public key from file
	snprintf(filename, FILENAME_MAX_LEN, "sm2signpub-%u.pem", uiIPKIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (sm2_public_key_info_from_pem(&sm2_key, file) != 1) {
		error_print();
		fclose(file);
		return SDR_KEYNOTEXIST;
	}
	fclose(file);

	if (pucData == NULL || uiDataLength != SM3_DIGEST_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucSignature == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucSignature->r[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucSignature->s[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}

	memcpy(sig.r, pucSignature->r + ECCref_MAX_LEN - 32, 32);
	memcpy(sig.s, pucSignature->s + ECCref_MAX_LEN - 32, 32);

	if (sm2_do_verify(&sm2_key, pucData, &sig) != 1) {
		error_print();
		return SDR_VERIFYERR;
	}

	return SDR_OK;
}

int SDF_ExternalEncrypt_ECC(
	void *hSessionHandle,
	unsigned int uiAlgID,
	ECCrefPublicKey *pucPublicKey,
	unsigned char *pucData,
	unsigned int uiDataLength,
	ECCCipher *pucEncData)
{
	SOFTSDF_SESSION *session;
	SM2_POINT point;
	SM2_Z256_POINT public_key;
	SM2_KEY sm2_key;
	SM2_CIPHERTEXT ctxt;
	unsigned int i;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM2_3) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucPublicKey->bits != 256) {
		error_print();
		return SDR_INARGERR;
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucPublicKey->x[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}
	for (i = 0; i < ECCref_MAX_LEN - 32; i++) {
		if (pucPublicKey->y[i] != 0) {
			error_print();
			return SDR_INARGERR;
		}
	}

	// parse public key
	memset(&point, 0, sizeof(point));
	memcpy(point.x, pucPublicKey->x + ECCref_MAX_LEN - 32, 32);
	memcpy(point.y, pucPublicKey->y + ECCref_MAX_LEN - 32, 32);
	if (sm2_z256_point_from_bytes(&public_key, (uint8_t *)&point) != 1) {
		error_print();
		return SDR_INARGERR;
	}
	if (sm2_key_set_public_key(&sm2_key, &public_key) != 1) {
		error_print();
		return SDR_INARGERR;
	}

	if (!pucData) {
		error_print();
		return SDR_INARGERR;
	}

	if(uiDataLength <=0 || uiDataLength > SM2_MAX_PLAINTEXT_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (sm2_do_encrypt(&sm2_key, pucData, uiDataLength, &ctxt) != 1) {
		error_print();
		return SDR_GMSSLERR;
	}

	memset(pucEncData, 0, sizeof(*pucEncData));
	memcpy(pucEncData->x + ECCref_MAX_LEN - 32, ctxt.point.x, 32);
	memcpy(pucEncData->y + ECCref_MAX_LEN - 32, ctxt.point.y, 32);
	memcpy(pucEncData->M, ctxt.hash, 32);
	pucEncData->L = ctxt.ciphertext_size;
	memcpy(pucEncData->C, ctxt.ciphertext, ctxt.ciphertext_size);

	return SDR_OK;
}

int SDF_Encrypt(
	void *hSessionHandle,
	void *hKeyHandle,
	unsigned int uiAlgID,
	unsigned char *pucIV, // XXX: IV is updated after calling
	unsigned char *pucData,
	unsigned int uiDataLength,
	unsigned char *pucEncData,
	unsigned int *puiEncDataLength)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *key;
	SM4_KEY sm4_key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (hKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	key = session->key_list;
	while (key != NULL && key != (SOFTSDF_KEY *)hKeyHandle) {
		key = key->next;
	}
	if (key == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (key->key_size < SM4_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucData == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (puiEncDataLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	switch (uiAlgID) {
	case SGD_SM4_CBC:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		if (uiDataLength % 16) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#if ENABLE_SM4_ECB
	case SGD_SM4_ECB:
		if (uiDataLength % 16) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
#if ENABLE_SM4_CFB
	case SGD_SM4_CFB:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
#if ENABLE_SM4_OFB
	case SGD_SM4_OFB:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
	default:
		error_print();
		return SDR_INARGERR;
	}

	// XXX: change this when add CBC-Padding mode
	*puiEncDataLength = uiDataLength;

	if (pucEncData == NULL) {
		return SDR_OK;
	}

	// TODO: cache `SM4_KEY` in `SOFTSDF_KEY`, reduce cost of calling `sm4_set_encrypt_key`
	sm4_set_encrypt_key(&sm4_key, key->key);

	switch (uiAlgID) {
	case SGD_SM4_CBC:
		sm4_cbc_encrypt_blocks(&sm4_key, pucIV, pucData, uiDataLength/16, pucEncData);
		break;
#if ENABLE_SM4_ECB
	case SGD_SM4_ECB:
		sm4_encrypt_blocks(&sm4_key, pucData, uiDataLength/16, pucEncData);
		break;
#endif
#if ENABLE_SM4_CFB
	case SGD_SM4_CFB:
		sm4_cfb_encrypt(&sm4_key, SM4_CFB_128, pucIV, pucData, uiDataLength, pucEncData);
		break;
#endif
#if ENABLE_SM4_OFB
	case SGD_SM4_OFB:
		sm4_ofb_encrypt(&sm4_key, pucIV, pucData, uiDataLength, pucEncData);
		break;
#endif
	default:
		gmssl_secure_clear(&sm4_key, sizeof(sm4_key));
		error_print();
		return SDR_INARGERR;
	}

	gmssl_secure_clear(&sm4_key, sizeof(sm4_key));

	return SDR_OK;
}

int SDF_Decrypt(
	void *hSessionHandle,
	void *hKeyHandle,
	unsigned int uiAlgID,
	unsigned char *pucIV, // XXX: IV is updated after calling
	unsigned char *pucEncData,
	unsigned int uiEncDataLength,
	unsigned char *pucData,
	unsigned int *puiDataLength)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *key;
	SM4_KEY sm4_key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (hKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	key = session->key_list;
	while (key != NULL && key != (SOFTSDF_KEY *)hKeyHandle) {
		key = key->next;
	}
	if (key == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (key->key_size < SM4_KEY_SIZE) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucEncData == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (puiDataLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	switch (uiAlgID) {
	case SGD_SM4_CBC:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		if (uiEncDataLength % 16) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#if ENABLE_SM4_ECB
	case SGD_SM4_ECB:
		if (uiEncDataLength % 16) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
#if ENABLE_SM4_CFB
	case SGD_SM4_CFB:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
#if ENABLE_SM4_OFB
	case SGD_SM4_OFB:
		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		break;
#endif
	default:
		error_print();
		return SDR_INARGERR;
	}

	*puiDataLength = uiEncDataLength;

	if (pucData == NULL) {
		return SDR_OK;
	}

	// TODO: cache `SM4_KEY` in `SOFTSDF_KEY`, reduce cost of calling `sm4_set_encrypt_key`

	switch (uiAlgID) {
	case SGD_SM4_CBC:
		sm4_set_decrypt_key(&sm4_key, key->key);
		sm4_cbc_decrypt_blocks(&sm4_key, pucIV, pucEncData, uiEncDataLength/16, pucData);
		break;
#if ENABLE_SM4_ECB
	case SGD_SM4_ECB:
		sm4_set_decrypt_key(&sm4_key, key->key);
		sm4_encrypt_blocks(&sm4_key, pucEncData, uiEncDataLength/16, pucData);
		break;
#endif
#if ENABLE_SM4_CFB
	case SGD_SM4_CFB:
		sm4_set_encrypt_key(&sm4_key, key->key);
		sm4_cfb_decrypt(&sm4_key, SM4_CFB_128, pucIV, pucEncData, uiEncDataLength, pucData);
		break;
#endif
#if ENABLE_SM4_OFB
	case SGD_SM4_OFB:
		sm4_set_encrypt_key(&sm4_key, key->key);
		sm4_ofb_encrypt(&sm4_key, pucIV, pucEncData, uiEncDataLength, pucData);
		break;
#endif
	default:
		gmssl_secure_clear(&sm4_key, sizeof(sm4_key));
		error_print();
		return SDR_INARGERR;
	}

	gmssl_secure_clear(&sm4_key, sizeof(sm4_key));

	return SDR_OK;
}

int SDF_CalculateMAC(
	void *hSessionHandle,
	void *hKeyHandle,
	unsigned int uiAlgID,
	unsigned char *pucIV,
	unsigned char *pucData,
	unsigned int uiDataLength,
	unsigned char *pucMAC,
	unsigned int *puiMACLength)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_KEY *key;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (hKeyHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	key = session->key_list;
	while (key != NULL && key != (SOFTSDF_KEY *)hKeyHandle) {
		key = key->next;
	}
	if (key == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucData == NULL || uiDataLength <= 0) {
		error_print();
		return SDR_INARGERR;
	}

	if (puiMACLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID == SGD_SM3) {
		SM3_HMAC_CTX hmac_ctx;

		if (pucIV != NULL) {
			error_print();
			return SDR_INARGERR;
		}
		if (key->key_size < 12) {
			error_print();
			return SDR_INARGERR;
		}

		*puiMACLength = SM3_HMAC_SIZE;

		if (!pucMAC) {
			return SDR_OK;
		}

		sm3_hmac_init(&hmac_ctx, key->key, key->key_size);
		sm3_hmac_update(&hmac_ctx, pucData, uiDataLength);
		sm3_hmac_finish(&hmac_ctx, pucMAC);

		memset(&hmac_ctx, 0, sizeof(hmac_ctx));

	} else if (uiAlgID == SGD_SM4_MAC) {
		SM4_CBC_MAC_CTX cbc_mac_ctx;

		if (pucIV == NULL) {
			error_print();
			return SDR_INARGERR;
		}
		if (key->key_size < SM4_KEY_SIZE) {
			error_print();
			return SDR_INARGERR;
		}
		*puiMACLength = SM4_CBC_MAC_SIZE;

		if (!pucMAC) {
			return SDR_OK;
		}

		sm4_cbc_mac_init(&cbc_mac_ctx, key->key);
		memcpy(cbc_mac_ctx.iv, pucIV, SM4_BLOCK_SIZE);
		sm4_cbc_mac_update(&cbc_mac_ctx, pucData, uiDataLength);
		sm4_cbc_mac_finish(&cbc_mac_ctx, pucMAC);
		memcpy(pucIV, cbc_mac_ctx.iv, SM4_BLOCK_SIZE);

		memset(&cbc_mac_ctx, 0, sizeof(cbc_mac_ctx));

	} else {
		error_print();
		return SDR_INARGERR;
	}

	return SDR_OK;
}

int SDF_HashInit(
	void *hSessionHandle,
	unsigned int uiAlgID,
	ECCrefPublicKey *pucPublicKey,
	unsigned char *pucID,
	unsigned int uiIDLength)
{
	SOFTSDF_SESSION *session;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (uiAlgID != SGD_SM3) {
		error_print();
		return SDR_INARGERR;
	}

	// FIXME: check step or return SDR_STEPERR;
	sm3_init(&session->sm3_ctx);

	if (pucPublicKey != NULL) {

		SM2_POINT point;
		SM2_Z256_POINT public_key;
		uint8_t z[32];

		if (pucID == NULL || uiIDLength <= 0) {
			error_print();
			return SDR_INARGERR;
		}

		memset(&point, 0, sizeof(point));
		memcpy(point.x, pucPublicKey->x + ECCref_MAX_LEN - 32, 32);
		memcpy(point.y, pucPublicKey->y + ECCref_MAX_LEN - 32, 32);
		if (sm2_z256_point_from_bytes(&public_key, (uint8_t *)&point) != 1) {
			error_print();
			return SDR_INARGERR;
		}

		if (sm2_compute_z(z, &public_key, (const char *)pucID, uiIDLength) != 1) {
			error_print();
			return SDR_GMSSLERR;
		}
		sm3_update(&session->sm3_ctx, z, sizeof(z));
	}

	return SDR_OK;
}

int SDF_HashUpdate(
	void *hSessionHandle,
	unsigned char *pucData,
	unsigned int uiDataLength)
{
	SOFTSDF_SESSION *session;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucData == NULL || uiDataLength <= 0) {
		error_print();
		return SDR_INARGERR;
	}

	sm3_update(&session->sm3_ctx, pucData, uiDataLength);

	return SDR_OK;
}

int SDF_HashFinal(void *hSessionHandle,
	unsigned char *pucHash,
	unsigned int *puiHashLength)
{
	SOFTSDF_SESSION *session;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucHash == NULL || puiHashLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	sm3_finish(&session->sm3_ctx, pucHash);

	*puiHashLength = SM3_DIGEST_SIZE;
	return SDR_OK;
}

int SDF_CreateFile(
	void *hSessionHandle,
	unsigned char *pucFileName,
	unsigned int uiNameLen,
	unsigned int uiFileSize)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	uint8_t buf[1024] = {0};
	size_t i;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucFileName == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiNameLen <= 0 || uiNameLen >= FILENAME_MAX_LEN - 5) {
		error_print();
		return SDR_INARGERR;
	}
	memcpy(filename, pucFileName, uiNameLen);
	filename[uiNameLen] = 0;
	if (strlen(filename) != uiNameLen) {
		error_print();
		return SDR_INARGERR;
	}
	strcat(filename, ".file");

	if (uiFileSize > 64 * 1024) {
		error_print();
		return SDR_INARGERR;
	}

	file = fopen(filename, "wb");
	if (file == NULL) {
		error_puts("Failed to create file");
		return SDR_GMSSLERR;
	}
	for (i = 0; i < uiFileSize/sizeof(buf); i++) {
		fwrite(buf, 1, sizeof(buf), file);
	}
	fwrite(buf, 1, uiFileSize % sizeof(buf), file);
	fclose(file);

	return SDR_OK;
}

int SDF_ReadFile(
	void *hSessionHandle,
	unsigned char *pucFileName,
	unsigned int uiNameLen,
	unsigned int uiOffset,
	unsigned int *puiReadLength,
	unsigned char *pucBuffer)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	size_t bytesRead;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucFileName == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiNameLen <= 0 || uiNameLen >= FILENAME_MAX_LEN - 5) {
		error_print();
		return SDR_INARGERR;
	}
	memcpy(filename, pucFileName, uiNameLen);
	filename[uiNameLen] = 0;
	if (strlen(filename) != uiNameLen) {
		error_print();
		return SDR_INARGERR;
	}
	strcat(filename, ".file");

	if (puiReadLength == NULL || *puiReadLength <= 0) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucBuffer == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	if (fseek(file, uiOffset, SEEK_SET) != 0) {
		fclose(file);
		error_print();
		return SDR_GMSSLERR;
	}
	bytesRead = fread(pucBuffer, 1, *puiReadLength, file);
	if (bytesRead == 0) {
		error_print();
		fclose(file);
		return SDR_GMSSLERR;
	}
	fclose(file);

	*puiReadLength = bytesRead;
	return SDR_OK;
}

int SDF_WriteFile(
	void *hSessionHandle,
	unsigned char *pucFileName,
	unsigned int uiNameLen,
	unsigned int uiOffset,
	unsigned int uiWriteLength,
	unsigned char *pucBuffer)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	size_t bytesWritten;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucFileName == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiNameLen <= 0 || uiNameLen >= FILENAME_MAX_LEN - 5) {
		error_print();
		return SDR_INARGERR;
	}
	memcpy(filename, pucFileName, uiNameLen);
	filename[uiNameLen] = 0;
	if (strlen(filename) != uiNameLen) {
		error_print();
		return SDR_INARGERR;
	}
	strcat(filename, ".file");

	if (uiWriteLength <= 0) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiWriteLength > 64 * 1024) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucBuffer == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	file = fopen(filename, "wb");
	if (file == NULL) {
		error_print();
		return SDR_GMSSLERR;
	}
	if (fseek(file, uiOffset, SEEK_SET) != 0) {
		error_print();
		fclose(file);
		return SDR_GMSSLERR;
	}
	bytesWritten = fwrite(pucBuffer, 1, uiWriteLength, file);
	if (bytesWritten != uiWriteLength) {
		error_print();
		fclose(file);
		return SDR_GMSSLERR;
	}
	fclose(file);

	return SDR_OK;
}

int SDF_DeleteFile(
	void *hSessionHandle,
	unsigned char *pucFileName,
	unsigned int uiNameLen)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (hSessionHandle == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucFileName == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiNameLen <= 0 || uiNameLen >= FILENAME_MAX_LEN - 5) {
		error_print();
		return SDR_INARGERR;
	}
	memcpy(filename, pucFileName, uiNameLen);
	filename[uiNameLen] = 0;
	if (strlen(filename) != uiNameLen) {
		error_print();
		return SDR_INARGERR;
	}
	strcat(filename, ".file");

	if (remove(filename) != 0) {
		error_print();
		return SDR_GMSSLERR;
	}

	return SDR_OK;
}

int SDF_InternalEncrypt_ECC(
	void *hSessionHandle,
	unsigned int uiIPKIndex,
	unsigned int uiAlgID,
	unsigned char *pucData,
	unsigned int uiDataLength,
	ECCCipher *pucEncData)
{
	SOFTSDF_SESSION *session;
	char filename[FILENAME_MAX_LEN];
	FILE *file = NULL;
	SM2_KEY sm2_key;
	SM2_CIPHERTEXT ciphertext;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// load public key by uiISKIndex
	snprintf(filename, FILENAME_MAX_LEN, "sm2encpub-%u.pem", uiIPKIndex);
	file = fopen(filename, "rb");
	if (file == NULL) {
		error_print();
		return SDR_KEYNOTEXIST;
	}
	if (sm2_public_key_info_from_pem(&sm2_key, file) != 1) {
		error_print();
		fclose(file);
		return SDR_KEYNOTEXIST;
	}
	fclose(file);

	// check uiAlgID
	if (uiAlgID != SGD_SM2_3) {
		error_print();
		return SDR_ALGNOTSUPPORT;
	}

	if (pucData == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (uiDataLength > SM2_MAX_PLAINTEXT_SIZE) {
		error_print();
		return SDR_INARGERR;
	}
	if (pucEncData == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// encrypt
	if (sm2_do_encrypt(&sm2_key, pucData, uiDataLength, &ciphertext) != 1) {
		error_print();
		return SDR_PKOPERR;
	}
	memset(pucEncData->x, 0, ECCref_MAX_LEN - 32);
	memcpy(pucEncData->x + ECCref_MAX_LEN - 32, ciphertext.point.x, 32);
	memset(pucEncData->y, 0, ECCref_MAX_LEN - 32);
	memcpy(pucEncData->y + ECCref_MAX_LEN - 32, ciphertext.point.y, 32);
	memcpy(pucEncData->M, ciphertext.hash, 32);
	memcpy(pucEncData->C, ciphertext.ciphertext, ciphertext.ciphertext_size);
	pucEncData->L = (unsigned int)ciphertext.ciphertext_size;

	return SDR_OK;
}

int SDF_InternalDecrypt_ECC(
	void *hSessionHandle,
	unsigned int uiISKIndex,
	unsigned int uiAlgID,
	ECCCipher *pucEncData,
	unsigned char *pucData,
	unsigned int *puiDataLength)
{
	SOFTSDF_SESSION *session;
	SOFTSDF_CONTAINER *container;
	SM2_CIPHERTEXT ciphertext;
	size_t plaintext_len;

	if (deviceHandle == NULL) {
		error_print();
		return SDR_STEPERR;
	}

	if (!hSessionHandle) {
		error_print();
		return SDR_INARGERR;
	}
	session = deviceHandle->session_list;
	while (session != NULL && session != hSessionHandle) {
		session = session->next;
	}
	if (session == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// load public key by uiISKIndex
	container = session->container_list;
	while (container != NULL && container->key_index != uiISKIndex) {
		container = container->next;
	}
	if (container == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	// check uiAlgID
	if (uiAlgID != SGD_SM2_3) {
		error_print();
		return SDR_ALGNOTSUPPORT;
	}

	// check ciphertext
	if (pucEncData == NULL) {
		error_print();
		return SDR_INARGERR;
	}
	if (pucEncData->L == 0) {
		error_print();
		return SDR_INARGERR;
	}

	/* ReqID:TECC4.0 - 按真机容量上限拒绝超长信封：真机 ECCCipher.C 固定
	 * ECCCIPHER_MAX_C（136）字节，装不下时返回 SWR_ECCCIPHER_TOO_LONG
	 * （0x01000804）。SoftSDF 内部布局留了 C[1]+C_[254]，并自带长明文解密
	 * 流程，于是这类信封只在软件密码机上能解开、在真机上必然失败——替身给出
	 * 了设备能力上限之外的「成功」。当前设备 KEK 下的 HE 私钥信封已按 128
	 * 字节分块（TECC_HE_ENV_CHUNK_MAX），正常用例不会触及本上限；本校验拦的
	 * 是分块机制之前创建的旧信封。此处不打印（真机也只写设备日志），诊断由
	 * 调用方按返回码给出（libpq 侧 fe-swxa-adapter 会打 ERROR ... 0x01000804），
	 * 避免污染回归 golden 的输出。 */
	if (pucEncData->L > ECCCIPHER_MAX_C) {
		return SWR_ECCCIPHER_TOO_LONG;
	}

	if (puiDataLength == NULL) {
		error_print();
		return SDR_INARGERR;
	}

	if (pucData == NULL) {
		*puiDataLength = pucEncData->L;
		return SDR_OK;
	}

	// convert ECCCipher to SM2_CIPHERTEXT
	if (memcmp(pucEncData->x, zeros, ECCref_MAX_LEN - 32) != 0) {
		error_print();
		return SDR_INARGERR;
	}
	if (memcmp(pucEncData->y, zeros, ECCref_MAX_LEN - 32) != 0) {
		error_print();
		return SDR_INARGERR;
	}
	memcpy(ciphertext.point.x, pucEncData->x + ECCref_MAX_LEN - 32, 32);
	memcpy(ciphertext.point.y, pucEncData->y + ECCref_MAX_LEN - 32, 32);
	memcpy(ciphertext.hash, pucEncData->M, 32);
	memcpy(ciphertext.ciphertext, pucEncData->C, pucEncData->L);
	ciphertext.ciphertext_size = pucEncData->L;

	if (sm2_do_decrypt(&container->enc_key, &ciphertext, pucData, &plaintext_len) != 1) {
		error_print();
		return SDR_PKOPERR;
	}

	*puiDataLength = (unsigned int)plaintext_len;
	return SDR_OK;
}

int SDF_InternalPublicKeyOperation_RSA(
	void *hSessionHandle,
	unsigned int uiKeyIndex,
	unsigned char *pucDataInput,
	unsigned int uiInputLength,
	unsigned char *pucDataOutput,
	unsigned int *puiOutputLength)
{
	error_print();
	return SDR_NOTSUPPORT;
}

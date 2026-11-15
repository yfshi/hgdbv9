/*-------------------------------------------------------------------------
 *
 * fe-swxa-adapter.c
 *	  三未信安 (SWXA) HSM adapter implementation
 *
 * Implements HsmAdapterInterface for 三未信安 password machines
 * using the SDF interface loaded at runtime via dlopen/dlsym.
 *
 * Portions Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * src/interfaces/libpq/tecc_hsm_adapter/fe-swxa-adapter.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
/* Windows lacks these POSIX dlopen flags.  The PostgreSQL port layer
 * provides dlopen() on Windows via LoadLibrary, which ignores mode. */
#define RTLD_LAZY     0
#define RTLD_LOCAL    0
#define RTLD_DEEPBIND 0
#endif

#include <dlfcn.h>
#ifndef _MSC_VER
#include <link.h>			/* dlinfo / RTLD_DI_ORIGIN */
#endif
#include <openssl/crypto.h>	/* OPENSSL_malloc/free for sm2_decrypt output */

#include "fe-swxa-adapter.h"
#include "fe-hsm-adapter.h"

/*
 * ReqID:TECC4.0 - swxa ECC structures & SM2 algorithm ID.
 * Mirror sdf_client.h (ECCrefPublicKey/ECCCipher, SGD_SM2); kept local
 * to avoid #pragma pack(1) side effects of including sdf_client.h.
 * ECCref_MAX_LEN = 64（sdf_def.h/sgd.h 默认 512 位预留，SM2 有效 32 字节
 * 写在 x/y 尾部）；ECCCipher 含 C_[254] 扩展以匹配 SoftSDF 布局，
 * swxa 设备忽略尾部 C_[254]，字段偏移一致。
 */
#define SWXA_ECC_MAX_LEN	64
#define SWXA_ECC_KEY_BYTES	32
/* ReqID:TECC4.0 - SM2 算法 ID（与 swxa sdf_def.h / SoftSDF sgd.h 一致）。
 * SGD_SM2_1 对应**签名密钥对**、SGD_SM2_3 对应**加密密钥对**，两者严格配对、
 * 交叉使用必 SDR_SKOPERR（真机实测矩阵见 plan/交付件/
 * 2026-09-21-libpq侧-SWXA-SM2授权与密钥对-交付件.md §2.2）。libpq 自身生成
 * 的信封用加密对（SGD_SM2_3）封装，而 pgJDBC 走 JCE 取到的是签名对，
 * 故解密需按 SM2_1 -> SM2_3 顺序试探以兼容两端互读。 */
#define SGD_SM2_1			0x00020200
#define SGD_SM2_3			0x00020800

/* ReqID:TECC4.0 - 真机 ECCCipher.C 容量上限（swsds.h:138 与厂商头文件一致）。
 * he-add 的 HE 私钥 32 字节（信封内 C 约 32B）可放入；he-mul 的 256 字节
 * FF-ElGamal 私钥（信封约 367B、C 约 256B）超出该上限，真机直接返回
 * 0x01000804。SoftSDF 侧现按同一上限拒绝（tecc_kms/src/softsdf.c 的
 * SDF_InternalDecrypt_ECC 返回 SWR_ECCCIPHER_TOO_LONG，同为 0x01000804），
 * 故两种模式下该能力都会失败——差别只在报错文案：真机走下面这个门禁、
 * 带可执行 HINT；SoftSDF 走设备的返回码。 */
#define SWXA_DEVICE_MAX_CIPHER_LEN	136

/* ReqID:TECC4.0 - 真机私钥访问授权要求用「密钥索引 + 该偏移」（厂商 demo
 * demo/ecc.c:529/572 即如此），传裸索引返回 SWR_KEYNOTEXIST。此偏移只用于
 * 授权接口；公钥导出等其它接口仍用裸索引。 */
#define SWXA_AUTH_KEY_INDEX_OFFSET	10000

/* ReqID:TECC4.0 - 真机授权要求口令非空**且长度不超过 8 字节**
 * （2026-09-23 真机探针实测：NULL/0 与非空+零长均 0x0100001D；长度 1/4/8 且
 * 内容任意均 0x00000000；长度 9/10 返回 0x01010005）。即该形态下设备只校验
 * 口令长度、不校验内容，故未提供 TECC_SDF_PASS 时用这 8 字节占位口令完成
 * 私钥装载。带访问口令的密钥仍需由 TECC_SDF_PASS 提供真实口令。 */
#define SWXA_AUTH_FALLBACK_PWD		"tecc0000"

#pragma pack(1)
typedef struct SwxaECCrefPublicKey_st
{
	unsigned int bits;
	unsigned char x[SWXA_ECC_MAX_LEN];
	unsigned char y[SWXA_ECC_MAX_LEN];
} SwxaECCrefPublicKey;

typedef struct SwxaECCCipher_st
{
	unsigned char x[SWXA_ECC_MAX_LEN];
	unsigned char y[SWXA_ECC_MAX_LEN];
	unsigned char M[32];
	unsigned int L;
	unsigned char C[1];
	unsigned char C_[254];
} SwxaECCCipher;
#pragma pack()

/*
 * SWXA SDF API function table.
 *
 * libswsds.so is loaded at runtime via dlopen in swxa_load_sdf_api()
 * to avoid symbol conflicts with libsdf.client.so (Tianzheng HSM),
 * which exports overlapping SDF symbols.
 */
typedef struct SwxaSdfApi
{
	void *handle;	/* dlopen handle */

	int (*SDF_OpenDeviceWithPathAndName)(char *pcCfgfile, void **phDeviceHandle);
	int (*SDF_OpenSession)(void *hDeviceHandle, void **phSessionHandle);
	int (*SDF_CloseDevice)(void *hDeviceHandle);
	int (*SDF_CloseSession)(void *hSessionHandle);
	int (*SDF_GetSymmKeyHandle)(void *hSessionHandle, unsigned int uiKeyIndex, void **phKeyHandle);
	int (*SDF_DestroyKey)(void *hSessionHandle, void *hKeyHandle);
	int (*SDF_GenerateRandom)(void *hSessionHandle, unsigned int uiLength, unsigned char *pucRandom);
	int (*SDF_Encrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
					   unsigned char *pucIV, unsigned char *pucData,
					   unsigned int uiDataLength, unsigned char *pucEncData,
					   unsigned int *puiEncDataLength);
	int (*SDF_Decrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
					   unsigned char *pucIV, unsigned char *pucEncData,
					   unsigned int uiEncDataLength, unsigned char *pucData,
					   unsigned int *puiDataLength);
	int (*SDF_ImportKey)(void *hSessionHandle, unsigned char *pucKey,
						 unsigned int uiKeyLength, void **phKeyHandle);
	int (*SDF_GetPrivateKeyAccessRight)(void *hSessionHandle, unsigned int uiKeyIndex,
										unsigned char *pucPassword, unsigned int uiPwdLength);
	/* ReqID:TECC4.0 - SM2 (asymmetric) interfaces, added for HE KEK support.
	 * swxa 真实接口句柄为 long long（sdf_client.h），此处声明 void* 与
	 * SoftSDF（libteccsdf.so，测试用）兼容，64 位下与 long long 值传递兼容。 */
	int (*SDF_ExportEncPublicKey_ECC)(void *hSessionHandle, unsigned int uiKeyIndex,
									  SwxaECCrefPublicKey *pucPublicKey);
	int (*SDF_InternalDecrypt_ECC)(void *hSessionHandle, unsigned int uiISKIndex,
								   unsigned int uiAlgID, SwxaECCCipher *pucEncData,
								   unsigned char *pucData, unsigned int *puiDataLength);
	/* ReqID:TECC4.0 - 可选：释放私钥使用权限（厂商手册 4.1.8）。标准接口，但并非所有
	 * 库版本都导出，故用可选 dlsym：缺失时保持 NULL，调用点跳过释放。 */
	int (*SDF_ReleasePrivateKeyAccessRight)(void *hSessionHandle, unsigned int uiKeyIndex);
} SwxaSdfApi;

/* Static SDF API table - initialized once by swxa_load_sdf_api() */
static SwxaSdfApi swxa_sdf = {NULL};

/*
 * ReqID:TECC4.0 - 会话密钥句柄连接级缓存
 *
 * 背景：列数据加解密（swxa_sm4_encrypt/swxa_sm4_decrypt）原先每个值都做
 * SDF_ImportKey + SDF_Encrypt/Decrypt + SDF_DestroyKey = 3 次设备往返；真机实测
 * 单次设备调用约 3.2 ms，即每值约 9.6 ms，是硬加密扫描类用例的绝对瓶颈。
 * 同一 DEK 在一段会话内被反复使用（列级 DEK 由连接级 key cache 缓存，见
 * tecc_key_manager_adapter.c），故把 SDF_ImportKey 得到的**会话密钥句柄**缓存在
 * 设备（连接）级，此后每值只需 1 次运算往返。
 *
 * 隔离/安全：句柄属于本连接的设备会话（SDF_OpenSession 在 swxa_connect 中每连接
 * 一次），即"连接级"而非进程级；以**密钥字节**为键（不按哈希），故 DEK 轮换后
 * 字节变化不会命中旧句柄；会话关闭前统一 SDF_DestroyKey 并清空。
 * 代价：DEK 以会话密钥形式在设备内驻留至连接结束（此前是每值即毁）。
 *
 * 失效：disconnect/cleanup 清空；运算失败且句柄来自缓存时丢弃该条目并重新导入
 * 重试一次（覆盖设备重启、设备侧清空会话句柄表等情况）。TECC_SDF_KEY_CACHE=0
 * （或 off/false/no）可关闭缓存，退回"每值导入/销毁"的旧行为，供现场排障。
 */
#define SWXA_KEY_CACHE_SLOTS	16
#define SWXA_KEY_MAX_LEN		32	/* 与 swxa_sm4_* 的 local_key 缓冲区一致 */
/* ReqID:TECC4.0 - 本会话已取得私钥使用权限的索引表容量（HE 场景实际 1~2 个） */
#define SWXA_PRIV_AUTH_SLOTS	8

typedef struct SwxaKeyCacheEntry
{
	bool				used;		/* 槽位是否占用 */
	unsigned char		key[SWXA_KEY_MAX_LEN];	/* 导入设备用的原始密钥字节 */
	size_t				key_len;	/* 密钥长度 */
	void			   *handle;		/* SDF_ImportKey 得到的会话密钥句柄 */
	unsigned long long	stamp;		/* LRU 时间戳（越大越新） */
} SwxaKeyCacheEntry;

/* ReqID:TECC4.0 - 设备会话访问的串行化锁：所有访问 device->session_handle 的入口都必须
 * 持有 SwxaDevicePrivate.lock（不变量见 swxa_device_op_lock 处说明）。HE 字段级并行的多个
 * worker 共用同一连接，故此锁不只是保护 key cache，更是设备调用的唯一串行点。 */
typedef struct SwxaDevicePrivate
{
	pthread_mutex_t		lock;		/* 见上方说明：设备调用的唯一串行点 */
	bool				cache_enabled;
	unsigned long long	stamp_counter;
	SwxaKeyCacheEntry	entries[SWXA_KEY_CACHE_SLOTS];
	/* ReqID:TECC4.0 - 本会话已取得的私钥使用权限索引（手册 4.1.7 取 / 4.1.8 放）：
	 * 同一会话内重复解 HE 信封不再重复取权限；断连时按此表逐个释放。 */
	unsigned int		priv_auth_index[SWXA_PRIV_AUTH_SLOTS];
	int					priv_auth_count;
} SwxaDevicePrivate;

/*
 * ReqID:TECC4.0 - 进程级共享设备句柄
 *
 * 厂商库（libswsds.so）与 SoftSDF 的设备都是**进程级**资源：
 *   - SDF_OpenDeviceWithPathAndName 会在库内建立一个连接池（swsds.ini 的
 *     [ConnectionPool] poolsize，实测一次 OpenDevice 可占十余条设备连接、失败时同样漏）；
 *   - SDF_CloseDevice 会连同该设备下的**所有会话**一起销毁（厂商库日志的
 *     "Exist some unavailable hsm, clear existing connections" 即清空进程内既有连接；
 *     SoftSDF 实现里设备更是全局单例，CloseDevice 直接 free 全局 deviceHandle，
 *     见 tecc_kms/src/softsdf.c）。
 *
 * 原实现每个语句周期都 init/connect（= 新开一个连接池 + 一次设备登录，真机实测单次
 * 建会话 5~70 秒），且从不 SDF_CloseDevice：一个 psql 进程即可占住上百条设备连接，
 * 共享密码机时耗尽设备侧会话槽（其它客户端 SDF_OpenDevice 报 0x01010202/
 * SWR_NO_AVAILABLE_CSM，建会话从数秒劣化到上百秒）。反过来，若在 disconnect/cleanup
 * 里关设备，又会把同进程其它正在进行的使用（如每值加解密用的连接级设备会话）
 * 一起关掉（实测 SoftSDF 下 SDF_ImportKey 报 SDR_STEPERR）。
 *
 * 故设备在进程内只开一次并复用：所有 HsmDevice 在这个设备上各开自己的会话（SDF 语义
 * 允许一设备多会话），进程退出时由 atexit 统一 SDF_CloseDevice——既把设备占用压到
 * "每进程一个连接池"，又保证任何情况下都会关闭（原先注释声称由 atexit 关闭，但代码里
 * 并没有注册过 atexit）。
 */
static pthread_mutex_t swxa_device_lock = PTHREAD_MUTEX_INITIALIZER;
static void *swxa_device_shared = NULL;		/* 进程级共享设备句柄；NULL 表示尚未打开 */
static bool swxa_device_atexit_done = false;

/* ReqID:TECC4.0 - libswsds.so 实际所在目录（由 swxa_load_sdf_api 填充）。
 * swsds.ini 与动态库同目录部署时（如 /usr/lib64），swxa_init 用它作为
 * swsds.ini 的最后一个搜索位置。 */
static char swxa_lib_dir[1024] = "";

/* ReqID:TECC4.0 - 当前加载的是否 SoftSDF 软件密码机（TECC_SDF_LIB 指向 libteccsdf.so）。
 * 决定私钥授权口令的第一轮试探形态：SoftSDF 的预置设备密钥**无口令**（空口令通过），
 * 真机要求口令非空且不少于 8 字节（空口令返回 SDR_INARGERR，且会写设备错误日志）。 */
static bool swxa_lib_is_softsdf = false;

/* SDF error/algorithm constants (from swsds.h - kept local to avoid header dependency) */
#define SDR_OK					0x0
#define SDR_BASE				0x01000000
#define SDR_NOTSUPPORT			(SDR_BASE + 0x00000002)
#define SDR_COMMFAIL			(SDR_BASE + 0x00000003)
#define SDR_HARDFAIL			(SDR_BASE + 0x00000004)
#define SDR_OPENDEVICE			(SDR_BASE + 0x00000005)
#define SDR_OPENSESSION			(SDR_BASE + 0x00000006)
#define SDR_PARDENY				(SDR_BASE + 0x00000007)
#define SDR_KEYNOTEXIST			(SDR_BASE + 0x00000008)
#define SDR_ALGNOTSUPPORT		(SDR_BASE + 0x00000009)
#define SDR_INARGERR			(SDR_BASE + 0x0000001D)

#define SGD_SMS4_CBC			0x00000402

/*
 * SGD_SMS4 - Base SM4 symmetric key type for GetKeyStatus API.
 * Defined as the common base 0x00000400.
 */
#define SGD_SMS4 0x00000400

/* Forward declarations */
static HsmErrorCode sdf_error_to_hsm_error(int sdf_error);
static HsmErrorCode swxa_load_sdf_api(void);
static HsmErrorCode swxa_init(HsmDevice **device, const char *config_file);
static HsmErrorCode swxa_connect(HsmDevice *device);
static HsmErrorCode swxa_disconnect(HsmDevice *device);
static void swxa_cleanup(HsmDevice *device);
static HsmErrorCode swxa_check_key_exists_core(HsmDevice *device, unsigned int key_index);
static HsmErrorCode swxa_get_key_info_core(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info);
static HsmErrorCode swxa_export_public_key_core(HsmDevice *device, unsigned int key_index,
										   void **key_data, size_t *key_len);
static HsmErrorCode swxa_sm2_decrypt_core(HsmDevice *device, unsigned int key_index,
									 const unsigned char *ciphertext, size_t cipher_len,
									 void **plaintext, size_t *plaintext_len);
static HsmErrorCode swxa_encrypt_core(HsmDevice *device, unsigned int key_index,
								 const void *plaintext, size_t plaintext_len,
								 void **ciphertext, size_t *ciphertext_len);
static HsmErrorCode swxa_decrypt_core(HsmDevice *device, unsigned int key_index,
								 const void *ciphertext, size_t ciphertext_len,
								 void **plaintext, size_t *plaintext_len);
static HsmErrorCode swxa_sign_core(HsmDevice *device, unsigned int key_index,
							  const void *data, size_t data_len,
							  void **signature, size_t *signature_len);
static HsmErrorCode swxa_verify_core(HsmDevice *device, unsigned int key_index,
								const void *data, size_t data_len,
								const void *signature, size_t signature_len);
static HsmErrorCode swxa_sm4_encrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *plaintext, size_t plain_len,
									 unsigned char *ciphertext, size_t *cipher_len);
static HsmErrorCode swxa_sm4_decrypt(HsmDevice *device,
									 const unsigned char *key, size_t key_len,
									 const unsigned char *iv,
									 const unsigned char *ciphertext, size_t cipher_len,
									 unsigned char *plaintext, size_t *plain_len);
static const char *swxa_error_string(HsmErrorCode error_code);
static bool swxa_is_connected(HsmDevice *device);
/* ReqID:TECC4.0 - 会话密钥句柄连接级缓存（见 SWXA_KEY_CACHE_SLOTS 处说明） */
static bool swxa_key_cache_enabled(void);
static SwxaDevicePrivate *swxa_private(HsmDevice *device);
static void swxa_key_cache_clear(HsmDevice *device, SwxaDevicePrivate *priv,
								 bool destroy_handles);
static void swxa_key_cache_reset(HsmDevice *device, bool destroy_handles);
static HsmErrorCode swxa_session_key_acquire(HsmDevice *device, SwxaDevicePrivate *priv,
											 const unsigned char *key, size_t key_len,
											 void **handle_out, bool *owned_by_cache,
											 bool *from_cache);
static void swxa_session_key_invalidate(HsmDevice *device, SwxaDevicePrivate *priv,
										const unsigned char *key, size_t key_len);
static void swxa_session_key_release(HsmDevice *device, void *handle,
									 bool owned_by_cache);
/* ReqID:TECC4.0 - 进程级共享设备句柄（见 swxa_device_shared 处说明） */
static void swxa_device_close_at_exit(void);
static void *swxa_device_get(const char *config_file, int *sdf_error);
static HsmErrorCode swxa_priv_auth_ensure(HsmDevice *device, SwxaDevicePrivate *priv,
										 unsigned int auth_index);
static void swxa_priv_auth_record(SwxaDevicePrivate *priv, unsigned int auth_index);
static void swxa_priv_auth_clear(SwxaDevicePrivate *priv);
static void swxa_priv_auth_release_all(HsmDevice *device);

/* ReqID:TECC4.0 - Begin：设备调用串行化（*_core 之上的取锁包装层）
 *
 * 不变量：凡访问设备会话（device->session_handle）的入口都必须持有该设备的会话锁
 * ——即 SwxaDevicePrivate.lock。本层 8 个包装函数与 swxa_sm4_encrypt/swxa_sm4_decrypt
 * 各自在入口取锁；*_core 内一律不得再取锁（本层之外没有别的取锁点）。
 *
 * 为什么需要：会话是**连接级**资源，而 HE 字段级并行的多个 worker 线程共用同一连接
 * （values_processor 的字段级 worker），GM/T 0018 并不承诺会话句柄可并发使用，SoftSDF
 * 的会话对象同样是共享可变状态（session_list 无锁遍历、SDF_GetPrivateKeyAccessRight
 * 会改会话内授权状态）。此前只有 sm4_encrypt/sm4_decrypt 取锁，其余入口（本层覆盖的
 * 8 个）一旦被并行路径调用，就会多线程并发打同一条会话。
 *
 * 例外：init/connect/disconnect/cleanup 不在此层——它们内部会调自带锁的
 * swxa_key_cache_reset，再套一层会自锁。
 */
static pthread_mutex_t swxa_device_op_fallback_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * swxa_device_op_lock - 取设备会话锁；*priv_out 回传实际使用的锁归属
 *
 * priv 非空用设备级锁；priv 为空（swxa_init 里 adapter_private 分配或
 * pthread_mutex_init 失败，已告警）时设备级锁不存在，回退到进程级锁：宁可粗粒度串行，
 * 也不静默地无锁访问设备。
 */
static void
swxa_device_op_lock(HsmDevice *device, SwxaDevicePrivate **priv_out)
{
	SwxaDevicePrivate *priv = swxa_private(device);

	if (priv != NULL)
	{
		pthread_mutex_lock(&priv->lock);
		*priv_out = priv;
		return;
	}

	pthread_mutex_lock(&swxa_device_op_fallback_lock);
	*priv_out = NULL;
}

/* swxa_device_op_unlock - 释放 swxa_device_op_lock 取得的锁 */
static void
swxa_device_op_unlock(SwxaDevicePrivate *priv)
{
	if (priv != NULL)
		pthread_mutex_unlock(&priv->lock);
	else
		pthread_mutex_unlock(&swxa_device_op_fallback_lock);
}

static HsmErrorCode
swxa_check_key_exists(HsmDevice *device, unsigned int key_index)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_check_key_exists_core(device, key_index);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_get_key_info(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_get_key_info_core(device, key_index, key_info);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_export_public_key(HsmDevice *device, unsigned int key_index,
					   void **key_data, size_t *key_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_export_public_key_core(device, key_index, key_data, key_len);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_sm2_decrypt(HsmDevice *device, unsigned int key_index,
				 const unsigned char *ciphertext, size_t cipher_len,
				 void **plaintext, size_t *plaintext_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_sm2_decrypt_core(device, key_index, ciphertext, cipher_len,
								plaintext, plaintext_len);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_encrypt(HsmDevice *device, unsigned int key_index,
			 const void *plaintext, size_t plaintext_len,
			 void **ciphertext, size_t *ciphertext_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_encrypt_core(device, key_index, plaintext, plaintext_len,
							ciphertext, ciphertext_len);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_decrypt(HsmDevice *device, unsigned int key_index,
			 const void *ciphertext, size_t ciphertext_len,
			 void **plaintext, size_t *plaintext_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_decrypt_core(device, key_index, ciphertext, ciphertext_len,
							plaintext, plaintext_len);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_sign(HsmDevice *device, unsigned int key_index,
		  const void *data, size_t data_len,
		  void **signature, size_t *signature_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_sign_core(device, key_index, data, data_len, signature, signature_len);
	swxa_device_op_unlock(priv);
	return ret;
}

static HsmErrorCode
swxa_verify(HsmDevice *device, unsigned int key_index,
			const void *data, size_t data_len,
			const void *signature, size_t signature_len)
{
	SwxaDevicePrivate *priv;
	HsmErrorCode ret;

	swxa_device_op_lock(device, &priv);
	ret = swxa_verify_core(device, key_index, data, data_len, signature, signature_len);
	swxa_device_op_unlock(priv);
	return ret;
}
/* ReqID:TECC4.0 - End：设备调用串行化 */

/*
 * sdf_error_to_hsm_error - Convert SDF error code to HSM error code
 */
static HsmErrorCode
sdf_error_to_hsm_error(int sdf_error)
{
	switch (sdf_error)
	{
		case SDR_OK:
			return HSM_SUCCESS;
		case SDR_KEYNOTEXIST:
			return HSM_ERROR_KEY_NOT_FOUND;
		case SDR_PARDENY:
			return HSM_ERROR_KEY_ACCESS_DENIED;
		case SDR_NOTSUPPORT:
		case SDR_ALGNOTSUPPORT:
			return HSM_ERROR_NOT_SUPPORTED;
		case SDR_OPENDEVICE:
		case SDR_OPENSESSION:
		case SDR_COMMFAIL:
			return HSM_ERROR_CONNECT_FAILED;
		case SDR_HARDFAIL:
			return HSM_ERROR_INIT_FAILED;
		default:
			return HSM_ERROR_OPERATION_FAILED;
	}
}

/* Static adapter instance */
static HsmAdapterInterface swxa_adapter = {0};
static pthread_once_t swxa_adapter_once = PTHREAD_ONCE_INIT;

static void swxa_adapter_init(void)
{
	swxa_adapter.init = swxa_init;
	swxa_adapter.connect = swxa_connect;
	swxa_adapter.disconnect = swxa_disconnect;
	swxa_adapter.cleanup = swxa_cleanup;
	swxa_adapter.check_key_exists = swxa_check_key_exists;
	swxa_adapter.get_key_info = swxa_get_key_info;
	swxa_adapter.export_public_key = swxa_export_public_key;
	swxa_adapter.encrypt = swxa_encrypt;
	swxa_adapter.decrypt = swxa_decrypt;
	swxa_adapter.error_string = swxa_error_string;
	swxa_adapter.is_connected = swxa_is_connected;
	/* Stage 3: sign/verify/sm4_encrypt/sm4_decrypt */
	swxa_adapter.sign = swxa_sign;
	swxa_adapter.verify = swxa_verify;
	swxa_adapter.sm4_encrypt = swxa_sm4_encrypt;
	swxa_adapter.sm4_decrypt = swxa_sm4_decrypt;
	/* ReqID:TECC4.0 - SM2 envelope decrypt for HE KEK support */
	swxa_adapter.sm2_decrypt = swxa_sm2_decrypt;
}

/*
 * get_swxa_adapter - Get SWXA adapter instance
 */
HsmAdapterInterface *
get_swxa_adapter(void)
{
	pthread_once(&swxa_adapter_once, swxa_adapter_init);
	return &swxa_adapter;
}

/*
 * swxa_load_sdf_api - Load SWXA SDF library via dlopen
 *
 * Loads libswsds.so at runtime using dlopen with RTLD_LOCAL to avoid
 * symbol conflicts with libsdf.client.so (Tianzheng HSM adapter).
 * After loading, resolves all SDF function pointers via dlsym.
 */
static HsmErrorCode
swxa_load_sdf_api(void)
{
	const char *dl_error;
	const char *sdf_lib;

	if (swxa_sdf.handle != NULL)
		return HSM_SUCCESS;

	/* ReqID:TECC4.0 - 支持通过 TECC_SDF_LIB 环境变量切换 SDF 动态库，
	 * 用于加载 SoftSDF 软件 stub（libteccsdf.so）替代真实 swxa 加密机
	 * （libswsds.so）。未设置时维持原有 libswsds.so 加载顺序。 */
	sdf_lib = getenv("TECC_SDF_LIB");
	if (sdf_lib != NULL && sdf_lib[0] != '\0')
	{
		swxa_sdf.handle = dlopen(sdf_lib, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (!swxa_sdf.handle)
		{
			fprintf(stderr, "ERROR: Failed to load %s (TECC_SDF_LIB): %s\n",
					sdf_lib, dlerror());
			return HSM_ERROR_INIT_FAILED;
		}

		/* ReqID:TECC4.0 - 记下软/真模式（用途见 swxa_lib_is_softsdf 说明） */
		swxa_lib_is_softsdf = (strstr(sdf_lib, "teccsdf") != NULL);
	}
	else
	{
		/* Try full paths first (common install locations), then bare name for
		 * LD_LIBRARY_PATH / RPATH discovery.
		 * Use RTLD_DEEPBIND so the SWXA library's own symbols take priority over
		 * libsdf.client.so (Tianzheng) which is already in the global namespace.
		 * Without RTLD_DEEPBIND, libswsds.so's internal SDF calls would resolve to
		 * libsdf.client.so's implementations, causing protocol errors. */
		swxa_sdf.handle = dlopen("/usr/local/lib/libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (!swxa_sdf.handle)
			swxa_sdf.handle = dlopen("/usr/lib64/libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (!swxa_sdf.handle)
			swxa_sdf.handle = dlopen("libswsds.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
		if (!swxa_sdf.handle)
		{
			fprintf(stderr, "ERROR: Failed to load libswsds.so: %s\n", dlerror());
			return HSM_ERROR_INIT_FAILED;
		}
	}

	/* ReqID:TECC4.0 - 记录动态库所在目录：厂商部署形态是 swsds.ini 与
	 * libswsds.so 放在同一目录（如 /usr/lib64），而 swsds.ini 的默认搜索
	 * 路径（$PGDATA / ./ / /etc）都不包含该目录，故把它作为最后一个候选
	 * （见 swxa_init），保持既有优先级不变。dlinfo 为 glibc 接口，
	 * Windows 走 MSVC 分支时跳过（其部署形态不适用）。 */
#ifndef _MSC_VER
	{
		char origin[1024];

		origin[0] = '\0';
		if (dlinfo(swxa_sdf.handle, RTLD_DI_ORIGIN, origin) == 0)
			strlcpy(swxa_lib_dir, origin, sizeof(swxa_lib_dir));
	}
#endif

	/* Resolve all SDF functions via dlsym */
#define LOAD_SDF_FUNC(name) \
	do { \
		swxa_sdf.name = dlsym(swxa_sdf.handle, #name); \
		if (!swxa_sdf.name) { \
			fprintf(stderr, "ERROR: Failed to resolve %s in libswsds.so: %s\n", \
					#name, dlerror()); \
			dlclose(swxa_sdf.handle); \
			swxa_sdf.handle = NULL; \
			return HSM_ERROR_INIT_FAILED; \
		} \
	} while(0)

	LOAD_SDF_FUNC(SDF_OpenDeviceWithPathAndName);
	LOAD_SDF_FUNC(SDF_OpenSession);
	LOAD_SDF_FUNC(SDF_CloseDevice);
	LOAD_SDF_FUNC(SDF_CloseSession);
	LOAD_SDF_FUNC(SDF_GetSymmKeyHandle);
	LOAD_SDF_FUNC(SDF_DestroyKey);
	LOAD_SDF_FUNC(SDF_GenerateRandom);
	LOAD_SDF_FUNC(SDF_Encrypt);
	LOAD_SDF_FUNC(SDF_Decrypt);
	LOAD_SDF_FUNC(SDF_ImportKey);
	LOAD_SDF_FUNC(SDF_GetPrivateKeyAccessRight);
	/* ReqID:TECC4.0 - SM2 interfaces for HE KEK support */
	LOAD_SDF_FUNC(SDF_ExportEncPublicKey_ECC);
	LOAD_SDF_FUNC(SDF_InternalDecrypt_ECC);

#undef LOAD_SDF_FUNC

	dl_error = dlerror();
	if (dl_error)
	{
		fprintf(stderr, "ERROR: Unexpected dlopen/dlsym error: %s\n", dl_error);
		dlclose(swxa_sdf.handle);
		swxa_sdf.handle = NULL;
		return HSM_ERROR_INIT_FAILED;
	}

	/* ReqID:TECC4.0 - 可选符号，必须放在上面的 dlerror() 校验**之后**：dlsym 失败会留下
	 * pending error，若放在校验之前，一次正常的「该库未导出此符号」会被误判成加载失败。 */
	swxa_sdf.SDF_ReleasePrivateKeyAccessRight =
		dlsym(swxa_sdf.handle, "SDF_ReleasePrivateKeyAccessRight");

	return HSM_SUCCESS;
}

/*
 * swxa_init - Initialize SWXA device
 *
 * Loads libswsds.so at runtime via dlopen, then checks swsds.ini from
 * $PGDATA/ directory. Validates that the config file exists before returning.
 * The config_file parameter (KEY_PATH like "hsm:id=1") is not used
 * for ini path resolution - ini path is always $PGDATA/swsds.ini.
 *
 * Note: HSM_ERROR_INIT_FAILED from this function may also indicate that
 * libswsds.so was not found or could not be loaded.
 */
static HsmErrorCode
swxa_init(HsmDevice **device, const char *config_file)
{
	HsmDevice *dev;
	char ini_path[1024];
	struct stat st;
	SwxaDevicePrivate *priv;

	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	/* Load libswsds.so at runtime via dlopen */
	{
		HsmErrorCode load_result = swxa_load_sdf_api();
		if (load_result != HSM_SUCCESS)
			return load_result;
	}

	(void)config_file;  /* KEY_PATH (e.g. "hsm:id=1"), not ini path */

	/* ReqID:TECC4.0 - SoftSDF 模式（TECC_SDF_LIB 指向 libteccsdf.so）无需
	 * swsds.ini（真实加密机配置），用占位配置跳过检查。 */
	if (getenv("TECC_SDF_LIB") != NULL)
	{
		snprintf(ini_path, sizeof(ini_path), "softsdf");
	}
	else
	{
		/* Try multiple paths in order */
		{
			const char *pgdata = getenv("PGDATA");
			const char *search_dirs[5];
			int ndirs = 0;
			int i;
			bool found = false;

			if (pgdata)
				search_dirs[ndirs++] = pgdata;
			search_dirs[ndirs++] = ".";
			search_dirs[ndirs++] = "/etc";
			/* ReqID:TECC4.0 - 厂商部署形态：swsds.ini 与 libswsds.so 同目录
			 * （如 /usr/lib64）。放在最后，不改变既有优先级。 */
			if (swxa_lib_dir[0] != '\0')
				search_dirs[ndirs++] = swxa_lib_dir;

			for (i = 0; i < ndirs; i++)
			{
				snprintf(ini_path, sizeof(ini_path), "%s/swsds.ini", search_dirs[i]);
				if (stat(ini_path, &st) == 0 && S_ISREG(st.st_mode))
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				fprintf(stderr, "ERROR: swsds.ini not found\n");
				fprintf(stderr, "HINT: Place swsds.ini in PGDATA/, current directory, /etc/,"
						" or next to libswsds.so (%s/)\n",
						(swxa_lib_dir[0] != '\0') ? swxa_lib_dir : "<lib dir>");
				return HSM_ERROR_INIT_FAILED;
			}
		}
	}

	/* Allocate device structure */
	dev = (HsmDevice *)calloc(1, sizeof(HsmDevice));
	if (!dev)
		return HSM_ERROR_INTERNAL;

	dev->config_file = strdup(ini_path);
	if (!dev->config_file)
	{
		free(dev);
		return HSM_ERROR_INTERNAL;
	}

	dev->device_handle = NULL;
	dev->session_handle = NULL;
	dev->connected = false;
	dev->adapter_private = NULL;

	/* ReqID:TECC4.0 - 连接级会话密钥句柄缓存：每个 HsmDevice 一份，
	 * 生命周期 = 连接（init 分配 / cleanup 释放），不跨连接共享。 */
	priv = (SwxaDevicePrivate *)calloc(1, sizeof(SwxaDevicePrivate));
	if (!priv)
	{
		free(dev->config_file);
		free(dev);
		return HSM_ERROR_INTERNAL;
	}
	priv->cache_enabled = swxa_key_cache_enabled();
	if (pthread_mutex_init(&priv->lock, NULL) != 0)
	{
		fprintf(stderr, "WARNING: failed to init SWXA key cache lock; caching disabled\n");
		free(priv);
		dev->adapter_private = NULL;
	}
	else
		dev->adapter_private = priv;

	*device = dev;
	return HSM_SUCCESS;
}

/* ReqID:TECC4.0 - Begin：进程级共享设备句柄（说明见 swxa_device_shared 处） */

/* swxa_device_close_at_exit - 进程退出时关闭共享设备（atexit 回调） */
static void
swxa_device_close_at_exit(void)
{
	void *handle;

	pthread_mutex_lock(&swxa_device_lock);
	handle = swxa_device_shared;
	swxa_device_shared = NULL;
	pthread_mutex_unlock(&swxa_device_lock);

	if (handle != NULL)
		swxa_sdf.SDF_CloseDevice(handle);
}

/*
 * swxa_device_get - 取进程级共享设备句柄
 *
 * 首次调用时用 config_file 打开设备并登记 atexit（此后一直复用，不再重复开池/登录）；
 * 打开失败时通过 *sdf_error 回传 SDF 错误码并返回 NULL。
 */
static void *
swxa_device_get(const char *config_file, int *sdf_error)
{
	void *handle = NULL;

	*sdf_error = SDR_OK;

	pthread_mutex_lock(&swxa_device_lock);

	if (swxa_device_shared == NULL)
	{
		void *hDevice = NULL;
		int ret;

		ret = swxa_sdf.SDF_OpenDeviceWithPathAndName((char *)config_file, &hDevice);
		if (ret != SDR_OK)
		{
			pthread_mutex_unlock(&swxa_device_lock);
			fprintf(stderr, "ERROR: SDF_OpenDeviceWithPathAndName failed: 0x%08x\n", ret);
			fprintf(stderr, "HINT: Check network connectivity and swsds.ini configuration\n");
			*sdf_error = ret;
			return NULL;
		}

		swxa_device_shared = hDevice;

		if (!swxa_device_atexit_done)
		{
			/* ReqID:TECC4.0 - 进程退出统一关闭设备：这是设备唯一的关闭点，
			 * 保证正常退出/错误返回都不会把设备连接池留给进程。 */
			atexit(swxa_device_close_at_exit);
			swxa_device_atexit_done = true;
		}
	}

	handle = swxa_device_shared;
	pthread_mutex_unlock(&swxa_device_lock);

	return handle;
}

/* ReqID:TECC4.0 - End：进程级共享设备句柄 */

/*
 * swxa_connect - Open session on the process-level shared device
 */
static HsmErrorCode
swxa_connect(HsmDevice *device)
{
	int ret;
	void *hDevice = NULL;
	void *hSession = NULL;

	if (!device || !device->config_file)
		return HSM_ERROR_INVALID_PARAM;

	if (device->connected)
		return HSM_SUCCESS;

	/* ReqID:TECC4.0 - 即将开新会话：上一个会话的密钥句柄已随会话失效，
	 * 只丢引用（不调设备回收，那时句柄已不存在，回收只会多一次失败往返）。 */
	swxa_key_cache_reset(device, false);

	/* ReqID:TECC4.0 - 新会话从"无授权"开始：清掉上一会话的私钥权限记录
	 * （旧会话的权限随旧会话销毁，设备侧不会带到新会话） */
	swxa_priv_auth_clear(swxa_private(device));

	/* ReqID:TECC4.0 - 取进程级共享设备句柄（首次使用时打开并登记 atexit，此后复用；
	 * 不再每语句新开连接池，也不再依赖"谁最后用完谁关闭"） */
	hDevice = swxa_device_get(device->config_file, &ret);
	if (hDevice == NULL)
		return sdf_error_to_hsm_error(ret);

	/* Open session */
	ret = swxa_sdf.SDF_OpenSession(hDevice, &hSession);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_OpenSession failed: 0x%08x\n", ret);
		/* 设备是进程级共享的，此处不关闭（关它会影响同进程其它会话） */
		return sdf_error_to_hsm_error(ret);
	}

	device->device_handle = hDevice;
	device->session_handle = hSession;
	device->connected = true;

	return HSM_SUCCESS;
}

/*
 * swxa_disconnect - Close session only
 *
 * ReqID:TECC4.0 - 只关会话。设备句柄是进程级共享资源（见文件头 swxa_device_shared
 * 说明）：关设备会连带销毁同进程其它会话，且设备连接池本就该每进程一份，故设备的
 * 生命周期由 swxa_device_get 的进程级缓存 + atexit 统一管理，不在此处关闭。
 */
static HsmErrorCode
swxa_disconnect(HsmDevice *device)
{
	if (!device)
		return HSM_ERROR_INVALID_PARAM;

	if (!device->connected)
		return HSM_SUCCESS;

	/* ReqID:TECC4.0 - 会话尚在：先回收缓存里的会话密钥句柄再关会话，
	 * 否则这些句柄会一直占着设备侧的会话密钥句柄表（会话关闭时才由设备释放）。 */
	swxa_key_cache_reset(device, true);

	/* ReqID:TECC4.0 - 厂商手册 4.1.8：本会话取过的私钥使用权限显式释放。会话关闭
	 * 虽会连带失效，但按最小权限应成对回收；老库未导出该符号时内部自动跳过。 */
	swxa_priv_auth_release_all(device);

	if (device->session_handle)
	{
		swxa_sdf.SDF_CloseSession(device->session_handle);
		device->session_handle = NULL;
	}

	/* ReqID:TECC4.0 - 只关会话，不关设备：设备是**进程级**共享资源（连接池建在设备
	 * 句柄上，且 SDF_CloseDevice 会连同该设备下所有会话一起销毁），由 swxa_device_get
	 * 首次使用时打开、进程退出时 atexit 统一关闭。这里若关设备，会连带关掉同进程其它
	 * 正在使用的会话（实测 SoftSDF 下 SDF_ImportKey 报 SDR_STEPERR）。 */
	device->connected = false;
	return HSM_SUCCESS;
}

/*
 * swxa_cleanup - Free device resources
 */
static void
swxa_cleanup(HsmDevice *device)
{
	SwxaDevicePrivate *priv;

	if (!device)
		return;

	if (device->connected)
		swxa_disconnect(device);

	/* ReqID:TECC4.0 - 释放连接级缓存（disconnect 已清空并回收句柄；这里兜住
	 * "未连接但对象仍存在"的情形，此时会话已无，只丢引用不碰设备）。 */
	priv = swxa_private(device);
	if (priv != NULL)
	{
		swxa_key_cache_clear(device, priv, false);
		pthread_mutex_destroy(&priv->lock);
		free(priv);
		device->adapter_private = NULL;
	}

	if (device->config_file)
		free(device->config_file);

	free(device);
}

/*
 * swxa_check_key_exists - Check if key exists at given index
 *
 * Uses SDF_GetSymmKeyHandle to attempt to open the key.
 * If the key handle is obtained successfully, the key exists.
 */
static HsmErrorCode
swxa_check_key_exists_core(HsmDevice *device, unsigned int key_index)
{
	int ret;
	void *hKeyHandle = NULL;

	if (!device || !device->connected)
		return HSM_ERROR_CONNECT_FAILED;

	/* Try to get key handle - succeeds if key exists */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		if (ret == SDR_KEYNOTEXIST)
			return HSM_ERROR_KEY_NOT_FOUND;
		/* SDR_INARGERR (invalid index) also means key doesn't exist */
		if (ret == SDR_INARGERR)
			return HSM_ERROR_KEY_NOT_FOUND;
		fprintf(stderr, "WARNING: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Key exists, release handle */
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
	return HSM_SUCCESS;
}

/*
 * swxa_error_string - Convert error code to string
 */
static const char *
swxa_error_string(HsmErrorCode error_code)
{
	switch (error_code)
	{
		case HSM_SUCCESS:
			return "Success";
		case HSM_ERROR_INIT_FAILED:
			return "SWXA device initialization failed (check swsds.ini)";
		case HSM_ERROR_CONNECT_FAILED:
			return "SWXA device connection failed (check network/config)";
		case HSM_ERROR_KEY_NOT_FOUND:
			return "Key not found in SWXA password machine";
		case HSM_ERROR_KEY_ACCESS_DENIED:
			return "Key access denied";
		case HSM_ERROR_INVALID_PARAM:
			return "Invalid parameter";
		case HSM_ERROR_OPERATION_FAILED:
			return "SWXA operation failed";
		case HSM_ERROR_NOT_SUPPORTED:
			return "Operation not supported";
		case HSM_ERROR_INTERNAL:
			return "Internal error";
		default:
			return "Unknown error";
	}
}

/*
 * swxa_is_connected - Check if device is connected
 */
static bool
swxa_is_connected(HsmDevice *device)
{
	return device && device->connected;
}

/*
 * swxa_get_key_info - Get key information from SWXA HSM
 *
 * Returns the key type (SGD_SMS4) and key bits (128) for SM4 symmetric keys.
 * Uses SDF_GetSymmKeyHandle to verify the key exists before returning info.
 */
static HsmErrorCode
swxa_get_key_info_core(HsmDevice *device, unsigned int key_index, HsmKeyInfo **key_info)
{
	int ret;
	void *hKeyHandle = NULL;
	HsmKeyInfo *info;

	if (!device || !device->connected || !key_info)
		return HSM_ERROR_INVALID_PARAM;

	/* Verify key exists by trying to get a handle */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		if (ret == SDR_INARGERR || ret == SDR_KEYNOTEXIST)
			return HSM_ERROR_KEY_NOT_FOUND;
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return HSM_ERROR_KEY_NOT_FOUND;
	}
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	/* Allocate key info */
	info = (HsmKeyInfo *)calloc(1, sizeof(HsmKeyInfo));
	if (!info)
		return HSM_ERROR_INTERNAL;

	info->key_index = key_index;
	info->key_bits = 128;		/* SM4 is 128-bit */
	info->key_type = SGD_SMS4;	/* SM4 symmetric key */
	info->key_name = NULL;
	info->key_data = NULL;
	info->key_len = 0;

	*key_info = info;
	return HSM_SUCCESS;
}

/*
 * swxa_export_public_key - Export public key from SWXA HSM.
 *
 * ReqID:TECC4.0 - 支持 SM2（ECC）公钥导出：用 SDF_ExportEncPublicKey_ECC 导出
 * **加密**密钥公钥，返回 raw 点 bits(4)+x(32)+y(32)（与 tzhsm_export_public_key
 * 布局一致，供前端转 65 字节 04||x||y）。SM4 对称 KEK 场景该索引无 ECC 密钥时
 * 返回错误（HE 非对称 KEK 才走此路径）。
 *
 * ReqID:TECC4.0 - Bug 9334：这里**不能**用签名公钥兜底。导出的公钥只用于加密
 * 同态私钥信封（CREATE DEK / ALTER KEK），而信封只能用设备内 SM2 **加密**私钥
 * 经 SDF_InternalDecrypt_ECC 解开；若在签名槽取公钥，信封会被签名的公钥加密、
 * 却永远无法用加密私钥解开，此后该 KEK 下所有同态列读取/运算都失败，而建
 * KEK、建 DEK、写入全部"成功"，现场只看到同态结果报错，无从定位。故此处在
 * 加密密钥对缺失时直接报错，由上层给出可执行的提示。
 */
static HsmErrorCode
swxa_export_public_key_core(HsmDevice *device, unsigned int key_index,
					   void **key_data, size_t *key_len)
{
	SwxaECCrefPublicKey ecc_pub_key;
	size_t total_size;
	unsigned char *buffer;
	int ret;

	if (!device || !device->connected || !device->session_handle ||
		!key_data || !key_len)
		return HSM_ERROR_INVALID_PARAM;

	ret = swxa_sdf.SDF_ExportEncPublicKey_ECC(device->session_handle, key_index, &ecc_pub_key);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_ExportEncPublicKey_ECC failed for index %u: 0x%08x\n",
				key_index, ret);
		fprintf(stderr, "HINT: HE (homomorphic) KEK requires an SM2 *encryption* key pair at this index;"
				" a signing key pair cannot be used for the HE private key envelope\n");
		return sdf_error_to_hsm_error(ret);
	}

	/* Return raw point: bits(4) + x_eff(32) + y_eff(32)（有效 32 字节在预留尾部） */
	total_size = sizeof(unsigned int) + SWXA_ECC_KEY_BYTES + SWXA_ECC_KEY_BYTES;
	buffer = (unsigned char *)malloc(total_size);
	if (!buffer)
		return HSM_ERROR_INTERNAL;

	memcpy(buffer, &ecc_pub_key.bits, sizeof(unsigned int));
	memcpy(buffer + sizeof(unsigned int),
		   ecc_pub_key.x + SWXA_ECC_MAX_LEN - SWXA_ECC_KEY_BYTES, SWXA_ECC_KEY_BYTES);
	memcpy(buffer + sizeof(unsigned int) + SWXA_ECC_KEY_BYTES,
		   ecc_pub_key.y + SWXA_ECC_MAX_LEN - SWXA_ECC_KEY_BYTES, SWXA_ECC_KEY_BYTES);

	*key_data = buffer;
	*key_len = total_size;
	return HSM_SUCCESS;
}

/*
 * swxa_sm2_decrypt - SM2 envelope decryption with device-internal SM2 private key.
 *
 * ReqID:TECC4.0 - HE 私钥信封解密（数据解密与 KEK 轮换）走此路径：调用 swxa 真实
 * 接口 SDF_InternalDecrypt_ECC（uiAlgID=SGD_SM2_3），SM2 私钥不出密码机。
 * 输入为 tecc_sm2_envelope_encrypt 输出的 DER 信封（OpenSSL SM2 EVP 的
 * ASN.1 SEQUENCE，非裸 C1||C3||C2），解析后填入 SwxaECCCipher 交给设备解密；
 * 输出明文用 OPENSSL_malloc 分配（调用方 OPENSSL_free）。
 */
static HsmErrorCode
swxa_sm2_decrypt_core(HsmDevice *device, unsigned int key_index,
				 const unsigned char *ciphertext, size_t cipher_len,
				 void **plaintext, size_t *plaintext_len)
{
	int ret;
	SwxaECCCipher *ecc_cipher;
	unsigned char *plain_buf;
	unsigned int plain_len;
	size_t c2_len = 0;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!ciphertext || cipher_len <= 96 || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	/* 解析 DER 信封（OpenSSL SM2 EVP 输出，tecc_sm2_envelope_encrypt）：
	 * 30 <len> | 02 <len> x | 02 <len> y | 04 20 M | 04 <len> C2
	 *
	 * ReqID:TECC4.0 - 长度域按 DER 规范支持短格式与长格式（1~4 字节）：
	 * he-add 的 EC-ElGamal 私钥 32 字节，信封约 141 字节（0x81 一字节长度）；
	 * he-mul 的 FF-ElGamal 私钥 256 字节，信封约 367 字节，长度域用 0x82
	 * 两字节表示。原实现只跳过 1 字节长度，长信封会解析错位导致解密失败，
	 * 且 C 缓冲固定按 255 字节分配、按实际 C2 长度写入会造成越界（Bug 9267）。 */
	{
		const unsigned char *p;
		const unsigned char *end = ciphertext + cipher_len;
		size_t c_buf_len = cipher_len;	/* C2 长度不会超过整个信封 */
		int idx = 0;

		if (ciphertext[1] & 0x80)
		{
			int nlen = ciphertext[1] & 0x7f;

			if (nlen < 1 || nlen > 4 || ciphertext + 2 + nlen > end)
				return HSM_ERROR_INVALID_PARAM;
			p = ciphertext + 2 + nlen;
		}
		else
			p = ciphertext + 2;

		ecc_cipher = (SwxaECCCipher *)malloc(sizeof(SwxaECCCipher) - 1 + c_buf_len);
		if (!ecc_cipher)
			return HSM_ERROR_INTERNAL;
		memset(ecc_cipher, 0, sizeof(SwxaECCCipher) - 1 + c_buf_len);

		while (p < end && idx < 4)
		{
			unsigned char tag = *p++;
			size_t len = 0;
			const unsigned char *val;

			if (p >= end)
				break;
			if (*p & 0x80)
			{
				int nlen = *p++ & 0x7f;
				while (nlen-- > 0 && p < end)
					len = (len << 8) | *p++;
			}
			else
				len = *p++;

			if (len == 0 || p + len > end)
				break;
			val = p;
			p += len;

			if (tag == 0x02)
			{
				/* INTEGER：可能带前导 00（正数符号位），按低字节对齐取至多 32 字节 */
				size_t take = (len > SWXA_ECC_KEY_BYTES) ? SWXA_ECC_KEY_BYTES : len;
				size_t skip = len - take;

				if (idx == 0)
					memcpy(ecc_cipher->x + SWXA_ECC_MAX_LEN - take, val + skip, take);
				else if (idx == 1)
					memcpy(ecc_cipher->y + SWXA_ECC_MAX_LEN - take, val + skip, take);
			}
			else if (tag == 0x04)
			{
				if (idx == 2 && len == 32)
					memcpy(ecc_cipher->M, val, 32);
				else if (idx == 3)
				{
					c2_len = len;
					memcpy(ecc_cipher->C, val, len);
				}
			}
			idx++;
		}

		if (idx < 4 || c2_len == 0)
		{
			free(ecc_cipher);
			return HSM_ERROR_INVALID_PARAM;
		}
		ecc_cipher->L = (unsigned int)c2_len;
	}

	/* ReqID:TECC4.0 - 真机前置校验：设备 ECCCipher.C 只有 136 字节，
	 * he-mul 的 256 字节 FF-ElGamal 私钥信封（C≈256B）根本放不进去，
	 * 真机必然返回 0x01000804，而该码不在厂商公开错误码清单内、现场无法
	 * 定位（见交付件 §五/§8.1 Q1）。这里提前给出结论与可执行方向，
	 * 不再把「设备能力上限」伪装成「密文格式错误」。SoftSDF 现已按同一上限
	 * 拒绝（返回 0x01000804，见 tecc_kms/src/softsdf.c），但本门禁的真机
	 * 文案带可执行 HINT、且发生在调用设备之前，故真机路径仍由本门禁先拦；
	 * 两种模式的报错文案不同，各自的 golden 如实记录。 */
	if (c2_len > SWXA_DEVICE_MAX_CIPHER_LEN && getenv("TECC_SDF_LIB") == NULL)
	{
		fprintf(stderr,
				"ERROR: SM2 envelope C length %zu exceeds the device ECCCipher.C limit (%d);"
				" this HE private key envelope cannot be unwrapped by a device-hosted KEK\n",
				c2_len, SWXA_DEVICE_MAX_CIPHER_LEN);
		fprintf(stderr,
				"HINT: a he-mul private key is 256 bytes and does not fit the device C buffer;"
				" use a he-add DEK, or keep the KEK off the device (localkms), or ask the vendor"
				" whether SDF_InternalDecrypt_BigData_ECC (swsds.h) lifts this limit\n");
		free(ecc_cipher);
		return HSM_ERROR_INVALID_PARAM;
	}

	/* OPENSSL_malloc so the caller (tecc_algorithm_scheduler) can OPENSSL_free */
	plain_buf = (unsigned char *)OPENSSL_malloc(c2_len);
	if (!plain_buf)
	{
		free(ecc_cipher);
		return HSM_ERROR_INTERNAL;
	}
	plain_len = (unsigned int)c2_len;

	/* ReqID:TECC4.0 - 使用设备内 SM2 私钥前必须先授权（GM/T 0018）：授权把私钥
	 * 加载进设备会话容器，否则后续解密必然失败。真机与 SoftSDF 的形态差异
	 * （授权索引要传 key_index + 10000、真机要求非空且 ≥8 字节口令）与"取一次
	 * 权限、按会话复用、断连释放"的配对关系都在 swxa_priv_auth_ensure 内处理，
	 * 详见该函数上方的说明。授权失败必须立即中止——继续调用
	 * SDF_InternalDecrypt_ECC 必然失败，且会让上层误判为密文格式错误。 */
	{
		HsmErrorCode auth_ret = swxa_priv_auth_ensure(device, swxa_private(device),
													 key_index + SWXA_AUTH_KEY_INDEX_OFFSET);

		if (auth_ret != HSM_SUCCESS)
		{
			free(ecc_cipher);
			OPENSSL_free(plain_buf);
			return auth_ret;
		}
	}

	/* ReqID:TECC4.0 - 算法标识必须与「信封加密所用公钥所属密钥对」配对：
	 * 加密对 ↔ SGD_SM2_3、签名对 ↔ SGD_SM2_1，交叉必 SDR_SKOPERR（真机实测，
	 * 见交付件 §2.2/§3）。libpq 自身生成的信封用加密对封装，pgJDBC 经 JCE
	 * 取到的是签名对、其写入的信封只能用 SGD_SM2_1 解开。跨端互读要求接收侧
	 * 按实际封装方选择算法，故先按本端形态试 SM2_3、失败再退 SM2_1（不改驱动
	 * 侧的最小互操作方案）。顺序不能反：探错的那一次会在 SDF 实现里留下
	 * 一条失败轨迹（SoftSDF 会打印到 stderr，污染回归 golden），本端信封占
	 * 多数，故把本端形态放在第一位。 */
	ret = swxa_sdf.SDF_InternalDecrypt_ECC(device->session_handle, key_index,
										   SGD_SM2_3, ecc_cipher, plain_buf, &plain_len);
	if (ret != SDR_OK)
	{
		int ret_sm2_3 = ret;

		plain_len = (unsigned int)c2_len;	/* 重试前恢复输出缓冲长度上界 */
		ret = swxa_sdf.SDF_InternalDecrypt_ECC(device->session_handle, key_index,
											   SGD_SM2_1, ecc_cipher, plain_buf, &plain_len);
		if (ret != SDR_OK)
		{
			fprintf(stderr, "ERROR: SDF_InternalDecrypt_ECC failed for index %u: 0x%08x"
					" (SGD_SM2_3 gave 0x%08x)\n",
					key_index, ret, ret_sm2_3);
		}
	}
	free(ecc_cipher);

	if (ret != SDR_OK)
	{
		/* ReqID:TECC4.0 - 明确暴露设备内 SM2 信封解密失败原因：此前静默返回
		 * 错误，上层仅表现为 HE 列返回密文 / "HE result out of range"，
		 * 现场无法定位（Bug 9267）。 */
		OPENSSL_free(plain_buf);
		return sdf_error_to_hsm_error(ret);
	}

	*plaintext = plain_buf;
	*plaintext_len = (size_t)plain_len;
	return HSM_SUCCESS;
}

/*
 * swxa_encrypt - Encrypt data using key at given index in SWXA HSM
 *
 * 1. Gets key handle via SDF_GetSymmKeyHandle
 * 2. Generates random IV via SDF_GenerateRandom
 * 3. Applies PKCS#7 padding to plaintext
 * 4. Encrypts via SDF_Encrypt (SGD_SMS4_CBC)
 * 5. Destroys key handle
 * 6. Returns ciphertext = IV (16 bytes) + encrypted data
 */
static HsmErrorCode
swxa_encrypt_core(HsmDevice *device, unsigned int key_index,
			 const void *plaintext, size_t plaintext_len,
			 void **ciphertext, size_t *ciphertext_len)
{
	int ret;
	void *hKeyHandle = NULL;
	unsigned char iv[16];
	size_t padded_len;
	unsigned char *padded_plaintext = NULL;
	unsigned char *enc_data = NULL;
	unsigned int enc_len;
	unsigned char pad_value;
	unsigned char *result_buf = NULL;
	unsigned char iv_save[16];

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!plaintext || plaintext_len == 0 || !ciphertext || !ciphertext_len)
		return HSM_ERROR_INVALID_PARAM;

	/* Step 1: Get key handle for the KEK at key_index */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 2: Generate random IV */
	ret = swxa_sdf.SDF_GenerateRandom(device->session_handle, sizeof(iv), iv);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GenerateRandom failed: 0x%08x\n", ret);
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return sdf_error_to_hsm_error(ret);
	}

	/*
	 * Backup IV before SDF_Encrypt potentially modifies the buffer.
	 * Some SDF implementations modify the IV in place.
	 */
	memcpy(iv_save, iv, sizeof(iv));

	/* Step 3: PKCS#7 padding to 16-byte block boundary */
	if (plaintext_len == 0)
	{
		padded_len = 16;
		pad_value = 16;
	}
	else
	{
		size_t remainder = plaintext_len % 16;
		if (remainder == 0)
		{
			padded_len = plaintext_len + 16;
			pad_value = 16;
		}
		else
		{
			padded_len = plaintext_len + (16 - remainder);
			pad_value = (unsigned char)(16 - remainder);
		}
	}

	padded_plaintext = (unsigned char *)malloc(padded_len);
	if (!padded_plaintext)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(padded_plaintext, plaintext, plaintext_len);
	memset(padded_plaintext + plaintext_len, pad_value, padded_len - plaintext_len);

	/* Step 4: Encrypt via HSM */
	enc_data = (unsigned char *)malloc(padded_len);
	if (!enc_data)
	{
		free(padded_plaintext);
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}

	enc_len = (unsigned int)padded_len;
	ret = swxa_sdf.SDF_Encrypt(device->session_handle, hKeyHandle, SGD_SMS4_CBC,
							  iv, padded_plaintext, (unsigned int)padded_len,
							  enc_data, &enc_len);

	free(padded_plaintext);
	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	if (ret != SDR_OK)
	{
		free(enc_data);
		fprintf(stderr, "ERROR: SDF_Encrypt failed for key index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 5: Build result = IV (from backup) + encrypted data */
	result_buf = (unsigned char *)malloc(sizeof(iv_save) + enc_len);
	if (!result_buf)
	{
		free(enc_data);
		return HSM_ERROR_INTERNAL;
	}
	memcpy(result_buf, iv_save, sizeof(iv_save));
	memcpy(result_buf + sizeof(iv_save), enc_data, enc_len);
	free(enc_data);

	*ciphertext = result_buf;
	*ciphertext_len = sizeof(iv_save) + enc_len;

	return HSM_SUCCESS;
}

/*
 * swxa_decrypt - Decrypt data using key at given index in SWXA HSM
 *
 * 1. Extracts IV from ciphertext (first 16 bytes)
 * 2. Gets key handle via SDF_GetSymmKeyHandle
 * 3. Decrypts via SDF_Decrypt (SGD_SMS4_CBC)
 * 4. Destroys key handle
 * 5. Removes PKCS#7 padding
 * 6. Returns plaintext
 */
static HsmErrorCode
swxa_decrypt_core(HsmDevice *device, unsigned int key_index,
			 const void *ciphertext, size_t ciphertext_len,
			 void **plaintext, size_t *plaintext_len)
{
	int ret;
	void *hKeyHandle = NULL;
	unsigned char iv[16];
	unsigned char *dec_data = NULL;
	unsigned int dec_len;
	unsigned char *result_buf = NULL;
	unsigned char pad_value;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!ciphertext || ciphertext_len == 0 || !plaintext || !plaintext_len)
		return HSM_ERROR_INVALID_PARAM;

	if (ciphertext_len < sizeof(iv))
		return HSM_ERROR_INVALID_PARAM;

	/* Step 1: Extract IV from ciphertext (first 16 bytes) */
	memcpy(iv, ciphertext, sizeof(iv));

	/* Step 2: Get key handle for the KEK at key_index */
	ret = swxa_sdf.SDF_GetSymmKeyHandle(device->session_handle, key_index, &hKeyHandle);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_GetSymmKeyHandle failed for index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 3: Decrypt (skip IV at beginning of ciphertext) */
	dec_data = (unsigned char *)malloc(ciphertext_len - sizeof(iv));
	if (!dec_data)
	{
		swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);
		return HSM_ERROR_INTERNAL;
	}

	ret = swxa_sdf.SDF_Decrypt(device->session_handle, hKeyHandle, SGD_SMS4_CBC,
							  iv,
							  (unsigned char *)ciphertext + sizeof(iv),
							  (unsigned int)(ciphertext_len - sizeof(iv)),
							  dec_data, &dec_len);

	swxa_sdf.SDF_DestroyKey(device->session_handle, hKeyHandle);

	if (ret != SDR_OK)
	{
		free(dec_data);
		fprintf(stderr, "ERROR: SDF_Decrypt failed for key index %u: 0x%08x\n",
				key_index, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* Step 4: Remove PKCS#7 padding */
	if (dec_len == 0)
	{
		free(dec_data);
		return HSM_ERROR_INVALID_PARAM;
	}

	pad_value = dec_data[dec_len - 1];
	if (pad_value > 0 && pad_value <= 16 && dec_len >= pad_value)
	{
		size_t unpadded_len = dec_len - pad_value;
		result_buf = (unsigned char *)malloc(unpadded_len);
		if (!result_buf)
		{
			free(dec_data);
			return HSM_ERROR_INTERNAL;
		}
		memcpy(result_buf, dec_data, unpadded_len);
		free(dec_data);
		*plaintext = result_buf;
		*plaintext_len = unpadded_len;
		return HSM_SUCCESS;
	}

		/* Invalid padding or data is not padded, return as-is */
		fprintf(stderr, "WARNING: swxa_decrypt: invalid PKCS#7 padding (pad_value=%u), "\
				"returning raw decrypted data\n", pad_value);
		*plaintext = dec_data;
		*plaintext_len = dec_len;
		return HSM_SUCCESS;
}


/*
 * swxa_sign - Sign data using key at given index in SWXA HSM
 *
 * Not supported: SWXA KEK is an SM4 symmetric key.
 * Sign/verify require asymmetric keys (SM2/RSA/ECDSA).
 */
static HsmErrorCode
swxa_sign_core(HsmDevice *device, unsigned int key_index,
		  const void *data, size_t data_len,
		  void **signature, size_t *signature_len)
{
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	return HSM_ERROR_NOT_SUPPORTED;
}

/*
 * swxa_verify - Verify signature using key at given index in SWXA HSM
 *
 * Not supported: SWXA KEK is an SM4 symmetric key.
 */
static HsmErrorCode
swxa_verify_core(HsmDevice *device, unsigned int key_index,
			const void *data, size_t data_len,
			const void *signature, size_t signature_len)
{
	(void)device;
	(void)key_index;
	(void)data;
	(void)data_len;
	(void)signature;
	(void)signature_len;

	return HSM_ERROR_NOT_SUPPORTED;
}

/* ReqID:TECC4.0 - Begin：会话密钥句柄连接级缓存的实现（说明见 SWXA_KEY_CACHE_SLOTS 处） */

/*
 * swxa_key_cache_enabled - 读取缓存开关
 *
 * TECC_SDF_KEY_CACHE=0/off/false/no（大小写不敏感）关闭缓存，其余/未设置则启用。
 */
static bool
swxa_key_cache_enabled(void)
{
	const char *value = getenv("TECC_SDF_KEY_CACHE");

	if (value == NULL || value[0] == '\0')
		return true;
	if (pg_strcasecmp(value, "0") == 0 ||
		pg_strcasecmp(value, "off") == 0 ||
		pg_strcasecmp(value, "false") == 0 ||
		pg_strcasecmp(value, "no") == 0)
		return false;
	return true;
}

/*
 * swxa_private - 取适配器私有状态
 *
 * swxa_init 分配、swxa_cleanup 释放；其它适配器/历史构造路径下为 NULL，
 * 此时缓存整体退化为关闭。
 */
static SwxaDevicePrivate *
swxa_private(HsmDevice *device)
{
	if (device == NULL || device->adapter_private == NULL)
		return NULL;
	return (SwxaDevicePrivate *)device->adapter_private;
}

/*
 * swxa_key_cache_clear - 清空缓存条目
 *
 * destroy_handles=true：逐个 SDF_DestroyKey（会话仍有效，如 disconnect 关会话
 * 之前）；false：只丢引用不调设备（会话已失效，如 connect 开新会话后接手旧对象，
 * 此时旧句柄已随会话销毁，再回收只是白付一次设备往返）。
 * 调用方需持有 priv->lock，或保证该设备当前无并发访问。
 */
static void
swxa_key_cache_clear(HsmDevice *device, SwxaDevicePrivate *priv, bool destroy_handles)
{
	int i;

	if (priv == NULL)
		return;

	for (i = 0; i < SWXA_KEY_CACHE_SLOTS; i++)
	{
		if (!priv->entries[i].used)
			continue;
		if (destroy_handles && priv->entries[i].handle != NULL &&
			device != NULL && device->session_handle != NULL)
			swxa_sdf.SDF_DestroyKey(device->session_handle, priv->entries[i].handle);
		priv->entries[i].used = false;
		priv->entries[i].handle = NULL;
		priv->entries[i].key_len = 0;
		memset(priv->entries[i].key, 0, sizeof(priv->entries[i].key));
	}
}

/* swxa_key_cache_reset - 加锁版清空（连接生命周期入口使用） */
static void
swxa_key_cache_reset(HsmDevice *device, bool destroy_handles)
{
	SwxaDevicePrivate *priv = swxa_private(device);

	if (priv == NULL)
		return;

	pthread_mutex_lock(&priv->lock);
	swxa_key_cache_clear(device, priv, destroy_handles);
	pthread_mutex_unlock(&priv->lock);
}

/* ReqID:TECC4.0 - Begin：私钥使用权限的取得与释放（厂商手册 4.1.7 / 4.1.8）
 *
 * 为什么这么写（规范性 + 稳定性）：
 *   - 手册 4.1.8 提供释放接口，官方 demo 在 ECC 流程里成对释放；本适配器此前只取不放。
 *   - 真机形态：授权口令必须非空且不少于 8 字节，传 NULL 返回 SDR_INARGERR 并**写设备
 *     错误日志**；SoftSDF 的预置设备密钥无口令，只接受空口令。故按模式决定第一轮试探
 *     形态，避免真机上每次解信封都先做一次注定失败的调用（实测一个用例 49 次）。
 *   - 权限按会话缓存：同一会话内重复解信封不再重复取权限（真机取一次权限 = 一次设备
 *     往返），断连时按记录表统一释放。
 *
 * 调用方需持有 priv->lock（均在 *_core 内被调用，锁由包装层持有）。
 */

/* 记录一个已取得权限的索引；已在表中则忽略，表满则丢弃记录（权限仍有效，只是断连时
 * 不显式释放，会话关闭会连带回收）。 */
static void
swxa_priv_auth_record(SwxaDevicePrivate *priv, unsigned int auth_index)
{
	int i;

	if (priv == NULL)
		return;

	for (i = 0; i < priv->priv_auth_count; i++)
	{
		if (priv->priv_auth_index[i] == auth_index)
			return;
	}

	if (priv->priv_auth_count < SWXA_PRIV_AUTH_SLOTS)
		priv->priv_auth_index[priv->priv_auth_count++] = auth_index;
}

/* 清空记录表（新会话开始时调用：新会话尚未取得任何权限）。 */
static void
swxa_priv_auth_clear(SwxaDevicePrivate *priv)
{
	if (priv != NULL)
		priv->priv_auth_count = 0;
}

/* 释放本会话取得过的全部私钥使用权限。会话仍有效时调用（见 swxa_disconnect）。
 * 老库未导出 SDF_ReleasePrivateKeyAccessRight 时静默跳过。 */
static void
swxa_priv_auth_release_all(HsmDevice *device)
{
	SwxaDevicePrivate *priv;
	int i;

	if (device == NULL || device->session_handle == NULL ||
		swxa_sdf.SDF_ReleasePrivateKeyAccessRight == NULL)
		return;

	priv = swxa_private(device);
	if (priv == NULL)
		return;

	pthread_mutex_lock(&priv->lock);
	for (i = 0; i < priv->priv_auth_count; i++)
	{
		int rr = swxa_sdf.SDF_ReleasePrivateKeyAccessRight(device->session_handle,
														  priv->priv_auth_index[i]);

		/* ReqID:TECC4.0 - 释放是尽力而为的收尾动作，失败**不告警**：断连路径往
		 * stderr 打字会进客户端输出、污染金标准比较，而此刻会话即将关闭，权限
		 * 无论释放成功与否都会随会话回收。排障时用 TECC_HSM_DEBUG=1 打开观察。 */
		if (rr != SDR_OK && getenv("TECC_HSM_DEBUG") != NULL)
			fprintf(stderr, "TECC_HSM_DEBUG: release private key access right failed"
					" (index %u): 0x%08x\n", priv->priv_auth_index[i], rr);
	}
	priv->priv_auth_count = 0;
	pthread_mutex_unlock(&priv->lock);
}

/* 确保本会话已取得 auth_index 的私钥使用权限；已取得则直接返回（不再打设备）。 */
static HsmErrorCode
swxa_priv_auth_ensure(HsmDevice *device, SwxaDevicePrivate *priv, unsigned int auth_index)
{
	const char *pass = getenv("TECC_SDF_PASS");
	bool pass_from_env = (pass != NULL && pass[0] != '\0');
	unsigned char *pwd;
	unsigned int pwd_len;
	int ar = SDR_OK;
	int attempt;
	int i;

	/* 快路径：本会话已取过该索引（HE 字段级并行下 worker 会反复进来） */
	if (priv != NULL)
	{
		for (i = 0; i < priv->priv_auth_count; i++)
		{
			if (priv->priv_auth_index[i] == auth_index)
				return HSM_SUCCESS;
		}
	}

	/* 两轮试探：第一轮按模式（真机=8 字节占位口令，SoftSDF=空口令），第二轮换另一种
	 * 形态兜底（现场可能出现与实测相反的库/密钥形态）。TECC_SDF_PASS 设了则只试它。 */
	for (attempt = 0; attempt < 2; attempt++)
	{
		if (pass_from_env)
		{
			pwd = (unsigned char *)pass;
			pwd_len = (unsigned int)strlen(pass);
		}
		else
		{
			/* 第一轮按模式定：SoftSDF 用空口令、真机用 8 字节占位口令；
			 * 第二轮换另一种形态兜底（现场可能出现与实测相反的库/密钥形态）。 */
			bool use_null = swxa_lib_is_softsdf ? (attempt == 0) : (attempt == 1);

			if (use_null)
			{
				/* 按 SDF 规范，无口令时 pucPassword 应为 NULL（非指向零字节的非空指针），
				 * 否则真机返回 SDR_INARGERR（Bug 9267 regression）。 */
				pwd = NULL;
				pwd_len = 0;
			}
			else
			{
				/* 真机形态：设备要求非空口令完成私钥装载（该形态下不校验口令内容，
				 * 具体语义待厂商确认，见交付件 §8.1 Q4）。 */
				pwd = (unsigned char *)SWXA_AUTH_FALLBACK_PWD;
				pwd_len = (unsigned int)(sizeof(SWXA_AUTH_FALLBACK_PWD) - 1);
			}
		}

		ar = swxa_sdf.SDF_GetPrivateKeyAccessRight(device->session_handle,
												   auth_index, pwd, pwd_len);
		if (ar == SDR_OK)
			break;
		if (pass_from_env)
			break;	/* 已按运维指定口令试过，不再用占位口令覆盖 */
	}

	if (ar != SDR_OK)
	{
		/* Begin - ReqID:TECC4.0 - Bug 9334 */
		/* ReqID:TECC4.0 - 设备内 SM2 私钥授权失败即同态私钥信封解不开，
		 * 上层只能降级（纯同态列回传密文、混合列报结果相关错误），现场
		 * 据此无法定位。这里补上可执行的排查方向：要么该索引上的 SM2
		 * 密钥带访问口令（用 TECC_SDF_PASS 传入口令），要么该索引上没有
		 * SM2 加密密钥对（HE KEK 必须由加密密钥对托管，签名密钥对无法
		 * 解开同态私钥信封）。 */
		fprintf(stderr, "ERROR: SDF_GetPrivateKeyAccessRight failed for index %u (auth index): 0x%08x\n",
				auth_index, ar);
		if (pass_from_env)
			fprintf(stderr, "HINT: the access password of the SM2 encryption private key at this index was rejected"
					" (TECC_SDF_PASS must match the password set on the device key)\n");
		else
			fprintf(stderr, "HINT: neither the empty password nor the 8-byte placeholder was accepted;"
					" if that key is protected by an access password, set TECC_SDF_PASS to it;"
					" if the index holds no SM2 encryption key pair, inject one (a signing key pair cannot"
					" unwrap the HE private key envelope)\n");
		/* End - ReqID:TECC4.0 - Bug 9334 */
		return sdf_error_to_hsm_error(ar);
	}

	swxa_priv_auth_record(priv, auth_index);
	return HSM_SUCCESS;
}

/*
 * swxa_session_key_acquire - 取会话密钥句柄（缓存命中复用，未命中导入后入缓存）
 *
 * 调用方需持有 priv->lock。
 * 成功时 *handle_out 为可用句柄，*owned_by_cache 表示句柄是否由缓存持有（false
 * 时调用方用完必须调 swxa_session_key_release 销毁，即缓存关闭时的旧行为），
 * *from_cache 表示本次是否命中缓存（供失败重试判断）。
 * priv 为 NULL 或 cache_enabled=false 时退化为"每次导入、用完销毁"。
 */
static HsmErrorCode
swxa_session_key_acquire(HsmDevice *device, SwxaDevicePrivate *priv,
						 const unsigned char *key, size_t key_len,
						 void **handle_out, bool *owned_by_cache, bool *from_cache)
{
	int ret;
	int i;
	int victim;
	unsigned long long oldest;
	SwxaKeyCacheEntry *entry;

	*handle_out = NULL;
	*owned_by_cache = false;
	*from_cache = false;

	if (priv != NULL && priv->cache_enabled)
	{
		for (i = 0; i < SWXA_KEY_CACHE_SLOTS; i++)
		{
			entry = &priv->entries[i];
			if (entry->used && entry->key_len == key_len &&
				memcmp(entry->key, key, key_len) == 0)
			{
				entry->stamp = ++priv->stamp_counter;
				*handle_out = entry->handle;
				*owned_by_cache = true;
				*from_cache = true;
				return HSM_SUCCESS;
			}
		}
	}

	/* 未命中：导入会话密钥句柄 */
	ret = swxa_sdf.SDF_ImportKey(device->session_handle,
								 (unsigned char *)key, (unsigned int)key_len,
								 handle_out);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_ImportKey failed for %zu-byte key: 0x%08x\n",
				key_len, ret);
		return sdf_error_to_hsm_error(ret);
	}

	/* 缓存关闭：句柄归调用方，用完销毁 */
	if (priv == NULL || !priv->cache_enabled)
		return HSM_SUCCESS;

	/* 入缓存：优先空槽，无空槽按 LRU 淘汰（旧句柄先销毁，避免会话密钥句柄泄漏） */
	victim = -1;
	for (i = 0; i < SWXA_KEY_CACHE_SLOTS; i++)
	{
		if (!priv->entries[i].used)
		{
			victim = i;
			break;
		}
	}
	if (victim < 0)
	{
		oldest = priv->entries[0].stamp;
		victim = 0;
		for (i = 1; i < SWXA_KEY_CACHE_SLOTS; i++)
		{
			if (priv->entries[i].stamp < oldest)
			{
				oldest = priv->entries[i].stamp;
				victim = i;
			}
		}
		if (priv->entries[victim].handle != NULL)
			swxa_sdf.SDF_DestroyKey(device->session_handle, priv->entries[victim].handle);
	}

	entry = &priv->entries[victim];
	entry->used = true;
	entry->key_len = key_len;
	memcpy(entry->key, key, key_len);
	entry->handle = *handle_out;
	entry->stamp = ++priv->stamp_counter;
	*owned_by_cache = true;

	return HSM_SUCCESS;
}

/*
 * swxa_session_key_invalidate - 丢弃指定密钥的缓存条目
 *
 * 用于"运算失败且句柄来自缓存"的情形：句柄可能已被设备侧失效，先丢弃再重新导入
 * 重试一次。SDF_DestroyKey 的返回值忽略——句柄若真已失效，回收必然报错且无意义；
 * 若句柄仍有效，回收则避免会话密钥句柄泄漏。
 * 调用方需持有 priv->lock。
 */
static void
swxa_session_key_invalidate(HsmDevice *device, SwxaDevicePrivate *priv,
							const unsigned char *key, size_t key_len)
{
	int i;
	SwxaKeyCacheEntry *entry;

	if (priv == NULL)
		return;

	for (i = 0; i < SWXA_KEY_CACHE_SLOTS; i++)
	{
		entry = &priv->entries[i];
		if (!entry->used || entry->key_len != key_len ||
			memcmp(entry->key, key, key_len) != 0)
			continue;
		if (entry->handle != NULL && device != NULL && device->session_handle != NULL)
			swxa_sdf.SDF_DestroyKey(device->session_handle, entry->handle);
		entry->used = false;
		entry->handle = NULL;
		entry->key_len = 0;
		memset(entry->key, 0, sizeof(entry->key));
		return;
	}
}

/* swxa_session_key_release - 释放句柄：只有非缓存持有（缓存关闭路径）才需销毁 */
static void
swxa_session_key_release(HsmDevice *device, void *handle, bool owned_by_cache)
{
	if (owned_by_cache || handle == NULL)
		return;
	if (device != NULL && device->session_handle != NULL)
		swxa_sdf.SDF_DestroyKey(device->session_handle, handle);
}

/* ReqID:TECC4.0 - End：会话密钥句柄连接级缓存的实现 */

/*
 * swxa_sm4_encrypt - Encrypt data using SM4-CBC with raw key via HSM
 *
 * Takes the SM4 key as a session key handle (connection-level cached, see
 * SWXA_KEY_CACHE_SLOTS), encrypts with PKCS#7 padding via SDF_Encrypt
 * (SGD_SMS4_CBC), then releases the session key.
 */
static HsmErrorCode
swxa_sm4_encrypt(HsmDevice *device,
				 const unsigned char *key, size_t key_len,
				 const unsigned char *iv,
				 const unsigned char *plaintext, size_t plain_len,
				 unsigned char *ciphertext, size_t *cipher_len)
{
	int ret;
	void *hKey = NULL;
	size_t padded_len;
	unsigned char *padded = NULL;
	unsigned char pad_val;
	unsigned int out_len;
	unsigned char local_key[32];
	unsigned char local_iv[16];
	SwxaDevicePrivate *priv;
	HsmErrorCode acquire_ret;
	bool owned_by_cache;
	bool from_cache;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!key || key_len == 0 || key_len > sizeof(local_key) || !iv || !plaintext || plain_len == 0 ||
		!ciphertext || !cipher_len || *cipher_len == 0)
		return HSM_ERROR_INVALID_PARAM;

	/* Copy key and IV to local buffers to protect against SDF modification */
	memcpy(local_key, key, key_len);
	memcpy(local_iv, iv, sizeof(local_iv));

	/* PKCS#7 padding: pad to 16-byte block boundary */
	padded_len = ((plain_len / 16) + 1) * 16;
	pad_val = (unsigned char)(padded_len - plain_len);
	padded = (unsigned char *)malloc(padded_len);
	if (!padded)
	{
		memset(local_key, 0, sizeof(local_key));
		return HSM_ERROR_INTERNAL;
	}
	memcpy(padded, plaintext, plain_len);
	memset(padded + plain_len, pad_val, padded_len - plain_len);

	/* ReqID:TECC4.0 - 取会话密钥句柄（缓存命中则跳过 SDF_ImportKey，每值省 1 次设备
	 * 往返）。缓存打开时句柄跨值复用，故"取句柄 + 运算"整段在 priv->lock 内完成，
	 * 避免同一连接并发调用时句柄正被淘汰/销毁。 */
	priv = swxa_private(device);
	if (priv != NULL)
		pthread_mutex_lock(&priv->lock);

	acquire_ret = swxa_session_key_acquire(device, priv, local_key, key_len,
										   &hKey, &owned_by_cache, &from_cache);
	if (acquire_ret != HSM_SUCCESS)
	{
		if (priv != NULL)
			pthread_mutex_unlock(&priv->lock);
		free(padded);
		memset(local_key, 0, sizeof(local_key));
		return acquire_ret;
	}

	/* SM4-CBC encrypt via HSM */
	out_len = (unsigned int)padded_len;
	ret = swxa_sdf.SDF_Encrypt(device->session_handle, hKey, SGD_SMS4_CBC,
							  local_iv,
							  padded, (unsigned int)padded_len,
							  ciphertext, &out_len);

	/* ReqID:TECC4.0 - 缓存句柄可能已被设备侧失效（设备重启、设备侧清空会话句柄表）：
	 * 丢弃该条目、重新导入并重试一次。IV 可能被 SDF_Encrypt 原地改写，重试前复位。 */
	if (ret != SDR_OK && from_cache)
	{
		swxa_session_key_invalidate(device, priv, local_key, key_len);
		memcpy(local_iv, iv, sizeof(local_iv));
		acquire_ret = swxa_session_key_acquire(device, priv, local_key, key_len,
											   &hKey, &owned_by_cache, &from_cache);
		if (acquire_ret == HSM_SUCCESS)
		{
			out_len = (unsigned int)padded_len;
			ret = swxa_sdf.SDF_Encrypt(device->session_handle, hKey, SGD_SMS4_CBC,
									  local_iv,
									  padded, (unsigned int)padded_len,
									  ciphertext, &out_len);
		}
	}

	swxa_session_key_release(device, hKey, owned_by_cache);
	if (priv != NULL)
		pthread_mutex_unlock(&priv->lock);

	free(padded);
	memset(local_key, 0, sizeof(local_key));

	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_encrypt: SDF_Encrypt failed: 0x%08x\n", ret);
		return sdf_error_to_hsm_error(ret);
	}

	*cipher_len = out_len;
	return HSM_SUCCESS;
}

/*
 * swxa_sm4_decrypt - Decrypt data using SM4-CBC with raw key via HSM
 *
 * Takes the SM4 key as a session key handle (connection-level cached, see
 * SWXA_KEY_CACHE_SLOTS), decrypts via SDF_Decrypt (SGD_SMS4_CBC), removes
 * PKCS#7 padding, then releases the session key.
 */
static HsmErrorCode
swxa_sm4_decrypt(HsmDevice *device,
				 const unsigned char *key, size_t key_len,
				 const unsigned char *iv,
				 const unsigned char *ciphertext, size_t cipher_len,
				 unsigned char *plaintext, size_t *plain_len)
{
	int ret;
	void *hKey = NULL;
	unsigned int out_len;
	unsigned char *decrypted = NULL;
	unsigned char pad_val;
	unsigned char local_key[32];
	unsigned char local_iv[16];
	SwxaDevicePrivate *priv;
	HsmErrorCode acquire_ret;
	bool owned_by_cache;
	bool from_cache;

	if (!device || !device->connected || !device->session_handle)
		return HSM_ERROR_CONNECT_FAILED;

	if (!key || key_len == 0 || key_len > sizeof(local_key) || !iv || !ciphertext || cipher_len == 0 ||
		!plaintext || !plain_len)
		return HSM_ERROR_INVALID_PARAM;

	/* Copy key and IV to local buffers to protect against SDF modification */
	memcpy(local_key, key, key_len);
	memcpy(local_iv, iv, sizeof(local_iv));

	/* Allocate output buffer */
	decrypted = (unsigned char *)malloc(cipher_len);
	if (!decrypted)
	{
		memset(local_key, 0, sizeof(local_key));
		return HSM_ERROR_INTERNAL;
	}

	/* ReqID:TECC4.0 - 取会话密钥句柄（缓存命中则跳过 SDF_ImportKey，每值省 1 次设备
	 * 往返）。缓存打开时句柄跨值复用，故"取句柄 + 运算"整段在 priv->lock 内完成，
	 * 避免同一连接并发调用时句柄正被淘汰/销毁。 */
	priv = swxa_private(device);
	if (priv != NULL)
		pthread_mutex_lock(&priv->lock);

	acquire_ret = swxa_session_key_acquire(device, priv, local_key, key_len,
										   &hKey, &owned_by_cache, &from_cache);
	if (acquire_ret != HSM_SUCCESS)
	{
		if (priv != NULL)
			pthread_mutex_unlock(&priv->lock);
		free(decrypted);
		memset(local_key, 0, sizeof(local_key));
		return acquire_ret;
	}

	/* SM4-CBC decrypt via HSM */
	out_len = (unsigned int)cipher_len;
	ret = swxa_sdf.SDF_Decrypt(device->session_handle, hKey, SGD_SMS4_CBC,
							  local_iv,
							  (unsigned char *)ciphertext, (unsigned int)cipher_len,
							  decrypted, &out_len);

	/* ReqID:TECC4.0 - 缓存句柄可能已被设备侧失效（设备重启、设备侧清空会话句柄表）：
	 * 丢弃该条目、重新导入并重试一次。IV 可能被 SDF_Decrypt 原地改写，重试前复位。 */
	if (ret != SDR_OK && from_cache)
	{
		swxa_session_key_invalidate(device, priv, local_key, key_len);
		memcpy(local_iv, iv, sizeof(local_iv));
		acquire_ret = swxa_session_key_acquire(device, priv, local_key, key_len,
											   &hKey, &owned_by_cache, &from_cache);
		if (acquire_ret == HSM_SUCCESS)
		{
			out_len = (unsigned int)cipher_len;
			ret = swxa_sdf.SDF_Decrypt(device->session_handle, hKey, SGD_SMS4_CBC,
									  local_iv,
									  (unsigned char *)ciphertext, (unsigned int)cipher_len,
									  decrypted, &out_len);
		}
	}

	swxa_session_key_release(device, hKey, owned_by_cache);
	if (priv != NULL)
		pthread_mutex_unlock(&priv->lock);

	memset(local_key, 0, sizeof(local_key));

	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: swxa_sm4_decrypt: SDF_Decrypt failed: 0x%08x\n", ret);
		free(decrypted);
		return sdf_error_to_hsm_error(ret);
	}

	/* Remove PKCS#7 padding */
	if (out_len == 0)
	{
		free(decrypted);
		return HSM_ERROR_INVALID_PARAM;
	}

	pad_val = decrypted[out_len - 1];
	if (pad_val > 0 && pad_val <= 16 && out_len >= pad_val)
	{
		size_t unpadded_len = out_len - pad_val;
		memcpy(plaintext, decrypted, unpadded_len);
		*plain_len = unpadded_len;
	}
	else
	{
		/* Invalid padding, return data as-is */
		fprintf(stderr, "WARNING: swxa_sm4_decrypt: invalid PKCS#7 padding (pad_value=%u), "\
				"returning raw decrypted data\n", pad_val);
		memcpy(plaintext, decrypted, out_len);
		*plain_len = out_len;
	}
	free(decrypted);
	return HSM_SUCCESS;
}

#define _POSIX_C_SOURCE 200809L

/*
 * probe_sdf_conc.c —— 探针：加密机多会话并发能否线性扩展
 *
 * 背景：读路径拼接批量解密落地后（1 万行 13288ms -> 346ms），10 万行实测 1.3 秒，
 * 按每值 64 字节设备流量 + 客户端开销折算，100 万行约 10.5 秒，仍超 8 秒要求。
 * 单会话时设备带宽约 10MB/s 是硬顶，故要问：同时开多个 SDF 会话并行调用，
 * 吞吐能否近似线性增长（设备连接池 poolsize=10）。
 *
 * 本探针：一次 SDF_OpenDevice，然后 N 个 SDF_OpenSession（各自 import 同一把会话
 * 密钥），N 个线程各在自己的会话上反复做同长度的 SDF_Decrypt，比较 N=1/2/4/8 的
 * 聚合吞吐与单线程耗时。只看设备/厂商库行为，不涉及数据库。
 *
 * 编译: gcc -O2 -Wall -Wextra -std=c99 -o probe_sdf_conc probe_sdf_conc.c -ldl -lpthread
 *
 * 用法: probe_sdf_conc [swsds.ini路径] [libswsds.so路径] [每次调用字节数] [每线程调用次数]
 *   默认 /etc/swsds.ini、/usr/lib64/libswsds.so、262144 字节、8 次
 *   真机模式建议在含 swsds.ini 的目录下执行（适配器的搜索顺序为 PGDATA → 当前目录 → /etc）
 *   注意：会话数 ≥4 时设备吞吐可能不升反降，8 会话实测出现"不报错但吞吐归零"，勿盲目放大
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>
#include <pthread.h>

#define SGD_SMS4_CBC 0x00000402
#define SDR_OK 0
#define IV_LEN 16
#define MAX_THREADS 8

typedef int (*fp_OpenDeviceWithPathAndName)(char *pcCfgfile, void **phDeviceHandle);
typedef int (*fp_OpenSession)(void *hDeviceHandle, void **phSessionHandle);
typedef int (*fp_CloseDevice)(void *hDeviceHandle);
typedef int (*fp_CloseSession)(void *hSessionHandle);
typedef int (*fp_ImportKey)(void *hSessionHandle, unsigned char *pucKey,
							unsigned int uiKeyLength, void **phKeyHandle);
typedef int (*fp_DestroyKey)(void *hSessionHandle, void *hKeyHandle);
typedef int (*fp_Encrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
						  unsigned char *pucIV, unsigned char *pucData,
						  unsigned int uiDataLength, unsigned char *pucEncData,
						  unsigned int *puiEncDataLength);
typedef int (*fp_Decrypt)(void *hSessionHandle, void *hKeyHandle, unsigned int uiAlgorithmID,
						  unsigned char *pucIV, unsigned char *pucEncData,
						  unsigned int uiEncDataLength, unsigned char *pucData,
						  unsigned int *puiDataLength);

static fp_OpenDeviceWithPathAndName p_OpenDevice;
static fp_OpenSession p_OpenSession;
static fp_CloseDevice p_CloseDevice;
static fp_CloseSession p_CloseSession;
static fp_ImportKey p_ImportKey;
static fp_DestroyKey p_DestroyKey;
static fp_Encrypt p_Encrypt;
static fp_Decrypt p_Decrypt;

static void *g_device = NULL;
static unsigned char g_key[16];
static unsigned char *g_ct = NULL;			/* 共享只读密文（一次调用长度） */
static unsigned int g_ct_len = 0;
static int g_calls = 8;

typedef struct ThreadArg
{
	int idx;
	void *session;
	void *hkey;
	unsigned char *out;
	double ms;
	int errors;
} ThreadArg;

static volatile int g_start = 0;

static double
now_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec * 1000.0 + (double) ts.tv_nsec / 1000000.0;
}

static void *
worker(void *arg)
{
	ThreadArg *t = (ThreadArg *) arg;
	double t0;
	int c;
	int ret = SDR_OK;

	/* 自旋等待统一发令，避免线程创建时间混进测量 */
	while (g_start == 0)
		;

	t0 = now_ms();
	for (c = 0; c < g_calls; c++)
	{
		unsigned char iv[IV_LEN];
		unsigned int out_len = g_ct_len;

		memset(iv, 0, sizeof(iv));			/* 只影响被丢弃的首块 */
		ret = p_Decrypt(t->session, t->hkey, SGD_SMS4_CBC, iv,
						g_ct, g_ct_len, t->out, &out_len);
		if (ret != SDR_OK)
		{
			t->errors++;
			ret = SDR_OK;
		}
	}
	t->ms = now_ms() - t0;
	return NULL;
}

static unsigned char *
pkcs7_pad(const unsigned char *in, size_t in_len, size_t *out_len)
{
	size_t padded = ((in_len / 16) + 1) * 16;
	unsigned char *buf = malloc(padded);
	unsigned char pad = (unsigned char) (padded - in_len);

	memcpy(buf, in, in_len);
	memset(buf + in_len, pad, padded - in_len);
	*out_len = padded;
	return buf;
}

/* 用 0 号会话把 g_ct 准备好：一段合法的 SM4-CBC 密文（够长即可） */
static int
prepare_ciphertext(void *session, void *hkey, unsigned int want_len)
{
	size_t pt_len;
	unsigned char *pt;
	unsigned char *padded;
	size_t padded_len;
	unsigned char iv[IV_LEN];
	unsigned int ct_len = want_len;
	int ret;

	/* 明文长度取 want_len-16 再补齐，使密文长度尽量接近 want_len */
	pt_len = (want_len > 32) ? (size_t) want_len - 16 : 16;
	pt = malloc(pt_len);
	memset(pt, 'Q', pt_len);
	padded = pkcs7_pad(pt, pt_len, &padded_len);
	free(pt);

	/* 向量按 16 字节对齐后可能略大于 want_len，直接用它作为调用长度 */
	g_ct = malloc(padded_len + 64);
	memset(iv, 0, sizeof(iv));
	ret = p_Encrypt(session, hkey, SGD_SMS4_CBC, iv, padded,
					(unsigned int) padded_len, g_ct, &ct_len);
	free(padded);
	if (ret != SDR_OK)
	{
		fprintf(stderr, "ERROR: 准备密文失败: 0x%08x\n", ret);
		return -1;
	}
	g_ct_len = ct_len;
	return 0;
}

static void
run_concurrent(int nthreads)
{
	ThreadArg args[MAX_THREADS];
	pthread_t tids[MAX_THREADS];

	int i;
	double t0;
	double total_bytes;
	double max_ms = 0.0;
	int errors = 0;

	memset(args, 0, sizeof(args));
	for (i = 0; i < nthreads; i++)
	{
		void *sess = NULL;
		void *hkey = NULL;

		if (p_OpenSession(g_device, &sess) != SDR_OK)
		{
			printf("  %d 会话: SDF_OpenSession 失败，无法并发\n", nthreads);
			return;
		}
		if (p_ImportKey(sess, g_key, sizeof(g_key), &hkey) != SDR_OK)
		{
			p_CloseSession(sess);
			printf("  %d 会话: SDF_ImportKey 失败，无法并发\n", nthreads);
			return;
		}
		args[i].idx = i;
		args[i].session = sess;
		args[i].hkey = hkey;
		args[i].out = malloc(g_ct_len + 64);
	}

	g_start = 0;
	t0 = now_ms();
	for (i = 0; i < nthreads; i++)
		pthread_create(&tids[i], NULL, worker, &args[i]);
	g_start = 1;						/* 发令 */
	for (i = 0; i < nthreads; i++)
		pthread_join(tids[i], NULL);
	{
		double wall = now_ms() - t0;

		for (i = 0; i < nthreads; i++)
		{
			if (args[i].ms > max_ms)
				max_ms = args[i].ms;
			errors += args[i].errors;
		}
		total_bytes = (double) g_ct_len * (double) g_calls * (double) nthreads;
		printf("  会话数=%d  总字节=%.2f MB  墙钟=%.1f ms  单线程最长=%.1f ms  "
			   "聚合=%.1f MB/s  单线程=%.1f MB/s  错误调用=%d\n",
			   nthreads, total_bytes / 1048576.0, wall, max_ms,
			   (total_bytes / 1048576.0) / (wall / 1000.0),
			   ((double) g_ct_len * g_calls / 1048576.0) / (max_ms / 1000.0),
			   errors);
	}

	for (i = 0; i < nthreads; i++)
	{
		p_DestroyKey(args[i].session, args[i].hkey);
		p_CloseSession(args[i].session);
		free(args[i].out);
	}
}

int
main(int argc, char **argv)
{
	const char *ini_path = (argc > 1) ? argv[1] : "/etc/swsds.ini";
	const char *lib_path = (argc > 2) ? argv[2] : "/usr/lib64/libswsds.so";
	unsigned int call_bytes = (argc > 3) ? (unsigned int) strtoul(argv[3], NULL, 10) : 262144;
	void *bootstrap = NULL;
	void *bootkey = NULL;
	void *handle;
	int i;

	if (argc > 4)
		g_calls = atoi(argv[4]);
	if (g_calls < 1)
		g_calls = 1;

	for (i = 0; i < 16; i++)
		g_key[i] = (unsigned char) (0x20 + i);

	printf("=== 加密机多会话并发探针 ===\n");
	printf("swsds.ini=%s  库=%s  每次调用=%u 字节  每线程=%d 次\n\n",
		   ini_path, lib_path, call_bytes, g_calls);

	handle = dlopen(lib_path, RTLD_NOW);
	if (handle == NULL)
	{
		fprintf(stderr, "ERROR: dlopen 失败: %s\n", dlerror());
		return 2;
	}

#define LOAD(field, type, sym) \
	p_##field = (type) dlsym(handle, sym); \
	if (p_##field == NULL) { fprintf(stderr, "ERROR: 缺少符号 %s\n", sym); return 2; }

	LOAD(OpenDevice, fp_OpenDeviceWithPathAndName, "SDF_OpenDeviceWithPathAndName");
	LOAD(OpenSession, fp_OpenSession, "SDF_OpenSession");
	LOAD(CloseDevice, fp_CloseDevice, "SDF_CloseDevice");
	LOAD(CloseSession, fp_CloseSession, "SDF_CloseSession");
	LOAD(ImportKey, fp_ImportKey, "SDF_ImportKey");
	LOAD(DestroyKey, fp_DestroyKey, "SDF_DestroyKey");
	LOAD(Encrypt, fp_Encrypt, "SDF_Encrypt");
	LOAD(Decrypt, fp_Decrypt, "SDF_Decrypt");
#undef LOAD

	if (p_OpenDevice((char *) ini_path, &g_device) != SDR_OK)
	{
		fprintf(stderr, "ERROR: SDF_OpenDeviceWithPathAndName 失败\n");
		return 3;
	}
	if (p_OpenSession(g_device, &bootstrap) != SDR_OK ||
		p_ImportKey(bootstrap, g_key, sizeof(g_key), &bootkey) != SDR_OK)
	{
		fprintf(stderr, "ERROR: 引导会话/密钥导入失败\n");
		return 3;
	}
	if (prepare_ciphertext(bootstrap, bootkey, call_bytes) != 0)
		return 4;
	p_DestroyKey(bootstrap, bootkey);
	p_CloseSession(bootstrap);
	printf("密文准备完成：调用长度 %u 字节\n\n", g_ct_len);

	printf("并发扩展测量（同样每个线程做 %d 次调用）：\n", g_calls);
	run_concurrent(1);
	run_concurrent(2);
	run_concurrent(4);
	run_concurrent(8);

	/* 每个配置再来一轮，避免单次抖动误判 */
	printf("\n重复一轮：\n");
	run_concurrent(1);
	run_concurrent(4);

	printf("\n探针结束。\n");
	return 0;
}

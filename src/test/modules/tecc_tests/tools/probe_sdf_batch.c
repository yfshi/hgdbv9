/*
 * probe_sdf_batch.c —— 一次性探针：验证"多个 IV||CT 拼成一段、单次 SDF_Decrypt"方案
 *
 * 目的（对应 plan 讨论）：
 *   硬加密列的密文格式是 [16B IV][SM4-CBC 密文]。当前 libpq 逐值调用设备
 *   （swxa_sm4_decrypt -> SDF_Decrypt），每值一次 TCP 往返，真机约 1.3~1.8ms，
 *   1 万行全量返回约 18 秒。
 *   若把 N 个值拼成 IV1||CT1||IV2||CT2||... 用一次 SDF_Decrypt 解出（CBC 解密
 *   每块只依赖自己与前一块，故显式插入 IV 块即可让每个值独立解对），往返次数可从
 *   N 次降到 1 次（受单次长度上限约束则降到 N/上限）。
 *
 * 本探针回答三个问题：
 *   A. 正确性：拼接后单次解密，各值能否还原（含 PKCS#7 padding / 16 字节整数倍
 *      需要补整块的情况）；
 *   B. 长度上限：单次调用能处理多大的缓冲（1KB ~ 16MB 逐档），失败返回什么错误码；
 *   C. 收益：同样数量的值，"一次拼批"与"逐值循环"的总耗时对比。
 *
 * 编译: gcc -O2 -Wall -Wextra -std=c99 -o probe_sdf_batch probe_sdf_batch.c -ldl
 *
 * 用法: probe_sdf_batch [swsds.ini路径] [libswsds.so路径]
 *   默认 swsds.ini = /etc/swsds.ini，默认库 = /usr/lib64/libswsds.so
 *   真机模式建议在含 swsds.ini 的目录下执行（适配器的搜索顺序为 PGDATA → 当前目录 → /etc）
 *
 * 本程序只读设备、只用自己的临时密钥（进程内 import 的会话密钥），不触碰数据库
 * 里的任何 DEK/KEK，可随时终止。
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>

#define SGD_SMS4_CBC 0x00000402
#define SDR_OK 0
#define IV_LEN 16

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

static double
now_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec * 1000.0 + (double) ts.tv_nsec / 1000000.0;
}

static void *
load_sym(void *handle, const char *name)
{
	void *p = dlsym(handle, name);

	if (p == NULL)
	{
		fprintf(stderr, "ERROR: 找不到符号 %s: %s\n", name, dlerror());
		exit(2);
	}
	return p;
}

/* PKCS#7 补齐到 16 字节倍数。设备侧不做 padding（libpq 适配器 swxa_sm4_encrypt
 * 自己补），故探针必须照做，否则设备直接报长度错误（实测 0x01000606）。 */
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

/* 校验一个值的明文段：前缀应等于原始明文，尾部应是合法的 PKCS#7 填充 */
static int
check_segment(const unsigned char *seg, size_t seg_len,
			  const unsigned char *pt, size_t pt_len,
			  size_t *plain_out, unsigned int *pad_out)
{
	size_t pad;
	size_t k;
	int pad_ok = 1;

	if (seg_len < pt_len)
		return 0;
	if (memcmp(seg, pt, pt_len) != 0)
		return 0;

	pad = seg[seg_len - 1];
	if (pad == 0 || pad > 16 || pad > seg_len)
		return 0;
	for (k = seg_len - pad; k < seg_len; k++)
	{
		if (seg[k] != (unsigned char) pad)
		{
			pad_ok = 0;
			break;
		}
	}
	if (!pad_ok)
		return 0;

	*plain_out = seg_len - pad;
	*pad_out = (unsigned int) pad;
	return 1;
}

int
main(int argc, char **argv)
{
	const char *ini_path = (argc > 1) ? argv[1] : "/etc/swsds.ini";
	const char *lib_path = (argc > 2) ? argv[2] : "/usr/lib64/libswsds.so";
	void *handle;
	void *dev = NULL;
	void *session = NULL;
	void *hkey = NULL;
	unsigned char key[16];
	int ret;
	int i;

	for (i = 0; i < 16; i++)
		key[i] = (unsigned char) (0x10 + i);

	printf("=== SDF 拼接批量解密探针 ===\n");
	printf("swsds.ini : %s\n", ini_path);
	printf("库        : %s\n\n", lib_path);

	handle = dlopen(lib_path, RTLD_NOW);
	if (handle == NULL)
	{
		fprintf(stderr, "ERROR: dlopen(%s) 失败: %s\n", lib_path, dlerror());
		return 2;
	}

	p_OpenDevice = (fp_OpenDeviceWithPathAndName) load_sym(handle, "SDF_OpenDeviceWithPathAndName");
	p_OpenSession = (fp_OpenSession) load_sym(handle, "SDF_OpenSession");
	p_CloseDevice = (fp_CloseDevice) load_sym(handle, "SDF_CloseDevice");
	p_CloseSession = (fp_CloseSession) load_sym(handle, "SDF_CloseSession");
	p_ImportKey = (fp_ImportKey) load_sym(handle, "SDF_ImportKey");
	p_DestroyKey = (fp_DestroyKey) load_sym(handle, "SDF_DestroyKey");
	p_Encrypt = (fp_Encrypt) load_sym(handle, "SDF_Encrypt");
	p_Decrypt = (fp_Decrypt) load_sym(handle, "SDF_Decrypt");

	{
		double t0 = now_ms();

		ret = p_OpenDevice((char *) ini_path, &dev);
		printf("SDF_OpenDeviceWithPathAndName : ret=0x%08x  (%.1f ms)\n", ret, now_ms() - t0);
		if (ret != SDR_OK)
			return 3;
	}
	{
		double t0 = now_ms();

		ret = p_OpenSession(dev, &session);
		printf("SDF_OpenSession               : ret=0x%08x  (%.1f ms)\n", ret, now_ms() - t0);
		if (ret != SDR_OK)
			return 3;
	}
	{
		double t0 = now_ms();

		ret = p_ImportKey(session, key, sizeof(key), &hkey);
		printf("SDF_ImportKey(16B 裸密钥)      : ret=0x%08x  (%.1f ms)\n\n", ret, now_ms() - t0);
		if (ret != SDR_OK)
			return 3;
	}

	/* ================= A. 正确性 ================= */
	printf("--- A. 拼接正确性（8 个不同长度的值） ---\n");
	{
		static const size_t pt_lens[] = {1, 15, 16, 17, 31, 32, 48, 63};
		const int nvals = (int) (sizeof(pt_lens) / sizeof(pt_lens[0]));
		unsigned char *pt[8];
		unsigned char *ct[8];
		unsigned char iv[8][IV_LEN];
		unsigned int ct_len[8];
		unsigned int total = 0;
		unsigned char *stream;
		unsigned char *out;
		unsigned int out_len;
		unsigned int off;
		int bad = 0;

		for (i = 0; i < nvals; i++)
		{
			size_t k;
			size_t plen;
			unsigned char *pbuf;

			pt[i] = malloc(pt_lens[i] + 32);
			ct[i] = malloc(pt_lens[i] + 64);
			for (k = 0; k < pt_lens[i]; k++)
				pt[i][k] = (unsigned char) ('A' + i);
			for (k = 0; k < IV_LEN; k++)
				iv[i][k] = (unsigned char) (0xA0 + i * 3 + k);
			pbuf = pkcs7_pad(pt[i], pt_lens[i], &plen);
			{
				/* 设备会原地改写传入的 IV 缓冲（适配器为此专门拷副本），
				 * 这里必须用副本，否则 iv[i] 被污染、后续拼流/对照都会错。 */
				unsigned char iv_enc[IV_LEN];

				memcpy(iv_enc, iv[i], IV_LEN);
				ret = p_Encrypt(session, hkey, SGD_SMS4_CBC, iv_enc, pbuf,
								(unsigned int) plen, ct[i], &ct_len[i]);
			}
			free(pbuf);
			if (ret != SDR_OK)
			{
				printf("  SDF_Encrypt 值 %d 失败: 0x%08x\n", i, ret);
				return 4;
			}
			printf("  值 %d: 明文 %2zu B -> 密文 %2u B (IV 16 B, 合计 %u B)\n",
				   i, pt_lens[i], ct_len[i], ct_len[i] + IV_LEN);
			total += IV_LEN + ct_len[i];
		}

		stream = malloc(total);
		out = malloc(total + 64);
		off = 0;
		for (i = 0; i < nvals; i++)
		{
			memcpy(stream + off, iv[i], IV_LEN);
			off += IV_LEN;
			memcpy(stream + off, ct[i], ct_len[i]);
			off += ct_len[i];
		}

		out_len = total;
		{
			unsigned char dummy_iv[IV_LEN];
			double t0 = now_ms();
			double ms;

			memset(dummy_iv, 0, sizeof(dummy_iv));
			ret = p_Decrypt(session, hkey, SGD_SMS4_CBC, dummy_iv,
							stream, total, out, &out_len);
			ms = now_ms() - t0;
			printf("\n  拼接 1 次 SDF_Decrypt: ret=0x%08x, 输入 %u B, 输出 %u B, 耗时 %.2f ms\n",
				   ret, total, out_len, ms);
		}
		if (ret != SDR_OK)
			return 5;

		off = 0;
		for (i = 0; i < nvals; i++)
		{
			const unsigned char *seg = out + off + IV_LEN;
			size_t seg_len;
			size_t plain;
			unsigned int pad;

			if (i == nvals - 1)
				seg_len = (size_t) out_len - (size_t) (off + IV_LEN);
			else
				seg_len = ct_len[i];

			if (check_segment(seg, seg_len, pt[i], pt_lens[i], &plain, &pad))
				printf("  还原 值 %d (%2zu B): OK  (段长 %zu, 剥掉 padding %u -> 明文 %zu)\n",
					   i, pt_lens[i], seg_len, pad, plain);
			else
			{
				printf("  还原 值 %d (%2zu B): 失败 (段长 %zu)\n", i, pt_lens[i], seg_len);
				bad++;
			}
			off += IV_LEN + ct_len[i];
		}

		/* 对照：逐值解密同样的 8 个值 */
		{
			double t0 = now_ms();
			int ok = 0;

			for (i = 0; i < nvals; i++)
			{
				unsigned char tmp_iv[IV_LEN];
				unsigned char *tmp_out = malloc(ct_len[i] + 64);
				unsigned int tmp_len = ct_len[i];

				memcpy(tmp_iv, iv[i], IV_LEN);
				if (p_Decrypt(session, hkey, SGD_SMS4_CBC, tmp_iv, ct[i], ct_len[i],
							  tmp_out, &tmp_len) == SDR_OK
					&& tmp_len >= pt_lens[i]
					&& memcmp(tmp_out, pt[i], pt_lens[i]) == 0
					&& (size_t) tmp_len - (size_t) tmp_out[tmp_len - 1] == pt_lens[i])
					ok++;
				free(tmp_out);
			}
			printf("  对照：逐值 8 次 SDF_Decrypt 耗时 %.2f ms，其中还原正确 %d/8"
				   "（设备返回补齐后的明文，PKCS#7 由调用方剥）\n",
				   now_ms() - t0, ok);
		}

		printf("  >>> A 结论: %s (总长 %u B)\n\n", bad == 0 ? "全部还原正确" : "有值还原失败", total);
	}

	/* ================= B/C. 长度上限与耗时 ================= */
	printf("--- B/C. 长度上限与耗时（值为 32 B 明文 -> 64 B blob） ---\n");
	printf("  %-12s %8s %10s %10s %14s %10s %14s\n",
		   "目标长度", "值个数", "调用字节", "返回值", "单次耗时(ms)", "吞吐MB/s", "逐值耗时(ms)");
	{
		static const unsigned int targets_kb[] = {1, 16, 64, 256, 1024, 4096, 16384};
		const size_t pt_len = 32;
		unsigned int t_idx;

		for (t_idx = 0; t_idx < sizeof(targets_kb) / sizeof(targets_kb[0]); t_idx++)
		{
			unsigned int target = targets_kb[t_idx] * 1024;
			int nvals = (int) (target / 64);
			unsigned char *pt;
			unsigned char *ct;
			unsigned char *ivs;
			unsigned int ct_len = 0;
			unsigned int total;
			unsigned char *stream;
			unsigned char *out;
			unsigned int out_len;
			unsigned int off;
			double t_batch;
			double t_each = -1.0;
			int b;
			int verified = 0;

			if (nvals < 1)
				nvals = 1;

			pt = malloc(pt_len);
			ct = malloc(pt_len + 64);
			ivs = malloc((size_t) nvals * IV_LEN);
			memset(pt, 'Z', pt_len);
			memset(ivs, 0x5A, (size_t) nvals * IV_LEN);

			/* 用 0 号 IV 加一次密，得到密文长度；后续 blob 靠重复同一密文块放大长度，
			 * 只是为了让设备处理大缓冲，正确性由 A 段保证。 */
			{
				unsigned char iv0[IV_LEN];
				size_t plen;
				unsigned char *pbuf;

				memcpy(iv0, ivs, IV_LEN);
				pbuf = pkcs7_pad(pt, pt_len, &plen);
				ret = p_Encrypt(session, hkey, SGD_SMS4_CBC, iv0, pbuf,
								(unsigned int) plen, ct, &ct_len);
				free(pbuf);
				if (ret != SDR_OK)
				{
					printf("  SDF_Encrypt 失败: 0x%08x，停止放大测试\n", ret);
					break;
				}
			}

			total = (unsigned int) nvals * (IV_LEN + ct_len);
			stream = malloc(total);
			out = malloc(total + 64);
			off = 0;
			for (b = 0; b < nvals; b++)
			{
				memcpy(stream + off, ivs + (size_t) b * IV_LEN, IV_LEN);
				off += IV_LEN;
				memcpy(stream + off, ct, ct_len);
				off += ct_len;
			}

			out_len = total;
			{
				unsigned char dummy_iv[IV_LEN];
				double t0;

				memset(dummy_iv, 0, sizeof(dummy_iv));
				t0 = now_ms();
				ret = p_Decrypt(session, hkey, SGD_SMS4_CBC, dummy_iv,
								stream, total, out, &out_len);
				t_batch = now_ms() - t0;
			}

			if (ret == SDR_OK)
			{
				/* 抽查首/中/尾三个值的明文前缀 */
				int probe_idx[3];
				int p;

				probe_idx[0] = 0;
				probe_idx[1] = nvals / 2;
				probe_idx[2] = nvals - 1;
				for (p = 0; p < 3; p++)
				{
					size_t o = (size_t) probe_idx[p] * (IV_LEN + ct_len) + IV_LEN;

					if (o + pt_len <= out_len && memcmp(out + o, pt, pt_len) == 0)
						verified++;
				}
			}

			/* 逐值对照：值个数限制在 4096 以内，否则太慢 */
			if (nvals <= 4096)
			{
				double t0 = now_ms();
				int v;

				for (v = 0; v < nvals; v++)
				{
					unsigned char tmp_iv[IV_LEN];
					unsigned char *tmp_out = malloc(ct_len + 64);
					unsigned int tmp_len = ct_len;

					memcpy(tmp_iv, ivs + (size_t) v * IV_LEN, IV_LEN);
					(void) p_Decrypt(session, hkey, SGD_SMS4_CBC, tmp_iv, ct, ct_len,
									 tmp_out, &tmp_len);
					free(tmp_out);
				}
				t_each = now_ms() - t0;
			}

			char size_label[32];

			if (targets_kb[t_idx] >= 1024)
				snprintf(size_label, sizeof(size_label), "%uMB", targets_kb[t_idx] / 1024);
			else
				snprintf(size_label, sizeof(size_label), "%uKB", targets_kb[t_idx]);

			printf("  %-12s %8d %10u 0x%08x %14.2f %10.1f %14s  抽查明文 %d/3\n",
				   size_label,
				   nvals, total, ret, t_batch,
				   t_batch > 0 ? ((double) total / 1048576.0) / (t_batch / 1000.0) : 0.0,
				   t_each < 0 ? "未测" : "见下行",
				   verified);

			if (ret == SDR_OK && t_each > 0)
			{
				/* 把逐值耗时单独打一行，便于对比 */
				printf("               ^ 同样 %d 个值逐值调用合计 %.1f ms -> 拼接把设备往返 "
					   "从 %d 次降到 1 次，快 %.1f 倍\n",
					   nvals, t_each, nvals, t_each / (t_batch > 0 ? t_batch : 1));
			}
			if (ret != SDR_OK)
				printf("               ^ 返回值非 0，单次调用在此长度上失败，停止放大\n");

			free(pt);
			free(ct);
			free(ivs);
			free(stream);
			free(out);

			if (ret != SDR_OK)
				break;
		}
	}

	(void) p_DestroyKey;
	(void) p_CloseSession;
	(void) p_CloseDevice;
	printf("\n探针结束。\n");
	return 0;
}

/*-------------------------------------------------------------------------
 *
 * tecc_ec_elgamal.h
 *	  EC-ElGamal additive homomorphic encryption algorithm
 *
 * Implements EC-ElGamal on P-256 (or SM2) curve for additive homomorphism:
 *   - Encryption: E(m) = (C1, C2) = (r*G, m*G + r*Q)
 *   - Decryption: m = DLP(C2 - x*C1) via BSGS
 *   - Homomorphic operations: E(a+b) = E(a) + E(b), E(k*a) = k*E(a)
 *
 * CRT (Chinese Remainder Theorem) optimization splits a large integer into
 * multiple sub-values, each encrypted independently with a smaller BSGS
 * table, then recombined on decryption.
 *
 * Portions Copyright (c) 2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2021, HighGo Software Co.,Ltd.
 *
 * src/include/common/tecc_algorithm/tecc_ec_elgamal.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_EC_ELGAMAL_H
#define TECC_EC_ELGAMAL_H

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <stddef.h>
#include <stdint.h>

/* ==================== Curve Selection ==================== */

/* Default curve: NIST P-256. Can be switched to SM2 (NID_sm2) for GM/T compliance. */
#define TECC_EC_ELGAMAL_CURVE	NID_X9_62_prime256v1

/* Compressed point size on P-256 = 33 bytes */
#define TECC_EC_POINT_SIZE		33

/* ==================== CRT Parameters ==================== */

/*
 * 8 primes of 12 bits each, product ≈ 2^96 ≈ 7.9×10^28.
 * Shared by int2/int4/int8_enc for ciphertext interoperability.
 */
#define TECC_EC_CRT_NUM_PRIMES	8
#define TECC_EC_CRT_BITS		12

/* BSGS table size: 2^14 = 16384 entries (covers full 12-bit value range in 0 steps) */
#define TECC_EC_BSGS_TABLE_SIZE	16384

/* ReqID:TECC4.0 - BSGS giant-step 最大迭代次数（独立于 baby-step 表容量）。
 * 可解离散对数上限 = TECC_EC_BSGS_TABLE_SIZE × TECC_EC_BSGS_MAX_ITER
 *                 = 2^14 × 2^18 = 2^32 ≈ 42.9 亿。
 * 单独增大 max_iter（而非同步增大 capacity）可在不增加建表成本的前提下，
 * 覆盖同态加法聚合（SUM 等）结果的明文上限（单值解密仍由 CRT 分块保证，
 * giant step 仅需 <4 步，不受影响）。 */
#define TECC_EC_BSGS_MAX_ITER	262144

/* Max plaintext value for BSGS (8 × 12-bit primes → 96 bits) */
#define TECC_EC_MAX_BITS		96

/* Max parallel threads for CRT decrypt (sane default) */
#define TECC_EC_CRT_MAX_THREADS 16

/* ==================== Algorithm Constants ==================== */

/* Maximum CRTHE ciphertext size: num(1) + (C1 + C2) × primes = 1 + 66×8 = 529 */
#define TECC_EC_CRT_MAX_CIPHERTEXT_SIZE	(1 + TECC_EC_POINT_SIZE * 2 * TECC_EC_CRT_NUM_PRIMES)

/* Dynamic ciphertext size for n partitions: num(1) + (C1 + C2) × n */
#define TECC_EC_CRT_MAX_CIPHERTEXT_SIZE_EX(n)	(1 + TECC_EC_POINT_SIZE * 2 * (n))

/* Maximum plaintext BIGNUM byte size for encoding */
#define TECC_EC_MAX_PLAINTEXT_SIZE		16

/* ==================== Error Codes ==================== */

#define TECC_EC_ELGAMAL_SUCCESS			0
#define TECC_EC_ELGAMAL_ERR_NULL		-1
#define TECC_EC_ELGAMAL_ERR_MEMORY		-2
#define TECC_EC_ELGAMAL_ERR_CRYPTO		-3
#define TECC_EC_ELGAMAL_ERR_DECRYPT		-4
#define TECC_EC_ELGAMAL_ERR_INVALID		-5
#define TECC_EC_ELGAMAL_ERR_SIZE		-6
#define TECC_EC_ELGAMAL_ERR_CURVE		-7
#define TECC_EC_ELGAMAL_ERR_NOT_SUPPORTED -8

/* ==================== Type Definitions ==================== */

/* CRT parameters: N primes of bits bits each */
typedef struct TeccECCRTParams
{
	int			num_primes;			/* Number of CRT partitions */
	int			bits_per_prime;		/* Bits per prime */
	BIGNUM	  **primes;			/* Array of prime BIGNUMs */
	BIGNUM	   *modulus;			/* Product of all primes */
} TeccECCRTParams;

/* CRT ciphertext: array of (C1, C2) per partition */
typedef struct TeccECCRTCiphertext
{
	int			num_ciphertexts;	/* Number of ciphertext partitions */
	EC_POINT  **C1;				/* Array of C1 points */
	EC_POINT  **C2;				/* Array of C2 points */
} TeccECCRTCiphertext;

/* ==================== BSGS Table ==================== */

/* Single BSGS entry: hash index → discrete log value */
typedef struct TeccECBSGSEntry
{
	uint8_t		point_key[TECC_EC_POINT_SIZE];	/* Compressed EC point bytes */
	uint32_t	value;							/* Discrete log value */
} TeccECBSGSEntry;

/* BSGS table for baby-step-giant-step DLP solving */
typedef struct TeccECBSGSTable
{
	TeccECBSGSEntry *entries;	/* Array of entries (sorted by point_key) */
	uint32_t	size;			/* Current number of entries */
	uint32_t	capacity;		/* Maximum capacity */
	EC_POINT   *mG;				/* Giant step: size * G */
	EC_POINT   *mG_inv;			/* Giant step: -size * G */
	const EC_GROUP *group;		/* Reference to EC group (not owned) */
	/* ReqID:TECC4.0 - 1=entries 由运行时构建（free 时释放），0=指向静态表数据（不 free） */
	unsigned char entries_owned;
} TeccECBSGSTable;

/* ==================== Key Management ==================== */

/*
 * Generate an EC-ElGamal key pair.
 *
 * @param group EC group (created externally)
 * @param priv_key Output: private key x (BIGNUM)
 * @param pub_key Output: public key Q = x*G (EC_POINT)
 * @return 0 on success, negative on error
 */
int tecc_ec_elgamal_generate_keypair(const EC_GROUP *group,
									  BIGNUM **priv_key,
									  EC_POINT **pub_key);

/* ==================== CRT Parameter Management ==================== */

/*
 * Initialize CRT parameters with 4 primes of 21 bits each.
 * Creates the primes array and computes the modulus.
 */
int tecc_ec_crt_params_init(TeccECCRTParams *params);

/*
 * Initialize CRT parameters with configurable partition count.
 * Auto-generates num_primes distinct primes of ~(84/num_primes) bits each,
 * ensuring total modulus > 2^63 (safe for int8 range).
 *
 * num_primes: number of CRT partitions (4-16 recommended)
 */
int tecc_ec_crt_params_init_ex(TeccECCRTParams *params, int num_primes);

/*
 * Free CRT parameters.
 */
void tecc_ec_crt_params_free(TeccECCRTParams *params);

/*
 * Allocate CRT ciphertext structure.
 */
int tecc_ec_crt_ciphertext_init(TeccECCRTCiphertext *ct,
								const EC_GROUP *group,
								int num_splits);

/*
 * Free CRT ciphertext structure.
 */
void tecc_ec_crt_ciphertext_free(TeccECCRTCiphertext *ct,
								 const EC_GROUP *group);

/* ==================== Core Encryption / Decryption ==================== */

/*
 * EC-ElGamal encrypt a single integer value.
 * E(m) = (C1, C2) = (r*G, m*G + r*Q)
 *
 * @param group EC group
 * @param Q Public key (EC point)
 * @param m Plaintext value (BIGNUM, must be < curve order)
 * @param C1 Output: C1 = r*G
 * @param C2 Output: C2 = m*G + r*Q
 * @param ctx BN_CTX for temporary BIGNUMs (or NULL for auto-create)
 * @return 0 on success, negative on error
 */
int tecc_ec_elgamal_encrypt(const EC_GROUP *group, const EC_POINT *Q,
							const BIGNUM *m, EC_POINT *C1, EC_POINT *C2,
							BN_CTX *ctx);

/*
 * EC-ElGamal decrypt and solve DLP.
 * M = C2 - x*C1, then BSGS to find m.
 *
 * @param group EC group
 * @param x Private key
 * @param C1 First component of ciphertext
 * @param C2 Second component of ciphertext
 * @param m Output: decrypted plaintext (BIGNUM)
 * @param bsgs_table BSGS table (optional, NULL = brute force)
 * @param ctx BN_CTX (or NULL for auto-create)
 * @return 0 on success, negative on error
 */
int tecc_ec_elgamal_decrypt(const EC_GROUP *group, const BIGNUM *x,
							const EC_POINT *C1, const EC_POINT *C2,
							BIGNUM *m, TeccECBSGSTable *bsgs_table,
							BN_CTX *ctx);

/* ==================== Homomorphic Operations ==================== */

/*
 * EC-ElGamal homomorphic addition: E(a) + E(b) = E(a+b)
 * (C1, C2) = (C1_a + C1_b, C2_a + C2_b)
 */
int tecc_ec_elgamal_add(const EC_GROUP *group,
						EC_POINT *res_C1, EC_POINT *res_C2,
						const EC_POINT *a_C1, const EC_POINT *a_C2,
						const EC_POINT *b_C1, const EC_POINT *b_C2,
						BN_CTX *ctx);

/*
 * EC-ElGamal homomorphic subtraction: E(a) - E(b) = E(a-b)
 * (C1, C2) = (C1_a - C1_b, C2_a - C2_b)
 */
int tecc_ec_elgamal_sub(const EC_GROUP *group,
						EC_POINT *res_C1, EC_POINT *res_C2,
						const EC_POINT *a_C1, const EC_POINT *a_C2,
						const EC_POINT *b_C1, const EC_POINT *b_C2,
						BN_CTX *ctx);

/*
 * EC-ElGamal scalar multiplication: k * E(a) = E(k*a)
 * (C1, C2) = (k*C1, k*C2)
 *
 * k is a positive integer (BIGNUM).
 */
int tecc_ec_elgamal_scalar_mul(const EC_GROUP *group,
							   EC_POINT *res_C1, EC_POINT *res_C2,
							   const EC_POINT *C1, const EC_POINT *C2,
							   const BIGNUM *k, BN_CTX *ctx);

/* ==================== CRT Operations ==================== */

/*
 * CRT-mode EC-ElGamal encryption.
 * Splits plaintext across CRT partitions and encrypts each.
 *
 * @param ct Output: CRT ciphertext
 * @param group EC group
 * @param Q Public key
 * @param plaintext Plaintext as BIGNUM
 * @param params CRT parameters
 * @param ctx BN_CTX (or NULL)
 */
int tecc_ec_crt_encrypt(TeccECCRTCiphertext *ct,
						const EC_GROUP *group, const EC_POINT *Q,
						const BIGNUM *plaintext,
						const TeccECCRTParams *params,
						BN_CTX *ctx);

/*
 * CRT-mode EC-ElGamal decryption.
 * Decrypts each partition and recombines via CRT.
 */
int tecc_ec_crt_decrypt(BIGNUM *result,
						const EC_GROUP *group, const BIGNUM *x,
						const TeccECCRTCiphertext *ct,
						const TeccECCRTParams *params,
						TeccECBSGSTable **bsgs_tables,
						BN_CTX *ctx);

/*
 * CRT-mode EC-ElGamal parallel decryption.
 * Each thread handles a subset of CRT partitions independently,
 * then the main thread recombines via CRT.
 *
 * @param num_threads Number of worker threads (clamped to num_primes).
 *                    Use 0 or 1 for serial fallback.
 */
int tecc_ec_crt_decrypt_parallel(BIGNUM *result,
								const EC_GROUP *group, const BIGNUM *x,
								const TeccECCRTCiphertext *ct,
								const TeccECCRTParams *params,
								TeccECBSGSTable **bsgs_tables,
								int num_threads,
								BN_CTX *ctx);

/*
 * CRT-mode homomorphic addition.
 */
int tecc_ec_crt_add(const EC_GROUP *group,
					TeccECCRTCiphertext *res,
					const TeccECCRTCiphertext *a,
					const TeccECCRTCiphertext *b,
					BN_CTX *ctx);

/*
 * CRT-mode homomorphic subtraction.
 */
int tecc_ec_crt_sub(const EC_GROUP *group,
					TeccECCRTCiphertext *res,
					const TeccECCRTCiphertext *a,
					const TeccECCRTCiphertext *b,
					BN_CTX *ctx);

/*
 * CRT-mode scalar multiplication.
 */
int tecc_ec_crt_scalar_mul(const EC_GROUP *group,
						   TeccECCRTCiphertext *res,
						   const TeccECCRTCiphertext *a,
						   const BIGNUM *k,
						   BN_CTX *ctx);

/* ==================== Serialization ==================== */

/*
 * Serialize a CRT ciphertext to binary bytes.
 * Format: [num_splits:1] [C1(33B) C2(33B)] × num_splits
 *
 * @param buf Output buffer (must be at least TECC_EC_CRT_MAX_CIPHERTEXT_SIZE)
 * @param buf_size Size of output buffer
 * @param ct CRT ciphertext to serialize
 * @param group EC group
 * @return Number of bytes written, or negative on error
 */
int tecc_ec_crt_serialize(uint8_t *buf, int buf_size,
						  const TeccECCRTCiphertext *ct,
						  const EC_GROUP *group);

/* ReqID:TECC4.0 - Serialize with caller-provided BN_CTX (reuse across
 * aggregate rows; pass NULL to allocate internally). */
int tecc_ec_crt_serialize_ex(uint8_t *buf, int buf_size,
							 const TeccECCRTCiphertext *ct,
							 const EC_GROUP *group, BN_CTX *ctx);

/*
 * Deserialize binary bytes to a CRT ciphertext.
 *
 * @param ct Output: initialized CRT ciphertext
 * @param buf Input buffer with serialized data
 * @param buf_size Size of input buffer
 * @param group EC group
 * @return 0 on success, negative on error
 */
int tecc_ec_crt_deserialize(TeccECCRTCiphertext *ct,
							const uint8_t *buf, int buf_size,
							const EC_GROUP *group);

/* ReqID:TECC4.0 - Deserialize with caller-provided BN_CTX (reuse across
 * aggregate rows; pass NULL to allocate internally). */
int tecc_ec_crt_deserialize_ex(TeccECCRTCiphertext *ct,
							   const uint8_t *buf, int buf_size,
							   const EC_GROUP *group, BN_CTX *ctx);

/* ==================== BSGS Table Management ==================== */

/*
 * Initialize and build a BSGS table for DLP solving.
 * Table is built by computing baby steps: i*G for i=[0, size).
 *
 * @param table Output: initialized BSGS table
 * @param group EC group
 * @param size Number of baby steps (table capacity)
 * @return 0 on success, negative on error
 */
int tecc_ec_bsgs_table_init(TeccECBSGSTable *table,
							 const EC_GROUP *group,
							 uint32_t size);

/* ReqID:TECC4.0 - 强制运行时构建 BSGS 表（不查静态表数据）。
 * 生成程序（tecc_gen_bsgs_table）复用本函数生成静态表数据，保证
 * 表数据与核心算法逻辑始终一致；tecc_ec_bsgs_table_init 在参数
 * 指纹不匹配时也回退到本函数。 */
int tecc_ec_bsgs_table_build_runtime(TeccECBSGSTable *table,
									 const EC_GROUP *group,
									 uint32_t size);

/*
 * Free a BSGS table.
 */
void tecc_ec_bsgs_table_free(TeccECBSGSTable *table);

/* ReqID:TECC4.0 - 并行初始化 count 个 BSGS 表（每个素数一个表）。
 * 各表完全独立，多线程并行建表，缩短一次性建表开销（控制 HE 客户端冷启动）。 */
int tecc_ec_bsgs_tables_init_parallel(TeccECBSGSTable **tables,
									  const EC_GROUP *group,
									  uint32_t size,
									  int count);

/*
 * Solve discrete log using BSGS.
 * Given point M, find m such that M = m*G.
 *
 * @param result Output: discrete log value
 * @param table BSGS table (must be initialized with init)
 * @param group EC group
 * @param M Point to solve
 * @param max_iter Maximum giant steps
 * @param ctx BN_CTX (or NULL)
 * @return 0 on success, negative if not found
 */
int tecc_ec_bsgs_solve(uint64_t *result, TeccECBSGSTable *table,
					   const EC_GROUP *group, const EC_POINT *M,
					   uint64_t max_iter, BN_CTX *ctx, int *is_negative);

/* ==================== Negative Value Handling ==================== */

/*
 * Given a decrypted value m (0 <= m < modulus), check if the intended
 * integer is negative by the convention: if m > modulus/2, return m - modulus.
 * Otherwise return m.
 *
 * @param m Input/output: decrypted value, adjusted for sign
 * @param modulus CRT modulus
 */
void tecc_ec_adjust_negative(BIGNUM *m, const BIGNUM *modulus);

/* ==================== Utility Functions ==================== */

/*
 * Create a new EC group for the default curve (P-256 or SM2).
 * Caller must free with EC_GROUP_free().
 */
EC_GROUP *tecc_ec_elgamal_create_group(void);

/*
 * Get the compressed point size for the default group.
 */
int tecc_ec_elgamal_get_point_size(void);

/* ReqID:TECC4.0 - 算法内部并行（he-add 8 分块 / he-mul 2 模幂）开关：
 * tecc_he_algo_parallel_check() 返回是否启用（环境变量 TECC_HE_ALGO_PARALLEL，
 * 默认启用；可被 tecc_he_set_algo_parallel_override 线程局部覆盖）。
 * 字段级并行 worker 内调用 tecc_he_set_algo_parallel_override(0) 关闭算法内部
 * 并行，避免 N worker × M 内部线程超订阅；处理完恢复 -1。 */
int tecc_he_algo_parallel_check(void);
void tecc_he_set_algo_parallel_override(int enabled);

#endif /* TECC_EC_ELGAMAL_H */

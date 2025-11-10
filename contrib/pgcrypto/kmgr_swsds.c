/*-------------------------------------------------------------------------
 *
 * kmgr_swsds.c
 *	  kmgr interfaces for HSM swsds.
 *
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/pgcrypto/kmgr_swsds.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "pgcrypto.h"
#include "common/kmgr_utils.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "utils/builtins.h"
#include "funcapi.h"

#ifdef USE_SWSDS
#include <swsds.h>
#endif

#ifdef USE_SWSDS
static HTAB        *HSM_session_keys_hash = NULL;
static SGD_HANDLE   hSessionHandle = NULL;
static unsigned int SessionKeyId = 1;

#define NUM_HSM_SESSION_KEYS_HASH   128

#define NUM_HSM_KEY_MODE    3
const char *hsm_key_name_mode[NUM_HSM_KEY_MODE] = {
    "all", "enc", "sign"
};
#define HSM_KEY_MODE_SUPPORT_ENC(id) (id == 0 || id == 1)
#define HSM_KEY_MODE_SUPPORT_SIGN(id) (id == 0 || id == 2)

typedef struct hsm_session_keys_entry {
	unsigned int        id;
    unsigned int        kAlgId;
    unsigned int        keymode;
    union {
        struct {
            CryptoKey           cryptoKey;
            unsigned int        kekid;
	        SGD_HANDLE          hKeyHandle;
        } rkey;
        struct {
            RSArefPublicKey     stPublicKey;
            RSArefPrivateKey    stPrivateKey;
        } rsaKeypair;
        struct {
            ECCrefPublicKey     pucPublicKey;
            ECCrefPrivateKey    pucPrivateKey;
        } sm2Keypair;
    } keyCxt;
} hsm_session_keys_entry;
#endif

PG_FUNCTION_INFO_V1(hsm_generate_key);
PG_FUNCTION_INFO_V1(hsm_import_key);
PG_FUNCTION_INFO_V1(hsm_generate_key_pair);
PG_FUNCTION_INFO_V1(hsm_import_key_pair);
PG_FUNCTION_INFO_V1(hsm_destroy_key);
PG_FUNCTION_INFO_V1(hsm_show_keys);
PG_FUNCTION_INFO_V1(hsm_encrypt);
PG_FUNCTION_INFO_V1(hsm_decrypt);
PG_FUNCTION_INFO_V1(hsm_gen_random_bytes);
PG_FUNCTION_INFO_V1(hsm_hash);
PG_FUNCTION_INFO_V1(hsm_pub_encrypt);
PG_FUNCTION_INFO_V1(hsm_pub_decrypt);
PG_FUNCTION_INFO_V1(hsm_sign);
PG_FUNCTION_INFO_V1(hsm_verify);

#ifdef USE_SWSDS
static inline unsigned int
HSM_key_get_mode_by_name(const char *name)
{
    unsigned int i;
    for (i = 0; i < NUM_HSM_KEY_MODE; i++)
    {
        if (strcasecmp(hsm_key_name_mode[i], name) == 0)
            return i;
    }

    elog(ERROR, "key mode only support: \"all\", \"enc\", \"sign\"");
    return 0;
}

/*
 * Create hash table for HSM session keys.
 */
static void
HSM_init_session_keys_hash(void)
{
	HASHCTL		hashCtl;

	hashCtl.keysize = sizeof(unsigned int);
	hashCtl.entrysize = sizeof(hsm_session_keys_entry);
	hashCtl.hcxt = TopMemoryContext;

	HSM_session_keys_hash = hash_create("HSM session keys", NUM_HSM_SESSION_KEYS_HASH, &hashCtl,
                                        HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
}

#endif

/* SQL function: hsm_generate_key(algname cstring, lenbit int, kekid int) returns rkey bytea */
Datum
hsm_generate_key(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char  *algName = PG_GETARG_CSTRING(0);
    unsigned int lenBit = PG_GETARG_INT32(1);
    unsigned int kek_id = PG_GETARG_INT32(2);
    CryptoKey    key;
    bytea       *res;

    if (lenBit > KMGR_MAX_KEY_LEN)
        elog(ERROR, "key length cannot exceed 256 bits");
	
    key.kAlgId = HSM_AlgName2Id(algName);
    if (!HSM_IS_SYM_ALG(key.kAlgId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    hsm_generate_crypto_key(kek_id, lenBit, &key);

    res = palloc(VARHDRSZ + key.klen);
	SET_VARSIZE(res, VARHDRSZ + key.klen);
    memcpy(VARDATA(res), key.key, key.klen);

    PG_RETURN_BYTEA_P(res);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/* SQL function: hsm_import_key(algname cstring, kekid int, rkey bytea) returns id int */
Datum
hsm_import_key(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    unsigned int    kek_id = PG_GETARG_INT32(1);
    bytea          *keyBytea = PG_GETARG_BYTEA_P(2);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    id = 0;
    bool            found = false;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_SYM_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    /* Create session keys hash table */
    if (HSM_session_keys_hash == NULL)
    {
        HSM_init_session_keys_hash();
    }

    /* Find a free slot in hash table */
    for (int i = 0; i < NUM_HSM_SESSION_KEYS_HASH; i++)
    {
        id = SessionKeyId++;
        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &id, HASH_FIND, &found);
        if (!found)
            break;
    }

    /* Insert a new entry in hash table */
    if (!found)
    {
        hash_entry = hash_search(HSM_session_keys_hash, &id, HASH_ENTER, &found);
        hash_entry->kAlgId = algId;
        hash_entry->keymode = 0;
        hash_entry->keyCxt.rkey.kekid = kek_id;
        hash_entry->keyCxt.rkey.cryptoKey.kAlgId = algId;
        hash_entry->keyCxt.rkey.cryptoKey.klen = VARSIZE_ANY_EXHDR(keyBytea);
        memcpy(hash_entry->keyCxt.rkey.cryptoKey.key, VARDATA_ANY(keyBytea), 
               hash_entry->keyCxt.rkey.cryptoKey.klen);
        hash_entry->keyCxt.rkey.hKeyHandle = NULL;
	    HSM_GetKeyHandle(&hSessionHandle, &hash_entry->keyCxt.rkey.hKeyHandle, 
                         &hash_entry->keyCxt.rkey.cryptoKey, hash_entry->keyCxt.rkey.kekid, true);
    }
    else
    {
        elog(ERROR, "The hash table of HSM session keys is full");
    }

    PG_RETURN_INT32(hash_entry->id);
#else
	swsds_failure();
    PG_RETURN_INT32(0);
#endif
}

/* SQL function: hsm_generate_key_pair(algname cstring, lenbit int) returns keypair tuple */
Datum
hsm_generate_key_pair(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    FuncCallContext *funcctx;
    int             call_cntr;
    const char     *algName = PG_GETARG_CSTRING(0);
    unsigned int    lenBit = PG_GETARG_INT32(1);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    pukLen, prkLen;
    void           *pubKey, *priKey;
    bytea          *keyPair[2];

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (SRF_IS_FIRSTCALL())
    {
        funcctx = SRF_FIRSTCALL_INIT();
        funcctx->max_calls = 2;
    }

    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;

    if (call_cntr >= 2)
        SRF_RETURN_DONE(funcctx);

    /* Alloc memory for key pair */
    if (algId == SGD_RSA)
    {
        if (lenBit < 1024 || lenBit > 4096)
            elog(ERROR, "The key length of rsa support 1024 ~ 4096 bits");

        pubKey = palloc(sizeof(RSArefPublicKey));
        priKey = palloc(sizeof(RSArefPrivateKey));
        pukLen = sizeof(RSArefPublicKey);
        prkLen = sizeof(RSArefPrivateKey);
    }
    else if (algId == SGD_SM2 || algId == SGD_SM2_1 || algId == SGD_SM2_3)
    {
        if (lenBit != 256)
            elog(ERROR, "The key length of sm2 only support 256 bits");

        pubKey = palloc(sizeof(ECCrefPublicKey));
        priKey = palloc(sizeof(ECCrefPrivateKey));
        pukLen = sizeof(ECCrefPublicKey);
        prkLen = sizeof(ECCrefPrivateKey);
    }

    /* Generate key pair */
    HSM_Generate_Key_Pair(&hSessionHandle, lenBit, algId, pubKey, priKey);

    /* Reset key length */
    if (algId == SGD_RSA)
    {
        if (((RSArefPrivateKey*)priKey)->bits > 2048)
        {
            pukLen = sizeof(RSArefPublicKeyEx);
            prkLen = sizeof(RSArefPrivateKeyEx);
        }
        else
        {
            pukLen = sizeof(RSArefPublicKeyLite);
			prkLen = sizeof(RSArefPrivateKeyLite);
        }
    }

    /* Construct result */
    keyPair[0] = palloc(VARHDRSZ + pukLen);
    SET_VARSIZE(keyPair[0], VARHDRSZ + pukLen);
    keyPair[1] = palloc(VARHDRSZ + prkLen);
    SET_VARSIZE(keyPair[1], VARHDRSZ + prkLen);
    memcpy(VARDATA(keyPair[0]), pubKey, pukLen);
    memcpy(VARDATA(keyPair[1]), priKey, prkLen);

    /* Return result */
    SRF_RETURN_NEXT(funcctx, PointerGetDatum(keyPair[call_cntr]));
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/* 
 * SQL function: hsm_import_key_pair(algname cstring, pubkey bytea, prikey bytea, 
 *                                   mode cstring default 'all') returns id int 
 */
Datum
hsm_import_key_pair(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    bytea          *pukBytea = PG_GETARG_BYTEA_P(1);
    bytea          *prkBytea = PG_GETARG_BYTEA_P(2);
    const char     *keymode = PG_GETARG_CSTRING(3);
    unsigned int    pukLen = VARSIZE_ANY_EXHDR(pukBytea);
    unsigned int    prkLen = VARSIZE_ANY_EXHDR(prkBytea);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    id = 0;
    bool            found = false;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    /* Create session keys hash table */
    if (HSM_session_keys_hash == NULL)
    {
        HSM_init_session_keys_hash();
    }

    /* Find a free slot in hash table */
    for (int i = 0; i < NUM_HSM_SESSION_KEYS_HASH; i++)
    {
        id = SessionKeyId++;
        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &id, HASH_FIND, &found);
        if (!found)
            break;
    }

    /* Insert a new entry in hash table */
    if (!found)
    {
        hash_entry = hash_search(HSM_session_keys_hash, &id, HASH_ENTER, &found);
        hash_entry->kAlgId = algId;
        hash_entry->keymode = HSM_key_get_mode_by_name(keymode);
        if (algId == SGD_RSA)
        {
            memcpy(&hash_entry->keyCxt.rsaKeypair.stPublicKey, VARDATA_ANY(pukBytea), pukLen);
            memcpy(&hash_entry->keyCxt.rsaKeypair.stPrivateKey, VARDATA_ANY(prkBytea), prkLen);
        }
        else if (algId == SGD_SM2 || algId == SGD_SM2_1 || algId == SGD_SM2_3)
        {
            memcpy(&hash_entry->keyCxt.sm2Keypair.pucPublicKey, VARDATA_ANY(pukBytea), pukLen);
            memcpy(&hash_entry->keyCxt.sm2Keypair.pucPrivateKey, VARDATA_ANY(prkBytea), prkLen);
        }
    }
    else
    {
        elog(ERROR, "The hash table of HSM session keys is full");
    }

    PG_RETURN_INT32(hash_entry->id);
#else
	swsds_failure();
    PG_RETURN_INT32(0);
#endif
}

/* SQL function: hsm_destroy_key(id int) returns void */
Datum
hsm_destroy_key(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    unsigned int id = PG_GETARG_INT32(0);
    hsm_session_keys_entry *hash_entry;
    bool found = false;

    if (HSM_session_keys_hash == NULL)
        elog(ERROR, "could not find key by id %u", id);

    /* Search in hash table */
    hash_entry = hash_search(HSM_session_keys_hash, &id, HASH_FIND, &found);
    if (!found)
        elog(ERROR, "could not find key by id %u", id);

    if (HSM_IS_SYM_ALG(hash_entry->kAlgId))
        HSM_DestroyKey(&hSessionHandle, hash_entry->keyCxt.rkey.hKeyHandle);

    /* overwrite the key */
    memset(&hash_entry->keyCxt, 0, sizeof(hash_entry->keyCxt));
    hash_search(HSM_session_keys_hash, &hash_entry->id, HASH_REMOVE, NULL);

#else
	swsds_failure();
#endif
    PG_RETURN_VOID();
}

/* SQL function: hsm_show_keys(void) returns record */
#define NUM_HSM_SHOW_KEYS_ATTS 5
Datum
hsm_show_keys(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    FuncCallContext *funcctx;
    int call_cntr;
    int max_calls;
    hsm_session_keys_entry *hash_entry;
    HASH_SEQ_STATUS *status = NULL;

    /* Init at first call */
    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;

        funcctx = SRF_FIRSTCALL_INIT();

        if (HSM_session_keys_hash == NULL)
        {
            funcctx->max_calls = 0;
        }
        else
        {
            oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
            status = palloc(sizeof(HASH_SEQ_STATUS));
            hash_seq_init(status, HSM_session_keys_hash);
            funcctx->max_calls = hash_get_num_entries(HSM_session_keys_hash);
            funcctx->user_fctx = status;
            MemoryContextSwitchTo(oldcontext);
        }
    }

    /* Get func context */
    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;
    status = (HASH_SEQ_STATUS *)funcctx->user_fctx;

    /* Traverse the hash table */
    if (call_cntr < max_calls)
    {
        /* Get the next entry */
        hash_entry = (hsm_session_keys_entry *)hash_seq_search(status);
        if (hash_entry)
        {
            unsigned int    kekid = 0;
            unsigned int    keylen = 0;
            Datum result[NUM_HSM_SHOW_KEYS_ATTS];
            bool  nulls[NUM_HSM_SHOW_KEYS_ATTS];
            HeapTuple tuple;
            TupleDesc tupdesc;

            /* Get kek id */
            if (HSM_IS_SYM_ALG(hash_entry->kAlgId))
                kekid = Int32GetDatum(hash_entry->keyCxt.rkey.kekid);
            else
                kekid= 0;

            /* Get key length */
            if (HSM_IS_SYM_ALG(hash_entry->kAlgId))
                keylen = Int32GetDatum((hash_entry->keyCxt.rkey.cryptoKey.klen - KMGR_PADDING_KEY_BYTES) * 8);
            else if (HSM_IS_PUB_ALG(hash_entry->kAlgId))
            {
                if (hash_entry->kAlgId == SGD_RSA)
                    keylen = hash_entry->keyCxt.rsaKeypair.stPublicKey.bits;
                else if (hash_entry->kAlgId == SGD_SM2 || hash_entry->kAlgId == SGD_SM2_1 || hash_entry->kAlgId == SGD_SM2_3)
                    keylen = hash_entry->keyCxt.sm2Keypair.pucPublicKey.bits;
            }

            memset(result, 0, NUM_HSM_SHOW_KEYS_ATTS * sizeof(Datum));
            memset(nulls, false, NUM_HSM_SHOW_KEYS_ATTS);

            result[0] = Int32GetDatum(hash_entry->id);
            result[1] = CStringGetTextDatum(HSM_AlgId2Name(hash_entry->kAlgId));
            result[2] = CStringGetTextDatum(hsm_key_name_mode[hash_entry->keymode]);
            result[3] = kekid;
            result[4] = keylen;

            if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
                elog(ERROR, "return type must be a row type");
            
            tupdesc = CreateTupleDescCopy(tupdesc);
            tuple = heap_form_tuple(tupdesc, result, nulls);

            /* return a record */
            SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
        }
    }

    if (status != NULL)
    {
        hash_seq_term(status);
        pfree(status);
    }

    SRF_RETURN_DONE(funcctx);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/*
 * SQL function: hsm_encrypt(algname cstring, id int, plaintext bytea, iv bytea default '') 
 * returns ciphertext bytea
 */
Datum
hsm_encrypt(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    unsigned int    keyid = PG_GETARG_INT32(1);
    bytea          *plainbytea = PG_GETARG_BYTEA_P(2);
    bytea          *ivbytea = PG_GETARG_BYTEA_P(3);
    unsigned int    algId = HSM_AlgName2Id(algName);
    bool            found = false;
    unsigned char  *plaintext = (unsigned char *)VARDATA_ANY(plainbytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(plainbytea);
    unsigned char  *iv;
    bytea          *cipherbytea;
    unsigned int    out_len;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_SYM_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (HSM_session_keys_hash == NULL)
        elog(ERROR, "could not find key by id %u", keyid);

    /* Search in hash table */
    hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
    if (!found)
        elog(ERROR, "could not find key by id %u", keyid);

    if (!HSM_KEY_MODE_SUPPORT_ENC(hash_entry->keymode))
        elog(ERROR, "could not encrypt with sign key");

    /* Construct IV */
    if (VARSIZE_ANY_EXHDR(ivbytea) > 0)
    {
        bytea *ivbytea = PG_GETARG_BYTEA_P(3);
        unsigned int iv_len = VARSIZE_ANY_EXHDR(ivbytea);

        if (iv_len < (hash_entry->keyCxt.rkey.cryptoKey.klen - KMGR_PADDING_KEY_BYTES))
            elog(ERROR, "iv could not shorter than key length");

        iv = (unsigned char *)VARDATA_ANY(ivbytea);
    }
    else
    {
        iv = palloc0((hash_entry->keyCxt.rkey.cryptoKey.klen - KMGR_PADDING_KEY_BYTES));
    }

    cipherbytea = palloc(VARHDRSZ + in_len);
	SET_VARSIZE(cipherbytea, VARHDRSZ + in_len);

    /* Do encrypt */
	HSM_Encrypt(&hSessionHandle, &hash_entry->keyCxt.rkey.hKeyHandle,
                &hash_entry->keyCxt.rkey.cryptoKey, hash_entry->keyCxt.rkey.kekid,
                algId, iv, plaintext, in_len, (unsigned char *)VARDATA(cipherbytea), &out_len);

    Assert(in_len == out_len);

    PG_RETURN_BYTEA_P(cipherbytea);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}


/* 
 * SQL function: hsm_decrypt(algname cstring, id int, ciphertext bytea, iv bytea default '')
 * returns plaintext bytea 
 */
Datum
hsm_decrypt(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char    *algName = PG_GETARG_CSTRING(0);
    unsigned int   keyid = PG_GETARG_INT32(1);
    bytea          *cipherbytea = PG_GETARG_BYTEA_P(2);
    bytea          *ivbytea = PG_GETARG_BYTEA_P(3);
    unsigned int    algId = HSM_AlgName2Id(algName);
    bool            found = false;
    unsigned char  *ciphertext = (unsigned char *)VARDATA_ANY(cipherbytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(cipherbytea);
    unsigned char  *iv;
    bytea          *plainbytea;
    unsigned int    out_len;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_SYM_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (HSM_session_keys_hash == NULL)
        elog(ERROR, "could not find key by id %u", keyid);

    /* Search in hash table */
    hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
    if (!found)
        elog(ERROR, "could not find key by id %u", keyid);

    if (!HSM_KEY_MODE_SUPPORT_ENC(hash_entry->keymode))
        elog(ERROR, "could not decrypt with sign key");

    /* Construct IV */
    if (VARSIZE_ANY_EXHDR(ivbytea) > 0)
    {
        bytea *ivbytea = PG_GETARG_BYTEA_P(3);
        unsigned int iv_len = VARSIZE_ANY_EXHDR(ivbytea);

        if (iv_len < (hash_entry->keyCxt.rkey.cryptoKey.klen - KMGR_PADDING_KEY_BYTES))
            elog(ERROR, "iv could not shorter than key length");

        iv = (unsigned char *)VARDATA_ANY(ivbytea);
    }
    else
    {
        iv = palloc0((hash_entry->keyCxt.rkey.cryptoKey.klen - KMGR_PADDING_KEY_BYTES));
    }

    plainbytea = palloc(VARHDRSZ + in_len);
	SET_VARSIZE(plainbytea, VARHDRSZ + in_len);

    /* Do decrypt */
	HSM_Decrypt(&hSessionHandle, &hash_entry->keyCxt.rkey.hKeyHandle,
                &hash_entry->keyCxt.rkey.cryptoKey, hash_entry->keyCxt.rkey.kekid,
                algId, iv, ciphertext, in_len,
                (unsigned char *)VARDATA(plainbytea), &out_len);

    Assert(in_len == out_len);

    PG_RETURN_BYTEA_P(plainbytea);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/* SQL function: hsm_gen_random_bytes(len int) returns randStr bytea */
Datum
hsm_gen_random_bytes(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    unsigned int len = PG_GETARG_INT32(0);
    bytea       *res;

    res = palloc(VARHDRSZ + len);
	SET_VARSIZE(res, VARHDRSZ + len);

    HSM_Generate_Random(&hSessionHandle, len, (unsigned char *)VARDATA(res));

    PG_RETURN_BYTEA_P(res);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/* SQL function: hsm_hash(algname cstring, datatext bytea) returns hashval bytea */
Datum
hsm_hash(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    bytea          *databytea = PG_GETARG_BYTEA_P(1);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned char  *datatext = (unsigned char *)VARDATA_ANY(databytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(databytea);
    unsigned char   hashval[64]; /* Maximum is SHA512, 64 bytes */
    unsigned int    hash_len;
    bytea          *res;

    if (!HSM_IS_HASH_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    HSM_Hash(&hSessionHandle, algId, datatext, in_len, (unsigned char *)&hashval, &hash_len);

    res = palloc(VARHDRSZ + hash_len);
	SET_VARSIZE(res, VARHDRSZ + hash_len);
    memcpy(VARDATA_ANY(res), hashval, hash_len);

    PG_RETURN_BYTEA_P(res);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/*
 * SQL function: hsm_pub_encrypt(algname cstring, id int, plaintext bytea, keylenbits int default 0)
 * returns ciphertext bytea
 */
Datum
hsm_pub_encrypt(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    int             keyid = PG_GETARG_INT32(1);
    bytea          *plainbytea = PG_GETARG_BYTEA_P(2);
    int             innerkeylenbits = PG_GETARG_INT32(3);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    innerkeylen = innerkeylenbits / 8;
    unsigned int    innerKeyId = 0;
    bool            found = false;
    unsigned char  *plaintext = (unsigned char *)VARDATA_ANY(plainbytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(plainbytea);
    void           *key = NULL;
    bytea          *cipherbytea;
    unsigned char  *ciphertext;
    unsigned int    out_len;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (keyid < 0)
    {
        innerKeyId = (unsigned int)-keyid;
    }

    if (innerKeyId == 0)
    {
        if (HSM_session_keys_hash == NULL)
            elog(ERROR, "could not find key by id %d", keyid);

        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
        if (!found)
            elog(ERROR, "could not find key by id %d", keyid);

        if (!HSM_KEY_MODE_SUPPORT_ENC(hash_entry->keymode))
            elog(ERROR, "could not encrypt with sign key");

        if (algId == SGD_RSA)
        {
            unsigned int key_len = hash_entry->keyCxt.rsaKeypair.stPublicKey.bits / 8;

            key = (void*)&hash_entry->keyCxt.rsaKeypair.stPublicKey;
            if (in_len != key_len)
                elog(ERROR, "the key len is %u bytes, but the plaintext len is %u", key_len, in_len);
            out_len = key_len;
        }   
        else if (algId == SGD_SM2 || algId == SGD_SM2_3)
        {
            algId = SGD_SM2_3;
            key = (void*)&hash_entry->keyCxt.sm2Keypair.pucPublicKey;
            if (in_len != 32)
                elog(ERROR, "sm2 only support encrypt 32 bytes len");
            out_len = 64;
        }
    }
    else
    {
        if (innerkeylenbits <= 0)
            elog(ERROR, "need to specify the keylen bits when use the inner key");

        if (algId == SGD_RSA)
        {
            if (in_len != innerkeylen)
                elog(ERROR, "the key len is %u bytes, but the plaintext len is %u", innerkeylen, in_len);
            out_len = innerkeylen;
        }
        else if (algId == SGD_SM2 || algId == SGD_SM2_3)
        {
            algId = SGD_SM2_3;
            if (in_len != 32)
                elog(ERROR, "sm2 only support encrypt 32 bytes len");
            out_len = 64;
        }            
    }

    ciphertext = palloc(out_len);

    /* Do encrypt */
	HSM_Pub_Encrypt(&hSessionHandle, algId, innerKeyId, key, plaintext, in_len, 
                    ciphertext, &out_len);

    cipherbytea = palloc(VARHDRSZ + out_len);
	SET_VARSIZE(cipherbytea, VARHDRSZ + out_len);
    memcpy((unsigned char *)VARDATA(cipherbytea), ciphertext, out_len);

    PG_RETURN_BYTEA_P(cipherbytea);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/*
 * SQL function: hsm_pub_decrypt(algname cstring, id int, ciphertext bytea, 
 *                               passwd cstring default '', keylenbits int default 0)
 * returns plaintext bytea
 */
Datum
hsm_pub_decrypt(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    int             keyid = PG_GETARG_INT32(1);
    bytea          *cipherbytea = PG_GETARG_BYTEA_P(2);
    const char     *passwd = PG_GETARG_CSTRING(3);
    int             innerkeylenbits = PG_GETARG_INT32(4);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    innerkeylen = innerkeylenbits / 8;
    unsigned int    innerKeyId = 0;
    bool            found = false;
    unsigned char  *ciphertext = (unsigned char *)VARDATA_ANY(cipherbytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(cipherbytea);
    void           *key = NULL;
    bytea          *plainbytea;
    unsigned char  *plaintext;
    unsigned int    out_len;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (keyid < 0)
    {
        innerKeyId = (unsigned int)-keyid;
        if (strlen(passwd) < 1)
            elog(ERROR, "use inner private key need passwd");
    }

    if (innerKeyId == 0)
    {
        if (HSM_session_keys_hash == NULL)
            elog(ERROR, "could not find key by id %d", keyid);

        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
        if (!found)
            elog(ERROR, "could not find key by id %d", keyid);

        if (!HSM_KEY_MODE_SUPPORT_ENC(hash_entry->keymode))
            elog(ERROR, "could not decrypt with sign key");

        if (algId == SGD_RSA)
        {
            unsigned int key_len = hash_entry->keyCxt.rsaKeypair.stPrivateKey.bits / 8;

            key = (void*)&hash_entry->keyCxt.rsaKeypair.stPrivateKey;
            if (in_len != key_len)
                elog(ERROR, "the key len is %u bytes, but the ciphertext len is %u", key_len, in_len);
            out_len = key_len;
        }   
        else if (algId == SGD_SM2 || algId == SGD_SM2_1)
        {
            key = (void*)&hash_entry->keyCxt.sm2Keypair.pucPrivateKey;
            if (in_len != 32)
                elog(ERROR, "sm2 only support encrypt 32 bytes len");
            out_len = 64;
        }
    }
    else
    {
        if (innerkeylenbits <= 0)
            elog(ERROR, "need to specify the keylen bits when use the inner key");

        if (algId == SGD_RSA)
        {
            if (in_len != innerkeylen)
                elog(ERROR, "the key len is %u bytes, but the ciphertext len is %u", innerkeylen, in_len);
            out_len = innerkeylen;
        }
        else if (algId == SGD_SM2 || algId == SGD_SM2_3)
        {
            algId = SGD_SM2_3;
            if (in_len != 32)
                elog(ERROR, "sm2 only support encrypt 32 bytes len");
            out_len = 64;
        }        
    }

    plaintext = palloc(out_len);

    /* Do decrypt */
	HSM_Pub_Decrypt(&hSessionHandle, algId, innerKeyId, key, ciphertext, in_len, 
                    plaintext, &out_len, (unsigned char *)passwd);

    plainbytea = palloc(VARHDRSZ + out_len);
	SET_VARSIZE(plainbytea, VARHDRSZ + out_len);
    memcpy((unsigned char *)VARDATA(plainbytea), plaintext, out_len);

    PG_RETURN_BYTEA_P(plainbytea);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}


/*
 * SQL function: hsm_sign(algname cstring, id int, datatext bytea, 
 *                        passwd cstring default '', keylenbits int default 0)
 * returns signature bytea
 */
Datum
hsm_sign(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    int             keyid = PG_GETARG_INT32(1);
    bytea          *databytea = PG_GETARG_BYTEA_P(2);
    const char     *passwd = PG_GETARG_CSTRING(3);
    int             innerkeylenbits = PG_GETARG_INT32(4);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    innerkeylen = innerkeylenbits / 8;
    unsigned int    innerKeyId = 0;
    bool            found = false;
    unsigned char  *datatext = (unsigned char *)VARDATA_ANY(databytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(databytea);
    void           *key = NULL;
    bytea          *signbytea;
    unsigned char  *signtext;
    unsigned int    out_len;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (keyid < 0)
    {
        innerKeyId = (unsigned int)-keyid;
        if (strlen(passwd) < 1)
            elog(ERROR, "use inner private key need passwd");
    }

    if (innerKeyId == 0)
    {
        if (HSM_session_keys_hash == NULL)
            elog(ERROR, "could not find key by id %d", keyid);

        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
        if (!found)
            elog(ERROR, "could not find key by id %d", keyid);

        if (!HSM_KEY_MODE_SUPPORT_SIGN(hash_entry->keymode))
            elog(ERROR, "could not sign with enc key");

        if (algId == SGD_RSA)
        {
            unsigned int key_len = hash_entry->keyCxt.rsaKeypair.stPrivateKey.bits / 8;
            key = (void*)&hash_entry->keyCxt.rsaKeypair.stPrivateKey;
            out_len = key_len;
        }   
        else if (algId == SGD_SM2 || algId == SGD_SM2_1)
        {
            key = (void*)&hash_entry->keyCxt.sm2Keypair.pucPrivateKey;
            out_len = 64;
        }
    }
    else
    {
        if (innerkeylenbits <= 0)
            elog(ERROR, "need to specify the keylen bits when use the inner key");

        if (algId == SGD_RSA)
        {
            out_len = innerkeylen;
        }
        else if (algId == SGD_SM2 || algId == SGD_SM2_1)
        {
            algId = SGD_SM2_1;
            out_len = 64;
        }        
    }

    signtext = palloc(out_len);

    /* Do sign */
	HSM_Sign(&hSessionHandle, algId, innerKeyId, key, innerkeylen,
             datatext, in_len, signtext, &out_len, (unsigned char *)passwd);

    signbytea = palloc(VARHDRSZ + out_len);
	SET_VARSIZE(signbytea, VARHDRSZ + out_len);
    memcpy((unsigned char *)VARDATA(signbytea), signtext, out_len);

    PG_RETURN_BYTEA_P(signbytea);
#else
	swsds_failure();
    PG_RETURN_NULL();
#endif
}

/*
 * SQL function: hsm_verify(algname cstring, id int, datatext bytea,
 *                          signtext bytea, keylenbits int default 0)
 * returns ciphertext bytea
 */
Datum
hsm_verify(PG_FUNCTION_ARGS)
{
#ifdef USE_SWSDS
    const char     *algName = PG_GETARG_CSTRING(0);
    int             keyid = PG_GETARG_INT32(1);
    bytea          *databytea = PG_GETARG_BYTEA_P(2);
    bytea          *signbytea = PG_GETARG_BYTEA_P(3);
    int             innerkeylenbits = PG_GETARG_INT32(4);
    unsigned int    algId = HSM_AlgName2Id(algName);
    unsigned int    innerkeylen = innerkeylenbits / 8;
    unsigned int    innerKeyId = 0;
    bool            found = false;
    unsigned char  *datatext = (unsigned char *)VARDATA_ANY(databytea);
	unsigned int    in_len = VARSIZE_ANY_EXHDR(databytea);
    unsigned char  *signtext = (unsigned char *)VARDATA_ANY(signbytea);
	unsigned int    sign_len = VARSIZE_ANY_EXHDR(signbytea);
    void           *key = NULL;
    hsm_session_keys_entry *hash_entry;

    if (!HSM_IS_PUB_ALG(algId))
        elog(ERROR, "invalid usage for algorithm: %s", algName);

    if (keyid < 0)
    {
        innerKeyId = (unsigned int)-keyid;
    }

    if (innerKeyId == 0)
    {
        if (HSM_session_keys_hash == NULL)
            elog(ERROR, "could not find key by id %d", keyid);

        /* Search in hash table */
        hash_entry = hash_search(HSM_session_keys_hash, &keyid, HASH_FIND, &found);
        if (!found)
            elog(ERROR, "could not find key by id %d", keyid);

        if (!HSM_KEY_MODE_SUPPORT_SIGN(hash_entry->keymode))
            elog(ERROR, "could not verify with enc key");

        if (algId == SGD_RSA)
        {
            key = (void*)&hash_entry->keyCxt.rsaKeypair.stPublicKey;
        }   
        else if (algId == SGD_SM2 || algId == SGD_SM2_3)
        {
            algId = SGD_SM2_3;
            key = (void*)&hash_entry->keyCxt.sm2Keypair.pucPublicKey;
        }
    }
    else
    {
        if (innerkeylenbits <= 0)
            elog(ERROR, "need to specify the keylen bits when use the inner key");

        if (algId == SGD_SM2 || algId == SGD_SM2_3)
        {
            algId = SGD_SM2_3;
        }            
    }

    /* Do verify */
	HSM_Verify(&hSessionHandle, algId, innerKeyId, key, innerkeylen,
               datatext, in_len, signtext, sign_len);

    PG_RETURN_VOID();
#else
	swsds_failure();
    PG_RETURN_VOID();
#endif
}

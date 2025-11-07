/*-------------------------------------------------------------------------
 *
 * test_libpgtde_demo.c
 *    A demo to test the libpgtde library.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/bin/pg_alterckey/test_libpgtde_demo.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */


#define FRONTEND 1

#include "postgres_fe.h"

#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "getopt_long.h"
#include "crypto/walenc.h"
#include "crypto/bufenc.h"
#include "common/base64.h"
#include "access/xlog_internal.h"
#include "common/logging.h"

static const char *progname;
static char *DataDir = NULL;

int
main(int argc, char *argv[])
{
    int	    c;
    char   *cluster_key_cmd = "echo 181210f8f9c779c26da1d9b2075bde0127302ee0e3fca38c9a83f5b1dd8e5d3b";
    unsigned char cluster_key[MAX_WRAPPED_KEY_LENGTH] = {0};
    const char *walkeyB64 = "jj6TRlDwqj9Ya5uOWi+l1ROHD3TidPdlvZg9Hg0INxAR7B7DQdJZL1UwtE6xJnlb3j5BYlJo0u11F2bl7d6JtQ==";
    const char *relKeyB64 = "LIwIZzQKPENU8ry56GNbmkErzk8Kwaql7xmCzmbsk+XUnYiFk2DsJZFK3O3fu+kdnkqF8/4Dy1hLkVr/PjuLDQ==";
    int     walkeyStrlen, relkeyStrlen;
    char   *walkeyStr, *relkeyStr;
    char   *walkeyFileName = "wal.wkey";
    char   *relkeyFileName = "rel.wkey";
    FILE   *walkeyFile, *relkeyFile;
    size_t  write_len;
    CryptoKey wal_data_key, rel_data_key;
    const char *testEncWalB64 = "T1RigFyI4+vra5esBoG+1nG0fHslmSBgXszJEapG7W9wgvANRsw/G27Qd8gbg3SX6wcecFcZlKKzI1Mz";
    const char *testDecWalB64 = "C2kAAAABACAAADIAAAAAAAAA4ML+AQAAAAAQCAAAcg4hIP8YAAAAAAAAAAAAY5aqIQMAACEDAAAgAwAA";
    const char *testEncRelB64 = "AAAAADjk1QEAAAAAKACwH/y+KzsYz8XRb9SYid0Vs5aVWv1dnioWggmOpogkv8iyh3nqwRDtX7NGMnsQ";
    const char *testDecRelB64 = "AAAAADjk1QEAAAAAKACwH/AfBCAAAAAAwJ8gAOCfIACwnyAA0J8gAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    int     walStrlen, walB64len;
    int     relStrlen, relB64len;
    char   *walStr, *walStrB64;
    char   *relStr, *relStrB64;
    char   *walFileName = "000000010000000000000002";
    int     tli, segno;

    static struct option long_options[] = {
		{"datadir", required_argument, NULL, 'D'},
		{NULL, 0, NULL, 0}
	};

    pg_logging_init(argv[0]);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_alterckey"));
	progname = get_progname(argv[0]);

	while ((c = getopt_long(argc, argv, "D:", long_options, NULL)) != -1)
	{
		switch (c)
		{
			case 'D':
				DataDir = optarg;
				break;
			default:
				break;
		}
	}

    if (DataDir == NULL)
    {
        fprintf(stderr, _("missing datadir.\n"));
		exit(1);
    }

    if (chdir(DataDir) < 0)
	{
        fprintf(stderr, _("could not change directory.\n"));
		exit(1);
	}

    /* ----------------------------------
     * test <kmgr_retrieve_cluster_key>.
     * Retrieve the cluster key
     * ----------------------------------
     */
    if (!kmgr_retrieve_cluster_key(cluster_key_cmd, cluster_key, NULL, -1))
    {
        fprintf(stderr, _("kmgr_retrieve_cluster_key failed.\n"));
		exit(1);
    }

    /* Create wal.wkey */
    walkeyStrlen = pg_b64_dec_len(strlen(walkeyB64));
	walkeyStr = palloc(walkeyStrlen);
    walkeyStrlen = pg_b64_decode(walkeyB64, strlen(walkeyB64), walkeyStr, walkeyStrlen);
    walkeyFile = fopen(walkeyFileName, "wb");
    if (walkeyFile == NULL)
    {
        fprintf(stderr, _("create wal key file failed.\n"));
        exit(1);
    }
    write_len = fwrite(walkeyStr, sizeof(char), walkeyStrlen, walkeyFile);
    if (write_len != walkeyStrlen)
    {
        fprintf(stderr, _("write wal key file failed.\n"));
        exit(1);
    }
    fclose(walkeyFile);

    /* ----------------------------------
     * test <kmgr_retrieve_data_key>.
     * Retrieve the wal encryt key
     * ----------------------------------
     */
    if (!kmgr_retrieve_data_key(cluster_key, walkeyFileName, &wal_data_key))
    {
        fprintf(stderr, _("kmgr_retrieve_data_key wal_data_key failed.\n"));
		exit(1);
    }

    /* Create rel.wkey */
    relkeyStrlen = pg_b64_dec_len(strlen(relKeyB64));
	relkeyStr = palloc(relkeyStrlen);
    relkeyStrlen = pg_b64_decode(relKeyB64, strlen(relKeyB64), relkeyStr, relkeyStrlen);
    relkeyFile = fopen(relkeyFileName, "wb");
    if (relkeyFile == NULL)
    {
        fprintf(stderr, _("create wal key file failed.\n"));
        exit(1);
    }
    write_len = fwrite(relkeyStr, sizeof(char), relkeyStrlen, relkeyFile);
    if (write_len != relkeyStrlen)
    {
        fprintf(stderr, _("write wal key file failed.\n"));
        exit(1);
    }
    fclose(relkeyFile);

    /* Retrieve the rel encryt key */
    if (!kmgr_retrieve_data_key(cluster_key, relkeyFileName, &rel_data_key))
    {
        fprintf(stderr, _("kmgr_retrieve_data_key rel_data_key failed.\n"));
		exit(1);
    }

    /* ----------------------------------
     * Must init encryption context.
     * ----------------------------------
     */
    WALEncrypted = true;
    PG_CIPHER_ENCRYPT_CTR_WAL = PG_CIPHER_AES_CTR;
    LocalXLogKey = &wal_data_key;
    InitializeXLogEncryption();

    /* ----------------------------------
     * test <XLogDecryptInplace>.
     * Decrypt the Xlog.
     * ----------------------------------
     */
    walStrlen = pg_b64_dec_len(strlen(testEncWalB64));
	walStr = palloc(walStrlen);
    walStrlen = pg_b64_decode(testEncWalB64, strlen(testEncWalB64), walStr, walStrlen);
    XLogFromFileName(walFileName, &tli, &segno, DEFAULT_XLOG_SEG_SIZE);
    XLogDecryptInplace(walStr, walStrlen, tli, segno, 30);
    walB64len = pg_b64_enc_len(walStrlen);
	walStrB64 = palloc(walB64len + 1);
    walB64len = pg_b64_encode(walStr, walStrlen, walStrB64, walB64len);
    if (strncmp(walStrB64, testDecWalB64, walB64len) != 0)
    {
        fprintf(stderr, _("XLogDecryptInplace failed.\n"));
		exit(1);
    }

    /* ----------------------------------
     * Must init encryption context.
     * ----------------------------------
     */
    FileEncryptionEnabled = true;
    PG_CIPHER_ENCRYPT_CTR_REL = PG_CIPHER_AES_CTR;
    LocalRelKey = &rel_data_key;
    InitializeBufferEncryption();

    /* ----------------------------------
     * test <DecryptPage>.
     * Decrypt the rel page.
     * ----------------------------------
     */
    relStrlen = pg_b64_dec_len(strlen(testEncRelB64));
	relStr = palloc(relStrlen);
    relStrlen = pg_b64_decode(testEncRelB64, strlen(testEncRelB64), relStr, relStrlen);
    DecryptPage((Page)relStr, true, 1, relStrlen);
    relB64len = pg_b64_enc_len(relStrlen);
	relStrB64 = palloc(relB64len + 1);
    relB64len = pg_b64_encode(relStr, relStrlen, relStrB64, relB64len);
    if (strncmp(relStrB64, testDecRelB64, relB64len) != 0)
    {
        fprintf(stderr, _("DecryptPage failed.\n"));
		exit(1);
    }

    CleanupXLogEncryption();
    CleanupBufferEncryption();

    printf( _("Passed all tests.\n"));
}
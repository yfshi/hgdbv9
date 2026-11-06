/* Simple test for HKDF-SM3 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>

/* Include HKDF header - need to declare the function */
extern int hkdf_sm3_derive(const unsigned char *salt, size_t salt_len,
                          const unsigned char *ikm, size_t ikm_len,
                          const unsigned char *info, size_t info_len,
                          unsigned char *okm, size_t okm_len);

int main()
{
    printf("Testing HKDF-SM3 function...\n");

    /* Test data */
    unsigned char ikm[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    unsigned char salt[] = "test-salt-1234567890";
    unsigned char info[] = "test-info-1234567890";
    unsigned char okm[32];

    printf("Testing hkdf_sm3_derive...\n");

    int result = hkdf_sm3_derive(salt, strlen((char*)salt),
                                ikm, sizeof(ikm),
                                info, strlen((char*)info),
                                okm, sizeof(okm));

    if (result == 0) {
        printf("HKDF-SM3 test PASSED\n");

        /* Print first few bytes of output */
        printf("Output (first 16 bytes): ");
        int i;
        for (i = 0; i < 16 && i < sizeof(okm); i++) {
            printf("%02X", okm[i]);
        }
        printf("\n");

        return 0;
    } else {
        printf("HKDF-SM3 test FAILED with error code %d\n", result);

        /* SM3 is now provided by internal implementation */
        printf("Using internal SM3 implementation (no OpenSSL dependency)\n");

        return 1;
    }
}
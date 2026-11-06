/* Begin - ReqID:TECC4.0 */
/*
 * Simple HMAC-SM3 test program using internal SM3 implementation
 */
#include <stdio.h>
#include <string.h>
#include "common/tecc_algorithm/tecc_sm3_internal.h"

int main()
{
    /* Test data */
    unsigned char key[32] = {0};
    unsigned char data[100] = {0};
    unsigned char mac1[32], mac2[32];
    int i;

    /* Simple test: all zeros */
    printf("Test 1: All zeros\n");
    memset(key, 0, sizeof(key));
    memset(data, 0, sizeof(data));

    if (tecc_sm3_hmac(key, sizeof(key), data, sizeof(data), mac1) != 1) {
        printf("HMAC calculation failed\n");
        return 1;
    }

    printf("HMAC (first 8 bytes): ");
    for (i = 0; i < 8; i++) {
        printf("%02X", mac1[i]);
    }
    printf("\n");

    /* Test 2: Same calculation again */
    if (tecc_sm3_hmac(key, sizeof(key), data, sizeof(data), mac2) != 1) {
        printf("Second HMAC calculation failed\n");
        return 1;
    }

    printf("Second HMAC (first 8 bytes): ");
    for (i = 0; i < 8; i++) {
        printf("%02X", mac2[i]);
    }
    printf("\n");

    /* Compare */
    if (memcmp(mac1, mac2, sizeof(mac1)) == 0) {
        printf("HMACs match - function is deterministic\n");
    } else {
        printf("ERROR: HMACs don't match!\n");
        return 1;
    }

    /* Test 3: Different data */
    printf("\nTest 2: Different data\n");
    data[0] = 0x01;

    if (tecc_sm3_hmac(key, sizeof(key), data, sizeof(data), mac2) != 1) {
        printf("HMAC calculation failed\n");
        return 1;
    }

    printf("HMAC with different data (first 8 bytes): ");
    for (i = 0; i < 8; i++) {
        printf("%02X", mac2[i]);
    }
    printf("\n");

    if (memcmp(mac1, mac2, sizeof(mac1)) != 0) {
        printf("HMACs are different - function is sensitive to input\n");
    } else {
        printf("ERROR: HMACs are the same despite different input!\n");
        return 1;
    }

    printf("\nAll tests passed\n");
    return 0;
}
/* End - ReqID:TECC4.0 */

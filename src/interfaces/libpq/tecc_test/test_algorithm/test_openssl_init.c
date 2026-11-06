#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

int main() {
    printf("Testing OpenSSL initialization...\n");
    
    /* Try OpenSSL 1.1.0+ style initialization */
    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
    printf("OpenSSL 1.1.0+ detected\n");
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS, NULL) != 1) {
        printf("OPENSSL_init_ssl failed\n");
        return 1;
    }
    #else
    printf("OpenSSL < 1.1.0 detected\n");
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    #endif
    
    printf("OpenSSL initialized successfully\n");
    
    /* Clean up */
    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
    /* In OpenSSL 1.1.0+, cleanup is automatic */
    #else
    EVP_cleanup();
    ERR_free_strings();
    #endif
    
    printf("Test completed\n");
    return 0;
}

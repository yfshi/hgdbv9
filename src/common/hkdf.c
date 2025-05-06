
#include "postgres.h"

#ifdef USE_GMSSL
#include "common/hmac.h"
#include "common/hkdf.h"

bool sm3_hkdf_expand(unsigned char *out,
					const unsigned char *id, int id_len,
					const unsigned char *mac, int mac_len,
					const unsigned char *ckey, int ckey_len,
					int counter)
{
	pg_hmac_ctx *hmac_ctx = NULL;
	int i = 0;

	const unsigned char key[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
								 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	const unsigned char salt[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
								0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

	if(counter <= 0)
		counter = 3;

	hmac_ctx = pg_hmac_create(PG_SM3);
	if(hmac_ctx == NULL)
		return false;

	for(i = 0; i < counter; i++) {
		pg_hmac_init(hmac_ctx, key, sizeof(key));
		pg_hmac_update(hmac_ctx, salt, sizeof(salt));
		if(id_len > 0)
			pg_hmac_update(hmac_ctx, id, id_len);
		if(mac_len > 0)
			pg_hmac_update(hmac_ctx, mac, mac_len);
		if(ckey_len > 0)
			pg_hmac_update(hmac_ctx, ckey, ckey_len);
		pg_hmac_final(hmac_ctx, out, 32);
	}

	return true;
}
#endif

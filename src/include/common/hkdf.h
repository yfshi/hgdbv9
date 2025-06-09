#ifndef PG_HKDF_H
#define PG_HKDF_H
bool hkdf_expand_key(unsigned char *out,
					const unsigned char *id, int id_len,
					const unsigned char *mac, int mac_len,
					const unsigned char *ckey, int ckey_len,
					int counter);
#endif
#include <errno.h>				/* Recommended by debian */
#include <limits.h>

#include "postgres.h"
#include "../charset/ctype.h"

#define UNSIGNED_FLAG 1
#define SIGNED_FLAG   0

/* Hooks */
int16 my_atoi_int2(const char *s);

extern uint64 my_strntoull10rnd_8bit(const char *str, size_t length, int unsigned_flag, 
									char **endptr, int *error);
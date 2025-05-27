#ifndef __MYS_STRTOD__
#define __MYS_STRTOD__

#include "postgres.h"

#include <float.h>

extern double my_strtod_8bit(char *str, size_t length, char **end, int *err);

#endif
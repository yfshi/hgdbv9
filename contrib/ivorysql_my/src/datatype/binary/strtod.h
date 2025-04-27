#include "postgres.h"

#include <float.h>

#include "access/detoast.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"
#include "utils/array.h"
#include "utils/elog.h"
#include "utils/fmgrprotos.h"


#if defined(WORDS_BIGENDIAN) || (defined(__FLOAT_WORD_ORDER) && \
                                 (__FLOAT_WORD_ORDER == __BIG_ENDIAN))
    #define word0(x) (x)->L[0]
    #define word1(x) (x)->L[1]
#else
    #define word0(x) (x)->L[1]
    #define word1(x) (x)->L[0]
#endif

#define dval(x)         (x)->d

#define Ten_pmax        22
#define Exp_msk1        0x100000
#define Exp_mask        0x7ff00000
#define Bndry_mask      0xfffff
#define Bndry_mask1     0xfffff
#define Log2P           1
#define Tiny1           1
#define LSB             1
#define Scale_Bit       0x10
#define n_bigtens       5
#define P               53
#define Exp_shift       20
#define Exp_shift1      20
#define Ebits           11
#define Frac_mask       0xfffff
#define Frac_mask1      0xfffff
#define Emin            (-1022)
#define Bias            1023
#define Exp_1           0x3ff00000
#define Exp_11          0x3ff00000
#define Big0            (Frac_mask1 | Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
#define Big1            0xffffffff
#define FFFFFFFF        0xffffffffUL
#define Kmax            15
#define SIZEOF_CHARP    8
#define Bcopy(x, y)     memcpy((char *)&x->sign, (char *)&y->sign, \
                                2 * sizeof(int) + y->wds * sizeof(uint32))
#define DTOA_BUFF_SIZE (460 * sizeof(void *))

#ifndef Flt_Rounds
#ifdef FLT_ROUNDS
    #define Flt_Rounds FLT_ROUNDS
#else
    #define Flt_Rounds 1
#endif
#endif /*Flt_Rounds*/

#define rounded_product(a, b)   (a *= b)
#define rounded_quotient(a, b)  (a /= b)

#define MY_ALIGN(A, L)          (((A) + (L) - 1) & ~((L) - 1))
#define ALIGN_SIZE(A)           MY_ALIGN((A), sizeof(double))

typedef struct Bigint
{
    union
    {
        uint32 *x;           /* points right after this Bigint object */
        struct Bigint *next; /* to maintain free lists */
    } p;
    int k;      /* 2^k = maxwds */
    int maxwds; /* maximum length in 32-bit words */
    int sign;   /* not zero if number is negative */
    int wds;    /* current length in 32-bit words */
} Bigint;

typedef struct Stack_alloc
{
    char *begin;
    char *free;
    char *end;
    /*
      Having list of free blocks lets us reduce maximum required amount
      of memory from ~4000 bytes to < 1680 (tested on x86).
    */
    Bigint *freelist[16];
} Stack_alloc;

double my_strtod_8bit(char *str, size_t length, char **end, int *err);
double my_strtod(const char *str, char **end, int *error);
double my_strtod_int(const char *s00, char **se, int *error, char *buf, size_t buf_size);

#include "strtod.h"

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

typedef union
{
    double d;
    uint32 L[2];
} U;

static const double tens[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22
};

static const double bigtens[] = {1e16, 1e32, 1e64, 1e128, 1e256};

static const double tinytens[] = {
    1e-16, 1e-32, 1e-64, 1e-128,
    9007199254740992. * 9007199254740992.e-256 /* = 2^106 * 1e-53 */
};

static double my_strtod_int(const char *s00, char **se, int *error, char *buf, size_t buf_size);

static Bigint *
Balloc(int k, Stack_alloc *alloc)
{
    Bigint *rv;
    if (k <= Kmax && alloc->freelist[k])
    {
        rv = alloc->freelist[k];
        alloc->freelist[k] = rv->p.next;
    }
    else
    {
        int x, len;

        x = 1 << k;
        len = MY_ALIGN(sizeof(Bigint) + x * sizeof(uint32), SIZEOF_CHARP);

        if (alloc->free + len <= alloc->end)
        {
            rv = (Bigint *)alloc->free;
            alloc->free += len;
        }
        else
            rv = (Bigint *)malloc(len);

        rv->k = k;
        rv->maxwds = x;
    }
    rv->sign = rv->wds = 0;
    rv->p.x = (uint32 *)(rv + 1);
    return rv;
}

static void 
Bfree(Bigint *v, Stack_alloc *alloc)
{
    char *gptr = (char *)v; /* generic pointer */
    if (gptr < alloc->begin || gptr >= alloc->end)
        free(gptr);
    else if (v->k <= Kmax)
    {
        /*
          Maintain free lists only for stack objects: this way we don't
          have to bother with freeing lists in the end of dtoa;
          heap should not be used normally anyway.
        */
        v->p.next = alloc->freelist[v->k];
        alloc->freelist[v->k] = v;
    }
}

static Bigint *
multadd(Bigint *b, int m, int a, Stack_alloc *alloc)
{
    int i, wds;
    uint32 *x;
    uint64 carry, y;
    Bigint *b1;

    wds = b->wds;
    x = b->p.x;
    i = 0;
    carry = a;

    do
    {
        y = *x * (uint64)m + carry;
        carry = y >> 32;
        *x++ = (uint32)(y & FFFFFFFF);
    } while (++i < wds);

    if (carry)
    {
        if (wds >= b->maxwds)
        {
            b1 = Balloc(b->k + 1, alloc);
            Bcopy(b1, b);
            Bfree(b, alloc);
            b = b1;
        }

        b->p.x[wds++] = (uint32)carry;
        b->wds = wds;
    }

    return b;
}

static Bigint *
s2b(const char *s, int nd0, int nd, uint32 y9, Stack_alloc *alloc)
{
    Bigint *b;
    int i, k;
    int32 x, y;

    x = (nd + 8) / 9;
    
    for (k = 0, y = 1; x > y; y <<= 1, k++) {};

    b = Balloc(k, alloc);
    b->p.x[0] = y9;
    b->wds = 1;

    i = 9;
    if (9 < nd0)
    {
        s += 9;
        do
            b = multadd(b, 10, *s++ - '0', alloc);
        while (++i < nd0);
        s++; /* skip '.' */
    }
    else
        s += 10;
    
        /* now do the fractional part */
    for (; i < nd; i++)
        b = multadd(b, 10, *s++ - '0', alloc);

    return b;
}

static int 
hi0bits(uint32 x)
{
    int k = 0;

    if (!(x & 0xffff0000))
    {
        k = 16;
        x <<= 16;
    }
    if (!(x & 0xff000000))
    {
        k += 8;
        x <<= 8;
    }
    if (!(x & 0xf0000000))
    {
        k += 4;
        x <<= 4;
    }
    if (!(x & 0xc0000000))
    {
        k += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000))
    {
        k++;
        if (!(x & 0x40000000))
            return 32;
    }
    return k;
}

static int 
lo0bits(uint32 *y)
{
    int k;
    uint32 x = *y;

    if (x & 7)
    {
        if (x & 1)
            return 0;
        if (x & 2)
        {
            *y = x >> 1;
            return 1;
        }
        *y = x >> 2;
        return 2;
    }

    k = 0;
    
    if (!(x & 0xffff))
    {
        k = 16;
        x >>= 16;
    }
    if (!(x & 0xff))
    {
        k += 8;
        x >>= 8;
    }
    if (!(x & 0xf))
    {
        k += 4;
        x >>= 4;
    }
    if (!(x & 0x3))
    {
        k += 2;
        x >>= 2;
    }
    if (!(x & 1))
    {
        k++;
        x >>= 1;
        if (!x)
            return 32;
    }

    *y = x;
    return k;
}

static Bigint *
i2b(int i, Stack_alloc *alloc)
{
    Bigint *b;

    b = Balloc(1, alloc);
    b->p.x[0] = i;
    b->wds = 1;
    return b;
}

static Bigint *
mult(Bigint *a, Bigint *b, Stack_alloc *alloc)
{
    Bigint *c;
    int k, wa, wb, wc;
    uint32 *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    uint32 y;
    uint64 carry, z;

    if (a->wds < b->wds)
    {
        c = a;
        a = b;
        b = c;
    }

    k = a->k;
    wa = a->wds;
    wb = b->wds;
    wc = wa + wb;
    
    if (wc > a->maxwds)
        k++;
    
    c = Balloc(k, alloc);
    
    for (x = c->p.x, xa = x + wc; x < xa; x++)
    {
        *x = 0;
    }

    xa = a->p.x;
    xae = xa + wa;
    xb = b->p.x;
    xbe = xb + wb;
    xc0 = c->p.x;

    for (; xb < xbe; xc0++)
    {
        if ((y = *xb++))
        {
            x = xa;
            xc = xc0;
            carry = 0;

            do
            {
                z = *x++ * (uint64)y + *xc + carry;
                carry = z >> 32;
                *xc++ = (uint32)(z & FFFFFFFF);
            } while (x < xae);

            *xc = (uint32)carry;
        }
    }

    for (xc0 = c->p.x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) {};

    c->wds = wc;
    return c;
}

static uint32 powers5[] = {
    625UL,
    390625UL,
    2264035265UL, 35UL,
    2242703233UL, 762134875UL, 1262UL,
    3211403009UL, 1849224548UL, 3668416493UL, 3913284084UL, 1593091UL,
    781532673UL, 64985353UL, 253049085UL, 594863151UL, 3553621484UL,
    3288652808UL, 3167596762UL, 2788392729UL, 3911132675UL, 590UL,
    2553183233UL, 3201533787UL, 3638140786UL, 303378311UL, 1809731782UL,
    3477761648UL, 3583367183UL, 649228654UL, 2915460784UL, 487929380UL,
    1011012442UL, 1677677582UL, 3428152256UL, 1710878487UL, 1438394610UL,
    2161952759UL, 4100910556UL, 1608314830UL, 349175UL
};

static Bigint p5_a[] = {
    /*  { x } - k - maxwds - sign - wds */
    {{powers5}, 1, 1, 0, 1},
    {{powers5 + 1}, 1, 1, 0, 1},
    {{powers5 + 2}, 1, 2, 0, 2},
    {{powers5 + 4}, 2, 3, 0, 3},
    {{powers5 + 7}, 3, 5, 0, 5},
    {{powers5 + 12}, 4, 10, 0, 10},
    {{powers5 + 22}, 5, 19, 0, 19}
};

#define P5A_MAX (sizeof(p5_a) / sizeof(*p5_a) - 1)

static Bigint *
pow5mult(Bigint *b, int k, Stack_alloc *alloc)
{
    Bigint *b1, *p5, *p51 = NULL;
    int i;
    static int p05[3] = {5, 25, 125};
    bool overflow = false;

    if ((i = k & 3))
        b = multadd(b, p05[i - 1], 0, alloc);

    if (!(k >>= 2))
        return b;
    
    p5 = p5_a;
    for (;;)
    {
        if (k & 1)
        {
            b1 = mult(b, p5, alloc);
            Bfree(b, alloc);
            b = b1;
        }

        if (!(k >>= 1))
            break;

        /* Calculate next power of 5 */
        if (overflow)
        {
            p51 = mult(p5, p5, alloc);
            Bfree(p5, alloc);
            p5 = p51;
        }
        else if (p5 < p5_a + P5A_MAX)
            ++p5;
        else if (p5 == p5_a + P5A_MAX)
        {
            p5 = mult(p5, p5, alloc);
            overflow = true;
        }
    }

    if (p51)
        Bfree(p51, alloc);
    
    return b;
}

static Bigint *
lshift(Bigint *b, int k, Stack_alloc *alloc)
{
    int i, k1, n, n1;
    Bigint *b1;
    uint32 *x, *x1, *xe, z;

    n = k >> 5;
    k1 = b->k;
    n1 = n + b->wds + 1;
    
    for (i = b->maxwds; n1 > i; i <<= 1)
    {
        k1++;
    }

    b1 = Balloc(k1, alloc);
    x1 = b1->p.x;

    for (i = 0; i < n; i++)
    {
        *x1++ = 0;
    }

    x = b->p.x;
    xe = x + b->wds;

    if (k &= 0x1f)
    {
        k1 = 32 - k;
        z = 0;

        do
        {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
        } while (x < xe);

        if ((*x1 = z))
            ++n1;
    }
    else
    {

        do
        {
            *x1++ = *x++;
        }
        while (x < xe);
    }

    b1->wds = n1 - 1;
    Bfree(b, alloc);
    return b1;
}

static int 
cmp(Bigint *a, Bigint *b)
{
    uint32 *xa, *xa0, *xb, *xb0;
    int i, j;

    i = a->wds;
    j = b->wds;
    if (i -= j)
        return i;
    xa0 = a->p.x;
    xa = xa0 + j;
    xb0 = b->p.x;
    xb = xb0 + j;
    for (;;)
    {
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
        if (xa <= xa0)
            break;
    }
    return 0;
}

static Bigint *
diff(Bigint *a, Bigint *b, Stack_alloc *alloc)
{
    Bigint *c;
    int i, wa, wb;
    uint32 *xa, *xae, *xb, *xbe, *xc;
    uint64 borrow, y;

    i = cmp(a, b);
    if (!i)
    {
        c = Balloc(0, alloc);
        c->wds = 1;
        c->p.x[0] = 0;
        return c;
    }
    if (i < 0)
    {
        c = a;
        a = b;
        b = c;
        i = 1;
    }
    else
        i = 0;

    c = Balloc(a->k, alloc);
    c->sign = i;
    wa = a->wds;
    xa = a->p.x;
    xae = xa + wa;
    wb = b->wds;
    xb = b->p.x;
    xbe = xb + wb;
    xc = c->p.x;
    borrow = 0;

    do
    {
        y = (uint64)*xa++ - *xb++ - borrow;
        borrow = y >> 32 & (uint32)1;
        *xc++ = (uint32)(y & FFFFFFFF);
    } while (xb < xbe);

    while (xa < xae)
    {
        y = *xa++ - borrow;
        borrow = y >> 32 & (uint32)1;
        *xc++ = (uint32)(y & FFFFFFFF);
    }

    while (!*--xc)
        wa--;

    c->wds = wa;
    return c;
}

static double 
ulp(U *x)
{
    int32 L;
    U u;

    L = (word0(x) & Exp_mask) - (P - 1) * Exp_msk1;
    word0(&u) = L;
    word1(&u) = 0;
    return dval(&u);
}

static double 
b2d(Bigint *a, int *e)
{
    uint32 *xa, *xa0, w, y, z;
    int k;
    U d;
#define d0 word0(&d)
#define d1 word1(&d)

    xa0 = a->p.x;
    xa = xa0 + a->wds;
    y = *--xa;
    k = hi0bits(y);
    *e = 32 - k;
    if (k < Ebits)
    {
        d0 = Exp_1 | y >> (Ebits - k);
        w = xa > xa0 ? *--xa : 0;
        d1 = y << ((32 - Ebits) + k) | w >> (Ebits - k);
        goto ret_d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits)
    {
        d0 = Exp_1 | y << k | z >> (32 - k);
        y = xa > xa0 ? *--xa : 0;
        d1 = z << k | y >> (32 - k);
    }
    else
    {
        d0 = Exp_1 | y;
        d1 = z;
    }
ret_d:
#undef d0
#undef d1
    return dval(&d);
}

static Bigint *
d2b(U *d, int *e, int *bits, Stack_alloc *alloc)
{
    Bigint *b;
    int de, k;
    uint32 *x, y, z;
    int i;
#define d0 word0(d)
#define d1 word1(d)

    b = Balloc(1, alloc);
    x = b->p.x;

    z = d0 & Frac_mask;
    d0 &= 0x7fffffff; /* clear sign bit, which we ignore */
    if ((de = (int)(d0 >> Exp_shift)))
        z |= Exp_msk1;
    if ((y = d1))
    {
        if ((k = lo0bits(&y)))
        {
            x[0] = y | z << (32 - k);
            z >>= k;
        }
        else
            x[0] = y;
        i = b->wds = (x[1] = z) ? 2 : 1;
    }
    else
    {
        k = lo0bits(&z);
        x[0] = z;
        i = b->wds = 1;
        k += 32;
    }
    if (de)
    {
        *e = de - Bias - (P - 1) + k;
        *bits = P - k;
    }
    else
    {
        *e = de - Bias - (P - 1) + 1 + k;
        *bits = 32 * i - hi0bits(x[i - 1]);
    }
    return b;
#undef d0
#undef d1
}

static double 
ratio(Bigint *a, Bigint *b)
{
    U da, db;
    int k, ka, kb;

    dval(&da) = b2d(a, &ka);
    dval(&db) = b2d(b, &kb);
    k = ka - kb + 32 * (a->wds - b->wds);
    if (k > 0)
        word0(&da) += k * Exp_msk1;
    else
    {
        k = -k;
        word0(&db) += k * Exp_msk1;
    }
    return dval(&da) / dval(&db);
}

/*
  strtod for IEEE--arithmetic machines.

  This strtod returns a nearest machine number to the input decimal
  string (or sets errno to EOVERFLOW). Ties are broken by the IEEE round-even
  rule.

  Inspired loosely by William D. Clinger's paper "How to Read Floating
  Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].

  Modifications:

   1. We only require IEEE (not IEEE double-extended).
   2. We get by with floating-point arithmetic in a case that
     Clinger missed -- when we're computing d * 10^n
     for a small integer d and the integer n is not too
     much larger than 22 (the maximum integer k for which
     we can represent 10^k exactly), we may be able to
     compute (d*10^k) * 10^(e-k) with just one roundoff.
   3. Rather than a bit-at-a-time adjustment of the binary
     result in the hard case, we use floating-point
     arithmetic to determine the adjustment to within
     one bit; only in really hard cases do we need to
     compute a second residual.
   4. Because of 3., we don't need a large table of powers of 10
     for ten-to-e (just some small tables, e.g. of 10^k
     for 0 <= k <= 22).
*/

static double 
my_strtod(const char *str, char **end, int *error)
{
    char buf[DTOA_BUFF_SIZE];
    double res;

    res = my_strtod_int(str, end, error, buf, sizeof(buf));
    return (*error == 0) ? res : (res < 0 ? -DBL_MAX : DBL_MAX);
}

double 
my_strtod_8bit(char *str, size_t length, char **end, int *err)
{
    if (length == 65532)
        length = 65535; /* Should be big enough */
    *end = str + length;
    return my_strtod(str, end, err);
}

static double 
my_strtod_int(const char *s00, char **se, int *error, char *buf, size_t buf_size)
{
    int scale;
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c = 0, dsign,
                                              e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
    const char *s, *s0, *s1, *end = *se;
    double aadj, aadj1;
    U aadj2, adj, rv, rv0;
    int32 L;
    uint32 y, z;
    Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;
#ifdef Honor_FLT_ROUNDS
    int rounding;
#endif
    Stack_alloc alloc;

    *error = 0;

    alloc.begin = alloc.free = buf;
    alloc.end = buf + buf_size;
    memset(alloc.freelist, 0, sizeof(alloc.freelist));

    sign = nz0 = nz = 0;
    dval(&rv) = 0.;
    for (s = s00; s < end; s++)
    {
        switch (*s)
        {
            case '-':
                sign = 1;
            // Fall through.
            case '+':
                s++;
                goto break2;
            case '\t':
            case '\n':
            case '\v':
            case '\f':
            case '\r':
            case ' ':
                continue;
            default:
                goto break2;
        }
    }

break2:
    if (s >= end)
        goto ret0;

    if (*s == '0')
    {
        nz0 = 1;
        while (++s < end && *s == '0')
            ;
        if (s >= end)
            goto ret;

    }
    s0 = s;
    y = z = 0;

    for (nd = nf = 0; s < end && (c = *s) >= '0' && c <= '9'; nd++, s++)
    {
        if (nd < 9)
            y = 10 * y + c - '0';
        else if (nd < 16)
            z = 10 * z + c - '0';
    }

    nd0 = nd;
    if (s < end && c == '.')
    {
        c = *++s;
        if (!nd)
        {
            for (; s < end; ++s)
            {
                c = *s;
                if (c != '0')
                    break;
                nz++;
            }
            if (s < end && c > '0' && c <= '9')
            {
                s0 = s;
                nf += nz;
                nz = 0;
            }
            else
                goto dig_done;
        }
        for (; s < end; ++s)
        {
            c = *s;
            if (c < '0' || c > '9')
                break;
            /*
              Here we are parsing the fractional part.
              We can stop counting digits after a while: the extra digits
              will not contribute to the actual result produced by s2b().
              We have to continue scanning, in case there is an exponent part.
             */
            if (nd < 2 * DBL_DIG)
            {
                nz++;
                if (c -= '0')
                {
                    nf += nz;
                    for (i = 1; i < nz; i++)
                    {
                        if (nd++ < 9)
                            y *= 10;
                        else if (nd <= DBL_DIG + 1)
                            z *= 10;
                    }

                    if (nd++ < 9)
                        y = 10 * y + c;
                    else if (nd <= DBL_DIG + 1)
                        z = 10 * z + c;
                    nz = 0;
                }
            }
        }
    }

dig_done:
    e = 0;
    if (s < end && (c == 'e' || c == 'E'))
    {
        if (!nd && !nz && !nz0)
            goto ret0;
        s00 = s;
        esign = 0;
        if (++s < end)
        {
            switch (c = *s)
            {
            case '-':
                esign = 1;
                // Fall through.
            case '+':
                if (++s < end)
                    c = *s;
            }
        }

        if (s < end && c >= '0' && c <= '9')
        {
            while (s < end && c == '0')
                c = *++s;
            if (s < end && c > '0' && c <= '9')
            {
                L = c - '0';
                s1 = s;
                while (++s < end && (c = *s) >= '0' && c <= '9')
                    L = 10 * L + c - '0';
                if (s - s1 > 8 || L > 19999)
                    /* Avoid confusion from exponents
                     * so large that e might overflow.
                     */
                    e = 19999; /* safe for 16 bit ints */
                else
                    e = (int)L;
                if (esign)
                    e = -e;
            }
            else
                e = 0;
        }
        else
            s = s00;
    }
    if (!nd)
    {
        if (!nz && !nz0)
        {
        ret0:
            s = s00;
            sign = 0;
        }
        goto ret;
    }
    e1 = e -= nf;

    /*
      Now we have nd0 digits, starting at s0, followed by a
      decimal point, followed by nd-nd0 digits.  The number we're
      after is the integer represented by those digits times
      10**e
     */

    if (!nd0)
        nd0 = nd;
    k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
    dval(&rv) = y;
    if (k > 9)
    {
        dval(&rv) = tens[k - 9] * dval(&rv) + z;
    }
    bd0 = 0;
    if (nd <= DBL_DIG
#ifndef Honor_FLT_ROUNDS
        && Flt_Rounds == 1
#endif
    )
    {
        if (!e)
            goto ret;
        if (e > 0)
        {
            if (e <= Ten_pmax)
            {
#ifdef Honor_FLT_ROUNDS
                /* round correctly FLT_ROUNDS = 2 or 3 */
                if (sign)
                {
                    rv.d = -rv.d;
                    sign = 0;
                }
#endif
                /* rv = */ rounded_product(dval(&rv), tens[e]);
                goto ret;
            }
            i = DBL_DIG - nd;
            if (e <= Ten_pmax + i)
            {
                /*
                  A fancier test would sometimes let us do
                  this for larger i values.
                */
#ifdef Honor_FLT_ROUNDS
                /* round correctly FLT_ROUNDS = 2 or 3 */
                if (sign)
                {
                    rv.d = -rv.d;
                    sign = 0;
                }
#endif
                e -= i;
                dval(&rv) *= tens[i];
                /* rv = */ rounded_product(dval(&rv), tens[e]);
                goto ret;
            }
        }
#ifndef Inaccurate_Divide
        else if (e >= -Ten_pmax)
        {
#ifdef Honor_FLT_ROUNDS
            /* round correctly FLT_ROUNDS = 2 or 3 */
            if (sign)
            {
                rv.d = -rv.d;
                sign = 0;
            }
#endif
            /* rv = */ rounded_quotient(dval(&rv), tens[-e]);
            goto ret;
        }
#endif
    }
    e1 += nd - k;

    scale = 0;
#ifdef Honor_FLT_ROUNDS
    if ((rounding = Flt_Rounds) >= 2)
    {
        if (sign)
            rounding = rounding == 2 ? 0 : 2;
        else if (rounding != 2)
            rounding = 0;
    }
#endif

    /* Get starting approximation = rv * 10**e1 */

    if (e1 > 0)
    {
        if ((i = e1 & 15))
            dval(&rv) *= tens[i];
        if (e1 &= ~15)
        {
            if (e1 > DBL_MAX_10_EXP)
            {
            ovfl:
                *error = EOVERFLOW;
                /* Can't trust HUGE_VAL */
#ifdef Honor_FLT_ROUNDS
                switch (rounding)
                {
                case 0: /* toward 0 */
                case 3: /* toward -infinity */
                    word0(&rv) = Big0;
                    word1(&rv) = Big1;
                    break;
                default:
                    word0(&rv) = Exp_mask;
                    word1(&rv) = 0;
                }
#else  /*Honor_FLT_ROUNDS*/
                word0(&rv) = Exp_mask;
                word1(&rv) = 0;
#endif /*Honor_FLT_ROUNDS*/
                if (bd0)
                    goto retfree;
                goto ret;
            }

            e1 >>= 4;
            for (j = 0; e1 > 1; j++, e1 >>= 1)
            {
                if (e1 & 1)
                    dval(&rv) *= bigtens[j];
            }

            /* The last multiplication could overflow. */
            word0(&rv) -= P * Exp_msk1;
            dval(&rv) *= bigtens[j];
            if ((z = word0(&rv) & Exp_mask) > Exp_msk1 * (DBL_MAX_EXP + Bias - P))
                goto ovfl;
            if (z > Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P))
            {
                /* set to largest number (Can't trust DBL_MAX) */
                word0(&rv) = Big0;
                word1(&rv) = Big1;
            }
            else
                word0(&rv) += P * Exp_msk1;
        }
    }
    else if (e1 < 0)
    {
        e1 = -e1;
        if ((i = e1 & 15))
            dval(&rv) /= tens[i];
        if ((e1 >>= 4))
        {
            if (e1 >= 1 << n_bigtens)
                goto undfl;
            if (e1 & Scale_Bit)
                scale = 2 * P;
            for (j = 0; e1 > 0; j++, e1 >>= 1)
            {
                if (e1 & 1)
                    dval(&rv) *= tinytens[j];
            }

            if (scale && (j = 2 * P + 1 - ((word0(&rv) & Exp_mask) >> Exp_shift)) > 0)
            {
                /* scaled rv is denormal; zap j low bits */
                if (j >= 32)
                {
                    word1(&rv) = 0;
                    if (j >= 53)
                        word0(&rv) = (P + 2) * Exp_msk1;
                    else
                        word0(&rv) &= 0xffffffff << (j - 32);
                }
                else
                    word1(&rv) &= 0xffffffff << j;
            }
            if (!dval(&rv))
            {
            undfl:
                dval(&rv) = 0.;
                if (bd0)
                    goto retfree;
                goto ret;
            }
        }
    }

    /* Now the hard part -- adjusting rv to the correct value.*/
    /* Put digits into bd: true value = bd * 10^e */

    bd0 = s2b(s0, nd0, nd, y, &alloc);

    for (;;)
    {
        bd = Balloc(bd0->k, &alloc);
        Bcopy(bd, bd0);
        bb = d2b(&rv, &bbe, &bbbits, &alloc); /* rv = bb * 2^bbe */
        bs = i2b(1, &alloc);

        if (e >= 0)
        {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        }
        else
        {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
#ifdef Honor_FLT_ROUNDS
        if (rounding != 1)
            bs2++;
#endif
        j = bbe - scale;
        i = j + bbbits - 1; /* logb(rv) */
        if (i < Emin)       /* denormal */
            j += P - Emin;
        else
            j = P + 1 - bbbits;
        bb2 += j;
        bd2 += j;
        bd2 += scale;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0)
        {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0)
        {
            bs = pow5mult(bs, bb5, &alloc);
            bb1 = mult(bs, bb, &alloc);
            Bfree(bb, &alloc);
            bb = bb1;
        }
        if (bb2 > 0)
            bb = lshift(bb, bb2, &alloc);
        if (bd5 > 0)
            bd = pow5mult(bd, bd5, &alloc);
        if (bd2 > 0)
            bd = lshift(bd, bd2, &alloc);
        if (bs2 > 0)
            bs = lshift(bs, bs2, &alloc);
        delta = diff(bb, bd, &alloc);
        dsign = delta->sign;
        delta->sign = 0;
        i = cmp(delta, bs);
#ifdef Honor_FLT_ROUNDS
        if (rounding != 1)
        {
            if (i < 0)
            {
                /* Error is less than an ulp */
                if (!delta->p.x[0] && delta->wds <= 1)
                {
                    /* exact */
                    break;
                }
                if (rounding)
                {
                    if (dsign)
                    {
                        adj.d = 1.;
                        goto apply_adj;
                    }
                }
                else if (!dsign)
                {
                    adj.d = -1.;
                    if (!word1(&rv) && !(word0(&rv) & Frac_mask))
                    {
                        y = word0(&rv) & Exp_mask;
                        if (!scale || y > 2 * P * Exp_msk1)
                        {
                            delta = lshift(delta, Log2P, &alloc);
                            if (cmp(delta, bs) <= 0)
                                adj.d = -0.5;
                        }
                    }
                apply_adj:
                    if (scale && (y = word0(&rv) & Exp_mask) <= 2 * P * Exp_msk1)
                        word0(&adj) += (2 * P + 1) * Exp_msk1 - y;
                    dval(&rv) += adj.d * ulp(&rv);
                }
                break;
            }
            adj.d = ratio(delta, bs);
            if (adj.d < 1.)
                adj.d = 1.;
            if (adj.d <= 0x7ffffffe)
            {
                /* adj = rounding ? ceil(adj) : floor(adj); */
                y = adj.d;
                if (y != adj.d)
                {
                    if (!((rounding >> 1) ^ dsign))
                        y++;
                    adj.d = y;
                }
            }
            if (scale && (y = word0(&rv) & Exp_mask) <= 2 * P * Exp_msk1)
                word0(&adj) += (2 * P + 1) * Exp_msk1 - y;
            adj.d *= ulp(&rv);
            if (dsign)
                dval(&rv) += adj.d;
            else
                dval(&rv) -= adj.d;
            goto cont;
        }
#endif /*Honor_FLT_ROUNDS*/

        if (i < 0)
        {
            /*
              Error is less than half an ulp -- check for special case of mantissa
              a power of two.
            */
            if (dsign || word1(&rv) || word0(&rv) & Bndry_mask ||
                (word0(&rv) & Exp_mask) <= (2 * P + 1) * Exp_msk1)
            {
                break;
            }
            if (!delta->p.x[0] && delta->wds <= 1)
            {
                /* exact result */
                break;
            }
            delta = lshift(delta, Log2P, &alloc);
            if (cmp(delta, bs) > 0)
                goto drop_down;
            break;
        }
        if (i == 0)
        {
            /* exactly half-way between */
            if (dsign)
            {
                if ((word0(&rv) & Bndry_mask1) == Bndry_mask1 &&
                    word1(&rv) ==
                        ((scale && (y = word0(&rv) & Exp_mask) <= 2 * P * Exp_msk1) ? (0xffffffff & (0xffffffff << (2 * P + 1 - (y >> Exp_shift)))) : 0xffffffff))
                {
                    /*boundary case -- increment exponent*/
                    word0(&rv) = (word0(&rv) & Exp_mask) + Exp_msk1;
                    word1(&rv) = 0;
                    dsign = 0;
                    break;
                }
            }
            else if (!(word0(&rv) & Bndry_mask) && !word1(&rv))
            {
            drop_down:
                /* boundary case -- decrement exponent */
                if (scale)
                {
                    L = word0(&rv) & Exp_mask;
                    if (L <= (2 * P + 1) * Exp_msk1)
                    {
                        if (L > (P + 2) * Exp_msk1)
                            /* round even ==> accept rv */
                            break;
                        /* rv = smallest denormal */
                        goto undfl;
                    }
                }
                L = (word0(&rv) & Exp_mask) - Exp_msk1;
                word0(&rv) = L | Bndry_mask1;
                word1(&rv) = 0xffffffff;
                break;
            }
            if (!(word1(&rv) & LSB))
                break;
            if (dsign)
                dval(&rv) += ulp(&rv);
            else
            {
                dval(&rv) -= ulp(&rv);
                if (!dval(&rv))
                    goto undfl;
            }
            dsign = 1 - dsign;
            break;
        }
        if ((aadj = ratio(delta, bs)) <= 2.)
        {
            if (dsign)
                aadj = aadj1 = 1.;
            else if (word1(&rv) || word0(&rv) & Bndry_mask)
            {
                if (word1(&rv) == Tiny1 && !word0(&rv))
                    goto undfl;
                aadj = 1.;
                aadj1 = -1.;
            }
            else
            {
                /* special case -- power of FLT_RADIX to be rounded down... */
                if (aadj < 2. / FLT_RADIX)
                    aadj = 1. / FLT_RADIX;
                else
                    aadj *= 0.5;
                aadj1 = -aadj;
            }
        }
        else
        {
            aadj *= 0.5;
            aadj1 = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
            switch (Rounding)
            {
            case 2: /* towards +infinity */
                aadj1 -= 0.5;
                break;
            case 0: /* towards 0 */
            case 3: /* towards -infinity */
                aadj1 += 0.5;
            }
#else
            if (Flt_Rounds == 0)
                aadj1 += 0.5;
#endif /*Check_FLT_ROUNDS*/
        }
        y = word0(&rv) & Exp_mask;

        /* Check for overflow */

        if (y == Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
        {
            dval(&rv0) = dval(&rv);
            word0(&rv) -= P * Exp_msk1;
            adj.d = aadj1 * ulp(&rv);
            dval(&rv) += adj.d;
            if ((word0(&rv) & Exp_mask) >= Exp_msk1 * (DBL_MAX_EXP + Bias - P))
            {
                if (word0(&rv0) == Big0 && word1(&rv0) == Big1)
                    goto ovfl;
                word0(&rv) = Big0;
                word1(&rv) = Big1;
                goto cont;
            }
            else
                word0(&rv) += P * Exp_msk1;
        }
        else
        {
            if (scale && y <= 2 * P * Exp_msk1)
            {
                if (aadj <= 0x7fffffff)
                {
                    if ((z = (uint32)aadj) <= 0)
                        z = 1;
                    aadj = z;
                    aadj1 = dsign ? aadj : -aadj;
                }
                dval(&aadj2) = aadj1;
                word0(&aadj2) += (2 * P + 1) * Exp_msk1 - y;
                aadj1 = dval(&aadj2);
                adj.d = aadj1 * ulp(&rv);
                dval(&rv) += adj.d;
                if (rv.d == 0.)
                    goto undfl;
            }
            else
            {
                adj.d = aadj1 * ulp(&rv);
                dval(&rv) += adj.d;
            }
        }
        z = word0(&rv) & Exp_mask;
        if (!scale)
        {
            if (y == z)
            {
                /* Can we stop now? */
                L = (int32)aadj;
                aadj -= L;
                /* The tolerances below are conservative. */
                if (dsign || word1(&rv) || word0(&rv) & Bndry_mask)
                {
                    if (aadj < .4999999 || aadj > .5000001)
                        break;
                }
                else if (aadj < .4999999 / FLT_RADIX)
                    break;
            }
        }
    cont:
        Bfree(bb, &alloc);
        Bfree(bd, &alloc);
        Bfree(bs, &alloc);
        Bfree(delta, &alloc);
    }

    if (scale)
    {
        word0(&rv0) = Exp_1 - 2 * P * Exp_msk1;
        word1(&rv0) = 0;
        dval(&rv) *= dval(&rv0);
    }
    
retfree:
    Bfree(bb, &alloc);
    Bfree(bd, &alloc);
    Bfree(bs, &alloc);
    Bfree(bd0, &alloc);
    Bfree(delta, &alloc);
ret:
    *se = (char *)s;
    return sign ? -dval(&rv) : dval(&rv);
}

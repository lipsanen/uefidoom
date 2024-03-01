#include "dlibc.h"
#include "printf.h"
#include <stdint.h>

int d_abs(int __x) {
  int __sgn = __x >> (sizeof(int) * 8 - 1);
  return (__x ^ __sgn) - __sgn;
}

int d_isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

int d_isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int d_atoi(const char *s)
{
	int n=0, neg=0;
	while (d_isspace(*s)) s++;
	switch (*s) {
	case '-': neg=1;
	case '+': s++;
	}
	/* Compute n as a negative number to avoid overflow on INT_MIN */
	while (d_isdigit(*s))
		n = 10*n - (*s++ - '0');
	return neg ? n : -n;
}

void* d_memset( void* _dest, int ch, size_t count )
{
    uint8_t* dest = _dest;

    for(size_t i=0; i < count; ++i) {
        dest[i] = (uint8_t)ch;
    }

    return dest;
}

void* d_memcpy( void* _dest, const void* _src, size_t count )
{
    uint8_t* dest = _dest;
    const uint8_t* src = _src;
    const uint8_t* src_bound = _src + count;
    if(src < dest && src_bound >= dest) {
        for(size_t i=count-1;;--i) {
            dest[i] = src[i];

            if(i == 0) {
                break;
            }
        }
    }
    else {
        for(size_t i=0; i < count;++i) {
            dest[i] = src[i];
        }
    }

    return dest;
}

void* d_memmove( void* dest, const void* src, size_t count ) {
    return d_memcpy(dest, src, count);
}


void d_strcpy(char* dest, const char* src) {
	size_t size = d_strlen(src);
	d_memcpy(dest, src, size);
}

size_t d_strlen(const char* str) {
    size_t len;
    for(len=0; *str; ++len, ++str);
    return len;
}

int d_strcmp(const char *l, const char *r)
{
	for (; *l==*r && *l; l++, r++);
	return *(unsigned char *)l - *(unsigned char *)r;
}

int d_stricmp(const char *l, const char *r)
{
	for (; (d_toupper(*l)==d_toupper(*r)) && *l; l++, r++);
	return *(unsigned char *)l - *(unsigned char *)r;
}

int d_strnicmp(const char *l, const char *r, size_t len)
{
    size_t i;
    for (i=0; (d_toupper(*l)==d_toupper(*r)) && *l && i < len-1;++i,++l,++r);

	return d_toupper(*l) - d_toupper(*r);
}

int d_strncmp(const char *l, const char *r, size_t len)
{
    size_t i;
	for (i=0; *l==*r && *l && i < len-1;)
    {
        ++i;
        ++l;
        ++r;
    }

	return *(unsigned char *)l - *(unsigned char *)r;
}

char *d_strchr( const char *str, int ch )
{
    for(;*str != ch; ++str);

    return *str == ch ? (char*)str : NULL;
}

int d_toupper(int c)
{
	if (d_islower(c)) return c & 0x5f;
	return c;
}

int d_islower(int c)
{
	return (unsigned)c-'a' < 26;
}

char* d_strncpy(char *restrict d, const char *restrict s, size_t n)
{
	for (; n && (*d=*s); n--, s++, d++);
	d_memset(d, 0, n);
	return d;
}

int d_memcmp(const void *vl, const void *vr, size_t n)
{
	const unsigned char *l=vl, *r=vr;
	for (; n && *l == *r; n--, l++, r++);
	return n ? *l-*r : 0;
}

void* d_memchr(const void *src, int c, size_t n)
{
    const unsigned char *s = src;
	for (; n && *s != c; s++, n--);
	return n ? (void *)s : 0;
}

static char *twobyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint16_t nw = n[0]<<8 | n[1], hw = h[0]<<8 | h[1];
	for (h++; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-1 : 0;
}

static char *threebyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8;
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8;
	for (h+=2; *h && hw != nw; hw = (hw|*++h)<<8);
	return *h ? (char *)h-2 : 0;
}

static char *fourbyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8 | h[3];
	for (h+=3; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-3 : 0;
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define BITOP(a,b,op) \
 ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

static char *twoway_strstr(const unsigned char *h, const unsigned char *n)
{
	const unsigned char *z;
	size_t l, ip, jp, k, p, ms, p0, mem, mem0;
	size_t byteset[32 / sizeof(size_t)];
	d_memset(byteset, 0, 32 / sizeof(size_t));
	size_t shift[256];

	/* Computing length of needle and fill shift table */
	for (l=0; n[l] && h[l]; l++)
		BITOP(byteset, n[l], |=), shift[n[l]] = l+1;
	if (n[l]) return 0; /* hit the end of h */

	/* Compute maximal suffix */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] > n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	ms = ip;
	p0 = p;

	/* And with the opposite comparison */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] < n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	if (ip+1 > ms+1) ms = ip;
	else p = p0;

	/* Periodic needle? */
	if (d_memcmp(n, n+p, ms+1)) {
		mem0 = 0;
		p = MAX(ms, l-ms-1) + 1;
	} else mem0 = l-p;
	mem = 0;

	/* Initialize incremental end-of-haystack pointer */
	z = h;

	/* Search loop */
	for (;;) {
		/* Update incremental end-of-haystack pointer */
		if (z-h < l) {
			/* Fast estimate for MAX(l,63) */
			size_t grow = l | 63;
			const unsigned char *z2 = d_memchr(z, 0, grow);
			if (z2) {
				z = z2;
				if (z-h < l) return 0;
			} else z += grow;
		}

		/* Check last byte first; advance by shift on mismatch */
		if (BITOP(byteset, h[l-1], &)) {
			k = l-shift[h[l-1]];
			if (k) {
				if (k < mem) k = mem;
				h += k;
				mem = 0;
				continue;
			}
		} else {
			h += l;
			mem = 0;
			continue;
		}

		/* Compare right half */
		for (k=MAX(ms+1,mem); n[k] && n[k] == h[k]; k++);
		if (n[k]) {
			h += k-ms;
			mem = 0;
			continue;
		}
		/* Compare left half */
		for (k=ms+1; k>mem && n[k-1] == h[k-1]; k--);
		if (k <= mem) return (char *)h;
		h += p;
		mem = mem0;
	}
}

char *d_strstr(const char *h, const char *n)
{
	/* Return immediately on empty needle */
	if (!n[0]) return (char *)h;

	/* Use faster algorithms for short needles */
	h = d_strchr(h, *n);
	if (!h || !n[1]) return (char *)h;
	if (!h[1]) return 0;
	if (!n[2]) return twobyte_strstr((void *)h, (void *)n);
	if (!h[2]) return 0;
	if (!n[3]) return threebyte_strstr((void *)h, (void *)n);
	if (!h[3]) return 0;
	if (!n[4]) return fourbyte_strstr((void *)h, (void *)n);

	return twoway_strstr((void *)h, (void *)n);
}

static int internal_putchar(char c, void* user, size_t idx, size_t maxlen)
{
	return d_putchar(c);
}

int d_printf(const char* format, ...)
{
	printf_data data =
	{
		.fct = internal_putchar
	};

    va_list va;
    va_start(va, format);
    int result = func_printf(&data, format, va);
    va_end(va);
    return result;
}

int d_vprintf(const char* format, va_list va)
{
	printf_data data =
	{
		.fct = internal_putchar
	};

    int result = func_printf(&data, format, va);
    return result;
}

static int internal_writechar(char c, void* user, size_t idx, size_t maxlen)
{
	FILE* file = user;
	int result = d_fwrite(&c, 1, 1, file);
	return result;
}

int d_fprintf(FILE* file, const char* format, ...)
{
	printf_data data =
	{
		.fct = internal_putchar,
		.user = file,
	};

    va_list va;
    va_start(va, format);
    int result = func_printf(&data, format, va);
    va_end(va);
    return result;
}

typedef int (*cmpfun)(const void *, const void *, void *);

static inline int a_ctz_32(uint32_t x)
{
	static const char debruijn32[32] = {
		0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
		31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
	};
	return debruijn32[(x&-x)*0x076be629 >> 27];
}

static inline int a_ctz_64(uint64_t x)
{
	static const char debruijn64[64] = {
		0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
		62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
	};
	if (sizeof(long) < 8) {
		uint32_t y = x;
		if (!y) {
			y = x>>32;
			return 32 + a_ctz_32(y);
		}
		return a_ctz_32(y);
	}
	return debruijn64[(x&-x)*0x022fdd63cc95386dull >> 58];
}

static inline int a_ctz_l(unsigned long x)
{
	return (sizeof(long) < 8) ? a_ctz_32(x) : a_ctz_64(x);
}


#define ntz(x) a_ctz_l((x))

static inline int pntz(size_t p[2]) {
	int r = ntz(p[0] - 1);
	if(r != 0 || (r = 8*sizeof(size_t) + ntz(p[1])) != 8*sizeof(size_t)) {
		return r;
	}
	return 0;
}

static void cycle(size_t width, unsigned char* ar[], int n)
{
	unsigned char tmp[256];
	size_t l;
	int i;

	if(n < 2) {
		return;
	}

	ar[n] = tmp;
	while(width) {
		l = sizeof(tmp) < width ? sizeof(tmp) : width;
		d_memcpy(ar[n], ar[0], l);
		for(i = 0; i < n; i++) {
			d_memcpy(ar[i], ar[i + 1], l);
			ar[i] += l;
		}
		width -= l;
	}
}

/* shl() and shr() need n > 0 */
static inline void shl(size_t p[2], int n)
{
	if(n >= 8 * sizeof(size_t)) {
		n -= 8 * sizeof(size_t);
		p[1] = p[0];
		p[0] = 0;
	}
	p[1] <<= n;
	p[1] |= p[0] >> (sizeof(size_t) * 8 - n);
	p[0] <<= n;
}

static inline void shr(size_t p[2], int n)
{
	if(n >= 8 * sizeof(size_t)) {
		n -= 8 * sizeof(size_t);
		p[0] = p[1];
		p[1] = 0;
	}
	p[0] >>= n;
	p[0] |= p[1] << (sizeof(size_t) * 8 - n);
	p[1] >>= n;
}

static void sift(unsigned char *head, size_t width, cmpfun cmp, void *arg, int pshift, size_t lp[])
{
	unsigned char *rt, *lf;
	unsigned char *ar[14 * sizeof(size_t) + 1];
	int i = 1;

	ar[0] = head;
	while(pshift > 1) {
		rt = head - width;
		lf = head - width - lp[pshift - 2];

		if(cmp(ar[0], lf, arg) >= 0 && cmp(ar[0], rt, arg) >= 0) {
			break;
		}
		if(cmp(lf, rt, arg) >= 0) {
			ar[i++] = lf;
			head = lf;
			pshift -= 1;
		} else {
			ar[i++] = rt;
			head = rt;
			pshift -= 2;
		}
	}
	cycle(width, ar, i);
}

static void trinkle(unsigned char *head, size_t width, cmpfun cmp, void *arg, size_t pp[2], int pshift, int trusty, size_t lp[])
{
	unsigned char *stepson,
	              *rt, *lf;
	size_t p[2];
	unsigned char *ar[14 * sizeof(size_t) + 1];
	int i = 1;
	int trail;

	p[0] = pp[0];
	p[1] = pp[1];

	ar[0] = head;
	while(p[0] != 1 || p[1] != 0) {
		stepson = head - lp[pshift];
		if(cmp(stepson, ar[0], arg) <= 0) {
			break;
		}
		if(!trusty && pshift > 1) {
			rt = head - width;
			lf = head - width - lp[pshift - 2];
			if(cmp(rt, stepson, arg) >= 0 || cmp(lf, stepson, arg) >= 0) {
				break;
			}
		}

		ar[i++] = stepson;
		head = stepson;
		trail = pntz(p);
		shr(p, trail);
		pshift += trail;
		trusty = 0;
	}
	if(!trusty) {
		cycle(width, ar, i);
		sift(head, width, cmp, arg, pshift, lp);
	}
}

static void __qsort_r(void *base, size_t nel, size_t width, cmpfun cmp, void *arg)
{
	size_t lp[12*sizeof(size_t)];
	size_t i, size = width * nel;
	unsigned char *head, *high;
	size_t p[2] = {1, 0};
	int pshift = 1;
	int trail;

	if (!size) return;

	head = base;
	high = head + size - width;

	/* Precompute Leonardo numbers, scaled by element width */
	for(lp[0]=lp[1]=width, i=2; (lp[i]=lp[i-2]+lp[i-1]+width) < size; i++);

	while(head < high) {
		if((p[0] & 3) == 3) {
			sift(head, width, cmp, arg, pshift, lp);
			shr(p, 2);
			pshift += 2;
		} else {
			if(lp[pshift - 1] >= high - head) {
				trinkle(head, width, cmp, arg, p, pshift, 0, lp);
			} else {
				sift(head, width, cmp, arg, pshift, lp);
			}

			if(pshift == 1) {
				shl(p, 1);
				pshift = 0;
			} else {
				shl(p, pshift - 1);
				pshift = 1;
			}
		}

		p[0] |= 1;
		head += width;
	}

	trinkle(head, width, cmp, arg, p, pshift, 0, lp);

	while(pshift != 1 || p[0] != 1 || p[1] != 0) {
		if(pshift <= 1) {
			trail = pntz(p);
			shr(p, trail);
			pshift += trail;
		} else {
			shl(p, 2);
			pshift -= 2;
			p[0] ^= 7;
			shr(p, 1);
			trinkle(head - lp[pshift] - width, width, cmp, arg, p, pshift + 1, 1, lp);
			shl(p, 1);
			p[0] |= 1;
			trinkle(head - width, width, cmp, arg, p, pshift, 1, lp);
		}
		head -= width;
	}
}

static int wrapper_cmp(const void *v1, const void *v2, void *cmp)
{
	return ((d_cmpfun)cmp)(v1, v2);
}

void d_qsort(void *base, size_t nel, size_t width, d_cmpfun cmp)
{
	__qsort_r(base, nel, width, wrapper_cmp, (void *)cmp);
}

int d_aspect_ratio_comparison(uint32_t width, uint32_t height) {
    uint32_t compare_width = width * 10;
    uint32_t doom_equivalent_width = 16 * height;

    if(compare_width > doom_equivalent_width) {
        return 1;
    } else if(compare_width == doom_equivalent_width) {
        return 0;
    } else {
        return -1;
    }
}

void d_get_screen_params(uint32_t width, uint32_t height, uint32_t* skip_x, uint32_t* skip_y) {
	*skip_x = 0;
	*skip_y = 0;
	int comp = d_aspect_ratio_comparison(width, height);

	if(comp == 1) {
		uint32_t doom_width = height / 10 * 16;
		*skip_x = (width - doom_width) / 2;
	} else if(comp == -1) {
		uint32_t doom_height = width / 16 * 10;
		*skip_y = (height - doom_height) / 2;
	}
}


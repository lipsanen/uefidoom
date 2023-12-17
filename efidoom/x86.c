#include <stdbool.h>
#include <stdint.h>
#include "x86.h"
#include "dlibc.h"

#define printf d_printf
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define DECLARE_ARGS(val, low, high)	uint32_t low, high
#define EAX_EDX_VAL(val, low, high)	((low) | (uint64_t)(high) << 32)
#define EAX_EDX_RET(val, low, high)	"=a" (low), "=d" (high)
#define MAX_QUICK_PIT_MS 50
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICK_RATE / 1000 / 256)
#define PIT_TICK_RATE 1193182ul

#define BUILDIO(bwl, bw, type)						\
static inline void __out##bwl(type value, u16 port)		\
{									\
	__asm__ volatile("out" #bwl " %" #bw "0, %w1"			\
		     : : "a"(value), "Nd"(port));			\
}									\
									\
static inline type __in##bwl(u16 port)				\
{									\
	type value;							\
	__asm__ volatile("in" #bwl " %w1, %" #bw "0"			\
		     : "=a"(value) : "Nd"(port));			\
	return value;							\
}

BUILDIO(b, b, u8)
BUILDIO(w, w, u16)
BUILDIO(l,  , u32)
#undef BUILDIO

#define inb __inb
#define inw __inw
#define inl __inl
#define outb __outb
#define outw __outw
#define outl __outl

# define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

static bool cpu_calibrated = false;
static uint32_t cpu_calibrated_mul = 1;
static uint32_t cpu_calibrated_shift = 1;
static uint64_t cpu_base = 1;

uint64_t rdtsc()
{
	DECLARE_ARGS(val, low, high);

	__asm__ volatile("rdtsc" : EAX_EDX_RET(val, low, high));

	return EAX_EDX_VAL(val, low, high);
}


static uint8_t read_pit()
{
	inb(0x42);
	return inb(0x42);
}

/**
 * clocks_calc_mult_shift - calculate mult/shift factors for scaled math of clocks
 * @mult:	pointer to mult variable
 * @shift:	pointer to shift variable
 * @from:	frequency to convert from
 * @to:		frequency to convert to
 * @maxsec:	guaranteed runtime conversion range in seconds
 *
 * The function evaluates the shift/mult pair for the scaled math
 * operations of clocksources and clockevents.
 *
 * @to and @from are frequency values in HZ. For clock sources @to is
 * NSEC_PER_SEC == 1GHz and @from is the counter frequency. For clock
 * event @to is the counter frequency and @from is NSEC_PER_SEC.
 *
 * The @maxsec conversion range argument controls the time frame in
 * seconds which must be covered by the runtime conversion with the
 * calculated mult and shift factors. This guarantees that no 64bit
 * overflow happens when the input value of the conversion is
 * multiplied with the calculated mult factor. Larger ranges may
 * reduce the conversion accuracy by choosing smaller mult and shift
 * factors.
 */
void clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec)
{
	u64 tmp;
	u32 sft, sftacc= 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((u64)maxsec * from) >> 32;
	while (tmp) {
		tmp >>=1;
		sftacc--;
	}

	/*
	 * Find the conversion shift/mult pair which has the best
	 * accuracy and fits the maxsec conversion range:
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (u64) to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

static inline u64 mul_u32_u32(u32 a, u32 b)
{
	return (u64)a * b;
}

static inline u64 mul_u64_u32_shr(u64 a, u32 mul, uint32_t shift)
{
	u32 ah, al;
	u64 ret;

	al = a;
	ah = a >> 32;

	ret = mul_u32_u32(al, mul) >> shift;
	if (ah)
		ret += mul_u32_u32(ah, mul) << (32 - shift);

	return ret;
}

uint64_t pit_test(size_t count)
{
	uint64_t start = rdtsc();

	/* Set the Gate high, disable speaker */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);

	/*
	 * Counter 2, mode 0 (one-shot), binary count
	 *
	 * NOTE! Mode 2 decrements by two (and then the
	 * output is flipped each time, giving the same
	 * final output frequency as a decrement-by-one),
	 * so mode 0 is much better when looking at the
	 * individual counts.
	 */
	outb(0xb0, 0x43);

	/* Start at 0xffff */
	outb(0xff, 0x42);
	outb(0xff, 0x42);

	read_pit();

	for(size_t i=0; i < count; ++i)
	{
		while(read_pit() == (uint8_t)(0xff - i));
	}

	uint64_t end = rdtsc();

	return end - start;
}

static uint64_t calibration(size_t iterations)
{
	const size_t small_it = 5;
	uint64_t t1 = pit_test(small_it);
	uint64_t delta = pit_test(iterations + small_it) - t1;

	delta *= PIT_TICK_RATE;
	delta /= iterations*256*1000;
	return delta;
}

static int comp(const void* lhs, const void* rhs) {
	return (*(uint64_t*)lhs < *(uint64_t*)rhs) ? -1 : 1;
}

void calibrate_cpu()
{
	#define N 11
	#define iterations 100
	cpu_base = rdtsc();
	uint64_t values[N];
	for(size_t i=0; i < N; ++i) {
		values[i] = calibration(iterations);
	}

	d_qsort(values, N, sizeof(uint64_t), comp);
	d_printf("Calibration results: [");

	for(size_t i=0; i < N; ++i) {
		if(i == 0) {
			d_printf("%u", values[i]);
		} else {
			d_printf(", %u", values[i]);
		}
	}

	d_printf("]\n");

	clocks_calc_mult_shift(&cpu_calibrated_mul, &cpu_calibrated_shift, values[N/2], 1, 0);
	d_printf("TSC clock is %u kHz Calibration was done in %u msec\n", values[N/2], (uint32_t)clock_msec());
}


uint64_t clock_msec()
{
	uint64_t cycles = rdtsc() - cpu_base;
	return mul_u64_u32_shr(cycles, cpu_calibrated_mul, cpu_calibrated_shift);
}

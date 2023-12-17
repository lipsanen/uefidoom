#pragma once

#include <stdint.h>

void calibrate_cpu();
uint64_t rdtsc();
uint64_t clock_msec();

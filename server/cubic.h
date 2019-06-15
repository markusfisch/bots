#ifndef _cubiq_h_
#define _cubiq_h_

#include <stdint.h>

typedef struct {
	uint32_t seed;
	int32_t octave;
	int32_t periodx, periody;
} Cubic;

void cubic_init(Cubic *, const uint32_t, const int32_t, const int32_t);
void cubic_init2(Cubic *, const uint32_t, const int32_t, const int32_t,
	const int32_t);
float cubic_noise(const Cubic *, const float x);
float cubic_noise2(const Cubic *, const float, const float);

#endif

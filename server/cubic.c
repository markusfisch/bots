#include <math.h>

// Cubic Noise from
// https://github.com/jobtalle/CubicNoise

#include "cubic.h"

#define CUBIC_NOISE_RAND_A 134775813
#define CUBIC_NOISE_RAND_B 1103515245
static float cubic_noise_random(uint32_t seed, int32_t x, int32_t y) {
	return (float)
		((((x ^ y) * CUBIC_NOISE_RAND_A) ^ (seed + x)) *
		(((CUBIC_NOISE_RAND_B * x) << 16) ^ ((CUBIC_NOISE_RAND_B * y) -
			CUBIC_NOISE_RAND_A))) /
		UINT32_MAX;
}

static int32_t cubic_noise_tile(
		const int32_t coordinate,
		const int32_t period) {
	return coordinate % period;
}

static float cubic_noise_interpolate(
		const float a,
		const float b,
		const float c,
		const float d,
		const float x) {
	const float p = (d - c) - (a - b);
	return x * (x * (x * p + ((a - b) - p)) + (c - a)) + b;
}

void cubic_init(
		Cubic *cubic,
		const uint32_t seed,
		const int32_t octave,
		const int32_t period) {
	cubic->seed = seed;
	cubic->octave = octave;
	cubic->periodx = period / octave;
}

void cubic_init2(
		Cubic *cubic,
		const uint32_t seed,
		const int32_t octave,
		const int32_t periodx,
		const int32_t periody) {
	cubic->seed = seed;
	cubic->octave = octave;
	cubic->periodx = periodx / octave;
	cubic->periody = periody / octave;
}

float cubic_noise(const Cubic *cubic, const float x) {
	const int32_t xi = (int32_t)floorf(x / cubic->octave);
	const float lerp = x / cubic->octave - xi;

	return cubic_noise_interpolate(
		cubic_noise_random(cubic->seed,
			cubic_noise_tile(xi - 1, cubic->periodx), 0),
		cubic_noise_random(cubic->seed,
			cubic_noise_tile(xi, cubic->periodx), 0),
		cubic_noise_random(cubic->seed,
			cubic_noise_tile(xi + 1, cubic->periodx), 0),
		cubic_noise_random(cubic->seed,
			cubic_noise_tile(xi + 2, cubic->periodx), 0),
		lerp) * 0.5f + 0.25f;
}

float cubic_noise2(const Cubic *cubic, const float x, const float y) {
	uint32_t i;
	const int32_t xi = (int32_t)floorf(x / cubic->octave);
	const float lerpx = x / cubic->octave - xi;
	const int32_t yi = (int32_t)floorf(y / cubic->octave);
	const float lerpy = y / cubic->octave - yi;

	float samples[4];

	for(i = 0; i < 4; ++i) {
		samples[i] = cubic_noise_interpolate(
			cubic_noise_random(cubic->seed,
				cubic_noise_tile(xi - 1, cubic->periodx),
				cubic_noise_tile(yi - 1 + i, cubic->periody)),
			cubic_noise_random(cubic->seed,
				cubic_noise_tile(xi, cubic->periodx),
				cubic_noise_tile(yi - 1 + i, cubic->periody)),
			cubic_noise_random(cubic->seed,
				cubic_noise_tile(xi + 1, cubic->periodx),
				cubic_noise_tile(yi - 1 + i, cubic->periody)),
			cubic_noise_random(cubic->seed,
				cubic_noise_tile(xi + 2, cubic->periodx),
				cubic_noise_tile(yi - 1 + i, cubic->periody)),
			lerpx);
	}

	return cubic_noise_interpolate(
		samples[0], samples[1],
		samples[2], samples[3],
		lerpy) * 0.5f + 0.25f;
}

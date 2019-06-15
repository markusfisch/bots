#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "game.h"
#include "map.h"
#include "cubic.h"
#include "terrain.h"

extern struct Config config;

void terrain_generate(Map *map, const char *tiles) {
	unsigned int width = map->width;
	unsigned int height = map->height;
	unsigned int size = width * height;
	Cubic cubic;
	cubic_init2(&cubic, config.rand(), 5, width, height);
	double heightMap[size];
	unsigned int x, y, i;
	for (y = 0, i = 0; y < height; ++y) {
		for (x = 0; x < width; ++x, ++i) {
			heightMap[i] = cubic_noise2(&cubic, x, y);
		}
	}
	double mn = 99999;
	double mx = 0;
	for (i = 0; i < size; ++i) {
		double v = heightMap[i];
		if (v < mn) {
			mn = v;
		}
		if (v > mx) {
			mx = v;
		}
	}
	size_t ntiles = strlen(tiles);
	double range = mx - mn;
	for (y = 0, i = 0; y < height; ++y) {
		for (x = 0; x < width; ++x, ++i) {
			double v = heightMap[i];
			size_t idx = (size_t) floor(ntiles / range * (v - mn));
			if (idx >= ntiles) {
				idx = ntiles - 1;
			}
			map_set(map, x, y, tiles[idx]);
		}
	}
}

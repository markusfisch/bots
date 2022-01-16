#include <math.h>
#include <stdlib.h>

#include "../game.h"
#include "training.h"

static void prepare() {
	if (config.custom_map) {
		return;
	}
	const char *view =
		"..#.."\
		"....."\
		"....."\
		"....."\
		"..~..";
	const unsigned vw = 5;
	const unsigned vh = 5;
	const unsigned side = ceil(sqrt(game.nplayers));
	const unsigned width = side * vw;
	const unsigned height = side * vh;
	char *map = calloc(width * height, sizeof(char));
	char *mp = map;
	unsigned x, y;
	for (y = 0; y < height; ++y) {
		const char *vp = view + (y % vh) * vh;
		for (x = 0; x < width; ++x, ++mp) {
			*mp = vp[x % vw];
		}
	}
	config.custom_map = map;
	config.map_width = width;
	config.map_height = height;
	config.placing = PLACING_MANUAL;
	unsigned cx = vw >> 1;
	unsigned cy = vh >> 1;
	x = cx;
	y = cy;
	Coords *c = config.coords;
	Player *p = game.players, *e = game.players + game.nplayers;
	for (; p < e; ++p, ++c) {
		c->x = x;
		c->y = y;
		x += vw;
		if (x >= width) {
			y += vh;
			x = cx;
		}
	}
}

void training() {
	config.prepare = prepare;
}

#include <math.h>
#include <stdlib.h>

#include "game.h"
#include "player.h"
#include "placing.h"

extern struct Config config;
extern struct Game game;

static int fuzzy(const unsigned int max) {
	return max < 1 ? 0 : (rand() % (max + 1)) - (max >> 1);
}

void placing_circle(const unsigned int fuzz) {
	double between = 6.2831 / game.nplayers;
	double angle = between * rand();
	int cx = game.map.width / 2;
	int cy = game.map.height / 2;
	int radius = (cx < cy ? cx : cy) / 2;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->bearing = rand() % 4;
		p->x = round(cx + cos(angle) * radius) + fuzzy(fuzz);
		p->y = round(cy + sin(angle) * radius) + fuzzy(fuzz);
		map_set(&game.map, p->x, p->y, TILE_FLATLAND);
		angle += between;
	}
}

void placing_random(const unsigned int fuzz) {
	int width = game.map.width;
	int height = game.map.height;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		int px;
		int py;
		do {
			px = (rand() % width) + fuzzy(fuzz);
			py = (rand() % height) + fuzzy(fuzz);
		} while (config.impassable(&game.map, px, py) ||
			player_at(px, py, NULL));
		p->bearing = rand() % 4;
		p->x = px;
		p->y = py;
	}
}

void placing_grid(const unsigned int fuzz) {
	int sq = ceil(sqrt(game.nplayers));
	int xgap = game.map.width / sq;
	int ygap = game.map.height / sq;
	int x = 0;
	int y = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->bearing = rand() % 4;
		p->x = map_wrap(x + fuzzy(fuzz), game.map.width);
		p->y = map_wrap(y + fuzzy(fuzz), game.map.height);
		map_set(&game.map, p->x, p->y, TILE_FLATLAND);
		x += xgap;
		if (x >= (int) game.map.width) {
			x = 0;
			y += ygap;
		}
	}
}

void placing_manual(Coords *coords, const unsigned int fuzz) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++coords) {
		p->x = coords->x + fuzzy(fuzz);
		p->y = coords->y + fuzzy(fuzz);
		p->bearing = coords->bearing < 4 ? coords->bearing : rand() % 4;
		map_set(&game.map, p->x, p->y, TILE_FLATLAND);
	}
}

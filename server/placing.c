#include <math.h>
#include <stdlib.h>

#include "game.h"
#include "player.h"
#include "placing.h"

#define FIND_RADIUS 3

extern struct Config config;
extern struct Game game;

static int fuzzy(const unsigned int max) {
	return max < 1 ? 0 : (config.rand() % (max + 1)) - (max >> 1);
}

static int find_free_spot(int x, int y) {
	return !config.impassable(&game.map, x, y) && !player_at(x, y, NULL);
}

void placing_circle(const unsigned int fuzz) {
	double between = 6.2831 / game.nplayers;
	double angle = between * config.rand();
	int cx = game.map.width / 2;
	int cy = game.map.height / 2;
	int radius = (cx < cy ? cx : cy) / 2;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->bearing = config.rand() % 4;
		int x = round(cx + cos(angle) * radius) + fuzzy(fuzz);
		int y = round(cy + sin(angle) * radius) + fuzzy(fuzz);
		if (!map_find(&game.map, &x, &y, FIND_RADIUS, find_free_spot)) {
			map_set(&game.map, x, y, *config.flatland);
		}
		p->x = x;
		p->y = y;
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
			px = (config.rand() % width) + fuzzy(fuzz);
			py = (config.rand() % height) + fuzzy(fuzz);
		} while (config.impassable(&game.map, px, py) ||
			player_at(px, py, NULL));
		p->bearing = config.rand() % 4;
		p->x = px;
		p->y = py;
	}
}

void placing_grid(const unsigned int fuzz) {
	int sq = ceil(sqrt(game.nplayers));
	int xgap = game.map.width / sq;
	int ygap = game.map.height / sq;
	int gx = 0;
	int gy = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->bearing = config.rand() % 4;
		int x = map_wrap(gx + fuzzy(fuzz), game.map.width);
		int y = map_wrap(gy + fuzzy(fuzz), game.map.height);
		if (!map_find(&game.map, &x, &y, FIND_RADIUS, find_free_spot)) {
			map_set(&game.map, x, y, *config.flatland);
		}
		p->x = x;
		p->y = y;
		gx += xgap;
		if (gx >= (int) game.map.width) {
			gx = 0;
			gy += ygap;
		}
	}
}

void placing_diagonal(const unsigned int fuzz) {
	double dx = game.map.width;
	double dy = game.map.height;
	double d = sqrt(dx*dx + dy*dy);
	double step = d / game.nplayers;
	double start = step * .5;
	double nx = dx / d;
	double ny = dy / d;
	double stx = nx * step;
	double sty = ny * step;
	double sx = nx * start;
	double sy = ny * start;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		int x = map_wrap(round(sx) + fuzzy(fuzz), game.map.width);
		int y = map_wrap(round(sy) + fuzzy(fuzz), game.map.height);
		if (!map_find(&game.map, &x, &y, FIND_RADIUS, find_free_spot)) {
			map_set(&game.map, x, y, *config.flatland);
		}
		p->x = x;
		p->y = y;
		p->bearing = config.rand() % 4;
		sx += stx;
		sy += sty;
	}
}

void placing_manual(Coords *coords, const unsigned int fuzz) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++coords) {
		int x = map_wrap(coords->x + fuzzy(fuzz), game.map.width);
		int y = map_wrap(coords->y + fuzzy(fuzz), game.map.height);
		if (!map_find(&game.map, &x, &y, FIND_RADIUS, find_free_spot)) {
			map_set(&game.map, x, y, *config.flatland);
		}
		p->x = x;
		p->y = y;
		p->bearing = coords->bearing < 4 ? coords->bearing :
			config.rand() % 4;
		map_set(&game.map, p->x, p->y, TILE_FLATLAND);
	}
}

#include <math.h>
#include <stdlib.h>

#include "game.h"
#include "player.h"
#include "placing.h"

extern struct Game game;

void placing_circle() {
	double between = 6.2831 / game.nplayers;
	double angle = between * rand();
	int cx = game.map.width / 2;
	int cy = game.map.height / 2;
	int radius = (cx < cy ? cx : cy) / 2;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->bearing = rand() % 4;
		p->x = round(cx + cos(angle) * radius);
		p->y = round(cy + sin(angle) * radius);
		map_set(&game.map, p->x, p->y, TILE_FLATLAND);
		angle += between;
	}
}

void placing_random() {
	int width = game.map.width;
	int height = game.map.height;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		int px;
		int py;
		do {
			px = rand() % width;
			py = rand() % height;
		} while (map_impassable(&game.map, px, py) ||
			player_at(px, py));
		p->x = px;
		p->y = py;
	}
}

void placing_manual(Coords *coords) {
	int width = game.map.width;
	int height = game.map.height;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++coords) {
		int px = coords->x;
		int py = coords->y;
		while (map_impassable(&game.map, px, py) ||
				player_at(px, py)) {
			px = rand() % width;
			py = rand() % height;
		}
		p->x = px;
		p->y = py;
	}
}

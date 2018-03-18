#include <math.h>
#include <stdlib.h>

#include "game.h"
#include "player.h"
#include "placing.h"

void placing_circle(struct Game *game) {
	double between = 6.2831 / game->nplayers;
	double angle = between * rand();
	int cx = game->map.width / 2;
	int cy = game->map.height / 2;
	int radius = (cx < cy ? cx : cy) / 2;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		p->bearing = rand() % 4;
		p->x = round(cx + cos(angle) * radius);
		p->y = round(cy + sin(angle) * radius);
		map_set(&game->map, p->x, p->y, TILE_FLATLAND);
		angle += between;
	}
}

void placing_random(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	int width = game->map.width;
	int height = game->map.height;
	for (; p < e; ++p) {
		do {
			p->x = rand() % width;
			p->y = rand() % height;
		} while (map_impassable(&game->map, p->x, p->y) ||
			player_at(game, p->x, p->y));
	}
}

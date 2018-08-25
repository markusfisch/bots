#include <stdlib.h>

#include "../game.h"
#include "../player.h"
#include "collect.h"

extern struct Config config;
extern struct Game game;

static unsigned int collected;

unsigned int scatter(char tile, unsigned int amount) {
	unsigned int already = map_count(&game.map, tile);
	if (already > 0) {
		return already;
	}
	unsigned int i;
	for (i = 0; i < amount; ++i) {
		int x, y;
		do {
			x = config.rand() % game.map.width;
			y = config.rand() % game.map.height;
		} while (map_get(&game.map, x, y) == tile ||
			config.impassable(&game.map, x, y) || player_at(x, y, NULL));
		map_set(&game.map, x, y, tile);
	}
	return amount;
}

static void start() {
	collected = 0;
	config.gems = scatter(TILE_GEM, config.gems);
}

static void move(Player *p, char cmd) {
	player_move(p, cmd);
	if (map_get(&game.map, p->x, p->y) == TILE_GEM) {
		++p->score;
		map_set(&game.map, p->x, p->y, *config.flatland);
		if (++collected >= config.gems) {
			game_terminate();
		}
	}
}

void collect() {
	config.placing = config.placing ?: PLACING_RANDOM;

	config.start = start;
	config.move = move;
}

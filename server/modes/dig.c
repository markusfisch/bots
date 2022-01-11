#include <string.h>
#include <math.h>

#include "../game.h"
#include "../player.h"
#include "dig.h"

#define TILE_TREASURE '@'
#define TILE_NOTHING '.'

extern struct Config config;
extern struct Game game;

static Map treasure_map = {NULL, NULL, 0, 0, 0};
static unsigned int ntreasure = 0;

static void move_if_allowed(Map *map, Player *player, char cmd) {
	int x = player->x;
	int y = player->y;
	char level = map_get(&game.map, x, y);
	player_move(player, cmd);
	if (map_get(map, player->x, player->y) - level > 1) {
		player->x = x;
		player->y = y;
	}
}

static int dig_at(Map *map, int x, int y) {
	char level = map_get(map, x, y);
	if (level > '0') {
		--level;
		map_set(map, x, y, level);
		if (level < '1' && map_get(&treasure_map, x, y) == TILE_TREASURE) {
			map_set(&treasure_map, x, y, TILE_NOTHING);
			return 1;
		}
	}
	return 0;
}

static void move(Player *player, char cmd) {
	switch (cmd) {
	case 's':
		player->map = &treasure_map;
		break;
	case 'd':
		if (dig_at(&game.map, player->x, player->y)) {
			++player->score;
			if (--ntreasure < 1) {
				game_stop();
			}
		}
		break;
	default:
		move_if_allowed(&game.map, player, cmd);
		break;
	}
}

static void place_treasures(const unsigned int treasures, const int min_dist) {
	struct Position {
		int x;
		int y;
	} positions[treasures], *current = positions;
	memset(&positions, 0, sizeof(positions));
	unsigned int min_dist_sq = min_dist * min_dist;
	unsigned int i;
	for (i = 0; i < treasures; ++i, ++current) {
		int x, y, tries = 0;
		struct Position *p;
		do {
			x = config.rand() % game.map.width;
			y = config.rand() % game.map.height;
			for (p = positions; p < current; ++p) {
				int dx = x - p->x;
				int dy = y - p->y;
				unsigned int d = dx*dx + dy*dy;
				if (d < min_dist_sq) {
					break;
				}
			}
		} while (p < current && ++tries < 10);
		current->x = x;
		current->y = y;
		if (map_get(&game.map, x, y) == '0') {
			map_set(&game.map, x, y, '1');
		}
		map_set(&treasure_map, x, y, TILE_TREASURE);
	}
}

static void start() {
	map_create(&treasure_map, game.map.width, game.map.height);
	memset(treasure_map.data, TILE_NOTHING, treasure_map.size);
	ntreasure = config.gems;
	place_treasures(ntreasure,
		round((float) game.map.size / ntreasure * .125));
}

static void end() {
	map_free(&treasure_map);
}

void dig() {
	SET_IF_NULL(config.map_type, MAP_TYPE_TERRAIN)
	SET_IF_NULL(config.view_radius, 8)
	SET_IF_NULL(config.placing, PLACING_RANDOM)

	config.start = start;
	config.move = move;
	config.end = end;
}

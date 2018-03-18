#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../placing.h"
#include "find_exit.h"

#define TILE_EXIT 'O'

static void start(struct Game *game) {
	size_t ntiles = 16;
	char tiles[ntiles];
	memset(&tiles, TILE_FLATLAND, ntiles);
	tiles[0] = TILE_WATER;
	tiles[1] = TILE_WOOD;
	map_init(&game->map, 32, 32, tiles, ntiles);
	map_set(&game->map, 16, 16, TILE_EXIT);
	placing_circle(game);
}

static void moved(struct Game *game, struct Player *p) {
	if (map_get(&game->map, p->x, p->y) == TILE_EXIT) {
		printf("%c found the exit\n", p->name);
		game_end(game);
	}
}

void init_find_exit(struct Game *game) {
	game->min_players = 1;
	game->view_radius = 2;
	game->max_turns = 1024;
	game->start = start;
	game->impassable = map_impassable;
	game->moved = moved;
}

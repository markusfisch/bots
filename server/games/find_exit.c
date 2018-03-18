#include <stdio.h>

#include "../game.h"
#include "../placing.h"
#include "find_exit.h"

#define TILE_EXIT 'O'

static void start(struct Game *game) {
	map_init(&game->map, 32, 32);
	map_set(&game->map, 16, 16, TILE_EXIT);
	placing_circle(game);
}

static int impassable(struct Map *map, int x, int y) {
	return map_impassable(map, x, y);
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
	game->start = start;
	game->impassable = impassable;
	game->moved = moved;
}

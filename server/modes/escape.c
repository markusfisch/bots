#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "escape.h"

#define TILE_EXIT 'o'

static int points;

static int find_free_spot(int x, int y) {
	return !config.impassable(&game.map, x, y);
}

static void start() {
	points = MAX_PLAYERS;
	if (!map_count(&game.map, TILE_EXIT)) {
		int x = game.map.width / 2;
		int y = game.map.height / 2;
		map_find(&game.map, &x, &y, 5, find_free_spot);
		map_set(&game.map, x, y, TILE_EXIT);
	}
}

static void move(Player *p, char cmd) {
	player_move(p, cmd);
	if (map_get(&game.map, p->x, p->y) == TILE_EXIT) {
		p->score = points--;
		game_remove_player(p);
	}
}

void escape() {
	config.start = start;
	config.move = move;
}

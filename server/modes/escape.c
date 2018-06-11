#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "escape.h"

#define TILE_EXIT 'o'

extern struct Config config;
extern struct Game game;

static int points;

static void start() {
	points = MAX_PLAYERS;
	if (!map_count(&game.map, TILE_EXIT)) {
		map_set(&game.map, game.map.width / 2, game.map.height / 2,
			TILE_EXIT);
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

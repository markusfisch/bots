#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "last_man_standing.h"

extern struct Config config;
extern struct Game game;

static void move(Player *p, const char cmd) {
	switch (cmd) {
	case 'f':
		player_shoot(p);
		break;
	default:
		player_move(p, cmd);
		break;
	}
}

void last_man_standing() {
	config.min_players = config.min_players ?: 2;
	config.view_radius = config.view_radius ?: 4;
	config.max_turns = config.max_turns ?: game.map.size;
	config.shrink_after = config.shrink_after ?:
		game.map.width + game.map.height;

	config.move = move;
	config.marker = game_marker_show_life;
}

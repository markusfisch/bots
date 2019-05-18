#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "rumble.h"

extern struct Config config;
extern struct Game game;

void rumble() {
	config.min_players = config.min_players ?: 2;
	config.view_radius = config.view_radius ?: 4;
	config.placing = config.placing ?: PLACING_GRID;
	config.max_turns = config.max_turns ?: game.map.size;
	config.shrink_after = config.shrink_after ?: 64;
	config.can_shoot = config.can_shoot ?: 1;

	config.move = player_move;
	config.marker = player_marker_show_life;
}

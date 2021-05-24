#include "../game.h"
#include "../player.h"
#include "rumble.h"

extern struct Config config;
extern struct Game game;

void rumble() {
	SET_IF_NULL(config.min_players, 2)
	SET_IF_NULL(config.view_radius, 4)
	SET_IF_NULL(config.placing, PLACING_GRID)
	SET_IF_NULL(config.max_turns, game.map.size)
	SET_IF_NULL(config.shrink_after, 64)
	SET_IF_NULL(config.can_shoot, 1)

	config.marker = player_marker_show_life;
}

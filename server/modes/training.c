#include "../game.h"
#include "training.h"

extern struct Config config;

void training() {
	config.map_type = config.map_type ?: MAP_TYPE_CHESS;
}

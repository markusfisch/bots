#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "game.h"
#include "modes/asteroid_shower.h"
#include "modes/find_exit.h"
#include "modes/last_man_standing.h"
#include "modes/training.h"

#define PORT "--port"
#define MAP_SIZE "--map-size"
#define MAP_TYPE "--map-type"
#define MAP_TYPE_ARG_CHESS "chess"
#define MAP_TYPE_ARG_PLAIN "plain"
#define MAP_TYPE_ARG_RANDOM "random"
#define MAP_TYPE_ARG_MAZE "maze"
#define VIEW_RADIUS "--view-radius"
#define MAX_TURNS "--max-turns"
#define SHRINK_AFTER "--shrink-after"
#define PLAYER_LIFE "--player-life"
#define USECS_PER_TURN "--usecs-per-turn"

extern struct Config config;

static struct Mode {
	char letter;
	char *description;
	void *init;
} modes[] = {
	{ '0', "training ground", training },
	{ '1', "find exit", find_exit },
	{ '2', "last man standing", last_man_standing },
	{ '3', "avoid getting hit by floating astroids", asteroid_shower },
	{ 0, NULL, NULL }
};

static void help(char *bin) {
	printf("usage: %s MODE [FLAGS...]\n", basename(bin));
	printf("\nMODE must be one of:\n");
	struct Mode *m;
	for (m = modes; m->letter; ++m) {
		printf("%c - %s\n", m->letter, m->description);
	}
	printf("\nFLAGS can be any of:\n");
	printf(PORT" N\n");
	printf(MAP_SIZE" N[xN]\n");
	printf(MAP_TYPE" "MAP_TYPE_ARG_CHESS"|"MAP_TYPE_ARG_PLAIN"|"\
		MAP_TYPE_ARG_RANDOM"|"MAP_TYPE_ARG_MAZE"\n");
	printf(VIEW_RADIUS" N\n");
	printf(MAX_TURNS" N\n");
	printf(SHRINK_AFTER" N\n");
	printf(PLAYER_LIFE" N\n");
	printf(USECS_PER_TURN" N\n");
}

static void *pick_setup(const char letter) {
	struct Mode *m;
	for (m = modes; m->letter; ++m) {
		if (m->letter == letter) {
			return m->init;
		}
	}
	return NULL;
}

int main(int argc, char **argv) {
	#define HAS_ARG(flag) if (--argc < 1) {\
		fprintf(stderr, "error: missing argument for %s\n", flag);\
		return 1;\
	}

	memset(&config, 0, sizeof(config));
	config.port = 63187;
	config.map_width = config.map_height = 32;
	config.map_type = MAP_TYPE_CHESS;

	char *bin = *argv;
	while (--argc) {
		++argv;
		if (!strcmp(*argv, PORT)) {
			HAS_ARG(PORT)
			config.port = atoi(*++argv);
		} else if (!strcmp(*argv, MAP_SIZE)) {
			HAS_ARG(MAP_SIZE)
			int width = atoi(strtok(*++argv, "x"));
			char *height = strtok(NULL, "x");
			config.map_width = width;
			config.map_height = height ? atoi(height) : width;
		} else if (!strcmp(*argv, MAP_TYPE)) {
			HAS_ARG(MAP_TYPE)
			++argv;
			if (!strcmp(*argv, MAP_TYPE_ARG_CHESS)) {
				config.map_type = MAP_TYPE_CHESS;
			} else if (!strcmp(*argv, MAP_TYPE_ARG_PLAIN)) {
				config.map_type = MAP_TYPE_PLAIN;
			} else if (!strcmp(*argv, MAP_TYPE_ARG_RANDOM)) {
				config.map_type = MAP_TYPE_RANDOM;
			} else if (!strcmp(*argv, MAP_TYPE_ARG_MAZE)) {
				config.map_type = MAP_TYPE_MAZE;
			} else {
				fprintf(stderr, "error: unknown map type\n");
				return 1;
			}
		} else if (!strcmp(*argv, VIEW_RADIUS)) {
			HAS_ARG(VIEW_RADIUS)
			config.view_radius = atoi(*++argv);
		} else if (!strcmp(*argv, MAX_TURNS)) {
			HAS_ARG(MAX_TURNS)
			config.max_turns = atoi(*++argv);
		} else if (!strcmp(*argv, SHRINK_AFTER)) {
			HAS_ARG(SHRINK_AFTER)
			config.shrink_after = atoi(*++argv);
		} else if (!strcmp(*argv, PLAYER_LIFE)) {
			HAS_ARG(PLAYER_LIFE)
			config.player_life = atoi(*++argv);
		} else if (!strcmp(*argv, USECS_PER_TURN)) {
			HAS_ARG(USECS_PER_TURN)
			config.usec_per_turn = atoi(*++argv);
		} else if (!(config.init = pick_setup(**argv))) {
			fprintf(stderr, "error: invalid argument \"%s\"\n", *argv);
			help(bin);
			return 1;
		}
	}

	if (!config.init) {
		help(bin);
		return 0;
	}

	return game_serve();
}

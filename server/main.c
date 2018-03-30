#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "player.h"
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
#define DETERMINISTIC "--deterministic"

extern struct Config config;

static struct Mode {
	char *name;
	char *description;
	void *init;
} modes[] = {
	{ "training", "just learn to move and see", training },
	{ "escape", "find the exit field 'O'", find_exit },
	{ "rumble", "last man standing, shoot with 'f'", last_man_standing },
	{ "avoid", "survive inside an asteroid shower", asteroid_shower },
	{ NULL, NULL, NULL }
};

static void usage(char *bin) {
	printf("usage: %s [OPTION...] MODE\n", basename(bin));
	printf("\nMODE must be one of:\n");
	struct Mode *m;
	for (m = modes; m->name; ++m) {
		printf("%s - %s\n", m->name, m->description);
	}
	printf("\nOPTION can be any of:\n");
	printf(PORT" N\n");
	printf(MAP_SIZE" N[xN]\n");
	printf(MAP_TYPE" "MAP_TYPE_ARG_CHESS"|"MAP_TYPE_ARG_PLAIN"|"\
		MAP_TYPE_ARG_RANDOM"|"MAP_TYPE_ARG_MAZE"\n");
	printf(VIEW_RADIUS" N\n");
	printf(MAX_TURNS" N\n");
	printf(SHRINK_AFTER" N\n");
	printf(PLAYER_LIFE" N\n");
	printf(USECS_PER_TURN" N\n");
	printf(DETERMINISTIC"\n");
}

static void *pick_setup(const char *name) {
	struct Mode *m;
	for (m = modes; m->name; ++m) {
		if (!strcmp(m->name, name)) {
			return m->init;
		}
	}
	return NULL;
}

static int parse_map_type(const char *arg) {
	if (!strcmp(arg, MAP_TYPE_ARG_CHESS)) {
		return MAP_TYPE_CHESS;
	} else if (!strcmp(arg, MAP_TYPE_ARG_PLAIN)) {
		return MAP_TYPE_PLAIN;
	} else if (!strcmp(arg, MAP_TYPE_ARG_RANDOM)) {
		return MAP_TYPE_RANDOM;
	} else if (!strcmp(arg, MAP_TYPE_ARG_MAZE)) {
		return MAP_TYPE_MAZE;
	}
	fprintf(stderr, "error: unknown map type\n");
	exit(1);
}

static void has_args(int *argc, const char *flag, const int min) {
	if (--*argc < min) {
		fprintf(stderr, "error: missing argument for %s\n", flag);
		exit(1);
	}
}

static void has_arg(int *argc, const char *flag) {
	has_args(argc, flag, 1);
}

static void parse_arguments(int argc, char **argv) {
	char *bin = *argv;
	void (*init_mode)() = NULL;
	int deterministic = 0;

	while (--argc) {
		++argv;
		if (!strcmp(*argv, PORT)) {
			has_arg(&argc, PORT);
			config.port = atoi(*++argv);
		} else if (!strcmp(*argv, MAP_SIZE)) {
			has_arg(&argc, MAP_SIZE);
			int width = atoi(strtok(*++argv, "x"));
			char *height = strtok(NULL, "x");
			config.map_width = width;
			config.map_height = height ? atoi(height) : width;
		} else if (!strcmp(*argv, MAP_TYPE)) {
			has_arg(&argc, MAP_TYPE);
			config.map_type = parse_map_type(*++argv);
		} else if (!strcmp(*argv, VIEW_RADIUS)) {
			has_arg(&argc, VIEW_RADIUS);
			config.view_radius = atoi(*++argv);
		} else if (!strcmp(*argv, MAX_TURNS)) {
			has_arg(&argc, MAX_TURNS);
			config.max_turns = atoi(*++argv);
		} else if (!strcmp(*argv, SHRINK_AFTER)) {
			has_arg(&argc, SHRINK_AFTER);
			config.shrink_after = atoi(*++argv);
		} else if (!strcmp(*argv, PLAYER_LIFE)) {
			has_arg(&argc, PLAYER_LIFE);
			config.player_life = atoi(*++argv);
		} else if (!strcmp(*argv, USECS_PER_TURN)) {
			has_arg(&argc, USECS_PER_TURN);
			config.usec_per_turn = atoi(*++argv);
		} else if (!strcmp(*argv, DETERMINISTIC)) {
			deterministic = 1;
		} else if (!(init_mode = pick_setup(*argv))) {
			fprintf(stderr, "error: invalid argument \"%s\"\n", *argv);
			exit(1);
		}
	}

	if (!init_mode) {
		usage(bin);
		exit(0);
	}

	if (!deterministic) {
		srand(time(NULL));
	}

	init_mode();
}

int main(int argc, char **argv) {
	memset(&config, 0, sizeof(config));

	parse_arguments(argc, argv);

	config.port = config.port ?: 63187;
	config.map_width = config.map_width ?: 32;
	config.map_height = config.map_height ?: config.map_width;
	config.map_type = config.map_type ?: MAP_TYPE_PLAIN;
	config.usec_per_turn = config.usec_per_turn ?: USEC_PER_SEC;
	config.min_players = config.min_players ?: 1;
	config.view_radius = config.view_radius ?: 2;
	config.max_turns = config.max_turns ?: 1024;
	config.shrink_after = config.shrink_after ?: config.max_turns;
	config.move = config.move ?: player_move;
	config.impassable = config.impassable ?: map_impassable;

	return game_serve();
}

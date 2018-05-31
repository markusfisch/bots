#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "player.h"
#include "modes/asteroid_shower.h"
#include "modes/collect_gems.h"
#include "modes/find_exit.h"
#include "modes/last_man_standing.h"
#include "modes/snakes.h"
#include "modes/training.h"

#define MAP_TYPE_ARG_PLAIN "plain"
#define MAP_TYPE_ARG_RANDOM "random"
#define MAP_TYPE_ARG_MAZE "maze"
#define PLACING_ARG_CIRCLE "circle"
#define PLACING_ARG_RANDOM "random"
#define FORMAT_ARG_PLAIN "plain"
#define FORMAT_ARG_JSON "json"

extern struct Config config;

static const char flatland[] = { TILE_FLATLAND, 0 };
static const char obstacles[] = { TILE_WATER, TILE_WOOD, 0 };
static const struct Mode {
	char *name;
	char *description;
	void *init;
} modes[] = {
	{ "training", "just learn to see and move", training },
	{ "escape", "find the exit field 'o'", find_exit },
	{ "collect", "collect as many gems '@' as possible", collect_gems },
	{ "snakes", "eat gems '@' and grow a tail", snakes },
	{ "rumble", "last man standing, shoot with 'f'", last_man_standing },
	{ "avoid", "survive an asteroid shower of 'X'", asteroid_shower },
	{ NULL, NULL, NULL }
};

static void usage() {
	printf("usage: bots [OPTION...] MODE\n");
	printf("\nMODE must be one of:\n");
	const struct Mode *m;
	for (m = modes; m->name; ++m) {
		printf("  %s - %s\n", m->name, m->description);
	}
	printf("\nOPTION can be any of:\n"\
		"  -P, --port N                port to listen for players, "\
			"default is 63187\n"\
		"  -w, --spectator-port N      port to listen for spectators, "\
			"default is 63188\n"\
		"  -K, --key KEY               spectator key, default is unset\n"\
		"  -m, --min-players N         minimum number of alive players, "\
			"default depends\n"\
		"                              on mode\n"\
		"  -s, --map-size N[xN]        map size, default is 32x32\n"\
		"  -t, --map-type TYPE         map type, either \""\
			MAP_TYPE_ARG_PLAIN"\", \""\
			MAP_TYPE_ARG_RANDOM"\" or \""\
			MAP_TYPE_ARG_MAZE"\",\n"\
		"                              default is \""\
			MAP_TYPE_ARG_PLAIN"\"\n"\
		"  -c, --custom-map FILE       custom map\n"\
		"  -o, --obstacles STRING      characters a player cannot enter\n"\
		"  -f, --flatland STRING       characters a player can enter\n"\
		"  -x, --multiplier N          multiplier of flatland string, "\
			"default is 14\n"\
		"  -p, --placing TYPE          player placing, either \""\
			PLACING_ARG_CIRCLE"\" or \""\
			PLACING_ARG_RANDOM"\",\n"\
		"                              default depends on mode\n"\
		"  -A, --place-at X,Y[,D];...  place players at given "\
			"coordinates and in given\n"\
		"                              direction, either '^', '>', "\
			"'v' or '<'\n"
		"  -N, --non-exclusive         multiple players can occupy "\
			"the same cell\n"\
		"  -v, --view-radius N         how many fields a player can "\
			"see in every\n"\
		"                              direction, default is 2\n"\
		"  -G, --max-games N           maximum number of games, "\
			"default is unlimited\n"\
		"  -M, --max-turns N           maximum number of turns, "\
			"default is 1024\n"\
		"  -L, --max-lag N             number of turns a player can "\
			"miss,\n"\
		"                              default is 1024\n"\
		"  -S, --shrink-after N        shrink map after that many "\
			"turns, default is 1024\n"\
		"  -T, --shrink-step N         amount of turns until next "\
			"shrink, default is 1\n"\
		"  -l, --player-life N         life value of players, "\
			"default is 1\n"\
		"  -X, --shoot                 players can shoot, "\
			"default depends on mode\n"\
		"  -g, --gems N                number of gems if there are "\
			"gems, default equals\n"\
		"                              map width\n"\
		"  -F, --format TYPE           server output format, either \""\
			FORMAT_ARG_PLAIN"\" or \""\
			FORMAT_ARG_JSON"\",\n"\
		"                              default is \""\
			FORMAT_ARG_PLAIN"\"\n"\
		"  -W, --wait-for-joins N      number of seconds to wait "\
			"for joins,\n"\
		"                              default is 10\n"\
		"  -u, --usec-per-turn N       maximum number of milliseconds "\
			"per turn, \n"\
		"                              default is 1000000 (one second)\n"\
		"  -d, --deterministic         don't seed the random number "\
			"generator\n");
}

static void *pick_mode(const char *name) {
	const struct Mode *m;
	for (m = modes; m->name; ++m) {
		if (!strcmp(m->name, name)) {
			return m->init;
		}
	}
	return NULL;
}

static char *load_map(const char *file) {
	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}
	long size = fseek(fp, 0, SEEK_END) ? -1 : ftell(fp);
	if (size < 1 || fseek(fp, 0, SEEK_SET)) {
		perror("fseek");
		fclose(fp);
		exit(1);
	}
	char *buf = NULL, *p;
	#define MAX_LINE 4096
	char line[MAX_LINE];
	size_t width = 0;
	size_t height = 0;
	while (fgets(line, MAX_LINE, fp)) {
		size_t len = strcspn(line, "\n");
		if (len < 1) {
			break;
		} else if (!width) {
			width = len;
			height = size / (width + 1);
			if (height < 1 || height * (width + 1) != (size_t) size) {
				break;
			}
			p = buf = calloc(width * height, sizeof(char));
			if (!p) {
				perror("calloc");
				fclose(fp);
				exit(1);
			}
		} else if (width != len) {
			break;
		}
		memcpy(p, line, width);
		p += width;
	}
	if (ferror(fp)) {
		fprintf(stderr, "error: unknown map format in \"%s\"\n", file);
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	config.map_width = width;
	config.map_height = height;
	return buf;
}

static int parse_format(char *arg) {
	if (!strcmp(arg, FORMAT_ARG_PLAIN)) {
		return FORMAT_PLAIN;
	} else if (!strcmp(arg, FORMAT_ARG_JSON)) {
		return FORMAT_JSON;
	}
	fprintf(stderr, "error: unknown format \"%s\"\n", arg);
	exit(1);
}

static int map_bearing(const char bearing) {
	switch (bearing) {
	default:
	case '^':
		return 0;
	case '>':
		return 1;
	case 'v':
		return 2;
	case '<':
		return 3;
	}
}

static int parse_placing_at(Coords *coords, char *arg) {
	#define SEPARATOR ";"
	Coords *p = coords, *e = p + MAX_PLAYERS;
	char *s = strtok(arg, SEPARATOR);
	for (; s && coords < e; s = strtok(NULL, SEPARATOR), ++p) {
		char bearing = 0;
		sscanf(s, "%d,%d,%c", &p->x, &p->y, &bearing);
		p->bearing = bearing == 0 ? 4 : map_bearing(bearing);
	}
	if (p > coords) {
		// replicate last coordinate for remaining players
		Coords *last = p - 1;
		for (; p < e; ++p) {
			p->x = last->x;
			p->y = last->y;
			p->bearing = last->bearing;
		}
	}
	return PLACING_MANUAL;
}

static int parse_placing(const char *arg) {
	if (!strcmp(arg, PLACING_ARG_CIRCLE)) {
		return PLACING_CIRCLE;
	} else if (!strcmp(arg, PLACING_ARG_RANDOM)) {
		return PLACING_RANDOM;
	}
	fprintf(stderr, "error: unknown placing \"%s\"\n", arg);
	exit(1);
}

static int parse_map_type(const char *arg) {
	if (!strcmp(arg, MAP_TYPE_ARG_PLAIN)) {
		return MAP_TYPE_PLAIN;
	} else if (!strcmp(arg, MAP_TYPE_ARG_RANDOM)) {
		return MAP_TYPE_RANDOM;
	} else if (!strcmp(arg, MAP_TYPE_ARG_MAZE)) {
		return MAP_TYPE_MAZE;
	}
	fprintf(stderr, "error: unknown map type \"%s\"\n", arg);
	exit(1);
}

static void parse_arguments(int argc, char **argv) {
	int deterministic = 0;

	struct option longopts[] = {
		{ "port", required_argument, NULL, 'P' },
		{ "spectator-port", required_argument, NULL, 'w' },
		{ "spectator-key", required_argument, NULL, 'K' },
		{ "min-players", required_argument, NULL, 'm' },
		{ "map-size", required_argument, NULL, 's' },
		{ "map-type", required_argument, NULL, 't' },
		{ "custom-map", required_argument, NULL, 'c' },
		{ "obstacles", required_argument, NULL, 'o' },
		{ "flatland", required_argument, NULL, 'f' },
		{ "multiplier", required_argument, NULL, 'x' },
		{ "placing", required_argument, NULL, 'p' },
		{ "place-at", required_argument, NULL, 'A' },
		{ "non-exclusive", required_argument, &config.non_exclusive, 1 },
		{ "view-radius", required_argument, NULL, 'v' },
		{ "max-games", required_argument, NULL, 'G' },
		{ "max-turns", required_argument, NULL, 'M' },
		{ "max-lag", required_argument, NULL, 'L' },
		{ "shrink-after", required_argument, NULL, 'S' },
		{ "shrink-step", required_argument, NULL, 'T' },
		{ "player-life", required_argument, NULL, 'l' },
		{ "shoot", no_argument, &config.can_shoot, 1 },
		{ "gems", required_argument, NULL, 'g' },
		{ "format", required_argument, NULL, 'F' },
		{ "wait-for-joins", required_argument, NULL, 'W' },
		{ "usec-per-turn", required_argument, NULL, 'u' },
		{ "deterministic", no_argument, &deterministic, 1 },
		{ NULL, 0, NULL, 0 }
	};

	int ch;
	while ((ch = getopt_long(argc, argv,
			"P:w:K:m:s:t:c:o:f:x:p:A:Nv:G:M:L:S:T:l:Xg:F:W:u:d",
			longopts, NULL)) != -1) {
		switch (ch) {
		case 'P':
			config.port_player = atoi(optarg);
			break;
		case 'w':
			config.port_spectator = atoi(optarg);
			break;
		case 'K':
			config.spectator_key = optarg;
			break;
		case 'm':
			config.min_players = atoi(optarg);
			break;
		case 's': {
			if (config.map_width || config.map_height) {
				fprintf(stderr, "error: map size already set\n");
				exit(1);
			}
			int width = atoi(strtok(optarg, "x"));
			char *height = strtok(NULL, "x");
			config.map_width = width;
			config.map_height = height ? atoi(height) : width;
			break;
		}
		case 't':
			config.map_type = parse_map_type(optarg);
			break;
		case 'c':
			config.custom_map = load_map(optarg);
			break;
		case 'o':
			config.obstacles = optarg && *optarg ? optarg : NULL;
			break;
		case 'f':
			config.flatland = optarg && *optarg ? optarg : NULL;
			break;
		case 'x':
			config.multiplier = atoi(optarg);
			break;
		case 'p':
			config.placing = parse_placing(optarg);
			break;
		case 'A':
			config.placing = parse_placing_at(config.coords, optarg);
			break;
		case 'N':
			config.non_exclusive = 1;
			break;
		case 'v':
			config.view_radius = atoi(optarg);
			break;
		case 'G':
			config.max_games = atoi(optarg);
			break;
		case 'M':
			config.max_turns = atoi(optarg);
			break;
		case 'L':
			config.max_lag = atoi(optarg);
			break;
		case 'S':
			config.shrink_after = atoi(optarg);
			break;
		case 'T':
			config.shrink_step = atoi(optarg);
			break;
		case 'l':
			config.player_life = atoi(optarg);
			break;
		case 'X':
			config.can_shoot = 1;
			break;
		case 'g':
			config.gems = atoi(optarg);
			break;
		case 'F':
			config.output_format = parse_format(optarg);
			break;
		case 'W':
			config.wait_for_joins = atoi(optarg);
			break;
		case 'u':
			config.usec_per_turn = atoi(optarg);
			break;
		case 'd':
			deterministic = 1;
			break;
		case 'h':
		case '?':
			usage();
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	void (*init_mode)() = NULL;
	if (argc < 1 || !(init_mode = pick_mode(*argv))) {
		usage();
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

	config.port_player = config.port_player ?: 63187;
	config.port_spectator = config.port_spectator ?: 63188;
	config.min_players = config.min_players ?: 1;
	config.map_width = config.map_width ?: 32;
	config.map_height = config.map_height ?: config.map_width;
	config.map_type = config.map_type ?: MAP_TYPE_PLAIN;
	config.obstacles = config.obstacles ?: obstacles;
	config.flatland = config.flatland ?: flatland;
	config.multiplier = config.multiplier ?: 14;
	config.placing = config.placing ?: PLACING_CIRCLE;
	config.view_radius = config.view_radius ?: 2;
	config.max_turns = config.max_turns ?: 1024;
	config.max_lag = config.max_lag ?: config.max_turns;
	config.shrink_after = config.shrink_after ?: config.max_turns;
	config.shrink_step = config.shrink_step ?: 1;
	config.player_life = config.player_life ?: 1;
	config.gems = config.gems ?: config.map_width;
	config.wait_for_joins = config.wait_for_joins ?: 10;
	config.usec_per_turn = config.usec_per_turn ?: USEC_PER_SEC;
	config.spectator_key = config.spectator_key ?: "";
	config.move = config.move ?: player_move;
	config.impassable = config.impassable ?: map_impassable;

	int rv = game_serve();
	free(config.custom_map);
	return rv;
}

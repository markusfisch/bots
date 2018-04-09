#ifndef _game_h_
#define _game_h_

#include <sys/time.h>
#include <sys/types.h>

#include "map.h"

#define USEC_PER_SEC 1000000L
#define MAX_PLAYERS 16
#define MAX_SPECTATORS 16
#define MAP_TYPE_CHESS 1
#define MAP_TYPE_PLAIN 2
#define MAP_TYPE_RANDOM 3
#define MAP_TYPE_MAZE 4
#define PLACING_CIRCLE 1
#define PLACING_RANDOM 2
#define PLACING_MANUAL 3
#define FORMAT_PLAIN 0
#define FORMAT_JSON 1

typedef struct Game Game;
typedef struct Player Player;
typedef struct Spectator Spectator;
typedef struct Config Config;
typedef struct Coords Coords;

struct Game {
	struct timeval tick;
	fd_set watch;
	fd_set ready;
	int nfds;
	time_t started;
	time_t stopped;
	unsigned int turn;
	unsigned int shrink_level;
	unsigned int shrink_step;
	unsigned int nplayers;
	Map map;
	struct Player {
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
		unsigned int moves;
		int score;
		unsigned int life;
		char killed_by;
		void *trunk;
	} players[MAX_PLAYERS];
	unsigned int nspectators;
	struct Spectator {
		int fd;
		char *match;
		void *fp;
	} spectators[MAX_SPECTATORS];
};

struct Config {
	unsigned int port_player;
	unsigned int port_spectator;
	unsigned int min_players;
	unsigned int map_width;
	unsigned int map_height;
	unsigned int map_type;
	const char *obstacles;
	const char *flatland;
	unsigned int multiplier;
	unsigned int placing;
	struct Coords {
		int x;
		int y;
	} coords[MAX_PLAYERS];
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int shrink_after;
	unsigned int shrink_step;
	unsigned int player_life;
	unsigned int gems;
	unsigned int output_format;
	unsigned int keep_running;
	time_t wait_for_joins;
	time_t usec_per_turn;
	char *spectator_key;
	void (*start)();
	void (*turn_start)();
	unsigned char (*marker)(Player *);
	int (*impassable)(Map *, int, int);
	void (*move)(Player *, char);
	void (*end)();
};

void game_remove_player(Player *);
unsigned int game_joined();
unsigned char game_marker_show_life(Player *);
void game_end();
int game_serve();

#endif

#ifndef _game_h_
#define _game_h_

#include <sys/time.h>
#include <sys/types.h>

#include "map.h"

#define MAX_PLAYERS 16
#define MAP_TYPE_CHESS 0
#define MAP_TYPE_PLAIN 1
#define MAP_TYPE_RANDOM 2
#define MAP_TYPE_MAZE 3

typedef struct Game Game;
typedef struct Player Player;
typedef struct Config Config;

struct Game {
	struct timeval tick;
	fd_set watch;
	fd_set ready;
	int nfds;
	int listening_fd;
	time_t started;
	time_t stopped;
	time_t usec_per_turn;
	unsigned int min_players;
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int turn;
	unsigned int shrink_level;
	unsigned int shrink_after;
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
	} players[MAX_PLAYERS];
	void (*start)();
	void (*turn_start)();
	unsigned char (*marker)(Player *);
	int (*impassable)(Map *, int, int);
	void (*move)(Player *, char);
};

struct Config {
	void (*init)();
	unsigned int port;
	unsigned int map_width;
	unsigned int map_height;
	unsigned int map_type;
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int shrink_after;
	unsigned int player_life;
	time_t usec_per_turn;
};

void game_remove_player(Player *);
size_t game_joined();
unsigned char game_marker_show_life(Player *);
void game_end();
int game_serve();

#endif

#ifndef _game_h_
#define _game_h_

#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "websocket.h"
#include "map.h"

#define USEC_PER_SEC 1000000L
#define MAX_NAMES 256
#define MAX_PLAYERS 16
#define MAX_SPECTATORS 16
#define MAP_TYPE_PLAIN 1
#define MAP_TYPE_RANDOM 2
#define MAP_TYPE_MAZE 3
#define PLACING_CIRCLE 1
#define PLACING_RANDOM 2
#define PLACING_GRID 3
#define PLACING_MANUAL 4
#define FORMAT_PLAIN 0
#define FORMAT_JSON 1

typedef struct Game Game;
typedef struct Player Player;
typedef struct Spectator Spectator;
typedef struct Config Config;
typedef struct Coords Coords;
typedef struct Names Names;

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
	unsigned int nspectators;
	Map map;
	struct Player {
		char addr[INET_ADDRSTRLEN];
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
		unsigned int moves;
		int score;
		unsigned int life;
		int attack_x;
		int attack_y;
		char killed_by;
		void *trunk;
		unsigned int counter;
		unsigned int last_diagonal;
		WebSocket ws;
	} players[MAX_PLAYERS];
	struct Spectator {
		char addr[INET_ADDRSTRLEN];
		WebSocket ws;
	} spectators[MAX_SPECTATORS];
};

struct Config {
	unsigned int port;
	unsigned int port_websocket;
	unsigned int port_spectator;
	unsigned int max_spectators;
	int remote_spectators;
	unsigned int min_players;
	unsigned int min_starters;
	unsigned int map_width;
	unsigned int map_height;
	unsigned int map_type;
	char *custom_map;
	const char *obstacles;
	const char *flatland;
	unsigned int multiplier;
	unsigned int placing;
	unsigned int placing_fuzz;
	struct Names {
		char *address;
		char name;
	} names[MAX_NAMES];
	struct Coords {
		int x;
		int y;
		int bearing;
	} coords[MAX_PLAYERS];
	int non_exclusive;
	int translate_walls;
	unsigned int view_radius;
	unsigned int max_games;
	unsigned int max_turns;
	unsigned int max_lag;
	unsigned int shrink_after;
	unsigned int shrink_step;
	unsigned int player_life;
	int can_shoot;
	unsigned int diagonal_interval;
	unsigned int gems;
	unsigned int spawn_frequency;
	unsigned int output_format;
	time_t wait_for_joins;
	time_t usec_per_turn;
	char *word;
	void (*prepare)();
	void (*start)();
	void (*turn_start)();
	char (*marker)(Player *);
	int (*impassable)(Map *, int, int);
	void (*move_blocked_at)(Player *, int, int);
	void (*move)(Player *, char);
	int (*attacking)(Player *);
	void (*end)();
	int (*rand)();
};

void game_set_players_score(int);
unsigned int game_joined();
char game_marker_show_life(Player *);
void game_remove_player(Player *);
void game_terminate();
int game_serve();

#endif

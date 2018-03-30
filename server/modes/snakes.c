#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "collect_gems.h"
#include "snakes.h"

#define TILE_GEM '@'
#define TILE_TAIL '*'
#define MAX_TAIL 64

extern struct Config config;
extern struct Game game;

static unsigned int collected;

typedef struct Tail Tail;
struct Tail {
	char tile;
	int x;
	int y;
	Tail *next;
};

static void start() {
	collected = 0;
	scatter_gems();
}

static void free_tail(Tail *first) {
	while (first) {
		Tail *next = first->next;
		map_set(&game.map, first->x, first->y, first->tile);
		free(first);
		first = next;
	}
}

static void drag_tail(Player *p, const char tile) {
	Tail *tail = calloc(1, sizeof(Tail));
	tail->tile = tile;
	tail->x = p->x;
	tail->y = p->y;
	tail->next = (Tail *) p->trunk;
	p->trunk = tail;

	int length = p->score;
	while (tail) {
		Tail *next = tail->next;
		if (--length < 0) {
			tail->next = NULL;
			free_tail(next);
			break;
		}
		tail = next;
	}
}

static void move(Player *p, char cmd) {
	int x = p->x;
	int y = p->y;
	player_move(p, cmd);
	if (p->x == x && p->y == y) {
		return;
	}
	char tile = map_get(&game.map, p->x, p->y);
	switch (tile) {
	case TILE_GEM:
		++p->score;
		if (++collected >= config.gems) {
			game_end();
		}
		tile = *config.flatland;
		break;
	case TILE_TAIL:
		game_remove_player(p);
		break;
	}
	if (p->score > 0) {
		map_set(&game.map, p->x, p->y, TILE_TAIL);
		drag_tail(p, tile);
	}
}

static void end() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->trunk) {
			free_tail((Tail *) p->trunk);
			p->trunk = NULL;
		}
	}
}

void snakes() {
	config.placing = config.placing ?: PLACING_RANDOM;

	config.start = start;
	config.move = move;
	config.end = end;
}

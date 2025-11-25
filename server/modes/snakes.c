#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "collect.h"
#include "snakes.h"

#define TILE_TAIL '*'
#define MAX_TAIL 64

static unsigned int collected;

typedef struct Tail Tail;
struct Tail {
	char tile;
	int x;
	int y;
	Tail *next;
};

static void start(void) {
	collected = 0;
	config.gems = scatter(TILE_GEM, config.gems);
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
	if (!tail) {
		return;
	}
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
			game_stop();
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

static void end(void) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->trunk) {
			free_tail((Tail *) p->trunk);
			p->trunk = NULL;
		}
	}
}

void snakes(void) {
	SET_IF_NULL(config.placing, PLACING_RANDOM)

	config.start = start;
	config.move = move;
	config.end = end;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "maze.h"

// depth-first search maze generator from
// https://en.wikipedia.org/wiki/Maze_generation_algorithm

static struct Node {
	int x;
	int y;
	void *parent;
	char ch;
	char dirs;
} *nodes = NULL;

static struct Node *maze_link(struct Node *n, const int width,
		const int height) {
	while (n->dirs) {
		char dir = 1 << (rand() % 4);

		if (~n->dirs & dir) {
			continue;
		}
		n->dirs &= ~dir;

		int x = n->x;
		int y = n->y;
		switch (dir) {
		case 1:
			if (x + 2 < width) {
				x += 2;
			} else {
				continue;
			}
			break;
		case 2:
			if (y + 2 < height) {
				y += 2;
			} else {
				continue;
			}
			break;
		case 4:
			if (x - 2 > -1) {
				x -= 2;
			} else {
				continue;
			}
			break;
		case 8:
			if (y - 2 > -1) {
				y -= 2;
			} else {
				continue;
			}
			break;
		}

		struct Node *dest = nodes + y * width + x;
		if (dest->ch == TILE_FLATLAND) {
			if (dest->parent != NULL) {
				continue;
			}
			dest->parent = n;

			nodes[n->x + (x - n->x) / 2 +
				(n->y + (y - n->y) / 2) * width].ch = TILE_FLATLAND;

			return dest;
		}
	}

	return n->parent;
}

static void maze_init(const int width, const int height) {
	int x;
	int y;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			struct Node *n = nodes + y * width + x;
			if (x * y % 2) {
				n->x = x;
				n->y = y;
				n->dirs = 15;
				n->ch = TILE_FLATLAND;
			} else {
				n->ch = TILE_WOOD;
			}
		}
	}
}

void maze_generate(struct Map *map) {
	int width = map->width;
	int height = map->height;

	nodes = calloc(width * height, sizeof(struct Node));
	if (nodes == NULL) {
		return;
	}

	maze_init(width, height);

	struct Node *first = nodes + 1 + width;
	struct Node *last = first;
	first->parent = first;
	while ((last = maze_link(last, width, height)) != first);

	size_t i;
	for (i = 0; i < map->size; ++i) {
		map->data[i] = nodes[i].ch;
	}

	free(nodes);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "maze.h"

// depth-first search maze generator from
// https://en.wikipedia.org/wiki/Maze_generation_algorithm

typedef struct {
	int x;
	int y;
	void *parent;
	char wall;
	char dirs;
} Node;
Node *nodes = NULL;

static Node *maze_link(Node *n, const int width, const int height) {
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

		Node *dest = nodes + y * width + x;
		if (!dest->wall) {
			if (dest->parent != NULL) {
				continue;
			}
			dest->parent = n;

			nodes[n->x + (x - n->x) / 2 +
				(n->y + (y - n->y) / 2) * width].wall = 0;

			return dest;
		}
	}

	return n->parent;
}

static void maze_init(const int width, const int height) {
	int x;
	int y;
	Node *n = nodes;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x, ++n) {
			if (x * y % 2) {
				n->x = x;
				n->y = y;
				n->dirs = 15; // set first four bits
				n->wall = 0;
			} else {
				n->wall = 1;
			}
		}
	}
}

void maze_generate(Map *map, const unsigned int sx, const unsigned int sy,
		const char empty, const char wall) {
	int width = map->width;
	int height = map->height;

	nodes = calloc(width * height, sizeof(Node));
	if (!nodes) {
		return;
	}

	maze_init(width, height);

	// start x/y must be on uneven values
	Node *first = nodes + (sy | 1) * width + (sx | 1);
	Node *last = first;
	first->parent = first;
	while ((last = maze_link(last, width, height)) != first);

	size_t i;
	for (i = 0; i < map->size; ++i) {
		map->data[i] = nodes[i].wall ? wall : empty;
	}

	free(nodes);
}

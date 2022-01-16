#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "game.h"
#include "map.h"
#include "nosignal.h"

int map_write(const int fd, char *data, const unsigned int width,
		const unsigned int height) {
	char *d = data;
	size_t y;
	for (y = 0; y < height; ++y) {
		if (send(fd, d, width, MSG_NOSIGNAL) < (ssize_t) width ||
				send(fd, "\n", 1, MSG_NOSIGNAL) < 1) {
			return -1;
		}
		d += width;
	}
	return 0;
}

void map_free(Map *map) {
	free(map->data);
	memset(map, 0, sizeof(Map));
}

void map_create(Map *map, const unsigned int width,
		const unsigned int height) {
	map_free(map);
	map->width = width;
	map->height = height;
	map->size = width * height;
	map->data = calloc(map->size, sizeof(char));
}

void map_init_random(Map *map, const unsigned int multiplier,
		const char *flat, const char *obstacles) {
	size_t nflat = flat ? strlen(flat) : 0;
	size_t mflat = nflat * multiplier;
	size_t nobstacles = obstacles ? strlen(obstacles) : 0;
	size_t ntiles = mflat + nobstacles;
	char tiles[ntiles];
	char *p = tiles, *e = tiles + mflat;
	for (; p < e; p += nflat) {
		memcpy(p, flat, nflat);
	}
	memcpy(p, obstacles, nobstacles);
	char *offset = map->data;
	size_t i;
	for (i = 0; i < map->size; ++i) {
		*offset++ = tiles[config.rand() % ntiles];
	}
	map->obstacles = obstacles;
}

int map_wrap(const int pos, const unsigned int max) {
	return (pos + max) % max;
}

size_t map_offset(Map *map, const int x, const int y) {
	return map_wrap(y, map->height) * map->width + map_wrap(x, map->width);
}

void map_restore_at(Map *map, char *backup, const int x, const int y) {
	map_set(map, x, y, backup[map_offset(map, x, y)]);
}

char map_get(Map *map, const int x, const int y) {
	return map->data[map_offset(map, x, y)];
}

void map_set(Map *map, const int x, const int y, const char tile) {
	map->data[map_offset(map, x, y)] = tile;
}

int map_impassable(Map *map, const int x, const int y) {
	char tile = map_get(map, x, y);
	return tile == TILE_GONE ||
		(map->obstacles && strchr(map->obstacles, tile)) ? 1 : 0;
}

unsigned int map_count(Map *map, const char tile) {
	unsigned int count = 0;
	size_t size = map->size;
	char *p;
	for (p = map->data; (p = memchr(p, tile, size)); ++count) {
		size = map->size - (++p - map->data);
	}
	return count;
}

int map_find(Map *map, int *x, int *y, const int radius,
		int (*compare)(int, int)) {
	int i, sv, ev, v, eu, u, w, step;
	for (i = 0; i < radius; ++i) {
		for (sv = *y - i, v = sv, ev = *y + i; v <= ev; ++v) {
			w = map_wrap(v, map->height);
			step = v > sv && v < ev ? i << 1 : 1;
			for (u = *x - i, eu = *x + i; u <= eu; u += step) {
				if (compare(map_wrap(u, map->width), w)) {
					*x = u;
					*y = w;
					return 1;
				}
			}
		}
	}
	return 0;
}

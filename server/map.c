#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

void map_write(const int fd, char *data, const unsigned int width,
		const unsigned int height) {
	char *d = data;
	size_t y;
	for (y = 0; y < height; ++y) {
		if (send(fd, d, width, MSG_NOSIGNAL) < (ssize_t) width ||
				send(fd, "\n", 1, MSG_NOSIGNAL) < 1) {
			break;
		}
		d += width;
	}
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
		*offset++ = tiles[rand() % ntiles];
	}
	map->obstacles = obstacles;
}

int map_wrap(const int pos, const unsigned int max) {
	return (pos + max) % max;
}

static size_t map_offset(Map *map, const int x, const int y) {
	return map_wrap(y, map->height) * map->width + map_wrap(x, map->width);
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
		size -= ++p - map->data;
	}
	return count;
}

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"

void map_write(const int fd, char *data, const unsigned int width,
		const unsigned int height) {
	char *d = data;
	size_t y;
	for (y = 0; y < height; ++y) {
		write(fd, d, width);
		write(fd, "\n", 1);
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

void map_init_chess(Map *map, const char white, const char black) {
	char *offset = map->data;
	unsigned int x;
	unsigned int y;
	for (y = 0; y < map->height; ++y) {
		for (x = 0; x < map->height; ++x) {
			*offset++ = (x + y) % 2 ? white : black;
		}
	}
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"

void map_write(const int fd, char *data, const size_t width,
		const size_t height) {
	char *d = data;
	size_t y;
	for (y = 0; y < height; ++y) {
		write(fd, d, width);
		write(fd, "\n", 1);
		d += width;
	}
}

void map_free(struct Map *map) {
	free(map->data);
	memset(map, 0, sizeof(struct Map));
}

void map_create(struct Map *map, const size_t width, const size_t height) {
	map_free(map);
	map->width = width;
	map->height = height;
	map->size = width * height;
	map->data = calloc(map->size, sizeof(char));
}

void map_init_random(struct Map *map, char *tiles, const size_t ntiles) {
	char *offset = map->data;
	size_t i;
	for (i = 0; i < map->size; ++i) {
		*offset++ = tiles[rand() % ntiles];
	}
}

int map_wrap(const int pos, const size_t max) {
	return (pos + max) % max;
}

static size_t map_offset(struct Map *map, const int x, const int y) {
	return map_wrap(y, map->height) * map->width + map_wrap(x, map->width);
}

char map_get(struct Map *map, const int x, const int y) {
	return map->data[map_offset(map, x, y)];
}

void map_set(struct Map *map, const int x, const int y, const char tile) {
	map->data[map_offset(map, x, y)] = tile;
}

int map_impassable(struct Map *map, const int x, const int y) {
	switch (map_get(map, x, y)) {
	case TILE_WATER:
	case TILE_WOOD:
		return 1;
	default:
		return 0;
	}
}

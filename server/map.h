#ifndef __map_h__
#define __map_h__

#define TILE_FLATLAND '.'
#define TILE_WOOD '#'
#define TILE_WATER '~'

struct Map {
	char *data;
	size_t width;
	size_t height;
	size_t size;
};

void map_write(int, char *, size_t, size_t);
void map_free(struct Map *);
void map_init(struct Map *, size_t, size_t);
int map_wrap(int, size_t);
char map_get(struct Map *, int, int);
void map_set(struct Map *, int, int, char);
int map_impassable(struct Map *, int, int);

#endif

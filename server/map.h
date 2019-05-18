#ifndef _map_h_
#define _map_h_

#define TILE_FLATLAND '.'
#define TILE_WOOD '#'
#define TILE_WATER '~'
#define TILE_GONE 'X'
#define TILE_BLACK '_'

typedef struct {
	char *data;
	const char *obstacles;
	unsigned int width;
	unsigned int height;
	size_t size;
} Map;

int map_write(int, char *, unsigned int, unsigned int);
void map_free(Map *);
void map_create(Map *, unsigned int, unsigned int);
void map_init_random(Map *, unsigned int, const char *, const char *);
int map_wrap(int, unsigned int);
size_t map_offset(Map *, const int, const int);
void map_restore_at(Map *, char *, const int, const int);
char map_get(Map *, int, int);
void map_set(Map *, int, int, char);
int map_impassable(Map *, int, int);
unsigned int map_count(Map *, const char);
int map_find(Map *, int *, int *, const int, int (*)(int, int));

#endif

#ifndef _placing_h_
#define _placing_h_

struct Coords;
struct Player;

void placing_circle(unsigned int);
void placing_random_player(Player *, const unsigned int);
void placing_random(unsigned int);
void placing_grid(unsigned int);
void placing_diagonal(unsigned int);
void placing_manual(Coords *, unsigned int);

#endif

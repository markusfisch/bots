#ifndef _player_h_
#define _player_h_

struct Player;

Player *player_at(int, int);
char player_bearing(const int);
void player_send_view(Player *);
void player_move(Player *, char);
void player_shoot(Player *);

#endif

#ifndef _player_h_
#define _player_h_

struct Game;
struct Player;

Player *player_at(int, int);
void player_send_view(Player *);
void player_move(Player *, char);

#endif

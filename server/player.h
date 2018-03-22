#ifndef _player_h_
#define _player_h_

struct Game;
struct Player;

struct Player *player_at(struct Game *, int, int);
void player_send_view(struct Game *, struct Player *);
void player_move(struct Game *, struct Player *, char);

#endif

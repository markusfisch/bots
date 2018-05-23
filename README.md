# Bots

Terrain parsing game for bots.

The game world is a two-dimensional orthogonal grid.
The game is turn-based. A turn ends as soon as all players have moved *or*
after one second has passed and at least one player made a move.

To play the game, a bot needs to connect a streaming socket to port 63187
after the `bots` server has been started.

Please find templates for such bots in the `templates` directory.

## What a bot receives from the server

At the beginning of the game and in response to each command, a bot receives
its environment as a top-down section of the whole map. This section is always
made from the same amount of columns and lines. For example, a 5x5 section
would look like this:

	.....
	....~
	.~A..
	.....
	.#...

`A` is you. You can be any letter from A to P.
First line is in front of you, the last one is behind you.

`.` is flatland, `~` is water, `#` is wood and `X` is a wall or a rock.
You can't walk through water or wood or other players.

`^`, `<`, `>` and `v` is another player. The character points in the
direction the other player is looking. For example, here, a player is
right in the front of you, looking at you:

	..v..
	....~
	.~A..
	.....
	.#...

## What a bot can send to the server

A bot _should_ respond to a map with exactly _one_ command character.

A bot _needs_ to send a command to get a new map. As long as your bot fails
to send a command, things go on without your bot knowing.

Basic commands are:

	^ - go one step forward
	< - turn left
	> - turn right
	v - go one step backward

The server will process _one_ command per turn only.

Invalid commands are silently ignored but a map is still sent.
Sending multiple command characters simply fills the network buffer and the
server will process the accumulated commands in the following turns.

## Available games

### training

Just learn to move around and to parse the map. Upon start, every player
gets the same section so it's easy to introduce a group of people:

	..#..
	.....
	..A..
	.....
	..~..

### escape

Find and enter the exit field `o`.
The bots are placed in a circle around this field with a random orientation.
The game ends when all players found the exit.

### collect

Collect as many gems `@` as possible.
The bots are randomly placed with a random orientation.
The bot that collected the most gems wins the game.
The game ends when there are no more gems to collect.

### snakes

Same as `collect` but every bot grows a tail for every gem it finds.
Just the like the famous game Snake. If a bot hits a tail `*`, including
its own, it's destroyed. The bot with the most gems wins the game.

### rumble

Hunt down all other bots and be the last to survive. Send `f` to shoot.
A bot can only shoot straight up.
The bots are placed in a circle around a common center with a random
orientation.

By default, a bot is killed on the first hit but you can use `--player-life`
to change this (see below). If a bot is hit that has more than one hit point,
your name in the middle of the map, e.g. `A`, will change to a number showing
your remaining hit points.

After some time, a wall `X` will appear that shrinks the world to move
the remaining bots closer together (use `--shrink-after` and `--shrink-step`
to control that).

### avoid

Survive inside a moving asteroid shower.
The asteroids `X` move in the same random direction, one field every turn.
If a bot gets hit, it's destroyed.
The bots are randomly placed with a random orientation.
The bot that survives the longest wins.

## Available maps

Use `--map-size` to set a custom map size and `--view-radius` to control
how many fields a bot can see of it.

Use `--map-type` to choose a map.

All four edges of a map are connected to their opposite edge.
So if a bot leaves a map on one side, it will appear on the opposite side
of the map again.

### plain

A bot can enter all fields:

	.....
	.....
	..A..
	.....
	.....

### random

A bot can _not_ enter `~` or `#`:

	#....
	....~
	.~A..
	.....
	.#...

### maze

A bot can _not_ enter `X`:

	.X.XX
	.X.X.
	.XAX.
	.X...
	XX.XX

## Build and run the server

Clone this repository:

	$ git clone https://github.com/markusfisch/bots.git

Enter the repository and the server directory and run `make`:

	$ cd bots/server && make

Then start the server:

	$ ./bots

Without any arguments you will see all modes and options:

```
usage: bots [OPTION...] MODE

MODE must be one of:
  training - just learn to see and move
  escape - find the exit field 'O'
  collect - collect as many gems '@' as possible
  snakes - eat gems '@' to get longer
  rumble - last man standing, shoot with 'f'
  avoid - survive inside an asteroid 'X' shower

OPTION can be any of:
  -P, --port N            port to listen for players, default is 63187
  -w, --spectator-port N  port to listen for spectators, default is 63188
  -K, --key KEY           spectator key, default is unset
  -m, --min-players N     minimum number of players, default depends on mode
  -s, --map-size N[xN]    map size, default is 32x32
  -t, --map-type TYPE     map type, either 'plain', 'random' or 'maze',
                          default is 'plain'
  -c, --custom-map FILE   custom map
  -o, --obstacles STRING  characters a player cannot enter
  -f, --flatland STRING   characters a player can enter
  -x, --multiplier N      multiplier of flatland string, default is 14
  -p, --placing TYPE      player placing, either 'circle' or 'random',
                          default depends on mode
  -A, --place-at N,N;...  place players at given coordinates
  -v, --view-radius N     how many fields a player can see in every direction,
                          default is 2
  -G, --max-games N       maximum number of games, default is unlimited
  -M, --max-turns N       maximum number of turns, default is 1024
  -L, --max-lag N         number of turns a player can miss, default is 1024
  -S, --shrink-after N    shrink map after that many turns, default is 1024
  -T, --shrink-step N     amount of turns until next shrink, default is 1
  -l, --player-life N     life value of players, default is 1
  -g, --gems N            number of gems if there are gems, default equals
                          map width
  -F, --format TYPE       server output format, either 'plain' or 'json',
                          default is 'plain'
  -k, --keep-running      restart game after end
  -W, --wait-for-joins N  number of seconds to wait for joins, default is 10
  -u, --usec-per-turn N   maximum number of milliseconds per turn, default is
                          1000000
  -d, --deterministic     don't seed the random number generator
```

For example, to start the `escape` game, run:

	$ ./bots escape

### Requirements

Builds on everything with a decent C compiler.

For example, on a Raspberry Pi you want to install `gcc` and `make`:

	$ sudo apt-get install gcc make

## Playing manually

The repository contains a very simple bash client. Start it like this
when in the repository's root directory:

	$ templates/bash/client

If `bots` is running on another machine, you'd do:

	$ templates/bash/client HOSTNAME

Where HOSTNAME is either the IP address or hostname of the machine the
server is running on.

## Playing automatically

Of course, the challenge is to write a program that plays the game.

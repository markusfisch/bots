# Bots

Prototype server for a terrain parsing game played by bots.

The game world is an infinite two-dimensional orthogonal grid.
The game is turn-based. A turn ends as soon as all players have moved *or*
after one second has passed.

All bots need to connect a streaming socket to port 63187 where the server
is responding. Find client templates that do so in the `templates` directory.

## What a bot receives from the server

At the beginning of the game and after each command, a bot receives a
top-down map of its environment. This map is always made from the same
amount of columns and lines. For example, a 5x5 map would look like this:

	.....\n
	....~\n
	.~A..\n
	.....\n
	.#...\n

`A` is you. You can be any letter from A to P.
First line is in front of you, the last one is behind you.

`.` is flatland, `~` is water and `#` is wood.
You can't walk through water or wood or other players.

`^`, `<`, `>` and `v` is another player. The character points in the
direction the other player is looking. For example, here, a player is
right in the front of you, looking at you:

	..v..\n
	....~\n
	.~A..\n
	.....\n
	.#...\n

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

Just learn to move around and to parse the map. You move on a simple
chess-like pattern of `.` and `_` characters. There is no other goal.

### escape

Find and enter the exit field `O`.
The bots are placed in a circle around this field with a random orientation.
The game ends when all players found the exit.

## collect

Collect as many gems `@` as possible.
The bots are placed at random with a random orientation.
The bot that collected the most gems wins the game.
The game ends when there are no more gems to collect.

## snakes

Same as `collect` but every bot grows a tail for every gem it finds.
Just the like the famous game Snakes. If a bot hits a tail `*` including
its own, its destroyed. The bot with the most gems wins the game.

## rumble

Hunt down all other bots and be the last to survive. Send `f` to shoot.
A bot can only shoot straight up.
A bot can stand only one hit (use `--player-life` to change this, see below).
The bots are placed in a circle around a common center with a random
orientation.

After some time, a wall `X` will appear that shrinks the world to move
the remaining bots closer together (use `--shrink-after` and `--shrink-step`
to control that).

## avoid

Survive inside a moving asteroid shower.
The asteroids `X` move in the same random direction.
If a bot gets hit, its destroyed.
The bots are placed at random with a random orientation.
The bot that survives the longest wins.

## Available maps

Use `--map-size` to set a custom map size and `--view-radius` to control
how many fields a bot can see of it.

### chess

A bot can enter all fields:

	_._._\n
	._._.\n
	_.A._\n
	._._.\n
	_._._\n

### plain

A bot can enter all fields:

	.....\n
	.....\n
	..A..\n
	.....\n
	.....\n

### random

A bot can not enter `~` or `#`:

	#....\n
	....~\n
	.~A..\n
	.....\n
	.#...\n

### maze

A bot can not enter `X`:

	.X.XX\n
	.X.X.\n
	.XAX.\n
	.X...\n
	XX.XX\n

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
  training - just learn to move and see
  escape - find the exit field 'O'
  collect - collect as many gems '@' as possible
  snakes - eat gems '@' to get longer
  rumble - last man standing, shoot with 'f'
  avoid - survive inside an asteroid shower

OPTION can be any of:
  -P, --port N            port number to listen for players
  -M, --min-players N     minimum number of players for a game
  -s, --map-size N[xN]    map size
  -t, --map-type TYPE     map type, either chess, plain, random or maze
  -o, --obstacles STRING  characters a player cannot enter
  -f, --flatland STRING   characters a player can enter
  -x, --multiplier N      multiplier of flatland string
  -p, --placing TYPE      player placing, either circle or random
  -v, --view-radius N     how many fields a player can see in every direction
  -m, --max-turns N       maximum number of turns
  -S, --shrink-after N    shrink map after that many turns
  -T, --shrink-step N     amount of turns until next shrink, default is 1
  -l, --player-life N     life value of players, default is 1
  -g, --gems N            number of gems if there are gems
  -W, --wait-for-joins N  number of seconds to wait for joins
  -u, --usec-per-turn N   maximum number of milliseconds per turn
  -d, --deterministic     don't seed the random number generator
```

### Requirements

Builds on everything with a decent C compiler.

For example, on a Raspberry Pi you want to install `gcc` and `make`:

	$ sudo apt-get install gcc make

## Joining as a human

The repository contains a very simple bash client you can start like this
when you are in the repository's root directory:

	$ templates/bash/client

If `bots` is running on another machine, you'd do:

	$ templates/bash/client HOSTNAME

Where HOSTNAME is either the IP address or hostname of the machine the
bots server is running on.

## Automate!

Of course, the challenge is to write a program that plays the game.

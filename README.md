# Bots

A terrain parsing game where you write little programs that play the game
automatically.

## About the game

* The game world is a two-dimensional orthogonal grid.
* The game is turn-based.
* A turn ends as soon as
	* all players have moved *or*
	* after one second has passed *and* at least one player made a move.
* Everything happens in sequence.

To play the game, your program, a so-called bot, must connect a streaming
socket to port 63187 (or a WebSocket to port 63188) after the `bots` server
is started.

Find templates for such bot programs in the `templates` directory.

There is also a very useful
[WebGL visualizer](https://github.com/ChristianNorbertBraun/bots_spectator)
to replay and inspect the games in a browser.

## What a bot receives from the server

At the beginning of each game and in response to each command, a bot receives
its environment as a top-down section of the whole map. This section is always
made from the same amount of columns and lines. For example, a 5x5 section
would look like this:

	.....
	....~
	.~A..
	.....
	.#...

`A` is you. You can be any letter from A to P.
The first line is always in front of you, the last one is behind you.

`.` is flatland, `~` is water, `#` is wood and `X` is a wall or a rock.
You can't walk through water, wood, stone or other players.

`^`, `<`, `>` and `v` is another player. The letter points in the
direction the other player is looking. For example, here, a player is
right in the front of you, looking at you:

	..v..
	....~
	.~A..
	.....
	.#...

## What a bot can send to the server

A bot _should_ respond to every map with exactly _one_ command character.

A bot _needs_ to send a command to get a new map. As long as your bot fails
to send a command, things go on without your bot knowing.

The server will process _one_ command per turn only.
Sending multiple characters simply fills the network buffer and the server
will _either_ discard the collected commands _or_ process them in the
following turns.

Invalid commands are silently ignored but a map is still sent.

If a bot disconnects, it's immediately removed from the game and all pending
commands are discarded.

Basic commands are:

	^ - go one step forward
	< - turn left
	> - turn right
	v - go one step backward

If `--diagonal-interval` is set, a bot may also move with the following
commands:

	{ - move top left
	} - move top right
	( - move left
	) - move right
	[ - move bottom left
	] - move bottom right

Depending on the selected game mode, additional commands may be available.

## Available modes

### training

Just learn to move around and to parse the map. Upon start, every player
gets the same section so it's easy to introduce a group of people to the
game:

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
A bot can only shoot straight up and only as far as the view is.
Shots don't go through blocking cells.
The bots are placed at a regular interval.

By default, a bot is killed on the first hit but you can use `--player-life`
to change this (see below). If a bot is hit that has more than one hit point,
your name in the middle of the map, e.g. `A`, will change to a number showing
your remaining hit points.

After turn 64, a wall `X` will appear that shrinks the world to move the
remaining bots closer together (use `--shrink-after` and `--shrink-step`
to change that).

### avoid

Survive inside an asteroid shower.
All asteroids `X` move in random but diagonal directions, one field per turn.
Asteroids change direction every 10 turns.
If a bot gets hit, it's destroyed.
The bots are randomly placed with a random orientation.
The bot that survives the longest gets the most points.

`--diagonal-interval` is set by default for this mode to give you
additional options to move.

### word

Find a random word of random length at the middle of the map and send it
back to the server.
The bots are placed in a circle around the word with a random orientation.
Use `--word` to set a custom word.

### boom

Place bombs and blow up all other players. Send a single digit from `1` to
`9` to place a bomb. The digit gives the number of turns until the bomb
detonates. Bombs blast orthogonal only.

Pick up power-ups `+` to increase the detonation radius of your bombs.

### horde

Team up to survive an endless horde of enemies `e`.

Every second turn a new enemy appears at a portal `&` (use
`--spawn-frequency` to change when a new enemy appears).
There is always one more portal than there are bots.
The bots and portals are each placed evenly on a diagonal line.

You can kill enemies that are next to you by bumping into them.
A dead enemy will turn into an extra life `+`.
Picking it up will increase your life points by one.
If an enemy touches you, you'll loose one life point per turn.

`--diagonal-interval` is set by default for this mode to give you
additional options to move.

### dig

Dig a hilly terrain for gold `@`.

The map is composed from numbers that give the height of the
corresponding cell. A bot can only overcome one level of height
difference, so you need to dig a slope in order to get out of
holes.

Scan the terrain by sending `s`.
The server will respond with a map of gold items, if available.
All gold items are on height level 0.

Navigate your bot to a gold item `@` and raise it by digging the
environment as needed to be able to get back out of the hole.
Dig a level down by sending `d`.

The bot that collected the most gold wins the game.

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
  escape - find the exit field 'o'
  collect - collect as many gems '@' as possible
  snakes - eat gems '@' and grow a tail
  rumble - last man standing, shoot with 'f'
  avoid - survive an asteroid shower of 'X'
  word - find a word somewhere on the map
  boom - last man standing, place bomb with '1' to '9'
  horde - survive hordes of enemies
  dig - dig for gold '@', dig with 'd', scan with 's'

OPTION can be any of:
  -P, --port N                port to listen for players, default is 63187
  -W, --websocket-port N      port for WebSocket players, default is 63188
  -O, --spectator-port N      port for WebSocket spectators, default is 63189
  -V, --max-spectators N      maximum number of spectators, default is 0
  -r, --remote-spectators     allow remote spectators for multiplayer games
  -b, --min-starters N        minimum number of players to start a game,
                              default is 1
  -m, --min-players N         minimum number of alive players, default depends
                              on mode
  -n, --name-file FILE        list of IP addresses with player names in
                              "192.168.1.5 B [Bob]" format
  -s, --map-size N[xN]        map size, default is 32x32
  -t, --map-type TYPE         map type, either "plain", "random", "maze" or
                              "terrain", default is "plain"
  -c, --custom-map FILE       custom map
  -o, --obstacles STRING      characters a player cannot enter
  -f, --flatland STRING       characters a player can enter
  -x, --multiplier N          multiplier of flatland string, default is 14
  -p, --placing TYPE          player placing, either "circle", "random",
                              "grid" or "diagonal", default depends on mode
  -Z, --fuzzy N               maximum potential deviaton from calculated
                              position, default is 0
  -A, --place-at X,Y[,D]:...  manually place players at given coordinates and
                              in given direction, either '^', '>', 'v' or '<'
  -N, --non-exclusive         multiple players can occupy the same cell
  -Y, --translate-walls       translate '-' and '|' according to orientation
  -v, --view-radius N         how many fields a player can see in every
                              direction, default is 2
  -G, --max-games N           maximum number of games, default is 1,
                              use -1 for unlimited games
  -M, --max-turns N           maximum number of turns, default is 512
  -L, --max-lag N             number of turns a player can miss before getting
                              disconnected, unlimited by default
  -S, --shrink-after N        shrink map after that many turns, default is 512
  -T, --shrink-step N         amount of turns until next shrink, default is 1
  -l, --player-life N         life value of players, default is 1
  -X, --shoot                 players can shoot, default depends on mode
  -D, --diagonal-interval N   players can move diagonally every N turns,
                              default is 0 for no diagonal movement
  -g, --gems N                number of gems if there are gems, default equals
                              map width
  -Q, --spawn-frequency N     spawn a new enemy every N turns, for modes with
                              enemies, default is 2
  -R, --word STRING           custom word for "word" mode, random by default
  -F, --format TYPE           server output format, either "plain" or "json",
                              default is "plain"
  -w, --wait-for-joins N      number of seconds to wait for additional joins
                              when complete, default is 1
  -u, --usec-per-turn N       maximum number of milliseconds per turn,
                              default is 1000000 (one second)
  -d, --deterministic         don't seed the random number generator
```

For example, to start the `escape` game, run:

	$ ./bots escape

### Requirements

Builds on everything with a decent C compiler.

For example, on a Raspberry Pi you want to install `gcc` and `make`:

	$ sudo apt-get install gcc make

## Playing manually

The `templates` directory contains a number of simple bots in different
languages. All these bots ask you for a command when a section is received.

To try the bash bot for example, type:

	$ templates/bash/bot

If the server is running on another machine, you'd do:

	$ templates/bash/bot HOSTNAME

Where HOSTNAME is either the IP address or hostname of the machine the
server is running on.

If you don't have access to a shell, you can still play the game from any
device with a web browser. Just open the file `templates/web/bot.html` or
simply click [here](http://bots.markusfisch.de/) and write a bot
in JavaScript.

It's even possible to use
[JS Bin](http://jsbin.com/ninemajabu/edit?js,output),
[JSFiddle](http://jsfiddle.net/16gqbLth/)
or any other source code playground that runs JavaScript.

Please note that since the server does not implement Secure WebSockets,
it is necessary to be on HTTP because HTTPS only allows Secure WebSockets.
An exception is the localhost 127.0.0.1 that can be accessed from HTTPS.

## Playing automatically

Of course, the challenge is to write a program that plays the game.

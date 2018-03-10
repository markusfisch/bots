# Terrain Parsing Prototype Server

Prototype socket server for a multiplayer terrain parsing game in plain C.

The game world is an infinite two-dimensional orthogonal grid.
Each player can move on that grid.

The server starts a new game after 10 seconds if there is at least one
client connected. Otherweise it'll wait another 10 seconds and so on.

## What you get from the server

When a game is started, the server will send every second a 5x5 view matrix
to all clients that looks like this:

	.....\n
	....~\n
	.~X..\n
	.....\n
	.#...\n

`X` is where you are. There are three terrain types:

	. - flatland
	~ - water
	# - wood

At the moment you can move through all of them.

`^`, `<`, `>` and `V` stand for another player and his/her relative direction.
For example, he's looking at you, kid:

	..V..\n
	....~\n
	.~X..\n
	.....\n
	.#...\n

## What you can send to the server

A client may answer any view matrix with just one character.
There are four commands:

	^ - go one step forward
	< - turn left
	> - turn right
	V - go one step backward

The server will respond with a new view for each character.

## Build and run the server

Clone this repository, enter the directory and run

	$ make

Then start the server:

	$ ./srv

## Joining as a human

There's a simple bash client you can start like this:

	$ ./cli

If `srv` is running on another machine, you'd do:

	$ ./cli HOSTNAME

## Automate!

Of course, the challenge is to write a program that plays the game.
Alas, there are no goals defined, yet. This is *work in progress*.

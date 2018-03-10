# Terrain Parsing Prototype Server

Prototype socket server for a multiplayer terrain parsing game in plain C.

The game world is an infinite two-dimensional orthogonal grid.
Each player can move on that grid.

The server tries to start a new game every 10 seconds if there is at least
one client connected.

## What you get from the server

Every second, you get a top-down map of your environment that may look
like this:

	.....\n
	....~\n
	.~X..\n
	.....\n
	.#...\n

`X` is you. First line is in front of you, the last one is behind you.

There are three terrain types:

	. - flatland
	~ - water
	# - wood

You can't walk through water or wood or other players.

`^`, `<`, `>` and `V` is another player. The character points in the
direction the other player is looking. For example, here, a player is
right in the front of you, looking at you:

	..V..\n
	....~\n
	.~X..\n
	.....\n
	.#...\n

## What you can send to the server

You may answer a map with _one_ command character.
Available commands are:

	^ - go one step forward
	< - turn left
	> - turn right
	V - go one step backward

The server will process _one_ command per map only.
Invalid commands are dropped.

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

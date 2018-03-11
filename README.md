# Terrain Parsing Prototype Server

Prototype socket server for a terrain parsing game played by bots.

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
Invalid commands are ignored.

## Build and run the server

Clone this repository, enter the directory and run

	$ make

Then start the server:

	$ ./srv

### Requirements

Builds on everything with a C compiler.

For example, on a Raspberry Pi you want to install `gcc` and `make`:

	$ sudo apt-get install gcc make

## Joining as a human

The repository contains a very simple bash client you can start like this:

	$ ./cli

If `srv` is running on another machine, you'd do:

	$ ./cli HOSTNAME

## Automate!

Of course, the challenge is to write a program that plays the game.
Alas, there are no goals implemented, yet. This is *work in progress*.

## Possible Goals

### Find a certain field
All players are randomly oriented in a circle around the field to be found.

### Collect items
Find and collect as many items as possible. Items are evenly distributed.

### Labyrinth
Find a way out.

### Last man standing
Shoot all other players and be the last to survive. Every player has x life
points, each hit takes one.

### Search and Destroy
Hunt NPC's and take them down. The player with the most kills wins.

### Measure the world
Find the dimensions of the world matrix by identifying unique terrain
features and find them again.

### Find your match
There's a perfect match for every player. Find yours. All players start with
a random orientation in a circle around a common center. Your match is always
on the other side of the circle.

### Discover a secret
Every player has a part of a riddle. You need to contact each one and trade
secrets to combine all parts to discover the secret. The secret may be a word
or sentence. First player to solve the riddle wins.

### Survive the horde
All players are attacked by NPC's and need to work together to survive the
raid.

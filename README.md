# Terrain Parsing Prototype Server

Prototype TCP server for a multiplayer terrain parsing game in plain C.

The game starts after 10 seconds if there is at least one client connected.
If there is no client, it'll wait another 10 seconds and so on.

When the game is started, all clients get a 5x5 view matrix which may look
like this:

	.....
	....~
	.~X..
	.....
	.#...

X is where you are. There are three types of terrains:

	. - flatland
	~ - water
	# - wood

You can move through all of them.

A client can send one of just four commands:

	^ - go one step forward
	< - turn left
	> - turn right
	V - go one step backward

The server will respond with a new view.

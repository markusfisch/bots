# Troubleshooting

## The spectator doesn't connect

By default, the server doesn't allow any spectators to make sure your
opponents can't sniff the whole world map while the game is on.
To allow spectators use the `-V`/`--max-spectators` flag like this:

	$ ./bots -V1 escape

If the spectator (web site) is running on a different machine, make sure
you also set `-r`/`--remote-spectators`:

	$ ./bots -V1 -r escape

## The server is closing the connection immediately

A bot (or spectator) can only join a game before it started.

Make sure your bot connects to the correct port:

* The default port for streaming sockets is 63187.
* The default port for WebSockets is 63188.

If you're connecting from a browser, your bot uses WebSockets.

## The server is crashing

Most probably you didn't `clean` after pulling updates. Please run:

	$ make clean test

If that isn't successful or the server is still crashing for you, it's a bug
and you should file an issue.

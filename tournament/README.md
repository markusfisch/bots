# Bots Tournament

To run a tournament with many players and bots, you may use the accompanying
`Makefile` to compose the menu of games you like to play at that event.

Using that `Makefile`, it's very simple to run (and save) a given
pre-configured game. For example, to run the `escape` game, you'd run:

	$ make escape

If you need to temporarily override the number of required players to
start a game, you can easily do so:

	$ STARTERS=8 make escape

Please note that the server output is piped into a JSON file and you won't
see any output on the command line.
What shouldn't be a problem since, for a tournament, you will most likely
want to use the excellent [WebGL visualizer][spectator] anyway.
The accompanying `Makefile` already supports this by setting
`--remote-spectators` and `--max-spectators 1`.

The JSON records can be used to replay certain games (using the
[WebGL visualizer][spectator]) and for highscore generation.

## Highscores

After having run a number of games, you'll want to see a combined
highscore table. Run this:

	$ make scores

[spectator]: https://github.com/ChristianNorbertBraun/bots_spectator

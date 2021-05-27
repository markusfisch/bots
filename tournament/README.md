# Bots Tournament

To run a tournament with many players and bots accross many games, you
may simply use a `Makefile` to compose a menu of games you want to play
at this event.

Just put this into a file named `Makefile` (or copy the accompanying
`Makefile.template` to `Makefile`):

	BOTS ?= ../server/bots
	STARTERS ?= 7
	FLAGS ?= \
		--format json \
		--min-starters $(STARTERS) \
		--name-file names \
		--max-lag 3 \
		--max-spectators 1 \
		--remote-spectators
	REPLAY ?= $(shell date +%Y%m%d-%H%M%S).json

	escape:
		$(BOTS) $(FLAGS) $@ > $(REPLAY)

	rumble:
		$(BOTS) $(FLAGS) --diagonal-interval 1 $@ > $(REPLAY)

	# add more games here!

See below to learn what the `FLAGS` are about.

## Run a game

With a `Makefile` like this, it's very simple to run (and save!)
pre-configured games. For example, to run the `escape` game from the
`Makefile` above, you'd run:

	$ make escape

You can also override the definitions at the top of the file to match
your situation. For example, if you need to temporarily override the
number of required players to start a game, you can easily do so:

	$ STARTERS=8 make escape

Please note that the server output is redirected into a JSON file
(`... > $(REPLAY)`) and you won't see any output on the command line.

These JSON files can be used to replay games later (using the
[WebGL visualizer][spectator]) and, more importantly, to generate
the highscores (see below).

## Visualization

If you want to see the game while it's happening, you may also use
the [WebGL visualizer][spectator]) and add a spectator to the game.

This is what the `--max-spectators 1` and `--remote-spectators` options
in `FLAGS` are for.

## Mapping of bots to letters

By default, the server automatically assigns a letter to each connected bot.
This can become a problem when players change a device or when the number
of bots (or players) changes during the tournament.

To permanently assign a letter to an IP address, you should create a text
file that maps all IP address of participants to a letter (from "A" to "P"):

	192.168.1.3 A Alice
	192.168.1.7 A Bob
	192.168.1.9 N
	192.168.1.5 F
	...

The third column is optional and can contain a longer name. This long
name is then displayed instead of the IP address in the server output.

There can be _multiple_ entries for a letter so a team can use different
machines for different games. Within a game, however, the letters are
unique, of course. The first matching IP address will claim a letter.

Save the list as `names` and put it in the same directory where the
Makefile is. This is what the `--name-file` option in `FLAGS` is for.

## Highscores

After having run a number of games, you'll want to see a combined
highscore table like this:

```
Place Name  Address                           Score
---------------------------------------------------
    1 B     192.168.1.110                       186
    2 A     Alice                               185
    3 F     192.168.1.5                         184
    4 N     192.168.1.9                         183
    5 C     192.168.1.68                        181
    5 G     192.168.1.105                       181
```

For this, you just need to run the Python script `highscores`:

	$ ./highscores PATH_TO_JSON_FILES

To quickly update the highscores you can add another rule to the `Makefile`:

	scores:
		@./highscores .

So you can always run:

	$ make scores

[spectator]: https://github.com/ChristianNorbertBraun/bots_spectator

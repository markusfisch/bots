BIN = bots
OBJECTS = map.o maze.o placing.o player.o game.o \
	modes/asteroid_shower.o \
	modes/collect_gems.o \
	modes/find_exit.o \
	modes/last_man_standing.o \
	modes/snakes.o \
	modes/training.o \
	modes/word.o \
	main.o
LIBS = -lm
FLAGS = -O2 -Wall -Wextra

.c.o: $(OBJECTS)
	$(CC) -c $< -o $@ $(FLAGS)

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o modes/*.o $(BIN)

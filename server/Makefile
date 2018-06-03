BIN = bots
OBJECTS = map.o maze.o placing.o player.o game.o \
	modes/avoid.o \
	modes/collect.o \
	modes/escape.o \
	modes/rumble.o \
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

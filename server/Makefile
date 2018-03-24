BIN = bots
OBJECTS = map.o placing.o player.o game.o \
	setups/find_exit_plain.o \
	setups/find_exit_obstacles.o \
	setups/last_man_standing.o \
	main.o
LIBS = -lm
FLAGS = -O2 -Wall -Wextra

.c.o: $(OBJECTS)
	$(CC) -c $< -o $@ $(FLAGS)

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o setups/*.o $(BIN)

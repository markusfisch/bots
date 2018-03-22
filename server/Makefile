BIN = srv
OBJECTS = map.o placing.o player.o game.o \
	games/find_exit_plain.o \
	games/find_exit_obstacles.o \
	games/last_man_standing.o \
	main.o
LIBS = -lm
FLAGS = -O2 -Wall -Wextra

.c.o: $(OBJECTS)
	$(CC) -c $< -o $@ $(FLAGS)

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o games/*.o $(BIN)

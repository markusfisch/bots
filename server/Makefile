BIN = bots
OBJECTS = map.o placing.o player.o game.o \
	modes/find_exit_plain.o \
	modes/find_exit_obstacles.o \
	modes/last_man_standing.o \
	modes/asteroid_shower.o \
	main.o
LIBS = -lm
FLAGS = -O2 -Wall -Wextra

.c.o: $(OBJECTS)
	$(CC) -c $< -o $@ $(FLAGS)

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o modes/*.o $(BIN)

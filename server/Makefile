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
DEPS = map.h maze.h placing.h player.h game.h \
	modes/avoid.h \
	modes/collect.h \
	modes/escape.h \
	modes/rumble.h \
	modes/snakes.h \
	modes/training.h \
	modes/word.h
LIBS = -lm
CFLAGS = -O2 -Wall -Wextra

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

test: $(BIN)
	tests/test_escape
	tests/test_collect
	tests/test_word
	tests/test_rumble

clean:
	rm -f *.o modes/*.o $(BIN)

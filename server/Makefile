BIN = bots
OBJECTS = map.o maze.o placing.o player.o game.o \
	base64.o \
	sha1.o \
	websocket.o \
	modes/avoid.o \
	modes/collect.o \
	modes/escape.o \
	modes/rumble.o \
	modes/snakes.o \
	modes/training.o \
	modes/word.o \
	modes/boom.o \
	main.o
DEPS = map.h maze.h placing.h player.h game.h \
	base64.h \
	sha1.h \
	websocket.h \
	modes/avoid.h \
	modes/collect.h \
	modes/escape.h \
	modes/rumble.h \
	modes/snakes.h \
	modes/training.h \
	modes/word.h \
	modes/boom.h
LIBS = -lm
CFLAGS = -O2 -Wall -Wextra

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

test: $(BIN)
	tests/test_escape
	tests/test_collect
	tests/test_snakes
	tests/test_rumble
	tests/test_avoid
	tests/test_word
	tests/test_boom

clean:
	rm -f *.o modes/*.o websocket/*.o $(BIN)

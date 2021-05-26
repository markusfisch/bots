BIN = bot
OBJECTS = main.o
LIBS = -lm
CFLAGS = -O2 -Wall -Wextra -pedantic -std=c99

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(BIN)

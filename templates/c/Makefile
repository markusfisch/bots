BIN = bot
OBJECTS = main.o
LIBS = -lm
CFLAGS = -O2 -std=c99 -pedantic -Wall -Wextra

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(BIN)

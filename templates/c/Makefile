BIN = bot
OBJECTS = main.o
LIBS = -lm
FLAGS = -O2 -Wall -Wextra

.c.o: $(OBJECTS)
	$(CC) -c $< -o $@ $(FLAGS)

$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(BIN)

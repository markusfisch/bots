SOURCES = Main.kt
JAR = bot.jar

all: $(JAR)
	java -jar $(JAR)

$(JAR): $(SOURCES)
	kotlinc $^ -include-runtime -d $@

clean:
	rm -f $(JAR)

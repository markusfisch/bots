SOURCES = Main.kt
JAR = client.jar

all: $(JAR)
	java -jar $(JAR)

$(JAR): $(SOURCES)
	kotlinc $^ -include-runtime -d $@

clean:
	rm -f $(JAR)

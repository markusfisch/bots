OBJECTS = src/bots/*.java
JAR = bot.jar
BUILD = build

all: $(JAR)
	java -jar $(JAR)

$(JAR): $(OBJECTS)
	[ -d $(BUILD) ] || mkdir $(BUILD)
	javac -d $(BUILD) $(OBJECTS)
	echo 'Main-Class: bots.Bot' > $(BUILD)/manifest
	(cd $(BUILD) && jar cfm ../$(JAR) manifest .)

clean:
	rm -rf $(BUILD) $(JAR)

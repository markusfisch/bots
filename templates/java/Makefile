OBJECTS = src/bots/client/*.java
JAR = bots.jar
BUILD = build

all: $(JAR)
	java -jar $(JAR)

$(JAR): $(OBJECTS)
	[ -d $(BUILD) ] || mkdir $(BUILD)
	javac -d $(BUILD) $(OBJECTS)
	echo 'Main-Class: bots.client.BotsClient' > $(BUILD)/manifest
	(cd $(BUILD) && jar cfm ../$(JAR) manifest .)

clean:
	rm -rf $(BUILD) $(JAR)

BOTS ?= ../server/bots
STARTERS ?= 7
FLAGS ?= --format json \
	--min-starters $(STARTERS) \
	--remote-spectators \
	--max-spectators 1
REPLAY ?= $(shell date +%Y%m%d-%H%M%S).json

scores:
	@./highscores .

escape:
	$(BOTS) $(FLAGS) $@ > $(REPLAY)

rumble:
	$(BOTS) $(FLAGS) --diagonal-interval 1 $@ > $(REPLAY)

# add more games here!

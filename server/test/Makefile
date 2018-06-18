all: escape collect avoid word

escape:
	./test_escape

collect:
	./test_collect

avoid:
	./test_avoid

word:
	./test_word

check:
	shellcheck tester
	shellcheck -x test_escape
	shellcheck -x test_collect
	shellcheck -x test_avoid
	shellcheck -x test_word

clean:
	rm -f *.log

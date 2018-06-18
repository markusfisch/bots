all: escape collect avoid

escape:
	./test_escape

collect:
	./test_collect

avoid:
	./test_avoid

check:
	shellcheck tester
	shellcheck -x test_escape
	shellcheck -x test_collect

clean:
	rm -f *.log

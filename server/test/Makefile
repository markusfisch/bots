all: escape collect

escape:
	./test_escape

collect:
	./test_collect

check:
	shellcheck tester
	shellcheck -x test_escape
	shellcheck -x test_collect

clean:
	rm -f *.log

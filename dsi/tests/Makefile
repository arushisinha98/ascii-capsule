# Host-side test compilation
# These tests run on the development machine, NOT on the DSi.
# They test portable logic (content parsing, viewer scrolling, code matching).

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -I../source

.PHONY: test clean

test: test_content
	@echo ""
	@echo "Running tests..."
	@echo ""
	@./test_content

test_content: test_content.c ../source/content.c ../source/viewer.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_content

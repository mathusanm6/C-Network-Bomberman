CC=gcc
CFLAGS=-Wall -Wextra
EXEC=main

SRCDIR=src
OBJDIR=obj
TESTDIR=tests

VALGRIND=valgrind
VALGRIND_OPTS=--leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 -s


SRCFILES := $(shell find $(SRCDIR) -name "*.c")
ALLFILES := $(SRCFILES) $(shell find $(SRCDIR) -name "*.h")
OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCFILES))
TESTFILES := $(shell find $(TESTDIR) -name "*.c")
ALLFILES += $(TESTFILES) $(shell find $(TESTDIR) -name "*.h")


# Create obj directory at the beginning
$(shell mkdir -p $(OBJDIR))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: all 

all: $(EXEC)

main: $(OBJFILES) 
	$(CC) -o $@ $^ $(CFLAGS)
	
.PHONY: format fmt check-format

format fmt:
	clang-format -i $(ALLFILES)

check-format:
	clang-format --dry-run --Werror $(ALLFILES)

.PHONY: compile-tests

compile-tests: $(ALLFILES)
	$(CC) -o test $(ALLFILES) $(CFLAGS)

test: compile-tests
	./test 

.PHONY: test-valgrind

test-valgrind: compile-tests
	$(VALGRIND) $(VALGRIND_OPTS) ./test

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(EXEC) test




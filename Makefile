CC=gcc
CFLAGS=-Wall -Wextra
EXEC=main

TEST=test

SRCDIR=src
OBJDIR=obj

SRCOBJDIR=$(OBJDIR)/$(SRCDIR)

TESTDIR=tests
TESTOBJDIR=$(OBJDIR)/$(TESTDIR)

VALGRIND=valgrind
VALGRIND_OPTS=--leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 -s


SRCFILES := $(shell find $(SRCDIR) -type f -name "*.c")
TESTFILES := $(shell find $(TESTDIR) -type f -name "*.c")

OBJFILES := $(patsubst $(SRCDIR)/%.c,$(SRCOBJDIR)/%.o,$(SRCFILES))
TESTOBJFILES := $(patsubst $(TESTDIR)/%.c,$(TESTOBJDIR)/%.o,$(TESTFILES))

ALLFILES := $(SRCFILES) $(TESTFILES) $(shell find $(SRCDIR) $(TESTDIR) -type f -name "*.h") 


# Create obj directory at the beginning
$(shell mkdir -p $(SRCOBJDIR))
$(shell mkdir -p $(TESTOBJDIR))


$(SRCOBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TESTOBJDIR)/%.o: $(TESTDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: all 

all: $(EXEC)

$(EXEC): $(OBJFILES) 
	$(CC) -o $@ $^ $(CFLAGS)
	

.PHONY: format fmt check-format

format fmt:
	clang-format -i $(ALLFILES)

check-format:
	clang-format --dry-run --Werror $(ALLFILES)


test: compile-tests
	./test 

.PHONY: test-valgrind

test-valgrind: compile-tests-valgrind 
	$(VALGRIND) $(VALGRIND_OPTS) ./test

.PHONY: compile-tests compile-tests-valgrind

compile-tests-valgrind: $(filter-out $(SRCOBJDIR)/$(EXEC).o, $(OBJFILES)) $(TESTOBJFILES)
	$(CC) -o $(TEST) $^ -g $(CFLAGS)

compile-tests: $(filter-out $(SRCOBJDIR)/$(EXEC).o, $(OBJFILES)) $(TESTOBJFILES)
	$(CC) -o $(TEST) $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(EXEC) test



